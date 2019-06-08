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

#include "Skybox.h"
#include "Engine.h"
#include "Scene.h"
#include "SceneResourceView.h"

#include "Application/Application.h"

#include "Renderer/Renderer.h"

#include "Utilities/Log.h"

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
	const std::string root = Renderer::sTextureRoot;
	paths[ECubeMapPresets::NIGHT_SKY + 0] = root + "EnvironmentMaps/night_sky/nightsky_rt.png";
	paths[ECubeMapPresets::NIGHT_SKY + 1] = root + "EnvironmentMaps/night_sky/nightsky_lf.png";
	paths[ECubeMapPresets::NIGHT_SKY + 2] = root + "EnvironmentMaps/night_sky/nightsky_up.png";
	paths[ECubeMapPresets::NIGHT_SKY + 3] = root + "EnvironmentMaps/night_sky/nightsky_dn.png";
	paths[ECubeMapPresets::NIGHT_SKY + 4] = root + "EnvironmentMaps/night_sky/nightsky_ft.png";
	paths[ECubeMapPresets::NIGHT_SKY + 5] = root + "EnvironmentMaps/night_sky/nightsky_bk.png";

	// other cubemap presets
	// ...
	return paths;
}();

std::vector<Skybox> Skybox::s_Presets(ECubeMapPresets::CUBEMAP_PRESET_COUNT + EEnvironmentMapPresets::ENVIRONMENT_MAP_PRESET_COUNT);
std::pair < std::string, EnvironmentMapFileNames>  GetsIBLFiles(EEnvironmentMapPresets preset)
{
	const std::string sIBLDirectory = Renderer::sHDRTextureRoot + std::string("sIBL/");
	EnvironmentMapFileNames files;
	std::string root;
	switch (preset)
	{
	case MILKYWAY:
		root = sIBLDirectory + "Milkyway/";
		files = {
			"Milkyway_BG.jpg",
			"Milkyway_Light.hdr",
			"Milkyway_small.hdr",
		};
		break;
	case BARCELONA:
		root = sIBLDirectory + "Barcelona_Rooftops/";
		files = {
			"Barce_Rooftop_C_8k.jpg",
			"Barce_Rooftop_C_Env.hdr",
			"Barce_Rooftop_C_3k.hdr",
		};
		break;
	case TROPICAL_BEACH:
		root = sIBLDirectory + "Tropical_Beach/";
		files = {
			"Tropical_Beach_8k.jpg",
			"Tropical_Beach_Env.hdr",
			"Tropical_Beach_3k.hdr",
		};
		break;
	case TROPICAL_RUINS:
		root = sIBLDirectory + "Tropical_Ruins/";
		files = {
			"TropicalRuins_8k.jpg",
			"TropicalRuins_Env.hdr",
			"TropicalRuins_3k.hdr",
		};
		break;
	case WALK_OF_FAME:
		root = sIBLDirectory + "Walk_Of_Fame/";
		files = {
			"Mans_Outside_8k_TMap.jpg",
			"Mans_Outside_Env.hdr",
			"Mans_Outside_2k.hdr",
		};
		break;
	default:
		Log::Warning("Unknown Environment Map Preset");
		break;
	}
	return std::make_pair(root, files);
}

