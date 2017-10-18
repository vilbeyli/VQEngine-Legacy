//	DX11Renderer - VDemo | DirectX11 Renderer
//	Copyright(C) 2016  - Volkan Ilbeyli
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
#include "Application/GameObject.h"

#include "Utilities/Log.h"

#include <iostream>
#include <unordered_map>

using std::cout;
using std::endl;
using std::string;

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
	_type(ELightType::POINT),
	_color(LinearColor::white),
	_range(50),	// phong
	_brightness(300.0f),
	_castsShadow(false),
	_spotAngle(vec2()),
	_attenuation(vec2()),
	_renderMesh(sLightTypeMeshLookup.at(ELightType::POINT))
{
	SetLightRange(_range);
}

Light::Light(const Light& l)
	:
	_type(l._type),
	_color(l._color),
	_range(l._range),
	_brightness(l._brightness),
	_castsShadow(l._castsShadow),
	_spotAngle(l._spotAngle),
	_transform(l._transform),
	_renderMesh(l._renderMesh)
{}

Light::Light(ELightType type, LinearColor color, float range, float brightness, float spotAngle, bool castsShadows) 
	:
	_type(type),
	_color(color),
	_range(range),
	_brightness(brightness),
	_castsShadow(castsShadows)
{
	switch (_type)
	{
	case ELightType::POINT:
		SetLightRange(_range);
		break;
	case ELightType::SPOT:
		_spotAngle.x() = spotAngle;
		break;
	case ELightType::DIRECTIONAL:
		Log::Error("todo: directional lights...");
		break;
	}

	_renderMesh = sLightTypeMeshLookup.at(_type);
}

Light::~Light()
{}

void Light::SetLightRange(float range)
{
	_range = range;
	// ATTENUATION LOOKUP FOR POINT LIGHTS
	// find the first greater or equal range value (rangeIndex) 
	// to look up with in the attenuation map
	constexpr float ranges[] = { 7, 13, 20, 32, 50, 65, 100, 160, 200, 325, 600, 3250 };
	unsigned rangeIndex = static_cast<unsigned>(ranges[rangeAttenuationMap_.size() - 1]);	// default case = largest range
	for (size_t i = 0; i < rangeAttenuationMap_.size(); i++)
	{
		if (ranges[i] >= _range)
		{
			rangeIndex = static_cast<unsigned>(ranges[i]);
			break;
		}
	}

	std::pair<float, float> attn = rangeAttenuationMap_.at(rangeIndex);
	_attenuation = vec2(attn.first, attn.second);
}

XMMATRIX Light::GetLightSpaceMatrix() const
{
	XMMATRIX LSpaceMat = XMMatrixIdentity();
	switch (_type)
	{
	case ELightType::POINT:
		break;
	case ELightType::SPOT:
	{
		XMVECTOR pos = _transform._position;
		XMMATRIX view = GetViewMatrix();
		XMMATRIX proj = XMMatrixPerspectiveFovLH((_spotAngle.x() * 1.25f) * DEG2RAD, 1.0f, 0.1f, 500);
		
		// remember:
		//	when we're sending	 world * view * projection
		//		in shader		 projection * view * world;
		LSpaceMat =  view * proj;	// same as: XMMatrixTranspose(proj)  * XMMatrixTranspose(view);
		break;
	}

	default:
		//OutputDebugString("INVALID LIGHT TYPE for GetLightSpaceMatrix()\n");
		break;
	}

	return LSpaceMat;
}

XMMATRIX Light::GetViewMatrix() const
{
	const XMMATRIX ViewMatarix = [&]() -> XMMATRIX {
		switch (_type)
		{
		case ELightType::POINT:
		{
			return XMMatrixIdentity();
		}
		
		case ELightType::SPOT:
		{
			XMVECTOR up		= vec3::Back;
			XMVECTOR lookAt = vec3::Up;
			lookAt	= XMVector3TransformCoord(lookAt, _transform.RotationMatrix());
			up		= XMVector3TransformCoord(up,	  _transform.RotationMatrix());
			XMVECTOR pos = _transform._position;
			XMVECTOR taraget = pos + lookAt;
			return XMMatrixLookAtLH(pos, taraget, up);
		}	

		default:
			std::cout << "INVALID LIGHT TYPE for GetViewMatrix()" << std::endl;
			return XMMatrixIdentity();
		}
	}();
	return ViewMatarix;
}

XMMATRIX Light::GetProjectionMatrix() const
{
	const XMMATRIX proj = [&]() -> const XMMATRIX{
		switch (_type)
		{
		case ELightType::POINT:
		{
			return XMMatrixIdentity();
		}
			
		case ELightType::SPOT:
		{
			return XMMatrixPerspectiveFovLH((_spotAngle.x() * 1.25f) * DEG2RAD, 1.0f, 0.1f, 500.0f);
		}	

		default:
			std::cout << "INVALID LIGHT TYPE for GetProjectionMatrix()" << std::endl;
			return XMMatrixIdentity();

		}
	}();
	return proj;
}

LightShaderSignature Light::ShaderSignature() const
{
	const vec3 spotDirection = [&]() -> const vec3 {
		if (_type == ELightType::SPOT)
		{
			return XMVector3TransformCoord(vec3::Up, _transform.RotationMatrix());
		}
		return vec3();
	}();

	LightShaderSignature sl;
	sl.position    = _transform._position;
	sl.color       = _color.Value();
	sl.brightness  = _brightness;
	sl.halfAngle   = _spotAngle.x() * DEG2RAD / 2;
	sl.spotDir     = spotDirection;
	sl.attenuation = _attenuation;
	sl.range       = _range;
	return sl;
}
