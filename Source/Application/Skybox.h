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

struct EnvironmentMapFileNames
{
	std::string skyboxFileName;
	std::string irradianceMapFileName;
	std::string specularMapFileName;
};

class EnvironmentMap
{
public:
	// learnopengl:	https://learnopengl.com/#!PBR/IBL/Specular-IBL
	// stores the lookup table for BRDF's response given an input 
	// roughness and an input angle between normal and view (N dot V)
	static RenderTargetID		BRDFIntegrationLUTRT;
	static ShaderID				BRDFIntegrationLUTShader;
	static void Initialize(Renderer* pRenderer);	// renders BRDFIntegrationLUT

	EnvironmentMap(Renderer* pRenderer, const EnvironmentMapFileNames& files, const std::string& rootDirectory);
	EnvironmentMap();
	inline void Clear() { irradianceMap = specularMap = -1; }

	TextureID irradianceMap;
	TextureID specularMap; 
};

class Skybox
{
public:
	static void InitializePresets(Renderer* pRenderer);

	static std::vector<Skybox> s_Presets;
	
public:
	inline const EnvironmentMap& GetEnvironmentMap() const { return environmentMap; }
	inline const TextureID GetSkyboxTexture() const{ return skyboxTexture; }

	void Render(const DirectX::XMMATRIX& viewProj) const;

	// initializes a skybox with environment map data (used in image-based lighting)
	Skybox(Renderer* renderer, const EnvironmentMapFileNames& environmentMapFiles, const std::string& rootDirectory, bool bEquirectangular);
	
	// initializes a skybox with no environment map (no image-based lighting)
	Skybox(Renderer* renderer, const TextureID skydomeTexture, bool bEquirectangular);

	Skybox();

private:
	Renderer*		pRenderer;
	TextureID		skyboxTexture;
	ShaderID		skyboxShader;
	EnvironmentMap	environmentMap;
};

