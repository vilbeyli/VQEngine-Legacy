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

void InitializePreset_sIBL(Renderer* pRenderer, EEnvironmentMapPresets preset)
{
	const std::string sIBLDirectory = Renderer::sHDRTextureRoot + std::string("sIBL/");
	const bool bEquirectangular = true;

	auto initBarca = [&]()
	{	// BARCELONA ROOFTOP  
		EnvironmentMapFileNames files;
		const std::string root = sIBLDirectory + "Barcelona_Rooftops/";
		files = {
			"Barce_Rooftop_C_8k.jpg",
			"Barce_Rooftop_C_Env.hdr",
			"Barce_Rooftop_C_3k.hdr",
		};
		Skybox::s_Presets[EEnvironmentMapPresets::BARCELONA] = Skybox(pRenderer, files, root, bEquirectangular);
	};

	auto initTropical = [&]()
	{	// TROPICAL BEACH
		EnvironmentMapFileNames files;
		const std::string root = sIBLDirectory + "Tropical_Beach/";
		files = {
			"Tropical_Beach_8k.jpg",
			"Tropical_Beach_Env.hdr",
			"Tropical_Beach_3k.hdr",
		};
		Skybox::s_Presets[EEnvironmentMapPresets::TROPICAL_BEACH] = Skybox(pRenderer, files, root, bEquirectangular);
	};


	switch (preset)
	{
	case BARCELONA:
		initBarca();
		break;
	case TROPICAL_BEACH:
		break;
	case TROPICAL_RUINS:
		break;
	case WALK_OF_FAME:
		break;
	default:
		Log::Error("Preset %d is not an sIBL preset.", preset);
		break;
	}

}

void Skybox::InitializePresets(Renderer* pRenderer, bool loadEnvironmentMaps, bool bLoadAllMaps)
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

	if (loadEnvironmentMaps)
	{
		// HDR / IBL - Equirectangular Skyboxes
		//------------------------------------------------------------------------------------------------------------------------------------
		//EnvironmentMap::Initialize(pRenderer);
		const std::string sIBLDirectory = Renderer::sHDRTextureRoot + std::string("sIBL/");
		const bool bEquirectangular = true;

		EnvironmentMapFileNames files;

		//InitializePreset_sIBL(pRenderer, EEnvironmentMapPresets::BARCELONA);
		{	// BARCELONA ROOFTOP  
			const std::string root = sIBLDirectory + "Barcelona_Rooftops/";
			files = {
				"Barce_Rooftop_C_8k.jpg",
				"Barce_Rooftop_C_Env.hdr",
				"Barce_Rooftop_C_3k.hdr",
			};
			s_Presets[EEnvironmentMapPresets::BARCELONA] = Skybox(pRenderer, files, root, bEquirectangular);
		}

		if (bLoadAllMaps)
		{
			{	// TROPICAL BEACH
				const std::string root = sIBLDirectory + "Tropical_Beach/";
				files = {
					"Tropical_Beach_8k.jpg",
					"Tropical_Beach_Env.hdr",
					"Tropical_Beach_3k.hdr",
				};
				s_Presets[EEnvironmentMapPresets::TROPICAL_BEACH] = Skybox(pRenderer, files, root, bEquirectangular);
			}

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
		}
	}
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
	//pRenderer->SetSamplerState("samWrap", EDefaultSamplerState::WRAP_SAMPLER);
	pRenderer->SetSamplerState("samWrap", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);
	pRenderer->SetBufferObj(EGeometry::CUBE);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();
}


// ENVIRONMENT MAP
//==========================================================================================================

// Static Variables - BRDF LUT & Pre-filtered Environment Map
//---------------------------------------------------------------
RenderTargetID EnvironmentMap::sBRDFIntegrationLUTRT = -1;
ShaderID EnvironmentMap::sBRDFIntegrationLUTShader   = -1;
ShaderID EnvironmentMap::sPrefilterShader = -1;
ShaderID EnvironmentMap::sRenderIntoCubemapShader = -1;
Renderer* EnvironmentMap::spRenderer = nullptr;
//---------------------------------------------------------------

EnvironmentMap::EnvironmentMap() : irradianceMap(-1), environmentMap(-1) {}