void Skybox::InitializePresets_Async(Renderer* pRenderer, const Settings::Rendering& renderSettings)
{
	EnvironmentMap::Initialize(pRenderer);
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		EnvironmentMap::LoadShaders();
	}
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		Texture LUTTexture = EnvironmentMap::CreateBRDFIntegralLUTTexture();
		EnvironmentMap::sBRDFIntegrationLUTTexture = LUTTexture._id;
	}

	// Cubemap Skyboxes
	//------------------------------------------------------------------------------------------------------------------------------------
	{	// NIGHTSKY		
		// #AsyncLoad: Mutex DEVICE

		const bool bEquirectangular = false;
		const auto offsetIter = s_filePaths.begin() + ECubeMapPresets::NIGHT_SKY;
		const FilePaths filePaths = FilePaths(offsetIter, offsetIter + 6);

		TextureID skydomeTex = -1;
		{
			std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
			skydomeTex = pRenderer->CreateCubemapFromFaceTextures(filePaths, false);
		}
		s_Presets[ECubeMapPresets::NIGHT_SKY] = Skybox(pRenderer, skydomeTex, bEquirectangular);
	}

	if (renderSettings.bEnableEnvironmentLighting)
	{
		// HDR / IBL - Equirectangular Skyboxes
		//------------------------------------------------------------------------------------------------------------------------------------
		//EnvironmentMap::Initialize(pRenderer);
		const std::string sIBLDirectory = Renderer::sHDRTextureRoot + std::string("sIBL/");
		const bool bEquirectangular = true;

		EnvironmentMapFileNames files;

		const std::vector<EEnvironmentMapPresets> presets =
		{
			EEnvironmentMapPresets::BARCELONA     ,
			EEnvironmentMapPresets::TROPICAL_BEACH,
			EEnvironmentMapPresets::MILKYWAY	  ,
			EEnvironmentMapPresets::TROPICAL_RUINS,
			EEnvironmentMapPresets::WALK_OF_FAME
		};

		if (renderSettings.bPreLoadEnvironmentMaps)
		{
			std::for_each(RANGE(presets), [&](auto preset)
			{
				const auto rootAndFilesPair = GetsIBLFiles(preset);
				{
					s_Presets[preset] = Skybox(pRenderer, bEquirectangular);
					s_Presets[preset].Initialize(rootAndFilesPair.second, rootAndFilesPair.first);
				}
			});
		}
		else
		{
			auto& preset = presets.back();
			const auto rootAndFilesPair = GetsIBLFiles(preset);
			{
				s_Presets[preset] = Skybox(pRenderer, bEquirectangular);
				s_Presets[preset].Initialize(rootAndFilesPair.second, rootAndFilesPair.first);
			}
		}
	}
}

void Skybox::InitializePresets(Renderer* pRenderer, const Settings::Rendering& renderSettings)
{
	EnvironmentMap::Initialize(pRenderer);
	EnvironmentMap::LoadShaders();
	
	const std::string extension = ".hdr";
	const std::string BRDFLUTTextureFileName = "BRDFIntegrationLUT";
	const std::string BRDFLUTTextureFilePath = EnvironmentMap::sTextureCacheDirectory + BRDFLUTTextureFileName + extension;
	
	const bool bUseCache = Engine::GetSettings().bCacheEnvironmentMapsOnDisk && DirectoryUtil::FileExists(BRDFLUTTextureFilePath);

	if(bUseCache)
	{ 
		EnvironmentMap::sBRDFIntegrationLUTTexture = pRenderer->CreateHDRTexture(BRDFLUTTextureFileName + extension, EnvironmentMap::sTextureCacheDirectory);
	}
	else
	{
		Texture LUTTexture = EnvironmentMap::CreateBRDFIntegralLUTTexture();
		EnvironmentMap::sBRDFIntegrationLUTTexture = LUTTexture._id;

		if (Engine::GetSettings().bCacheEnvironmentMapsOnDisk)
		{
			pRenderer->SaveTextureToDisk(EnvironmentMap::sBRDFIntegrationLUTTexture, BRDFLUTTextureFilePath, false);
		}

		// todo: we can unload shaders / render targets here
	}


	// Cubemap Skyboxes
	//------------------------------------------------------------------------------------------------------------------------------------
	{	// NIGHTSKY		
		// #AsyncLoad: Mutex DEVICE

		const bool bEquirectangular = false;
		const auto offsetIter = s_filePaths.begin() + ECubeMapPresets::NIGHT_SKY;
		const FilePaths filePaths = FilePaths(offsetIter, offsetIter + 6);
		
		TextureID skydomeTex = pRenderer->CreateCubemapFromFaceTextures(filePaths, false);
		//Skybox::SetPreset(ECubeMapPresets::NIGHT_SKY, std::move(Skybox(pRenderer, skydomeTex, bEquirectangular)));
		s_Presets[ECubeMapPresets::NIGHT_SKY] = Skybox(pRenderer, skydomeTex, bEquirectangular);
	}
	
	if (renderSettings.bEnableEnvironmentLighting)
	{
		// HDR / IBL - Equirectangular Skyboxes
		//------------------------------------------------------------------------------------------------------------------------------------
		//EnvironmentMap::Initialize(pRenderer);
		
		const bool bEquirectangular = true;

		EnvironmentMapFileNames files;

		const std::vector<EEnvironmentMapPresets> presets = 
		{
			EEnvironmentMapPresets::BARCELONA     ,
			EEnvironmentMapPresets::TROPICAL_BEACH,
			EEnvironmentMapPresets::MILKYWAY	  ,
			EEnvironmentMapPresets::TROPICAL_RUINS,
			EEnvironmentMapPresets::WALK_OF_FAME
		};
		std::for_each(RANGE(presets), [&](auto preset)
		{
			const auto rootAndFilesPair = GetsIBLFiles(preset);
			s_Presets[preset] = Skybox(pRenderer, bEquirectangular);
			s_Presets[preset].Initialize(rootAndFilesPair.second, rootAndFilesPair.first);
		});
	}
}


