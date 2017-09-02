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

#include "GameObject.h"

class Renderer;

enum SKYBOX_PRESETS
{
	NIGHT_SKY = 0,

	SKYBOX_PRESET_COUNT
};

class Skybox
{
public:
	static std::vector<Skybox> s_Presets;
	static void InitializePresets(Renderer* pRenderer);
	
	void Render(const XMMATRIX& viewProj, float fovH) const;

	Skybox& Initialize(Renderer* renderer, const Texture& cubemapTexture, float scale, int shader);
private:
	GameObject	skydomeObj;	// do we really need a gameobj?
	TextureID	skydomeTex;
	ShaderID	skydomeShader;
	Renderer*	pRenderer;
};

