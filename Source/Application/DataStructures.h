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

#include "Utilities/utils.h"
#include "HandleTypedefs.h"

#include <array>

constexpr int MAX_POINT_LIGHT_COUNT = 20;
constexpr int MAX_SPOT_LIGHT_COUNT = 20;



struct LightShaderSignature
{
	vec3 position;
	float pad1;
	vec3 color;
	float brightness;

	vec3 spotDir;
	float halfAngle;

	vec2 attenuation;
	float range;
	float pad3;
};

struct ShadowView
{
	TextureID shadowMap = -1;
	SamplerID shadowSampler = -1;
	XMMATRIX  lightSpaceMatrix;
};

using LightDataArray = std::array<LightShaderSignature, MAX_POINT_LIGHT_COUNT>;
using ShadowViewArray = std::array<ShadowView, MAX_SPOT_LIGHT_COUNT>;

struct SceneLightData
{
	struct cb{
	int       pointLightCount;
	int        spotLightCount;
	int directionalLightCount;

	int       pointLightCount_shadow;
	int        spotLightCount_shadow;
	int directionalLightCount_shadow;
	int _pad0, _pad1;

	LightDataArray pointLights;
	LightDataArray pointLightsShadowing;
	// todo directional

	LightDataArray spotLights;
	LightDataArray spotLightsShadowing;
	// todo directional
	} _cb;

	ShadowViewArray shadowView;

	inline void ResetCounts() {
		_cb.pointLightCount = _cb.spotLightCount = _cb.directionalLightCount =
		_cb.pointLightCount_shadow = _cb.spotLightCount_shadow = _cb.directionalLightCount_shadow = 0;
	}
};