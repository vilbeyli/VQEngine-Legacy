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

#include "Utilities/Color.h"

#include <DirectXMath.h>

#define USE_NEW_LIGHT_IMPL 0

#if USE_NEW_LIGHT_IMPL
class Light
{
public:

	// returns the projection matrix for the light space transformation. 
	//
	virtual XMMATRIX GetProjectionMatrix() const = 0;

	// returns the (View * Projection) matrix for Directional/Spot lights
	//
	virtual XMMATRIX GetLightSpaceMatrix() const;

	// returns the view matrix for Directional/Spot lights. 
	// See PointLight struct for cube-face view matrices.
	//
	virtual XMMATRIX GetViewMatrix() const;

	Light()
		: mColor(LinearColor::white)
		, mBrightness(300.0f)
		, mbCastingShadows(false)
		, mDepthBias(0.0f)
		, mNearPlaneDistance(0.0f)
		, mFarPlaneDistance(0.0f)
		, mAttenuation(0,0) // sopotAngle/FalloOff | directional viewportSize
		, mRange(100.0f)
		, mTransform()
		, mMeshID(EGeometry::SPHERE)
	{}
	Light
	(
		  LinearColor color
		, float brightness
		, bool bCastShadows
		, float depthBias
		, float farPlaneDistance
		, float nearPlaneDistance
		, vec2 attenuation
		, float range
		, Transform transform
		, EGeometry mesh
	)
		: mColor(color)
		, mBrightness(brightness)
		, mbCastingShadows(bCastShadows)
		, mDepthBias(depthBias)
		, mNearPlaneDistance(nearPlaneDistance)
		, mFarPlaneDistance(farPlaneDistance)
		, mAttenuation(attenuation) // sopotAngle/FalloOff | directional viewportSize
		, mRange(range)
		, mTransform(transform)
		, mMeshID(mesh)
	{}

//private:
	LinearColor	mColor;
	float mBrightness;

	bool mbCastingShadows;
	float mDepthBias;
	float mFarPlaneDistance;
	float mNearPlaneDistance;

	union // each light type (point/spot) uses this vec2 for light-specific data
	{
		vec2 mAttenuation;				//       point light attenuation
		vec2 mSpotAngle_SpotFallOff;		//        spot light angle and falloff multiplier
		vec2 mShadowMapAndViewportSize;	// directional light viewport size
	};


	float mRange;	// point/spot

	Transform mTransform;
	EGeometry mMeshID;

};

class DirectionalLight : public Light
{
public:
	XMMATRIX GetProjectionMatrix() const override;
	XMMATRIX GetViewMatrix() const override;
	XMMATRIX GetLightSpaceMatrix() const override;

	DirectionalLight();
};


class PointLight : public Light
{
public:
	XMMATRIX GetViewMatrix(Texture::CubemapUtility::ECubeMapLookDirections direction) const;
	XMMATRIX GetLightSpaceMatrix(Texture::CubemapUtility::ECubeMapLookDirections direction) const;

	XMMATRIX GetProjectionMatrix() const override;
	inline XMMATRIX GetViewMatrix() const override { return GetViewMatrix(Texture::CubemapUtility::ECubeMapLookDirections::CUBEMAP_LOOK_FRONT); }
	inline XMMATRIX GetLightSpaceMatrix() const override { return GetLightSpaceMatrix(Texture::CubemapUtility::ECubeMapLookDirections::CUBEMAP_LOOK_FRONT); }

	PointLight(){}

};


class SpotLight : public Light
{
	XMMATRIX GetProjectionMatrix() const override;
	XMMATRIX GetViewMatrix() const override;
	XMMATRIX GetLightSpaceMatrix() const override;
	
	SpotLight();
};


// TODO: v0.6.0 linear lights from GPU Zen
#if 0
// Eric Heitz Slides: https://drive.google.com/file/d/0BzvWIdpUpRx_Z2pZWWFtam5xTFE/view
class AreaLight : public Light
{
	XMMATRIX GetProjectionMatrix() const override;
	XMMATRIX GetViewMatrix() const override;
	XMMATRIX GetLightSpaceMatrix() const override;

	AreaLight()
};
#endif


#else

// this needs refactoring. design is pretty old and convoluted.
//
struct Light
{
	friend class Graphics;

	enum ELightType : size_t
	{
		POINT = 0,
		SPOT,
		DIRECTIONAL,

		LIGHT_TYPE_COUNT
	};

	Light();
	Light(ELightType type
		, LinearColor color
		, float range
		, float brightness
		, float spotAngle
		, bool castsShadows = false
		, float farPlaneDistance = 500.0f
		, float depthBias = 0.0000005f
		//, bool bEnabled = true // #BreaksRelease
	);
	Light(const Light& l);
	Light(const Light&& l);
	~Light();

	void SetLightRange(float range);

	XMMATRIX		GetLightSpaceMatrix() const;
	XMMATRIX		GetViewMatrix() const;
	XMMATRIX		GetProjectionMatrix() const;
	PointLightGPU	GetPointLightData() const;
	SpotLightGPU	GetSpotLightData() const;
	FrustumPlaneset GetViewFrustumPlanes() const;
	//---------------------------------------------------------------------------------
	
	ELightType	type;
	LinearColor	color;
	float		range;
	float		brightness;	// 300.0f is a good default value for points/spots

	bool		castsShadow;
	float		depthBias;
	float		farPlaneDistance;
	float		nearPlaneDistance;

	union // each light type (point/spot) uses this vec2 for light-specific data
	{	
		vec2	attenuation;			// point light attenuation
		vec2	spotAngle_spotFallOff;	// spot light angle and falloff multiplier
	};	

	Transform	transform;
	EGeometry	renderMeshID;
	//bool		_bEnabled; 
};


// One Directional light is allowed per scene
//
struct DirectionalLight
{
	// ELightType	type;
	LinearColor color;
	// range
	float brightness;

	vec3 direction;
	int enabled = 0;

	float shadowMapDistance;	// view matrix position - distance from scene center 
	float depthBias;
	vec2 shadowMapAndViewportSize;


	DirectionalLightGPU GetGPUData() const;
	XMMATRIX GetLightSpaceMatrix() const;
	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;
	Settings::ShadowMap GetSettings() const;
};

#endif