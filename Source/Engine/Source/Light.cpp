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

#include "Light.h"
#include "Engine/GameObject.h"

#include "Utilities/Log.h"

#include <unordered_map>

using std::string;

#if 1 // NEW UNIFIED INTERFACE

#if USE_UNION_FOR_LIGHT_SPECIFIC_DATA

DirectX::XMMATRIX Light::GetProjectionMatrix() const
{
	switch (mType)
	{
	case Light::POINT:
	{
		constexpr float ASPECT_RATIO = 1.0f; // cubemap aspect ratio
		return XMMatrixPerspectiveFovLH(PI_DIV2, ASPECT_RATIO, mNearPlaneDistance, mFarPlaneDistance);
	}
	case Light::SPOT:
	{
		constexpr float ASPECT_RATIO = 1.0f;
		return XMMatrixPerspectiveFovLH(mSpotAngleDegrees * mSpotFalloffAngleDegrees * DEG2RAD, ASPECT_RATIO, mNearPlaneDistance, mFarPlaneDistance);
	}
	case Light::DIRECTIONAL:
	{
		if (mViewportX < 1.0f) return XMMatrixIdentity();
		return XMMatrixOrthographicLH(mViewportX, mViewportY, mNearPlaneDistance, mFarPlaneDistance);
	}
	default:
		Log::Warning("GetProjectionMatrix() called on invalid light type!");
		return XMMatrixIdentity();
	}
}

DirectX::XMMATRIX Light::GetViewMatrix(Texture::CubemapUtility::ECubeMapLookDirections lookDir /*= DEFAULT_POINT_LIGHT_LOOK_DIRECTION*/) const
{
	switch (mType)
	{
	case Light::POINT:
	{
		return Texture::CubemapUtility::GetViewMatrix(lookDir, mTransform._position);
	}
	case Light::SPOT:
	{
		XMVECTOR up = vec3::Up;
		XMVECTOR lookAt = vec3::Forward;	// spot light default orientation looks up
		lookAt = XMVector3TransformCoord(lookAt, mTransform.RotationMatrix());
		up = XMVector3TransformCoord(up, mTransform.RotationMatrix());
		XMVECTOR pos = mTransform._position;
		XMVECTOR taraget = pos + lookAt;
		return XMMatrixLookAtLH(pos, taraget, up);
	}
	case Light::DIRECTIONAL:
	{
		if (mViewportX < 1.0f) return XMMatrixIdentity();

		const XMVECTOR up = vec3::Up;
		const XMVECTOR lookAt = vec3::Zero;
		const XMMATRIX mRot = mTransform.RotationMatrix();
		const vec3 direction = XMVector3Transform(vec3::Forward, mRot);
		const XMVECTOR lightPos = direction * -mDistanceFromOrigin;	// away from the origin along the direction vector 
		return XMMatrixLookAtLH(lightPos, lookAt, up);
	}
	default:
		Log::Warning("GetViewMatrix() called on invalid light type!");
		return XMMatrixIdentity();
	}
}



void Light::GetGPUData(DirectionalLightGPU& l) const
{
	assert(mType == ELightType::DIRECTIONAL);
	const XMMATRIX mRot = mTransform.RotationMatrix();
	const vec3 direction = XMVector3Transform(vec3::Forward, mRot);

	l.brightness = this->mBrightness;
	l.color = this->mColor;
	l.lightDirection = direction;
	l.shadowFactor = this->mbEnabled ? 1.0f : 0.0f;
}
void Light::GetGPUData(SpotLightGPU& l) const
{
	assert(mType == ELightType::SPOT);
	const vec3 spotDirection = XMVector3TransformCoord(vec3::Forward, mTransform.RotationMatrix());

	l.position = mTransform._position;
	l.halfAngle = mSpotAngleDegrees * DEG2RAD * 0.5f;

	l.color = mColor.Value();
	l.brightness = mBrightness;

	l.spotDir = spotDirection;
	l.depthBias = mDepthBias;
}
void Light::GetGPUData(PointLightGPU& l) const
{
	assert(mType == ELightType::POINT);

	l.position = mTransform._position;
	l.range = mRange;

	l.color = mColor.Value();
	l.brightness = mBrightness;
	
	l.attenuation = vec3(mAttenuationConstant, mAttenuationLinear, mAttenuationQuadratic);
	l.depthBias = mDepthBias;
}

Settings::ShadowMap Light::GetSettings() const
{
	Settings::ShadowMap settings;
	settings.dimension = static_cast<size_t>(mViewportX);
	return settings;
}