// SKYBOX
//==========================================================================================================
Skybox::Skybox(Renderer* renderer, bool bEquirectangular)
	:
	pRenderer(renderer),
	environmentMap(EnvironmentMap()),
	skyboxTexture(-1),
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

bool Skybox::Initialize(const EnvironmentMapFileNames& environmentMapFiles, const std::string& rootDirectory)
{
	environmentMap.Initialize(pRenderer, environmentMapFiles, rootDirectory);
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		skyboxTexture = pRenderer->CreateTextureFromFile(environmentMapFiles.skyboxFileName, rootDirectory);
	}
	return skyboxTexture != -1;
}

// ENVIRONMENT MAP
//==========================================================================================================

// Static Variables - BRDF LUT & Pre-filtered Environment Map
//---------------------------------------------------------------
RenderTargetID EnvironmentMap::sBRDFIntegrationLUTRT = -1;
TextureID EnvironmentMap::sBRDFIntegrationLUTTexture = -1;
ShaderID EnvironmentMap::sBRDFIntegrationLUTShader   = -1;
ShaderID EnvironmentMap::sPrefilterShader = -1;
ShaderID EnvironmentMap::sRenderIntoCubemapShader = -1;
Renderer* EnvironmentMap::spRenderer = nullptr;
std::string EnvironmentMap::sTextureCacheDirectory = "";
//---------------------------------------------------------------

EnvironmentMap::EnvironmentMap() : irradianceMap(-1), environmentMap(-1) {}


void EnvironmentMap::Initialize(Renderer * pRenderer, const EnvironmentMapFileNames & files, const std::string & rootDirectory)
{
	const std::string envMapName = StrUtil::split(rootDirectory, '/').back();
	const std::string cacheFolderPath = sTextureCacheDirectory + "sIBL/" + envMapName + "/";
	const std::string extensionEnvMap = "." + DirectoryUtil::GetFileExtension(files.environmentMapFileName);
	const std::string extensionIrrMap = "." + DirectoryUtil::GetFileExtension(files.irradianceMapFileName);


	// input textures for pre-filtered environment map calculation
	const std::string irradianceTextureFilePath = rootDirectory + files.irradianceMapFileName;
	const std::string skyboxTextureFilePath = rootDirectory + files.environmentMapFileName;

	// cached mipped-cubemap version of the textures (pick one of them at random -> 0th element 0th mip for example)
	const std::string cachedPrefilteredEnvMap = cacheFolderPath + DirectoryUtil::GetFileNameWithoutExtension(files.irradianceMapFileName) + "_preFiltered_mip0_0" + extensionIrrMap;

	Log::Info("\tLoading Environment Map: %s", envMapName.c_str());

	// get the latest date of the environment map and irradiance map input textures
	// compare them with the corresponding cached textures
	//
	// NOTE: Using the cache is ~10-20% slower. GPU wins. Keeping it off with the override for now.
	//		see: https://github.com/vilbeyli/VQEngine/issues/106
	//
	constexpr bool USE_CACHE = false;
	const bool bCacheEnabled = Engine::GetSettings().bCacheEnvironmentMapsOnDisk && USE_CACHE;
	const bool bCacheExists  = bCacheEnabled && DirectoryUtil::FileExists(cachedPrefilteredEnvMap);
	const bool bUseCache     = bCacheExists && DirectoryUtil::IsFileNewer(cachedPrefilteredEnvMap, irradianceTextureFilePath);

	// irradiance map texture
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		this->irradianceMap = pRenderer->CreateHDRTexture(files.irradianceMapFileName, rootDirectory);
	}
	
	if (bUseCache)
	{
		// load pre-filtered & mipped environment map maps
		std::vector<std::string> cubemapTexturePaths;
		for (int mip = 0; mip < PREFILTER_MIP_LEVEL_COUNT; ++mip)
		{
			for (int i = 0; i < 6; ++i)
			{
				std::string cubeMapFaceTexturePath = cacheFolderPath
					+ DirectoryUtil::GetFileNameWithoutExtension(files.irradianceMapFileName)
					+ "_preFiltered_mip" + std::to_string(mip) + "_" + std::to_string(i)
					+ extensionIrrMap;;
				cubemapTexturePaths.push_back(cubeMapFaceTexturePath);
			}
		}
		{
			std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
			this->prefilteredEnvironmentMap = pRenderer->CreateCubemapFromFaceTextures(cubemapTexturePaths, true, PREFILTER_MIP_LEVEL_COUNT);
			this->environmentMap = this->prefilteredEnvironmentMap;
		}
	}
	else
	{
		{
			std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
			this->environmentMap = pRenderer->CreateHDRTexture(files.environmentMapFileName, rootDirectory);
		}
		InitializePrefilteredEnvironmentMap(pRenderer->GetTextureObject(environmentMap), pRenderer->GetTextureObject(irradianceMap), cacheFolderPath);
	}


	D3D11_SAMPLER_DESC envMapSamplerDesc = {};
	envMapSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	envMapSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	envMapSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	envMapSamplerDesc.MipLODBias = 0;
	envMapSamplerDesc.MinLOD = 0;
	envMapSamplerDesc.MaxLOD = PREFILTER_MIP_LEVEL_COUNT - 1;
	envMapSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	envMapSamplerDesc.MaxAnisotropy = 1;
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		this->envMapSampler = spRenderer->CreateSamplerState(envMapSamplerDesc);
	}

}

