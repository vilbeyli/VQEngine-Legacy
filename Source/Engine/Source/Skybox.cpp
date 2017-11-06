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

#define LOAD_ALL_ENVIRONMENT_MAPS 0

// SKYBOX PRESETS W/ CUBEMAP / ENVIRONMENT MAP
//==========================================================================================================
using FilePaths = std::vector<std::string>;
const  FilePaths s_filePaths = []{
	// cube face order: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
	//------------------------------------------------------------------------------------------------------
	// 0: RIGHT		1: LEFT
	// 2: UP		3: DOWN
	// 4: FRONT		5: BACK
	//------------------------------------------------------------------------------------------------------

	FilePaths paths(ECubeMapPresets::CUBEMAP_PRESET_COUNT * 6);	// use as an array to access using enum
	
	// night sky by: Hazel Whorley
	paths[ECubeMapPresets::NIGHT_SKY + 0] = "EnvironmentMaps/night_sky/nightsky_rt.png";
	paths[ECubeMapPresets::NIGHT_SKY + 1] = "EnvironmentMaps/night_sky/nightsky_lf.png";
	paths[ECubeMapPresets::NIGHT_SKY + 2] = "EnvironmentMaps/night_sky/nightsky_up.png";
	paths[ECubeMapPresets::NIGHT_SKY + 3] = "EnvironmentMaps/night_sky/nightsky_dn.png";
	paths[ECubeMapPresets::NIGHT_SKY + 4] = "EnvironmentMaps/night_sky/nightsky_ft.png";
	paths[ECubeMapPresets::NIGHT_SKY + 5] = "EnvironmentMaps/night_sky/nightsky_bk.png";

	// other cubemap presets
	// ...
	return paths;
}();

std::vector<Skybox> Skybox::s_Presets(ECubeMapPresets::CUBEMAP_PRESET_COUNT + EEnvironmentMapPresets::ENVIRONMENT_MAP_PRESET_COUNT);
void Skybox::InitializePresets(Renderer* pRenderer)
{
	EnvironmentMap::Initialize(pRenderer);
	EnvironmentMap::LoadShaders();
	EnvironmentMap::CalculateBRDFIntegralLUT();

	// Cubemap Skyboxes
	//------------------------------------------------------------------------------------------------------------------------------------
	{	// NIGHTSKY		
		const bool bEquirectangular = false;
		const auto offsetIter = s_filePaths.begin() + ECubeMapPresets::NIGHT_SKY;
		const FilePaths filePaths = FilePaths(offsetIter, offsetIter + 6);
		
		TextureID skydomeTex = pRenderer->CreateCubemapTexture(filePaths);
		s_Presets[ECubeMapPresets::NIGHT_SKY] = Skybox(pRenderer, skydomeTex, bEquirectangular);
	}

	// HDR / IBL - Equirectangular Skyboxes
	//------------------------------------------------------------------------------------------------------------------------------------
	//EnvironmentMap::Initialize(pRenderer);
	const std::string sIBLDirectory = Renderer::sHDRTextureRoot + std::string("sIBL/");
	const bool bEquirectangular = true;

	EnvironmentMapFileNames files;


	{	// BARCELONA ROOFTOP  
		const std::string root = sIBLDirectory + "Barcelona_Rooftops/";
		files = {
			"Barce_Rooftop_C_8k.jpg",
			"Barce_Rooftop_C_Env.hdr",
			"Barce_Rooftop_C_3k.hdr",
		};
		s_Presets[EEnvironmentMapPresets::BARCELONA] = Skybox(pRenderer, files, root, bEquirectangular);
	}

	{	// TROPICAL BEACH
		const std::string root = sIBLDirectory + "Tropical_Beach/";
		files = {
			"Tropical_Beach_8k.jpg",
			"Tropical_Beach_Env.hdr",
			"Tropical_Beach_3k.hdr",
		};
		s_Presets[EEnvironmentMapPresets::TROPICAL_BEACH] = Skybox(pRenderer, files, root, bEquirectangular);
	}

#if LOAD_ALL_ENVIRONMENT_MAPS
	{	// MILKYWAY 
		const std::string root = sIBLDirectory + "Milkyway/";
		files = {
			"Milkyway_BG.jpg",
			"Milkyway_Light.hdr",
			"Milkyway_small.hdr",
		};
		s_Presets[EEnvironmentMapPresets::MILKYWAY] = Skybox(pRenderer, files, root, bEquirectangular);
	}

	{	// TROPICAL RUINS
		const std::string root = sIBLDirectory + "Tropical_Ruins/";
		files = {
			"TropicalRuins_8k.jpg",
			"TropicalRuins_Env.hdr",
			"TropicalRuins_3k.hdr",
		};
		s_Presets[EEnvironmentMapPresets::TROPICAL_RUINS] = Skybox(pRenderer, files, root, bEquirectangular);
	}

	{	// WALK OF FAME
		const std::string root = sIBLDirectory + "Walk_Of_Fame/";
		files = {
			"Mans_Outside_8k_TMap.jpg",
			"Mans_Outside_Env.hdr",
			"Mans_Outside_2k.hdr",
		};
		s_Presets[EEnvironmentMapPresets::WALK_OF_FAME] = Skybox(pRenderer, files, root, bEquirectangular);
	}

	// ...
#endif
}