void EnvironmentMap::Initialize(Renderer * pRenderer)
{
	spRenderer = pRenderer;
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

	const std::vector<std::string> BRDFIntegrator = { "FullscreenQuad_vs", "IntegrateBRDF_IBL_ps" };// compute?
	const std::vector<std::string> IBLConvolution = { "Skybox_vs", "PreFilterConvolution_ps" };		// compute?
	const std::vector<std::string> RenderIntoCubemap = { "Skybox_vs", "RenderIntoCubemap_ps" };

	sBRDFIntegrationLUTShader = spRenderer->AddShader("BRDFIntegrator", BRDFIntegrator, VS_PS, layout);
	sPrefilterShader = spRenderer->AddShader("PreFilterConvolution", IBLConvolution, VS_PS, layout);
	sRenderIntoCubemapShader = spRenderer->AddShader("RenderIntoCubemap", RenderIntoCubemap, VS_PS, layout);
}

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

TextureID EnvironmentMap::InitializePrefilteredEnvironmentMap(const Texture& environmentMap, const Texture& irradienceMap)
{
	Renderer*& pRenderer = spRenderer;
	const TextureID envMap = environmentMap._id;

	const float screenAspect = 1;
	const float screenNear = 0.01f;
	const float screenFar = 10.0f;
	const float fovy = 90.0f * DEG2RAD;
	const XMMATRIX proj = XMMatrixPerspectiveFovLH(fovy, screenAspect, screenNear, screenFar);

	// cubemap look and up directions for left-handed view
	const XMVECTOR lookDirs[6] = {
		vec3::Right, vec3::Left,
		vec3::Up, vec3::Down,
		vec3::Forward, vec3::Back
	};

	const XMVECTOR upDirs[6] = {
		vec3::Up, vec3::Up,
		vec3::Back, vec3::Forward,
		vec3::Up, vec3::Up
	};

	// create environment map cubemap texture
	const unsigned cubemapDimension = environmentMap._height / 2;
	const unsigned textureSize[2] = { cubemapDimension, cubemapDimension };
	TextureDesc texDesc = {};
	texDesc.width = cubemapDimension;
	texDesc.height = cubemapDimension;
	texDesc.format = EImageFormat::RGBA32F;
	texDesc.texFileName = environmentMap._name + "_preFiltered";
	texDesc.pData = nullptr; // no CPU data
	texDesc.mipCount = PREFILTER_MIP_LEVEL_COUNT;
	texDesc.usage = RENDER_TARGET_RW;
	texDesc.bIsCubeMap = true;
	prefilteredEnvironmentMap = pRenderer->CreateTexture2D(texDesc);

	texDesc.bGenerateMips = true;
	texDesc.mipCount = PREFILTER_MIP_LEVEL_COUNT;
	mippedEnvironmentCubemap = pRenderer->CreateTexture2D(texDesc);

	const Texture& prefilteredEnvMapTex = pRenderer->GetTextureObject(prefilteredEnvironmentMap);
	const Texture& mippedEnvironmentCubemapTex = pRenderer->GetTextureObject(mippedEnvironmentCubemap);

	D3D11_TEXTURE2D_DESC desc = {};
	prefilteredEnvMapTex._tex2D->GetDesc(&desc);

	Viewport viewPort = {};
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.MaxDepth = 1.0f;
	viewPort.MinDepth = 0.0f;

	pRenderer->UnbindDepthTarget();
	pRenderer->SetShader(sRenderIntoCubemapShader);
	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
	pRenderer->SetBufferObj(EGeometry::CUBE);
	pRenderer->SetTexture("tEnvironmentMap", envMap);
	pRenderer->SetSamplerState("sLinear", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);

	viewPort.Width = static_cast<float>(textureSize[0]);
	viewPort.Height = static_cast<float>(textureSize[1]);
	pRenderer->SetViewport(viewPort);
	
	// RENDER INTO CUBEMAP PASS
	// render cubemap version of the environment map, generate mips, and then bind to pre-filter stage 
	// cube face order: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
	//------------------------------------------------------------------------------------------------------
	// 0: RIGHT		1: LEFT
	// 2: UP		3: DOWN
	// 4: FRONT		5: BACK
	//------------------------------------------------------------------------------------------------------
	// TODO: Compute shader in single pass.
	for (unsigned cubeFace = 0; cubeFace < 6; ++cubeFace)
	{
		D3D11_RENDER_TARGET_VIEW_DESC rtDesc = {};
		rtDesc.Format = desc.Format;
		rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtDesc.Texture2DArray.MipSlice = 0;
		rtDesc.Texture2DArray.ArraySize = 6 - cubeFace;
		rtDesc.Texture2DArray.FirstArraySlice = cubeFace;

		const RenderTargetID mipTarget = pRenderer->AddRenderTarget(mippedEnvironmentCubemapTex, rtDesc);	// todo: pool RTs.

		const XMMATRIX view = XMMatrixLookAtLH(vec3::Zero, lookDirs[cubeFace], upDirs[cubeFace]);
		const XMMATRIX viewProj = view * proj;

		pRenderer->BindRenderTarget(mipTarget);
		pRenderer->SetConstant4x4f("worldViewProj", viewProj);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
	}
	pRenderer->m_deviceContext->GenerateMips(mippedEnvironmentCubemapTex._srv);
	pRenderer->End();	// present frame or device removed...

	// PREFILTER PASS
	// pre-filter environment map into each cube face and mip level (~ roughness)
	// TODO: Compute shader in single pass.
	pRenderer->SetShader(sPrefilterShader);
	pRenderer->SetTexture("tEnvironmentMap", mippedEnvironmentCubemap);
	for (unsigned mipLevel = 0; mipLevel < PREFILTER_MIP_LEVEL_COUNT; ++mipLevel)
	{
		viewPort.Width = static_cast<float>(textureSize[0] >> mipLevel);
		viewPort.Height = static_cast<float>(textureSize[1] >> mipLevel);

		pRenderer->SetConstant1f("roughness", static_cast<float>(mipLevel) / (PREFILTER_MIP_LEVEL_COUNT - 1));
		pRenderer->SetConstant1f("resolution", viewPort.Width);

		// cube face order: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
		//------------------------------------------------------------------------------------------------------
		// 0: RIGHT		1: LEFT
		// 2: UP		3: DOWN
		// 4: FRONT		5: BACK
		//------------------------------------------------------------------------------------------------------
		for (unsigned cubeFace = 0; cubeFace < 6; ++cubeFace)
		{
			D3D11_RENDER_TARGET_VIEW_DESC rtDesc = {};
			rtDesc.Format = desc.Format;
			rtDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
			rtDesc.Texture2DArray.MipSlice = mipLevel;
			rtDesc.Texture2DArray.ArraySize = 6 - cubeFace;
			rtDesc.Texture2DArray.FirstArraySlice = cubeFace;

			const RenderTargetID mipTarget = pRenderer->AddRenderTarget(prefilteredEnvMapTex, rtDesc);	// todo: pool RTs.

			const XMMATRIX view = XMMatrixLookAtLH(vec3::Zero, lookDirs[cubeFace], upDirs[cubeFace]);
			const XMMATRIX viewProj = view * proj;

			pRenderer->BindRenderTarget(mipTarget);
			pRenderer->SetViewport(viewPort);
			pRenderer->SetConstant4x4f("worldViewProj", viewProj);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		}
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
	this->envMapSampler = spRenderer->CreateSamplerState(envMapSamplerDesc);
	pRenderer->End();	// present frame or device removed...

	// RENDER IRRADIANCE CUBEMAP PASS
	// TODO: irradiance cubemap
	// TODO: Compute shader in single pass.
	//pRenderer->End();	// present frame or device removed...

	ID3D11ShaderResourceView* pNull[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
	pRenderer->m_deviceContext->PSSetShaderResources(0, 6, &pNull[0]);
	return prefilteredEnvironmentMap;
}

EnvironmentMap::EnvironmentMap(Renderer* pRenderer, const EnvironmentMapFileNames& files, const std::string& rootDirectory)
{
	Log::Info("Loading Environment Map: %s", split(rootDirectory, '/').back().c_str());
	irradianceMap = pRenderer->CreateHDRTexture(files.irradianceMapFileName, rootDirectory);
	environmentMap = pRenderer->CreateHDRTexture(files.environmentMapFileName, rootDirectory);
	InitializePrefilteredEnvironmentMap(pRenderer->GetTextureObject(environmentMap), pRenderer->GetTextureObject(irradianceMap));
}