#else
// DIRECTIONAL LIGHT ----------------------------------------
//  
//   |  |  |  |  |
//   |  |  |  |  |
//   v  v  v  v  v
//   
DirectionalLightGPU DirectionalLight::GetGPUData() const
{
	const XMMATRIX mRot = mTransform.RotationMatrix();
	const vec3 direction = XMVector3Transform(vec3::Forward, mRot);

	DirectionalLightGPU l;
	l.brightness = this->mBrightness;
	l.color = this->mColor;
	l.lightDirection = direction;
	l.shadowFactor = this->mbEnabled ? 1.0f : 0.0f;
	return l;
}
Settings::ShadowMap DirectionalLight::GetSettings() const
{
	Settings::ShadowMap settings;
	settings.dimension = static_cast<size_t>(mViewportX);
	return settings;
}
XMMATRIX DirectionalLight::GetProjectionMatrix() const
{
	if (mViewportX < 1.0f) return XMMatrixIdentity();
	return XMMatrixOrthographicLH(mViewportX, mViewportY, mNearPlaneDistance, mFarPlaneDistance);
}
XMMATRIX DirectionalLight::GetViewMatrix() const
{
	if (mViewportX < 1.0f) return XMMatrixIdentity();

	const XMVECTOR up = vec3::Up;
	const XMVECTOR lookAt = vec3::Zero;
	const XMMATRIX mRot = mTransform.RotationMatrix();
	const vec3 direction = XMVector3Transform(vec3::Forward, mRot);

	float shadowMapDistance = 1500.0f; // TODO:
	const XMVECTOR lightPos = direction * -shadowMapDistance;	// away from the origin along the direction vector 
	return XMMatrixLookAtLH(lightPos, lookAt, up);
}



// POINT LIGHT -----------------------------------------------
//
//   \|/ 
//  --*--
//   /|\
//
PointLightGPU PointLight::GetGPUData() const
{
	PointLightGPU l;
	l.position = mTransform._position;
	l.color = mColor.Value();
	l.brightness = mBrightness;
	//l.attenuation = mAttenuation; // TODO
	assert(false);
	l.range = mRange;
	return l;
}
XMMATRIX PointLight::GetProjectionMatrix() const
{
	constexpr float ASPECT_RATIO = 1.0f; // cubemap aspect ratio
	return XMMatrixPerspectiveFovLH(PI_DIV2, ASPECT_RATIO, mNearPlaneDistance, mFarPlaneDistance);
}
DirectX::XMMATRIX PointLight::GetViewMatrix(Texture::CubemapUtility::ECubeMapLookDirections direction) const
{
	return Texture::CubemapUtility::GetViewMatrix(direction, mTransform._position);
}

// SPOT LIGHT --------------------------------------------------
//     
//       *
//     /   \
//    /_____\
//   ' ' ' ' ' 
//
SpotLightGPU SpotLight::GetGPUData() const
{
	const vec3 spotDirection = XMVector3TransformCoord(vec3::Up, mTransform.RotationMatrix());

	SpotLightGPU l;
	l.position = mTransform._position;
	l.color = mColor.Value();
	l.brightness = mBrightness;
	//l.halfAngle = spotAngle_spotFallOff.x() * DEG2RAD / 2; // TODO
	assert(false);
	l.spotDir = spotDirection;
	return l;
}
XMMATRIX SpotLight::GetProjectionMatrix() const
{
	constexpr float ASPECT_RATIO = 1.0f;
	return XMMatrixPerspectiveFovLH(mSpotAngleDegrees * mSpotFalloffAngleDegrees * DEG2RAD, ASPECT_RATIO, mNearPlaneDistance, mFarPlaneDistance);
}
XMMATRIX SpotLight::GetViewMatrix() const
{
	XMVECTOR up = vec3::Back;
	XMVECTOR lookAt = vec3::Up;	// spot light default orientation looks up
	lookAt = XMVector3TransformCoord(lookAt, mTransform.RotationMatrix());
	up = XMVector3TransformCoord(up, mTransform.RotationMatrix());
	XMVECTOR pos = mTransform._position;
	XMVECTOR taraget = pos + lookAt;
	return XMMatrixLookAtLH(pos, taraget, up);
}

#endif // USE_UNION




#else // OLD_IMPL