// ENVIRONMENT MAP
//==========================================================================================================

// Static Variables - BRDF LUT & Pre-filtered Environment Map
//---------------------------------------------------------------
RenderTargetID EnvironmentMap::sBRDFIntegrationLUTRT = -1;
ShaderID EnvironmentMap::sBRDFIntegrationLUTShader   = -1;
ShaderID EnvironmentMap::sPrefilterShader = -1;
Renderer* EnvironmentMap::spRenderer = nullptr;
//---------------------------------------------------------------

EnvironmentMap::EnvironmentMap() : irradianceMap(-1), specularMap(-1) {}


void EnvironmentMap::CalculateBRDFIntegralLUT()
{
	DXGI_FORMAT format = false ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_R32G32B32A32_FLOAT;

	D3D11_TEXTURE2D_DESC rtDesc = {};
	rtDesc.Width = 2048;
	rtDesc.Height = 2048;
	rtDesc.MipLevels = 1;
	rtDesc.ArraySize = 1;
	rtDesc.Format = format;
	rtDesc.Usage = D3D11_USAGE_DEFAULT;
	rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtDesc.CPUAccessFlags = 0;
	rtDesc.SampleDesc = { 1, 0 };
	rtDesc.MiscFlags = 0;

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;
	sBRDFIntegrationLUTRT = spRenderer->AddRenderTarget(rtDesc, RTVDesc);

	spRenderer->BindRenderTarget(sBRDFIntegrationLUTRT);
	spRenderer->UnbindDepthTarget();
	spRenderer->SetShader(sBRDFIntegrationLUTShader);
	spRenderer->SetViewport(2048, 2048);
	spRenderer->SetBufferObj(EGeometry::QUAD);
	spRenderer->Apply();
	spRenderer->DrawIndexed();
}


TextureID EnvironmentMap::InitializePrefilteredEnvironmentMap(const Texture & specularMap)
{
	// todo: create prefiltered env map texture mipped
	//		 create mipped render target
	//		 render directly into mips.
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476517(v=vs.85).aspx
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476244(v=vs.85).aspx

	Renderer*& pRenderer = spRenderer;
	const TextureID envMap = specularMap._id;
	//return -1;

	pRenderer->UnbindDepthTarget();
	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
	pRenderer->SetShader(sPrefilterShader);
	pRenderer->SetBufferObj(EGeometry::QUAD);
	pRenderer->SetTexture("tEnvironmentMap", envMap);
	pRenderer->SetSamplerState("sLinear", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);

	// set pre-filtered environment map texture with mips
	TextureDesc texDesc = {};
	texDesc.width = specularMap._width;
	texDesc.height = specularMap._height;
	texDesc.format = EImageFormat::RGBA32F;
	texDesc.texFileName = specularMap._name + "_preFiltered";
	texDesc.data = nullptr; // no CPU data
	texDesc.mipCount = PREFILTER_MIP_LEVEL_COUNT;
	texDesc.usage = GPU_RW;
	
	prefilteredEnvironmentMap = pRenderer->CreateTexture2D(texDesc);
	const Texture& prefilteredEnvMapTex = pRenderer->GetTextureObject(prefilteredEnvironmentMap);

	D3D11_TEXTURE2D_DESC desc = {};
	prefilteredEnvMapTex._tex2D->GetDesc(&desc);
	//desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

	Viewport viewPort = {};
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;

	unsigned textureSize[2] = { specularMap._width, specularMap._height};
	for (unsigned mipLevel = 0; mipLevel < PREFILTER_MIP_LEVEL_COUNT; ++mipLevel)
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtDesc = {};
		rtDesc.Format = desc.Format;
		rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtDesc.Texture2D.MipSlice = mipLevel;

		prefilterMipRenderTargets[mipLevel] = pRenderer->AddRenderTarget(prefilteredEnvMapTex, rtDesc);
		viewPort.Width  = static_cast<float>(textureSize[0] >> mipLevel);
		viewPort.Height = static_cast<float>(textureSize[1] >> mipLevel);

		const RenderTargetID mipTarget = prefilterMipRenderTargets[mipLevel];


		pRenderer->BindRenderTarget(mipTarget);
		pRenderer->SetViewport(viewPort);
		pRenderer->SetConstant1f("roughness", static_cast<float>(mipLevel) / (PREFILTER_MIP_LEVEL_COUNT-1));
		pRenderer->Apply();
		pRenderer->DrawIndexed();
	}

	return prefilteredEnvironmentMap;
}

