//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
//
//	This program is free software : you can redistribute it and / or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.If not, see <http://www.gnu.org/licenses/>.
//
//	Contact: volkanilbeyli@gmail.com

#pragma once

#include "Transform.h"
#include "DataStructures.h"
#include "Settings.h"

#include "Renderer/RenderingEnums.h"
#include "Renderer/Texture.h"

#include "Utilities/Color.h"

#include <DirectXMath.h>


// Design consideration here:
//
// INHERITANCE
// - if we were to use inheritance for different types of lights, then we can utilize pure virtual functions
//   to enforce class-specific behavior. However, now, we cannot store a vector<Light> due to pure virtuality.
//   most likely solution is the store pointers to derived types, which now requires a memory manager for lights
//   if we want to iterate over lights in a linear-memory-access fashion.
//
// C-STYLE 
// - instead, we can collect the light-specific data under a union and enforce light-specific behavior
//   through the ELightType enum. Currently favoring this approach over inheritance to avoid maintaining the memory
//   of the derived types and simply making use of a vector to hold all light data.
//
#define USE_UNION_FOR_LIGHT_SPECIFIC_DATA 1 // temporary, switching off might spot the project from compiling successfully


// Only used for point lights when querying LightSpaceMatrix, ViewMatrix and ViewFrustumPlanes.
//
#define DEFAULT_POINT_LIGHT_LOOK_DIRECTION Texture::CubemapUtility::ECubeMapLookDirections::CUBEMAP_LOOK_FRONT




struct Light
{
	enum ELightType : int
	{
		POINT = 0,
		SPOT,
		DIRECTIONAL,

		LIGHT_TYPE_COUNT
	};

	//--------------------------------------------------------------------------------------------------------------
	// INTERFACE
	//--------------------------------------------------------------------------------------------------------------

	// returns the projection matrix for the light space transformation. 
	//
#if USE_UNION_FOR_LIGHT_SPECIFIC_DATA
	XMMATRIX GetProjectionMatrix() const;
#else
	virtual XMMATRIX GetProjectionMatrix() const = 0;
#endif

	// returns the view matrix for Directional/Spot lights. 
	// Use 'Texture::CubemapUtility::ECubeMapLookDirections' to get view matrices for cubemap faces for PointLight.
	//
#if USE_UNION_FOR_LIGHT_SPECIFIC_DATA
	XMMATRIX GetViewMatrix(Texture::CubemapUtility::ECubeMapLookDirections lookDir = DEFAULT_POINT_LIGHT_LOOK_DIRECTION) const;
#else
	virtual XMMATRIX GetViewMatrix() const = 0;
#endif

	// returns the frustum plane data for the light.
	// Use 'Texture::CubemapUtility::ECubeMapLookDirections' to get frustum planes for each direction for PointLight.
	//
	inline FrustumPlaneset GetViewFrustumPlanes(Texture::CubemapUtility::ECubeMapLookDirections lookDir = DEFAULT_POINT_LIGHT_LOOK_DIRECTION) const
	{ 
		return FrustumPlaneset::ExtractFromMatrix(GetViewMatrix(lookDir) * GetProjectionMatrix());
	}

	// Returns the View*Projection matrix that describes the light-space transformation of a world space position.
	// Use 'Texture::CubemapUtility::ECubeMapLookDirections' to get the light space matrix for each direction for PointLight.
	//
	inline XMMATRIX GetLightSpaceMatrix(Texture::CubemapUtility::ECubeMapLookDirections lookDir = DEFAULT_POINT_LIGHT_LOOK_DIRECTION) const 
	{ 
		return GetViewMatrix(lookDir) * GetProjectionMatrix(); 
	}


#if USE_UNION_FOR_LIGHT_SPECIFIC_DATA
	void GetGPUData(PointLightGPU& refData) const;
	void GetGPUData(SpotLightGPU& refData) const;
	void GetGPUData(DirectionalLightGPU& refData) const;

	// TODO: remove this arbitrary function for directional lights
	Settings::ShadowMap GetSettings() const; // ?
#endif

	// Constructors
	//
	Light() // DEFAULTS
		: mColor(LinearColor::white)
		, mType(LIGHT_TYPE_COUNT)
		, mBrightness(300.0f)
		, mbCastingShadows(false)
		, mDepthBias(0.0f)
		, mNearPlaneDistance(0.0f)
		, mFarPlaneDistance(0.0f)
		, mRange(100.0f)
		, mTransform()
		, mMeshID(EGeometry::SPHERE)
	{}
	Light
	(
		  LinearColor color
		, ELightType type
		, float brightness
		, bool bCastShadows
		, float depthBias
		, float nearPlaneDistance
		, float range
		, Transform transform
		, EGeometry mesh
	)
		: mColor(color)
		, mType(type)
		, mBrightness(brightness)
		, mbCastingShadows(bCastShadows)
		, mDepthBias(depthBias)
		, mNearPlaneDistance(nearPlaneDistance)
		, mRange(range)
		, mTransform(transform)
		, mMeshID(mesh)
	{}


	//--------------------------------------------------------------------------------------------------------------
	// DATA
	//--------------------------------------------------------------------------------------------------------------
	ELightType mType;
	LinearColor	mColor;
	float mBrightness;

	bool mbCastingShadows;
	float mDepthBias;
	float mNearPlaneDistance;
	union 
	{
		float mFarPlaneDistance;
		float mRange;
	};

