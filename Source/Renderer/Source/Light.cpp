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
#include "GameObject.h"
#include "Components/Transform.h"

#include <iostream>

using std::cout;
using std::endl;
using std::string;

//			 range(distance)   -   (Linear, Quadratic) attenuation factor map
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

Light::Light()
	:
	lightType_(LightType::POINT),
	color_(Color::white),
	range_(50),
	brightness_(1.0f),
	spotAngle_(0.0f)
{
	SetLightRange(range_);
}

Light::Light(LightType type, Color color, float range, float brightness, float spotAngle) 
	:
	lightType_(type),
	color_(color),
	range_(range),
	brightness_(brightness),
	spotAngle_(spotAngle)
{
	SetLightRange(range_);
}

Light::~Light()
{}

void Light::SetLightRange(float range)
{	
	range_ = range;
	// ATTENUATION LOOKUP FOR POINT LIGHTS
	// find the first greater or equal range value (rangeIndex) 
	// to look up with in the attenuation map
	float ranges[] = { 7, 13, 20, 32, 50, 65, 100, 160, 200, 325, 600, 3250 };
	unsigned rangeIndex = static_cast<unsigned>(ranges[rangeAttenuationMap_.size() - 1]);	// default case = largest range
	for (size_t i = 0; i < rangeAttenuationMap_.size(); i++)
	{
		if (ranges[i] >= range_)
		{
			rangeIndex = static_cast<unsigned>(ranges[i]);
			break;
		}
	}

	std::pair<float, float> attn = rangeAttenuationMap_.at(rangeIndex);
	attenuation_ = XMFLOAT2(attn.first, attn.second);
}

XMMATRIX Light::GetLightSpaceMatrix() const
{
	XMMATRIX LSpaceMat = XMMatrixIdentity();
	switch (lightType_)
	{
	case Light::POINT:
		break;
	case Light::SPOT:
	{
		XMVECTOR pos = tf.GetPositionV();
		XMMATRIX view = GetViewMatrix();
		XMMATRIX proj = XMMatrixPerspectiveFovLH((spotAngle_* 1.25f) * DEG2RAD, 1.0f, 0.1f, 100.0f);
		LSpaceMat = proj * view;
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
	XMMATRIX ViewMatarix = XMMatrixIdentity();
	switch (lightType_)
	{
	case Light::POINT:
		break;
	case Light::SPOT:
	{
		XMVECTOR up		= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMVECTOR lookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		lookAt	= XMVector3TransformCoord(lookAt, tf.RotationMatrix());
		up		= XMVector3TransformCoord(up,	  tf.RotationMatrix());
		XMVECTOR pos = tf.GetPositionV();
		XMVECTOR taraget = pos + lookAt;
		ViewMatarix = XMMatrixLookAtLH(pos, taraget, up);
		break;
	}	

	default:
		std::cout << "INVALID LIGHT TYPE for GetViewMatrix()" << std::endl;
		break;
	}

	return ViewMatarix;
}

XMMATRIX Light::GetProjectionMatrix() const
{
	XMMATRIX proj = XMMatrixIdentity();
	switch (lightType_)
	{
	case Light::POINT:
		break;
	case Light::SPOT:
	{
		//proj = glm::perspective(glm::radians(spotAngle_* 1.25f), 1.0f, 0.1f, 100.0f);
	}	break;

	default:
		std::cout << "INVALID LIGHT TYPE for GetProjectionMatrix()" << std::endl;
		break;
	}

	return proj;
}



ShaderLight Light::ShaderLightStruct() const
{
	XMFLOAT3 spotDirection = XMFLOAT3();
	if (lightType_ == SPOT)
	{
		XMVECTOR up = XMVectorSet(0, 1, 0, 0);
		up = XMVector3TransformCoord(up, tf.RotationMatrix());
		XMStoreFloat3(&spotDirection, up);
	}

	ShaderLight sl;
	sl.position = tf.GetPositionF3();
	sl.color = color_.Value();
	sl.brightness = brightness_;
	sl.halfAngle = spotAngle_ * DEG2RAD / 2;
	sl.spotDir = spotDirection;
	sl.attenuation = attenuation_;
	sl.range = range_;
	return sl;
}
