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

#pragma once


#include "Engine/Transform.h"
#include "Application/DataStructures.h"
#include "Utilities/Color.h"

#include <DirectXMath.h>

using InputBufferID = int; // warning: duplicate definition

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
	Light(ELightType type, 
			LinearColor color, 
			float range,
			float brightness,
			float spotAngle,
			bool castsShadows = false);
	Light(const Light& l);
	~Light();

	void SetLightRange(float range);
	XMMATRIX GetLightSpaceMatrix() const;
	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetProjectionMatrix() const;
	LightShaderSignature ShaderSignature() const;

	//---------------------------------------------------------------------------------
	
	ELightType		_type;
	LinearColor			_color;
	float			_range;
	float			_brightness;	// 300.0f is a good default value
	bool			_castsShadow;

	union 
	{	// point light attributes
		vec2		_attenuation;
		vec2		_spotAngle;	// uses only X channel of the vec2
	};	

	Transform		_transform;
	InputBufferID	_renderMesh;
};