void EnvironmentMap::Initialize(Renderer * pRenderer)
{
	spRenderer = pRenderer;

	// create texture cache folders if they don't already exist
	EnvironmentMap::sTextureCacheDirectory = Application::s_WorkspaceDirectory + "/TextureCache/";
	DirectoryUtil::CreateFolderIfItDoesntExist(EnvironmentMap::sTextureCacheDirectory);
	DirectoryUtil::CreateFolderIfItDoesntExist(EnvironmentMap::sTextureCacheDirectory + "/sIBL/");
}

void EnvironmentMap::LoadShaders()
{
	const char* pSkyboxVS = "Skybox_vs.hlsl";

	const ShaderDesc BRDFIntegratorShaderDesc = { "BRDFIntegrator", 
		ShaderStageDesc{"FullscreenQuad_vs.hlsl", {}},
		ShaderStageDesc{"IntegrateBRDF_IBL_ps.hlsl", {}},
	};
	const ShaderDesc brdfIntegrationShaderDesc = { "PreFilterConvolution",
		ShaderStageDesc{pSkyboxVS, {}},
		ShaderStageDesc{"PreFilterConvolution_ps.hlsl", {}},
	};
	const ShaderDesc cubemapShaderDesc = { "RenderIntoCubemap",
		ShaderStageDesc{pSkyboxVS, {}},
		ShaderStageDesc{"RenderIntoCubemap_ps.hlsl", {}},
	};

	// #AsyncLoad: Mutex DEVICE
	sPrefilterShader          = spRenderer->CreateShader(brdfIntegrationShaderDesc);
	sBRDFIntegrationLUTShader = spRenderer->CreateShader(BRDFIntegratorShaderDesc);
	sRenderIntoCubemapShader  = spRenderer->CreateShader(cubemapShaderDesc);
}

Texture EnvironmentMap::CreateBRDFIntegralLUTTexture()
{
	constexpr int TEXTURE_DIMENSION = 2048;
	
	// create the render target
	RenderTargetDesc rtDesc = {};
	rtDesc.format = RGBA32F;
	rtDesc.textureDesc.width = TEXTURE_DIMENSION;
	rtDesc.textureDesc.height = TEXTURE_DIMENSION;
	rtDesc.textureDesc.mipCount = 1;
	rtDesc.textureDesc.arraySize = 1;
	rtDesc.textureDesc.format = RGBA32F;
	rtDesc.textureDesc.usage = ETextureUsage::RENDER_TARGET_RW;
	rtDesc.textureDesc.bGenerateMips = false;
	rtDesc.textureDesc.cpuAccessMode = ECPUAccess::NONE;
	sBRDFIntegrationLUTRT = spRenderer->AddRenderTarget(rtDesc);

	// render the lookup table
	const auto IABuffers = SceneResourceView::GetBuiltinMeshVertexAndIndexBufferID(EGeometry::FULLSCREENQUAD);
	spRenderer->BindRenderTarget(sBRDFIntegrationLUTRT);
	spRenderer->UnbindDepthTarget();
	spRenderer->SetShader(sBRDFIntegrationLUTShader);
	spRenderer->SetViewport(TEXTURE_DIMENSION, TEXTURE_DIMENSION);
	spRenderer->SetVertexBuffer(IABuffers.first);
	spRenderer->SetIndexBuffer(IABuffers.second);
	spRenderer->Apply();
	spRenderer->DrawIndexed();
	spRenderer->m_deviceContext->Flush();
	return spRenderer->GetTextureObject(spRenderer->GetRenderTargetTexture(sBRDFIntegrationLUTRT));
}

