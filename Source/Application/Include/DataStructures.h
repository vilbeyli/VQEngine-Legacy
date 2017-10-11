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

#include "utils.h"
#include "HandleTypedefs.h"

#include <array>

constexpr int MAX_POINT_LIGHT_COUNT = 20;
constexpr int MAX_SPOT_LIGHT_COUNT = 10;



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

struct ShadowCasterData
{
	TextureID shadowMap = -1;
	SamplerID shadowSampler = -1;
	XMMATRIX  lightSpaceMatrix;
};

using LightDataArray = std::array<LightShaderSignature, MAX_POINT_LIGHT_COUNT>;
using ShadowCasterDataArray = std::array<ShadowCasterData, MAX_SPOT_LIGHT_COUNT>;
struct SceneLightData
{
	size_t pointLightCount;
	LightDataArray pointLights;

	size_t spotLightCount;
	LightDataArray spotLights;

	ShadowCasterDataArray shadowCasterData;

	inline void ResetCounts() { pointLightCount = 0; spotLightCount = 0; /*todo: directional light count*/ }
};