	Transform mTransform;
	EGeometry mMeshID;
	bool mbEnabled;

#if USE_UNION_FOR_LIGHT_SPECIFIC_DATA
	union // LIGHT-SPECIFIC DATA
	{
		// DIRECTIONAL LIGHT ----------------------------------------
		//  
		//   |  |  |  |  |
		//   |  |  |  |  |
		//   v  v  v  v  v
		//    
		struct  
		{
			float mViewportX;
			float mViewportY;
			float mDistanceFromOrigin;
		};
		


		// POINT LIGHT -----------------------------------------------
		//
		//   \|/ 
		//  --*--
		//   /|\
		//
		struct // Point
		{
			float mAttenuationConstant ;
			float mAttenuationLinear   ;
			float mAttenuationQuadratic;
		};



		// SPOT LIGHT --------------------------------------------------
		//     
		//       *
		//     /   \
		//    /_____\
		//   ' ' ' ' ' 
		//
		struct // Spot
		{
			float mSpotAngleDegrees;
			float mSpotFalloffAngleDegrees;
			float dummy1;
		};



		// TODO: v0.6.0 linear lights from GPU Zen
		// Eric Heitz Slides: https://drive.google.com/file/d/0BzvWIdpUpRx_Z2pZWWFtam5xTFE/view
		// LINEAR LIGHT  ------------------------------------------------
		//
		//
		//
		//
		//
		//
		struct // Area
		{
			float dummy2;
			float dummy3;
			float dummy4;
		};
	};
#endif
};





#if !USE_UNION_FOR_LIGHT_SPECIFIC_DATA

// DIRECTIONAL LIGHT ----------------------------------------
//  
//   |  |  |  |  |
//   |  |  |  |  |
//   v  v  v  v  v
//    
struct DirectionalLight : public Light
{
public:
	XMMATRIX GetProjectionMatrix() const override;
	XMMATRIX GetViewMatrix() const override;
	DirectionalLightGPU GetGPUData() const;

	Settings::ShadowMap GetSettings() const; // ?

	DirectionalLight() 
#if USE_UNION_FOR_LIGHT_SPECIFIC_DATA
	{ 
		mbEnabled = false; 
		mViewportX = mViewportY = 0.0f;
	}
#else
		: mViewportX(0.0f)
		, mViewportY(0.0f)
	{
		mbEnabled = false;
	}
	float mViewportX;
	float mViewportY;
#endif
};



// POINT LIGHT -----------------------------------------------
//
//   \|/ 
//  --*--
//   /|\
//
#define DEFAULT_POINT_LIGHT_LOOK_DIRECTION Texture::CubemapUtility::ECubeMapLookDirections::CUBEMAP_LOOK_FRONT
struct PointLight : public Light
{
public:
	XMMATRIX GetViewMatrix(Texture::CubemapUtility::ECubeMapLookDirections direction) const;
	inline XMMATRIX GetLightSpaceMatrix(Texture::CubemapUtility::ECubeMapLookDirections direction) const { return GetViewMatrix(direction); }
	inline FrustumPlaneset GetViewFrustumPlanes(Texture::CubemapUtility::ECubeMapLookDirections direction) const { return FrustumPlaneset::ExtractFromMatrix(GetViewMatrix(direction) * GetProjectionMatrix()); }

	XMMATRIX GetProjectionMatrix() const override;
	inline XMMATRIX GetViewMatrix() const override       { return GetViewMatrix       (DEFAULT_POINT_LIGHT_LOOK_DIRECTION); }
	inline FrustumPlaneset GetViewFrustumPlanes() const  { return GetViewFrustumPlanes(DEFAULT_POINT_LIGHT_LOOK_DIRECTION); };


	PointLightGPU GetGPUData() const;

	PointLight()
#if USE_UNION_FOR_LIGHT_SPECIFIC_DATA
	{
		mAttenuationConstant  = (1.0f);
		mAttenuationLinear    = (0.0f);
		mAttenuationQuadratic = (0.0f);
	}
#else
		: mAttenuationConstant(1.0f)
		, mAttenuationLinear(0.0f)
		, mAttenuationQuadratic(0.0f)
	{}

	float mAttenuationConstant;
	float mAttenuationLinear;
	float mAttenuationQuadratic;
#endif
};



// SPOT LIGHT --------------------------------------------------
//     
//       *
//     /   \
//    /_____\
//   ' ' ' ' ' 
//
struct SpotLight : public Light
{
	XMMATRIX GetProjectionMatrix() const override;
	XMMATRIX GetViewMatrix() const override;

	SpotLightGPU GetGPUData() const;

	SpotLight()
#if USE_UNION_FOR_LIGHT_SPECIFIC_DATA
	{
		mSpotAngleDegrees        = (60.0f);
		mSpotFalloffAngleDegrees = (50.0f);
	}
#else
		: mSpotAngleDegrees(60.0f)
		, mSpotFalloffAngleDegrees(50.0f)
	{}
	float mSpotAngleDegrees;
	float mSpotFalloffAngleDegrees;
#endif
};



#if 0
// TODO: v0.6.0 linear lights from GPU Zen
// LINEAR LIGHT  ------------------------------------------------
//
//
//
//
//
//
struct AreaLight : public Light
{
	// Eric Heitz Slides: https://drive.google.com/file/d/0BzvWIdpUpRx_Z2pZWWFtam5xTFE/view
	XMMATRIX GetProjectionMatrix() const override;
	XMMATRIX GetViewMatrix() const override;
	XMMATRIX GetLightSpaceMatrix() const override;

	AreaLight()
};
#endif

#endif