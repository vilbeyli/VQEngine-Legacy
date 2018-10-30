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

#if USE_NEW_LIGHT_IMPL

#else

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

XMMATRIX Light::GetLightSpaceMatrix() const
{
	// remember:
	//	when we're sending	 world * view * projection
	//		in shader		 projection * view * world;
	return GetViewMatrix() * GetProjectionMatrix();
}

XMMATRIX Light::GetViewMatrix() const
{
	const XMMATRIX ViewMatarix = [&]() -> XMMATRIX {
		switch (type)
		{
		case ELightType::POINT:
		{
			return XMMatrixIdentity();
		}
		
		case ELightType::SPOT:
		{
			XMVECTOR up		= vec3::Back;
			XMVECTOR lookAt = vec3::Up;	// spot light default orientation looks up
			lookAt	= XMVector3TransformCoord(lookAt, transform.RotationMatrix());
			up		= XMVector3TransformCoord(up,	  transform.RotationMatrix());
			XMVECTOR pos = transform._position;
			XMVECTOR taraget = pos + lookAt;
			return XMMatrixLookAtLH(pos, taraget, up);
		}
		default:
			Log::Warning("INVALID LIGHT TYPE for GetViewMatrix()");
			return XMMatrixIdentity();
		}
	}();
	return ViewMatarix;
}

XMMATRIX Light::GetProjectionMatrix() const
{
	const XMMATRIX proj = [&]() -> const XMMATRIX
	{
		constexpr float ASPECT_RATIO = 1.0f;	// cube textures / cubemaps
		switch (type)
		{
		case ELightType::POINT:
		{
			return XMMatrixPerspectiveFovLH(PI_DIV2, ASPECT_RATIO, 0.01f /*todo: nearPlaneDistance*/, farPlaneDistance);
		}
		case ELightType::SPOT:
		{
			return XMMatrixPerspectiveFovLH((spotAngle_spotFallOff.x() * spotAngle_spotFallOff.y()) * DEG2RAD, ASPECT_RATIO, nearPlaneDistance, farPlaneDistance);
		}
		default:
			Log::Warning("INVALID LIGHT TYPE for GetProjectionMatrix()");
			return XMMatrixIdentity();
		}
	}();
	return proj;
}


PointLightGPU Light::GetPointLightData() const
{
	PointLightGPU l;
	l.position = transform._position;
	l.color = color.Value();
	l.brightness = brightness;
	l.attenuation = attenuation;
	l.range = range;
	return l;
}

SpotLightGPU Light::GetSpotLightData() const
{
	const vec3 spotDirection = [&]() -> const vec3{
		if (type == ELightType::SPOT)
		{
			return XMVector3TransformCoord(vec3::Up, transform.RotationMatrix());
		}
		return vec3();
	}();

	SpotLightGPU l;
	l.position = transform._position;
	l.color = color.Value();
	l.brightness = brightness;
	l.halfAngle = spotAngle_spotFallOff.x() * DEG2RAD / 2;
	l.spotDir = spotDirection;
	return l;
}

FrustumPlaneset Light::GetViewFrustumPlanes() const
{
	return FrustumPlaneset::ExtractFromMatrix(GetViewMatrix() * GetProjectionMatrix());
}


DirectionalLightGPU DirectionalLight::GetGPUData() const
{
	DirectionalLightGPU l;
	l.brightness = this->brightness;
	l.color = this->color;
	l.lightDirection = this->direction;
	l.shadowFactor = this->enabled > 0 ? 1.0f : 0.0f;
	return l;
}

XMMATRIX DirectionalLight::GetLightSpaceMatrix() const
{	// remember:
	//	when we're sending	 world * view * projection
	//		in shader		 projection * view * world;
	return GetViewMatrix() * GetProjectionMatrix();
}

XMMATRIX DirectionalLight::GetViewMatrix() const
{
	if (shadowMapAndViewportSize.y() < 1.0f) return XMMatrixIdentity();
	XMVECTOR up = vec3::Up;
	XMVECTOR lookAt = vec3::Zero;
	XMVECTOR lightPos = direction * -shadowMapDistance;	// away from the origin along the direction vector 
	return XMMatrixLookAtLH(lightPos, lookAt, up);
}

XMMATRIX DirectionalLight::GetProjectionMatrix() const
{
	const float sz = shadowMapAndViewportSize.y();
	if (sz < 1.0f) return XMMatrixIdentity();
	return XMMatrixOrthographicLH(sz, sz, 0.05f, shadowMapDistance * 1.5f);
	//return XMMatrixOrthographicLH(4096, 4096, 0.5f, shadowMapDistance * 1.5f);
	//return XMMatrixOrthographicLH(sz, sz, 0.1f, 1200.0f);
}

Settings::ShadowMap DirectionalLight::GetSettings() const
{
	Settings::ShadowMap settings;
	settings.dimension = static_cast<size_t>(shadowMapAndViewportSize.x());
	return settings;
}

#endif