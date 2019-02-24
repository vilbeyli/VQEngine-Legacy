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

struct PointLightGPU
{	
	// 48 Bytes | 3 registers
	//-----------------------
	vec3 position;
	float  range;
	//-----------------------
	vec3 color;
	float  brightness;
	//-----------------------
	vec3 attenuation;
	float depthBias;
	//-----------------------
};

struct SpotLightGPU
{
	// 48 bytes | 3 registers
	//-----------------------
	vec3 position;
	float  halfAngle;
	//-----------------------
	vec3 color;
	float  brightness;
	//-----------------------
	vec3 spotDir;
	float depthBias;
	//-----------------------
	float innerConeAngle;
	float dummy;
	float dummy1;
	float dummy2;
};

struct DirectionalLightGPU
{
	// 28(+4) Bytes | 2 registers
	//-----------------------
	vec3 lightDirection;
	float  brightness;
	//-----------------------
	vec3 color;
	float depthBias;
	//-----------------------
	int shadowing;
	int enabled;
};

struct CylinderLightGPU
{
	vec3 position;
	float brightness;
	vec3 color;
	float radius;
	float height;
};

struct RectangleLightGPU
{
	vec3 position;
	float brightness;
	vec3 color;
	float width;
	float height;
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

using PointLightDataArray			= std::array<PointLightGPU, NUM_POINT_LIGHT>;
using SpotLightDataArray			= std::array<SpotLightGPU, NUM_SPOT_LIGHT>;

using ShadowingPointLightDataArray	= std::array<PointLightGPU, NUM_POINT_LIGHT_SHADOW>;
using ShadowingSpotLightDataArray	= std::array<SpotLightGPU, NUM_SPOT_LIGHT_SHADOW>;

using SpotShadowViewArray = std::array<XMMATRIX, NUM_SPOT_LIGHT_SHADOW>;
using PointShadowProjMatArray = std::array<XMMATRIX, NUM_POINT_LIGHT_SHADOW>;

//#pragma pack(push, 1)
struct SceneLightingData
{
	struct cb	// shader constant buffer
	{	
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
		
		CylinderLightGPU cylinderLight;
		RectangleLightGPU rectangleLight;
	} _cb;


	inline void ResetCounts() 
	{
		_cb.pointLightCount = _cb.spotLightCount =
		_cb.pointLightCount_shadow = _cb.spotLightCount_shadow = 0;
	}
};
//#pragma pack(pop)

class TextRenderer;
struct TextDrawDescription;

#define DENDER_STATS_STRUCT_ELEM_COUNT 4
#define DEFINE_RENDER_STATS_STRUCT_MEMBERS\
		int numVertices;                  \
		int numIndices;	                  \
		int numDrawCalls;                 \
		int numTriangles;                 \

struct RendererStats
{
	union
	{
		struct
		{
			DEFINE_RENDER_STATS_STRUCT_MEMBERS
		};
		int arr[DENDER_STATS_STRUCT_ELEM_COUNT];
	};
};
struct SceneStats
{
	int numObjects;
	int numSpots;
	int numPoints;

	int numMainViewCulledObjects;
	int numSpotsCulledObjects;
	int numPointsCulledObjects;
	//int numDirectionalCulledObjects;

	int numCulledShadowingPointLights;
	int numCulledShadowingSpotLights;
	//int numCulledAreaLights;

	// a few more meaningful stats to keep:
	//
	// - numCulledTrianglesMainView
	// - numCulledTrianglesShadowCubemapView
	// - numCulledTrianglesDiractionalView
	// - numRenderedTrianglesMainView
	// - numRenderedTrianglesShadowCubemapView
	// - numRenderedTrianglesDirectionalView
	// - ...

};
struct FrameStats
{
	static const size_t numStat = (sizeof(int) + sizeof(RendererStats) + sizeof(SceneStats)) / sizeof(int);
	union 
	{
		struct
		{
			int fps;
			DEFINE_RENDER_STATS_STRUCT_MEMBERS;
			SceneStats scene;
		};
		struct
		{
			int fps;
			RendererStats rstats;
			SceneStats scene;
		};
		int stats[numStat];
	};
	int operator[](const size_t i) const { assert(i < numStat); return stats[i]; }
	static const char* statNames[numStat];	// defined in UI.cpp
};

// todo: rename members with more meaningful names
struct EngineConfig
{	// can use a bit field for all this.
	bool bDeferredOrForward;
	bool bSSAO;
	bool bBloom;
	bool bRenderTargets;
	bool bBoundingBoxes;

	bool mbShowProfiler;
	bool mbShowControls = true;
};

#include "PerfTree.h"
