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

#include "Utilities/vectormath.h"

#include <array>
#include <sstream>
#include <iomanip>

struct PointLightGPU	// 48 Bytes | 3 registers
{	
	vec3 position;
	float  range;
	vec3 color;
	float  brightness;
	vec2 attenuation;
	vec2 padding;
};

struct SpotLightGPU		// 48 bytes | 3 registers
{
	vec3 position;
	float  halfAngle;
	vec3 color;
	float  brightness;
	vec3 spotDir;
	float padding;
};

struct DirectionalLightGPU // 28(+4) Bytes | 2 registers
{
	vec3 lightDirection;
	float  brightness;
	vec3 color;
	float shadowFactor;
};

//struct ShadowView
//{
//	TextureID shadowMap = -1;
//	SamplerID shadowSampler = -1;
//	XMMATRIX  lightSpaceMatrix;
//};

// #SHADER: These defines should match the LightingCommon.hlsl

#define NUM_POINT_LIGHT 100
#define NUM_POINT_LIGHT_SHADOW 5

#define NUM_SPOT_LIGHT 20
#define NUM_SPOT_LIGHT_SHADOW 5

using PointLightDataArray					= std::array<PointLightGPU, NUM_POINT_LIGHT>;
using SpotLightDataArray					= std::array<SpotLightGPU, NUM_SPOT_LIGHT>;

using ShadowingPointLightDataArray			= std::array<PointLightGPU, NUM_POINT_LIGHT_SHADOW>;
using ShadowingSpotLightDataArray			= std::array<SpotLightGPU, NUM_SPOT_LIGHT_SHADOW>;

using SpotShadowViewArray = std::array<XMMATRIX, NUM_SPOT_LIGHT_SHADOW>;

//#pragma pack(push, 1)
struct SceneLightingData
{
	struct cb{	// shader constant buffer
		int pointLightCount;
		int spotLightCount;
		int pointLightCount_shadow;
		int spotLightCount_shadow;

		DirectionalLightGPU directionalLight;
		XMMATRIX shadowViewDirectional;

		PointLightDataArray pointLights;
		ShadowingPointLightDataArray pointLightsShadowing;

		SpotLightDataArray spotLights;
		ShadowingSpotLightDataArray spotLightsShadowing;

		SpotShadowViewArray shadowViews;
	} _cb;


	inline void ResetCounts() {
		_cb.pointLightCount = _cb.spotLightCount =
		_cb.pointLightCount_shadow = _cb.spotLightCount_shadow = 0;
	}
};
//#pragma pack(pop)

class TextRenderer;
struct TextDrawDescription;

struct RendererStats
{
	union
	{
		struct  
		{
			int numVertices;
			int numIndices;
			int numDrawCalls;
		};
		int arr[3];
	};
};
struct FrameStats
{
	static const float LINE_HEIGHT_IN_PX;
	static const size_t numStat = 6;
	union 
	{
		struct
		{
			int fps;
			int numSceneObjects;
			int numCulledObjects;
			int numDrawCalls;
			int numVertices;
			int numIndices;
		};
		struct
		{
			int fps;
			int numSceneObjects;
			int numCulledObjects;
			RendererStats rstats;
		};
		int stats[numStat];
	};

	bool bShow;
	void Render(TextRenderer* pTextRenderer, const vec2& screenPosition, const TextDrawDescription& drawDesc);	// implementation in Engine.cpp
	static const char* statNames[numStat];
};

#include "PerfTree.h"
