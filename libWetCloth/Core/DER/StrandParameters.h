//
// This file is part of the libWetCloth open source project
//
// The code is licensed solely for academic and non-commercial use under the
// terms of the Clear BSD License. The terms of the Clear BSD License are
// provided below. Other licenses may be obtained by contacting the faculty
// of the Columbia Computer Graphics Group or a Columbia University licensing officer.
//
// We would like to hear from you if you appreciate this work.
//
// The Clear BSD License
//
// Copyright 2018 Yun (Raymond) Fei, Christopher Batty, Eitan Grinspun, and Changxi Zheng
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its contributors may be used
//  to endorse or promote products derived from this software without specific
//  prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS
// LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.

#ifndef __STRAND_PARAMS_H__
#define __STRAND_PARAMS_H__

#include "Definitions.h"
#include "Dependencies/BendingProducts.h"
#include "Dependencies/ElasticStrandUtils.h"
#include <math.h> // exp

struct StrandEquilibriumParameters
{
	StrandEquilibriumParameters(const std::vector<Vec3>& vertices,
								scalar curl_radius,
								scalar curl_density,
								scalar dL,
								scalar root_length,
								scalar valid):
	m_vertices(vertices),
	m_curl_radius(curl_radius),
	m_curl_density(curl_density),
	m_dL(dL),
	m_root_length(root_length),
	m_valid (valid),
	m_dirty (false) {}
	
	mutable std::vector<Vec3> m_vertices;
	mutable double m_curl_radius;
	mutable double m_curl_density;
	mutable double m_dL;
	mutable double m_root_length;
	mutable bool m_valid;
	mutable bool m_dirty;
};

struct StrandParameters
{
	StrandParameters(
					 const VecX& radius,
					 scalar YoungsModulus,
					 scalar shearModulus,
					 scalar stretchingMultiplier,
					 scalar collisionMultiplier,
					 scalar attachMultiplier,
					 scalar density,
					 scalar viscosity,
					 scalar baseRotation,
					 scalar dt,
                     scalar friction_alpha,
                     scalar friction_beta,
					 scalar restVolumeFraction,
					 bool accumViscous = true,
					 bool accumViscousBend = true,
                     bool postProjectFixed = true,
					 scalar straightHairs = 1.,
					 const Vec3& color = Vec3(0, 0, 0)):
	m_density( density ),
	m_viscosity( viscosity ),
	// m_rootRadiusMultiplier( 1. ),
	// m_tipRadiusMultiplier( 1. ),
	m_physicalRadius( radius ),
	m_baseRotation( baseRotation ),
	m_bendingMatrixBase( m_physicalRadius, m_baseRotation ),
	m_youngsModulus( YoungsModulus ),
	m_shearModulus( shearModulus ),
	m_stretchingMultiplier( stretchingMultiplier ),
	m_collisionMultiplier( collisionMultiplier ),
	m_attachMultiplier( attachMultiplier ),
	m_ks( m_physicalRadius, m_youngsModulus ),
	m_kt( m_physicalRadius, m_shearModulus ),
	m_accumulateWithViscous( accumViscous ),
	m_accumulateViscousOnlyForBendingModes( accumViscousBend ),
	m_straightHairs( straightHairs ),
	m_color(color),
    m_friction_alpha(friction_alpha),
    m_friction_beta(friction_beta),
	m_restVolumeFraction(restVolumeFraction),
    m_postProjectFixed(postProjectFixed)
	{
  computeViscousForceCoefficients( dt );
	}
	
	Vec3 getColor() const
	{
		return m_color;
	}
	
	scalar getKs( int vtx ) const
	{
		return m_ks.get()(vtx) * m_stretchingMultiplier;
	}
	
	scalar getKt( int vtx ) const
	{
		return m_kt.get()(vtx);
	}
	
	scalar getRadiusA( int vtx ) const
	{
		return m_physicalRadius.get()(vtx * 2);
	}

	scalar getRadiusB( int vtx ) const
	{
		return m_physicalRadius.get()(vtx * 2 + 1);
	}
	
	scalar bendingCoefficient() const
	{
		return m_youngsModulus.get();
	}
	
	BendingMatrixBase& getBendingMatrixBase()
	{
		return m_bendingMatrixBase;
	}
	
	Mat2 bendingMatrix( int vtx ) const
	{
		return bendingCoefficient() * Mat2(m_bendingMatrixBase.get().block<2, 2>(vtx * 2, 0));
	}
	