TextureID EnvironmentMap::InitializePrefilteredEnvironmentMap(const Texture& specularMap_, const Texture& irradienceMap_, const std::string& cacheFolderPath)
{
	//---------------------------------------------------
	// Quick Bug Fix
	//---------------------------------------------------
	// Copy the Texture Objects to a local variable
	const Texture specularMap = specularMap_;
	const Texture irradienceMap = irradienceMap_;
	// As we're adding new textures to the renderer
	// the references are getting invalidated. hence keep a copy here.
	// this should be avoided with a better architecture design...
	//---------------------------------------------------

	Renderer*& pRenderer = spRenderer;
	const TextureID envMap = specularMap._id;

	const float screenAspect = 1;
	const float screenNear = 0.01f;
	const float screenFar = 10.0f;
	const float fovy = 90.0f * DEG2RAD;
	const XMMATRIX proj = XMMatrixPerspectiveFovLH(fovy, screenAspect, screenNear, screenFar);

	// render cubemap version of the environment map, generate mips, and then bind to pre-filter stage 
	// cube face order: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
	//------------------------------------------------------------------------------------------------------
	// 0: RIGHT		1: LEFT
	// 2: UP		3: DOWN
	// 4: FRONT		5: BACK
	//------------------------------------------------------------------------------------------------------
	// cubemap look and up directions for left-handed view


	// create environment map cubemap texture
	const unsigned cubemapDimension = specularMap._height / 2;
	const unsigned textureSize[2] = { cubemapDimension, cubemapDimension };
	const std::string textureFileExtension = StrUtil::split(specularMap._name).back();


	TextureDesc texDesc = {};
	texDesc.width = cubemapDimension;
	texDesc.height = cubemapDimension;
	texDesc.format = EImageFormat::RGBA32F;
	texDesc.pData = nullptr; // no CPU data
	texDesc.mipCount = PREFILTER_MIP_LEVEL_COUNT;
	texDesc.usage = RENDER_TARGET_RW;
	texDesc.bIsCubeMap = true;
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		texDesc.texFileName = DirectoryUtil::GetFileNameWithoutExtension(irradienceMap._name) + "_preFiltered";
		this->prefilteredEnvironmentMap = pRenderer->CreateTexture2D(texDesc);
	}

	texDesc.bGenerateMips = true;
	texDesc.mipCount = PREFILTER_MIP_LEVEL_COUNT;
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		texDesc.texFileName = DirectoryUtil::GetFileNameWithoutExtension(specularMap._name) + "_cubemap";
		this->mippedEnvironmentCubemap = pRenderer->CreateTexture2D(texDesc);
	}

	const Texture& prefilteredEnvMapTex = pRenderer->GetTextureObject(prefilteredEnvironmentMap);
	const Texture& mippedEnvironmentCubemapTex = pRenderer->GetTextureObject(mippedEnvironmentCubemap);

	D3D11_TEXTURE2D_DESC desc = {};
	prefilteredEnvMapTex._tex2D->GetDesc(&desc);

	Viewport viewPort = {};
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;

	const auto IABuffers = SceneResourceView::GetBuiltinMeshVertexAndIndexBufferID(EGeometry::CUBE);

	auto SetPreFilterStates = [&]()
	{
		pRenderer->UnbindDepthTarget();
		pRenderer->SetShader(sRenderIntoCubemapShader);
		pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
		pRenderer->SetVertexBuffer(IABuffers.first);
		pRenderer->SetIndexBuffer(IABuffers.second);
		pRenderer->SetTexture("tEnvironmentMap", envMap);
		pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_NONE);
		pRenderer->SetSamplerState("sLinear", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);

		viewPort.Width = static_cast<float>(textureSize[0]);
		viewPort.Height = static_cast<float>(textureSize[1]);
		pRenderer->SetViewport(viewPort);
	};

	// RENDER INTO CUBEMAP PASS

	// TODO: Compute shader in single pass.
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		SetPreFilterStates();
		for (unsigned cubeFace = 0; cubeFace < 6; ++cubeFace)
		{
			D3D11_RENDER_TARGET_VIEW_DESC rtDesc = {};
			rtDesc.Format = desc.Format;
			rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtDesc.Texture2DArray.MipSlice = 0;
			rtDesc.Texture2DArray.ArraySize = 6 - cubeFace;
			rtDesc.Texture2DArray.FirstArraySlice = cubeFace;

			const RenderTargetID mipTarget = pRenderer->AddRenderTarget(mippedEnvironmentCubemapTex, rtDesc);	// todo: pool RTs.
			const XMMATRIX view = Texture::CubemapUtility::CalculateViewMatrix(static_cast<Texture::CubemapUtility::ECubeMapLookDirections>(cubeFace)); 
			const XMMATRIX viewProj = view * proj;

			pRenderer->BindRenderTarget(mipTarget);
			pRenderer->SetConstant4x4f("worldViewProj", viewProj);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		}
		pRenderer->m_deviceContext->Flush();
	}
	
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		pRenderer->m_deviceContext->GenerateMips(mippedEnvironmentCubemapTex._srv);
	}

	// PREFILTER PASS
	// pre-filter environment map into each cube face and mip level (~ roughness)
	// TODO: Compute shader in single pass.
	{
		std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
		pRenderer->SetShader(sPrefilterShader, true);
		pRenderer->SetTexture("tEnvironmentMap", mippedEnvironmentCubemap);
		for (unsigned mipLevel = 0; mipLevel < PREFILTER_MIP_LEVEL_COUNT; ++mipLevel)
		{
			viewPort.Width = static_cast<float>(textureSize[0] >> mipLevel);
			viewPort.Height = static_cast<float>(textureSize[1] >> mipLevel);

			pRenderer->SetConstant1f("roughness", static_cast<float>(mipLevel) / (PREFILTER_MIP_LEVEL_COUNT - 1));
			pRenderer->SetConstant1f("resolution", viewPort.Width);

			for (unsigned cubeFace = 0; cubeFace < 6; ++cubeFace)
			{
				D3D11_RENDER_TARGET_VIEW_DESC rtDesc = {};
				rtDesc.Format = desc.Format;
				rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				rtDesc.Texture2DArray.MipSlice = mipLevel;
				rtDesc.Texture2DArray.ArraySize = 6 - cubeFace;
				rtDesc.Texture2DArray.FirstArraySlice = cubeFace;

				const RenderTargetID mipTarget = pRenderer->AddRenderTarget(prefilteredEnvMapTex, rtDesc);	// todo: pool RTs.
				const XMMATRIX view = Texture::CubemapUtility::CalculateViewMatrix(static_cast<Texture::CubemapUtility::ECubeMapLookDirections>(cubeFace));
				const XMMATRIX viewProj = view * proj;

				pRenderer->BindRenderTarget(mipTarget);
				pRenderer->SetViewport(viewPort);
				pRenderer->SetConstant4x4f("worldViewProj", viewProj);
				pRenderer->Apply();
				pRenderer->DrawIndexed();
			}
		}
		pRenderer->m_deviceContext->Flush();

		// cache the resulting texture to disk if enabled
		if (Engine::GetSettings().bCacheEnvironmentMapsOnDisk)
		{
			const std::string filePath = cacheFolderPath + prefilteredEnvMapTex._name + ".hdr";
			pRenderer->SaveTextureToDisk(prefilteredEnvMapTex._id, filePath, false);
		}
	}

	// RENDER IRRADIANCE CUBEMAP PASS
	// TODO: irradiance cubemap
	// TODO: Compute shader in single pass.
	//pRenderer->End();	// present frame or device removed...

	// ID3D11ShaderResourceView* pNull[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	// pRenderer->m_deviceContext->PSSetShaderResources(0, 6, &pNull[0]);
	return prefilteredEnvironmentMap;
}