// For Phong Lighting
// range(distance)   -   (Linear, Quadratic) attenuation factor map
// source: http://www.ogre3d.org/tikiwiki/tiki-index.php?page=-Point+Light+Attenuation
const std::map<unsigned, std::pair<float, float>> rangeAttenuationMap_ = 
{
	{ 7   , std::make_pair(0.7000f, 1.8f)    },
	{ 13  , std::make_pair(0.3500f, 0.44f)   },
	{ 20  , std::make_pair(0.2200f, 0.20f)   },
	{ 32  , std::make_pair(0.1400f, 0.07f)   },
	{ 50  , std::make_pair(0.0900f, 0.032f)  },
	{ 65  , std::make_pair(0.0700f, 0.017f)  },
	{ 100 , std::make_pair(0.0450f, 0.0075f) },
	{ 160 , std::make_pair(0.0270f, 0.0028f) },
	{ 200 , std::make_pair(0.0220f, 0.0019f) },
	{ 325 , std::make_pair(0.0140f, 0.0007f) },
	{ 600 , std::make_pair(0.0070f, 0.0002f) },
	{ 3250, std::make_pair(0.0014f, 0.00007f)},
};

static const std::unordered_map<Light::ELightType, EGeometry>		sLightTypeMeshLookup
{
	{ Light::ELightType::SPOT       , EGeometry::CYLINDER },
	{ Light::ELightType::POINT      , EGeometry::SPHERE },
	{ Light::ELightType::DIRECTIONAL, EGeometry::SPHERE }
};

Light::Light()
	:
	type(ELightType::POINT),
	color(LinearColor::white),
	range(50),	// phong
	brightness(300.0f),
	castsShadow(false),
	spotAngle_spotFallOff(vec2()),
	attenuation(vec2()),
	renderMeshID(sLightTypeMeshLookup.at(ELightType::POINT)),
	depthBias(0.0000005f),
	farPlaneDistance(500)
{
	SetLightRange(range);
}

Light::Light(const Light& l)
	:
	type(l.type),
	color(l.color),
	range(l.range),
	brightness(l.brightness),
	castsShadow(l.castsShadow),
	spotAngle_spotFallOff(l.spotAngle_spotFallOff),
	transform(l.transform),
	renderMeshID(l.renderMeshID),
	depthBias(l.depthBias),
	farPlaneDistance(l.farPlaneDistance)
{}

Light::Light(const Light && l)
	:
	type(l.type),
	color(std::move(l.color)),
	range(l.range),
	brightness(l.brightness),
	castsShadow(l.castsShadow),
	spotAngle_spotFallOff(l.spotAngle_spotFallOff),
	transform(std::move(l.transform)),
	renderMeshID(l.renderMeshID),
	depthBias(l.depthBias),
	farPlaneDistance(l.farPlaneDistance)
{}

Light::Light(
	  ELightType type
	, LinearColor color
	, float range
	, float brightness
	, float spotAngle
	, bool castsShadows
	, float farPlaneDistance
	, float depthBias
	//, bool bEnabled = true // #BreaksRelease
)
	: type(type)
	, color(color)
	, range(range)
	, brightness(brightness)
	, castsShadow(castsShadows)
	, farPlaneDistance(farPlaneDistance)
	, depthBias(depthBias)
	// , _bEnabled(bEnabled)
{
	switch (type)
	{
	case ELightType::POINT: SetLightRange(range);			break;
	case ELightType::SPOT:	this->spotAngle_spotFallOff.x() = spotAngle;break;
	case ELightType::DIRECTIONAL:  /* nothing to do */		break;
	}

	renderMeshID = sLightTypeMeshLookup.at(type);
}



Light::~Light()
{}

void Light::SetLightRange(float range)
{
	range = range;
	// ATTENUATION LOOKUP FOR POINT LIGHTS
	// find the first greater or equal range value (rangeIndex) 
	// to look up with in the attenuation map
	constexpr float ranges[] = { 7, 13, 20, 32, 50, 65, 100, 160, 200, 325, 600, 3250 };
	unsigned rangeIndex = static_cast<unsigned>(ranges[rangeAttenuationMap_.size() - 1]);	// default case = largest range
	for (size_t i = 0; i < rangeAttenuationMap_.size(); i++)
	{
		if (ranges[i] >= range)
		{
			rangeIndex = static_cast<unsigned>(ranges[i]);
			break;
		}
	}

	std::pair<float, float> attn = rangeAttenuationMap_.at(rangeIndex);
	attenuation = vec2(attn.first, attn.second);
}


#endif // OLD_IMPL