	Mat2 bendingMatrixBase(int vtx) const
	{
		return m_bendingMatrixBase.get().block<2, 2>(vtx * 2, 0);
	}
	
	void computeViscousForceCoefficients( scalar dt )
	{
		const int nverts = m_physicalRadius.get().size() / 2;
		m_viscousKs.resize(nverts);
		m_viscousKt.resize(nverts);

		for(int i = 0; i < nverts; ++i) {
			const scalar& radiusA = getRadiusA(i);
			const scalar& radiusB = getRadiusB(i);
			
			// Force coefficients are computed without the varying radius multiplier;
			// correct interpolation will be applied when they are accessed
			m_viscousKs(i) = M_PI * radiusA * radiusB * 3 * m_viscosity / dt;
			m_viscousKt(i) = M_PI_4 * radiusA * radiusB * ( radiusA * radiusA + radiusB * radiusB ) * m_viscosity / dt;
		}
		
		m_viscousBendingCoefficientBase = 3 * m_viscosity / dt;
	}
	
	scalar viscousBendingCoefficient( ) const
	{
		return m_viscousBendingCoefficientBase;
	}
	
	Mat2 viscousBendingMatrix( int vtx ) const
	{
		return viscousBendingCoefficient() * Mat2(m_bendingMatrixBase.get().block<2, 2>(vtx * 2, 0));
	}
	
	scalar getViscousKs( int vtx ) const
	{
		return m_viscousKs(vtx) * m_stretchingMultiplier;
	}
	
	scalar getViscousKt( int vtx ) const
	{
		return m_viscousKt(vtx);
	}
    
    scalar getFrictionAlpha(){
        return m_friction_alpha;
    }
    
    scalar getFrictionBeta(){
        return m_friction_beta;
    }
	
	void printParameters()
	{
		std::cout << "density: " << m_density << std::endl;
		std::cout << "viscosity: " << m_viscosity << std::endl;
		std::cout << "baseRotation: " << m_baseRotation.get() << std::endl;
		std::cout << "YoungsModulus: " << m_youngsModulus.get() << std::endl;
		std::cout << "shearModulus: " << m_shearModulus.get() << std::endl;
		std::cout << "m_viscousBendingCoefficientBase: " << m_viscousBendingCoefficientBase << std::endl;
		std::cout << "m_viscousKt: " << m_viscousKt << std::endl;
		std::cout << "m_viscousKs: " << m_viscousKs << std::endl;
		std::cout << "m_ks: " << m_ks.get() << std::endl;
		std::cout << "m_kt: " << m_kt.get() << std::endl;
		std::cout << "accumViscousBend: " << m_accumulateWithViscous << std::endl;
		std::cout << "accumulateViscousOnlyForBendingModes: " << m_accumulateViscousOnlyForBendingModes << std::endl;
		std::cout << "bendingMatrixBase: " << m_bendingMatrixBase.get() << std::endl;
        std::cout << "friction alpha" << m_friction_alpha << std::endl;
        std::cout << "friction beta" << m_friction_beta << std::endl;
	}
	
	
	double m_density;
	double m_viscosity;
	
	// double m_rootRadiusMultiplier;
	// double m_tipRadiusMultiplier;
	
	// Computed viscous force coefficients. MUST be called at the beginning of each time step if dt has changed.
	double m_viscousBendingCoefficientBase;
	VecX m_viscousKt;
	VecX m_viscousKs;
    
    // friction
    scalar m_friction_alpha;
    scalar m_friction_beta;
	
	// pore property
	scalar m_restVolumeFraction;
	
	// Dependencies
	mutable PhysicalRadius m_physicalRadius;
	mutable BaseRotation m_baseRotation;
	mutable BendingMatrixBase m_bendingMatrixBase;
	mutable YoungsModulus m_youngsModulus;
	mutable ShearModulus m_shearModulus;
	mutable double m_stretchingMultiplier;
	mutable double m_collisionMultiplier;
	mutable double m_attachMultiplier;
	mutable ElasticKs m_ks;
	mutable ElasticKt m_kt;
	
	mutable Vec3 m_color;
	
    bool m_postProjectFixed;
	bool m_accumulateWithViscous;
	bool m_accumulateViscousOnlyForBendingModes;
	scalar m_straightHairs;
};


#endif
