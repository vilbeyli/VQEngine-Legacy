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

#include "Application/HandleTypedefs.h"

#include <vector>
#include <array>
#include <mutex>

class Renderer;
struct Texture;
namespace DirectX { struct XMMATRIX; }
namespace Settings { struct Rendering; }

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
	std::string environmentMapFileName;
	std::string settingsFileName;
};

constexpr size_t PREFILTER_MIP_LEVEL_COUNT = 8;
using MipRenderTargets = std::array<RenderTargetID, PREFILTER_MIP_LEVEL_COUNT>;

struct sIBLSettings
{
	float gamma;
	float exposure;
	size_t environmentMapSize;
};

class EnvironmentMap
{
public:
	//--------------------------------------------------------
	// STATIC INTERFACE
	//--------------------------------------------------------
	static Renderer* spRenderer;

	// learnopengl:	https://learnopengl.com/#!PBR/IBL/Specular-IBL
	// stores the lookup table for BRDF's response given an input 
	// roughness and an input angle between normal and view (N dot V)
	static RenderTargetID		sBRDFIntegrationLUTRT;
	static ShaderID				sBRDFIntegrationLUTShader;
	static void CalculateBRDFIntegralLUT();

	// renders pre-filtered environment map texture into mip levels 
	// with the convolution being based on the roughness
	static ShaderID				sPrefilterShader;
	static ShaderID				sRenderIntoCubemapShader;
	
	static void Initialize(Renderer* pRenderer);
	static void LoadShaders();

	//--------------------------------------------------------
	// MEMBER INTERFACE
	//--------------------------------------------------------
	EnvironmentMap(Renderer* pRenderer, const EnvironmentMapFileNames& files, const std::string& rootDirectory);
	EnvironmentMap();
	TextureID InitializePrefilteredEnvironmentMap(const Texture& specularMap, const Texture& irradienceMap);

	//--------------------------------------------------------
	// DATA
	//--------------------------------------------------------
	TextureID irradianceMap;
	TextureID environmentMap; 

	TextureID mippedEnvironmentCubemap;
	TextureID prefilteredEnvironmentMap;

	SamplerID envMapSampler;
	sIBLSettings settings;
};

class Skybox
{
public:
	//--------------------------------------------------------
	// STATIC INTERFACE
	//--------------------------------------------------------
	static void InitializePresets(Renderer* pRenderer, const Settings::Rendering& renderSettings);
	static void InitializePresets_Async(Renderer* pRenderer, const Settings::Rendering& renderSettings);
	static std::vector<Skybox> s_Presets;
	
	//--------------------------------------------------------
	// MEMBER INTERFACE
	//--------------------------------------------------------
	inline const EnvironmentMap& GetEnvironmentMap() const { return environmentMap; }
	inline const TextureID GetSkyboxTexture() const{ return skyboxTexture; }
	void Render(const DirectX::XMMATRIX& viewProj) const;

	// initializes a skybox with environment map data (used in image-based lighting)
	//
	Skybox(Renderer* renderer, bool bEquirectangular);
	bool Initialize(const EnvironmentMapFileNames& environmentMapFiles, const std::string& rootDirectory);
	
	// initializes a skybox with no environment map (no image-based lighting)
	//
	Skybox(Renderer* renderer, const TextureID skydomeTexture, bool bEquirectangular);

	Skybox();

private:
	//--------------------------------------------------------
	// DATA
	//--------------------------------------------------------
	Renderer*		pRenderer;
	TextureID		skyboxTexture;
	ShaderID		skyboxShader;
	EnvironmentMap	environmentMap;
};

