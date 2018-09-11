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

	union // each light uses this vec2 for light-specific data
	{	
		vec2	attenuation;	// point light attenuation
		vec2	spotAngle;		// spot light angle (_spotAngle.x() and _spotAngle.y() are the same thing)
	};	

	Transform	transform;
	EGeometry	renderMesh;	// todo: rename to _builtinMeshID;
	//bool		_bEnabled; 
};


// One Directional light is allowed per scene
//
struct DirectionalLight
{
	LinearColor color;
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




// TODO #Refactoring 
#if 0
class Light
{
public:

	XMMATRIX GetLightSpaceMatrix() const;
	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;

private:
	vec3 mPosition;
	float mRange;
	float mBrightness;
	bool mCastingShadows;
};

class PointLight : public Light
{

};

class DirectionalLight : public Light
{

};

class SpotLight : public Light
{

};


// TODO: v0.6.0 linear lights from GPU Zen
class AreaLight : public Light
{

};
#endif