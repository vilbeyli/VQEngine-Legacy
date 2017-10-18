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

#include <vector>
#include "HandleTypedefs.h"

class Renderer;
struct Texture;
namespace DirectX { struct XMMATRIX; }

enum ESkyboxPreset : unsigned
{
	// Cubemaps
	NIGHT_SKY = 0,

	CUBEMAP_SKYBOX_COUNT,	// keep record of numCubemapSkyboxes

	// Equirectangular
	MILKYWAY = CUBEMAP_SKYBOX_COUNT,
	BARCELONA,
	TROPICAL_BEACH,
	TROPICAL_RUINS,
	WALK_OF_FAME,

	SKYBOX_PRESET_COUNT
};

class Skybox
{
public:
	static std::vector<Skybox> s_Presets;
	static void InitializePresets(Renderer* pRenderer);
	
	void Render(const DirectX::XMMATRIX& viewProj) const;

	Skybox& Initialize(Renderer* renderer, TextureID cubemapTexture, int shader, bool bEquirectangular);
private:
	TextureID	skydomeTex;
	bool		bEquirectangular;
	ShaderID	skydomeShader;
	Renderer*	pRenderer;
};