void EnvironmentMap::LoadShaders()
{
	// todo: layouts from reflection?
	const std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 },
	};

	const std::vector<EShaderType> VS_PS = { EShaderType::VS, EShaderType::PS };

	const std::vector<std::string> BRDFIntegrator = { "FullscreenQuad_vs", "IntegrateBRDF_IBL_ps" };	// compute?
	const std::vector<std::string> IBLConvolution = { "FullscreenQuad_vs", "PreFilterConvolution_ps" };		// compute?

	sBRDFIntegrationLUTShader = spRenderer->AddShader("BRDFIntegrator", BRDFIntegrator, VS_PS, layout);
	sPrefilterShader = spRenderer->AddShader("PreFilterConvolution", IBLConvolution, VS_PS, layout);
}

void EnvironmentMap::Initialize(Renderer * pRenderer)
{
	spRenderer = pRenderer;
}

EnvironmentMap::EnvironmentMap(
	Renderer* pRenderer, 
	const EnvironmentMapFileNames& files,
	const std::string& rootDirectory)
{
	irradianceMap = pRenderer->CreateHDRTexture(files.irradianceMapFileName, rootDirectory);
	specularMap = pRenderer->CreateHDRTexture(files.specularMapFileName, rootDirectory);
	InitializePrefilteredEnvironmentMap(pRenderer->GetTextureObject(specularMap));
}


// SKYBOX
//==========================================================================================================
Skybox::Skybox(Renderer* renderer, const EnvironmentMapFileNames& environmentMapFiles, const std::string& rootDirectory, bool bEquirectangular)
	:
	pRenderer(renderer),
	environmentMap(EnvironmentMap(renderer, environmentMapFiles, rootDirectory)),
	skyboxTexture(pRenderer->CreateTextureFromFile(environmentMapFiles.skyboxFileName, rootDirectory)),
	skyboxShader(bEquirectangular ? EShaders::SKYBOX_EQUIRECTANGULAR : EShaders::SKYBOX)
{}

Skybox::Skybox(Renderer * renderer, const TextureID skydomeTexture, bool bEquirectangular)
	:
	pRenderer(renderer),
	environmentMap(EnvironmentMap()),
	skyboxTexture(skydomeTexture),
	skyboxShader(bEquirectangular ? EShaders::SKYBOX_EQUIRECTANGULAR : EShaders::SKYBOX)
{}

Skybox::Skybox()
	:
	pRenderer(nullptr),
	environmentMap(EnvironmentMap()),
	skyboxTexture(-1),
	skyboxShader(EShaders::SHADER_COUNT)
{}


void Skybox::Render(const XMMATRIX& viewProj) const
{
	const XMMATRIX& wvp = viewProj;
	pRenderer->BeginEvent("Skybox Pass");
	pRenderer->SetShader(skyboxShader);
	pRenderer->SetConstant4x4f("worldViewProj", wvp);
	pRenderer->SetTexture("texSkybox", skyboxTexture);
	pRenderer->SetSamplerState("samWrap", EDefaultSamplerState::WRAP_SAMPLER);
	pRenderer->SetBufferObj(EGeometry::CUBE);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();
}
