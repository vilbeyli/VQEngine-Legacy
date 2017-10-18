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

#include "Skybox.h"
#include "Renderer/Renderer.h"

// preset file paths (todo: read from file)
using FilePaths = std::vector<std::string>;
const  FilePaths s_filePaths = []{
	// cube face order: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
	//------------------------------------------------------------------------------------------------------
	// 0: RIGHT		1: LEFT
	// 2: UP		3: DOWN
	// 4: FRONT		5: BACK
	//------------------------------------------------------------------------------------------------------

	FilePaths paths(ESkyboxPreset::SKYBOX_PRESET_COUNT * 6);	// use as an array to access using enum
	
	// night sky by: Hazel Whorley
	paths[ESkyboxPreset::NIGHT_SKY + 0] = "night_sky/nightsky_rt.png";
	paths[ESkyboxPreset::NIGHT_SKY + 1] = "night_sky/nightsky_lf.png";
	paths[ESkyboxPreset::NIGHT_SKY + 2] = "night_sky/nightsky_up.png";
	paths[ESkyboxPreset::NIGHT_SKY + 3] = "night_sky/nightsky_dn.png";
	paths[ESkyboxPreset::NIGHT_SKY + 4] = "night_sky/nightsky_ft.png";
	paths[ESkyboxPreset::NIGHT_SKY + 5] = "night_sky/nightsky_bk.png";

	// other cubemap presets
	// ...
	return paths;
}();

std::vector<Skybox> Skybox::s_Presets(ESkyboxPreset::SKYBOX_PRESET_COUNT);

void Skybox::InitializePresets(Renderer* pRenderer)
{
	// Cubemap Skyboxes
	//------------------------------------------------------------------------------------------------------------------------------------
	{	// NIGHTSKY
		Skybox skybox;
		
		const bool bEquirectangular = false;
		const auto offsetIter = s_filePaths.begin() + ESkyboxPreset::NIGHT_SKY;
		const FilePaths filePaths = FilePaths(offsetIter, offsetIter + 6);
		
		TextureID skydomeTex = pRenderer->CreateCubemapTexture(filePaths);
		s_Presets[ESkyboxPreset::NIGHT_SKY] = skybox.Initialize(pRenderer, skydomeTex, EShaders::SKYBOX, bEquirectangular);
	}

	// HDR / IBL - Equirectangular Skyboxes
	//------------------------------------------------------------------------------------------------------------------------------------
	const std::string sIBLDirectory = Renderer::sHDRTextureRoot + std::string("sIBL/");
	const bool bEquirectangular = true;
	const EShaders shader = EShaders::SKYBOX_EQUIRECTANGULAR;

	{	// MILKYWAY 
		Skybox skybox;
		TextureID skydomeTex = pRenderer->CreateTextureFromFile("Milkyway/Milkyway_BG.jpg", sIBLDirectory);
		s_Presets[ESkyboxPreset::MILKYWAY] = skybox.Initialize(pRenderer, skydomeTex, shader, bEquirectangular);
	}

	{	// BARCELONA ROOFTOP 
		Skybox skybox;
		TextureID skydomeTex = pRenderer->CreateTextureFromFile("Barcelona_Rooftops/Barce_Rooftop_C_8k.jpg", sIBLDirectory);
		s_Presets[ESkyboxPreset::BARCELONA] = skybox.Initialize(pRenderer, skydomeTex, shader, bEquirectangular);
	}

	{	// TROPICAL BEACH
		Skybox skybox;
		TextureID skydomeTex = pRenderer->CreateTextureFromFile("Tropical_Beach/Tropical_Beach_8k.jpg", sIBLDirectory);
		s_Presets[ESkyboxPreset::TROPICAL_BEACH] = skybox.Initialize(pRenderer, skydomeTex, shader, bEquirectangular);
	}

	{	// TROPICAL RUINS
		Skybox skybox;
		TextureID skydomeTex = pRenderer->CreateTextureFromFile("Tropical_Ruins/TropicalRuins_8k.jpg", sIBLDirectory);
		s_Presets[ESkyboxPreset::TROPICAL_RUINS] = skybox.Initialize(pRenderer, skydomeTex, shader, bEquirectangular);
	}

	{	// WALK OF FAME
		Skybox skybox;
		TextureID skydomeTex = pRenderer->CreateTextureFromFile("Walk_Of_Fame/Mans_Outside_8k_TMap.jpg", sIBLDirectory);
		s_Presets[ESkyboxPreset::WALK_OF_FAME] = skybox.Initialize(pRenderer, skydomeTex, shader, bEquirectangular);
	}

	// ...
}

void Skybox::Render(const XMMATRIX& viewProj) const
{
	assert(pRenderer);	// assert we initialized the skybox object
	const XMMATRIX wvp = viewProj;

	pRenderer->BeginEvent("Skybox Pass");
	pRenderer->SetShader(skydomeShader);
	pRenderer->SetConstant4x4f("worldViewProj", wvp);
	pRenderer->SetTexture("texSkybox", skydomeTex);
	pRenderer->SetSamplerState("samWrap", EDefaultSamplerState::WRAP_SAMPLER);
	pRenderer->SetBufferObj(EGeometry::CUBE);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();
}


Skybox& Skybox::Initialize(Renderer* renderer, TextureID cubemapTexture, int shader, bool bEquirectangular)
{
	pRenderer = renderer;
	skydomeTex = cubemapTexture;
	skydomeShader = shader;
	bEquirectangular = bEquirectangular;



	return *this;
}