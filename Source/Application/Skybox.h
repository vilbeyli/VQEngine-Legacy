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

enum ECubeMapPresets : unsigned
{
	NIGHT_SKY,

	CUBEMAP_PRESET_COUNT
};

enum EEnvironmentMapPresets : unsigned
{
	MILKYWAY = ECubeMapPresets::CUBEMAP_PRESET_COUNT,
	BARCELONA,
	TROPICAL_BEACH,
	TROPICAL_RUINS,
	WALK_OF_FAME,

	ENVIRONMENT_MAP_PRESET_COUNT = (WALK_OF_FAME - MILKYWAY + 1)
};

struct EnvironmentMap
{
	inline void Clear() { skydomeTex = irradianceMap = specularMap = -1; }
	TextureID skydomeTex	= -1;
	TextureID irradianceMap	= -1;
	TextureID specularMap	= -1;
};

class Skybox
{
public:
	static std::vector<Skybox> s_Presets;
	static void InitializePresets(Renderer* pRenderer);
	
	void Render(const DirectX::XMMATRIX& viewProj) const;

	Skybox& Initialize(Renderer* renderer, const EnvironmentMap& environmentMap, int shader, bool bEquirectangular);
	Skybox& Initialize(Renderer* renderer, const TextureID skydomeTexture, int shader, bool bEquirectangular);
	inline const EnvironmentMap& GetEnvironmentMap() const { return environmentMap; }
private:
	bool			bEquirectangular;
	ShaderID		skydomeShader;
	Renderer*		pRenderer;
	EnvironmentMap	environmentMap;
};

