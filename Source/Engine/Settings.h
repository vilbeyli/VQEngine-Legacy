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

#include <vector>
#include <string>

namespace Settings
{
	//------------------------------------------------------------
	// RENDERING
	//------------------------------------------------------------
	struct ShadowMap
	{
		size_t	dimension;
	};

	struct Bloom
	{
		bool bEnabled;
		float brightnessThreshold;
		int blurStrength = 1;
	};

	struct PostProcess
	{
		struct Tonemapping
		{
			float	exposure;
		} toneMapping;

		Bloom bloom;

		bool HDREnabled = false;
	};

	struct SSAO
	{
		bool bEnabled;
		float ambientFactor;
		float radius;
		float intensity;
	};

	struct Rendering
	{
		ShadowMap	shadowMap;
		PostProcess postProcess;
		bool		bUseDeferredRendering;
		bool		bUseBRDFLighting = true;	// should use enums when there's more than brdf and blinn-phong lighting
		bool		bAmbientOcclusion;
		bool		bEnableEnvironmentLighting;
		bool		bPreLoadEnvironmentMaps;
	};


	//------------------------------------------------------------
	// ENGINE
	//------------------------------------------------------------
	struct Logger
	{
		bool bConsole;
		bool bFile;
	};
	struct Window
	{
		int width;
		int height;
		int fullscreen;
		int vsync;
	};
	struct Camera
	{
		union
		{
			float fovH;
			float fovV;
		};
		float nearPlane;
		float farPlane;
		float aspect;
		float x, y, z;
		float yaw, pitch;
	};
	struct Engine 
	{
		Logger logger;
		Window window;
		Rendering rendering;
		int levelToLoad;
		std::vector<std::string> sceneNames;

		// caching is slower... keeping this false.
		// it can be useful: environment map textures can be dumped on disk.
		bool bCacheEnvironmentMapsOnDisk = false;
	};


	//------------------------------------------------------------
	// SCENE
	//------------------------------------------------------------
	struct Optimization
	{
		bool bViewFrustumCull_MainView = true;
		bool bViewFrustumCull_LocalLights = true;
		bool bShadowViewCull = false;	// not implemented yet
		bool bSortRenderLists = true;
	};
	struct SceneRender
	{
		SSAO ssao;
		bool bSkylightEnabled;			// ambient environment map lighting
		Bloom bloom;

		Optimization optimization;
	};
};