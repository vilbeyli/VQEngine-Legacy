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
#include "Renderer.h"

// preset file paths (todo: read from file)
using FilePaths = std::vector<std::string>;
const  FilePaths s_filePaths = []{
	// see cube face order: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
	// 0: RIGHT	 1: LEFT
	// 2: UP	 3: DOWN
	// 4: FRONT	 5: BACK
	//------------------------------------------------------------------------------------------------------

	FilePaths paths(SKYBOX_PRESETS::SKYBOX_PRESET_COUNT * 6);	// use as an array to access using enum
	
	// night sky by: Hazel Whorley
	paths[SKYBOX_PRESETS::NIGHT_SKY + 0] = "night_sky/nightsky_rt.png";
	paths[SKYBOX_PRESETS::NIGHT_SKY + 1] = "night_sky/nightsky_lf.png";
	paths[SKYBOX_PRESETS::NIGHT_SKY + 2] = "night_sky/nightsky_up.png";
	paths[SKYBOX_PRESETS::NIGHT_SKY + 3] = "night_sky/nightsky_dn.png";
	paths[SKYBOX_PRESETS::NIGHT_SKY + 4] = "night_sky/nightsky_ft.png";
	paths[SKYBOX_PRESETS::NIGHT_SKY + 5] = "night_sky/nightsky_bk.png";

	// other cubemap presets

	return paths;
}();

std::vector<Skybox> Skybox::s_Presets(SKYBOX_PRESETS::SKYBOX_PRESET_COUNT);

void Skybox::InitializePresets(Renderer* pRenderer)
{
	{
		Skybox skybox;
		
		const auto offsetIter = s_filePaths.begin() + SKYBOX_PRESETS::NIGHT_SKY;
		const FilePaths filePaths = FilePaths(offsetIter, offsetIter + 6);
		
		Texture skydomeTex = pRenderer->GetTexture(pRenderer->CreateTexture3D(filePaths));
		s_Presets[SKYBOX_PRESETS::NIGHT_SKY] = skybox.Initialize(pRenderer, skydomeTex, 1.0f, SHADERS::SKYBOX);
	}
}

void Skybox::Render(const XMMATRIX& viewProj) const
{
	const XMMATRIX wvp = skydomeObj.m_transform.WorldTransformationMatrix() * viewProj;
	pRenderer->SetShader(skydomeShader);
	pRenderer->SetConstant4x4f("worldViewProj", wvp);
	pRenderer->SetTexture("gSkybox", skydomeTex);
	pRenderer->SetBufferObj(MESH_TYPE::CUBE);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
}


Skybox& Skybox::Initialize(Renderer* renderer, const Texture& cubemapTexture, float scale, int shader)
{
	pRenderer = renderer;
	skydomeObj.m_transform.SetUniformScale(scale);
	skydomeTex = cubemapTexture.id;
	skydomeShader = shader;
	return *this;
}