//	DX11Renderer - VDemo | DirectX11 Renderer
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


#include "RenderPasses.h"
#include "Engine.h"
#include "GameObject.h"

#include "Renderer/Renderer.h"
#include "Utilities/Camera.h"
#include "Renderer/D3DManager.h"

#include "Utilities/Log.h"

#include <array>

void ShadowMapPass::Initialize(Renderer* pRenderer, ID3D11Device* device, const Settings::ShadowMap& shadowMapSettings)
{
	this->_shadowMapDimension = static_cast<int>(shadowMapSettings.dimension);

#if _DEBUG
	pRenderer->m_Direct3D->ReportLiveObjects("--------SHADOW_PASS_INIT");
#endif

	// check feature support & error handle:
	// https://msdn.microsoft.com/en-us/library/windows/apps/dn263150

	// create shadow map texture for the pixel shader stage
	const bool bDepthOnly = true;	// do we need stencil in depth map?
	TextureID shadowMapID = pRenderer->CreateDepthTexture(static_cast<unsigned>(_shadowMapDimension), static_cast<unsigned>(_shadowMapDimension), bDepthOnly);
	_shadowMap = shadowMapID;
	Texture& shadowMap = const_cast<Texture&>(pRenderer->GetTextureObject(_shadowMap));

	// depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = bDepthOnly ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	this->_shadowDepthTarget = pRenderer->AddDepthTarget(dsvDesc, shadowMap);

	// comparison sampler
	D3D11_SAMPLER_DESC comparisonSamplerDesc;
	ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
	comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.BorderColor[0] = 1.0f;
	comparisonSamplerDesc.BorderColor[1] = 1.0f;
	comparisonSamplerDesc.BorderColor[2] = 1.0f;
	comparisonSamplerDesc.BorderColor[3] = 1.0f;
	comparisonSamplerDesc.MinLOD = 0.f;
	comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	comparisonSamplerDesc.MipLODBias = 0.f;
	comparisonSamplerDesc.MaxAnisotropy = 0;
	//comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;	// todo: set default sampler elsewhere
	comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	//comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	comparisonSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	this->_shadowSampler = pRenderer->CreateSamplerState(comparisonSamplerDesc);

	// render states for front face culling 
	this->_shadowRenderState = pRenderer->AddRasterizerState(ERasterizerCullMode::FRONT, ERasterizerFillMode::SOLID, true, false);
	this->_drawRenderState   = pRenderer->AddRasterizerState(ERasterizerCullMode::BACK , ERasterizerFillMode::SOLID, true, false);

	// shader
	std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 } };
	this->_shadowShader = EShaders::SHADOWMAP_DEPTH;

	ZeroMemory(&_shadowViewport, sizeof(D3D11_VIEWPORT));
	_shadowViewport.Height = static_cast<float>(_shadowMapDimension);
	_shadowViewport.Width = static_cast<float>(_shadowMapDimension);
	_shadowViewport.MinDepth = 0.f;
	_shadowViewport.MaxDepth = 1.f;
}

void ShadowMapPass::RenderShadowMaps(Renderer* pRenderer, const std::vector<const Light*> shadowLights, const std::vector<const GameObject*> ZPassObjects) const
{
	if (shadowLights.empty()) return;

	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	pRenderer->BindDepthTarget(_shadowDepthTarget);			// only depth stencil buffer
	pRenderer->SetViewport(_shadowViewport);				// lights viewport 512x512
	pRenderer->SetShader(_shadowShader);					// shader for rendering z buffer
	pRenderer->SetConstant4x4f("viewProj", shadowLights.front()->GetLightSpaceMatrix());
	pRenderer->SetConstant4x4f("view", shadowLights.front()->GetViewMatrix());
	pRenderer->SetConstant4x4f("proj", shadowLights.front()->GetProjectionMatrix());
	pRenderer->Apply();
	pRenderer->Begin(clearColor, 1.0f);
	for (const GameObject* obj : ZPassObjects)
	{
		obj->RenderZ(pRenderer);
	}
}



void PostProcessPass::Initialize(Renderer* pRenderer, const Settings::PostProcess& postProcessSettings)
{
	_settings = postProcessSettings;
	DXGI_SAMPLE_DESC smpDesc;
	smpDesc.Count = 1;
	smpDesc.Quality = 0;

	constexpr const EImageFormat HDR_Format = RGBA16F;
	constexpr const EImageFormat LDR_Format = RGBA8UN;

	const std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 },
	};
	const std::vector<EShaderType> VS_PS = { EShaderType::VS, EShaderType::PS };

	const std::vector<std::string> BlurShaders = { "FullscreenQuad_vs", "Blur_ps" };	// compute?
	const std::vector<std::string> BloomShaders = { "FullscreenQuad_vs", "Bloom_ps" };
	const std::vector<std::string> CombineShaders = { "FullscreenQuad_vs", "BloomCombine_ps" };
	const std::vector<std::string> TonemapShaders = { "FullscreenQuad_vs", "Tonemapping_ps" };


	EImageFormat format = _settings.HDREnabled ? HDR_Format : LDR_Format;

	D3D11_TEXTURE2D_DESC rtDesc = {};
	rtDesc.Width = pRenderer->WindowWidth();
	rtDesc.Height = pRenderer->WindowHeight();
	rtDesc.MipLevels = 1;
	rtDesc.ArraySize = 1;
	rtDesc.Format = (DXGI_FORMAT)format;
	rtDesc.Usage = D3D11_USAGE_DEFAULT;
	rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtDesc.CPUAccessFlags = 0;
	rtDesc.SampleDesc = smpDesc;
	rtDesc.MiscFlags = 0;

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	RTVDesc.Format = (DXGI_FORMAT)format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;

	// Bloom effect
	this->_bloomPass._colorRT = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._brightRT = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._finalRT = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._blurPingPong[0] = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._blurPingPong[1] = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._bloomFilterShader = pRenderer->AddShader("Bloom", BloomShaders, VS_PS, layout);
	this->_bloomPass._blurShader = pRenderer->AddShader("Blur", BlurShaders, VS_PS, layout);
	this->_bloomPass._bloomCombineShader = pRenderer->AddShader("BloomCombine", CombineShaders, VS_PS, layout);

	D3D11_SAMPLER_DESC blurSamplerDesc = {};
	blurSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	this->_bloomPass._blurSampler = pRenderer->CreateSamplerState(blurSamplerDesc);
	
	// Tonemapping
	this->_tonemappingPass._finalRenderTarget = pRenderer->GetDefaultRenderTarget();
	this->_tonemappingPass._toneMappingShader = pRenderer->AddShader("Tonemapping", TonemapShaders, VS_PS, layout);

	// World Render Target
	this->_worldRenderTarget = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
}

void PostProcessPass::Render(Renderer * pRenderer, bool bUseBRDFLighting) const
{
	pRenderer->BeginEvent("Post Processing");

	const TextureID worldTexture = pRenderer->GetRenderTargetTexture(_worldRenderTarget);

	// ======================================================================================
	// BLOOM  PASS
	// ======================================================================================
	if (_bloomPass._isEnabled)
	{
		const Settings::PostProcess::Bloom& s = _settings.bloom;
		const ShaderID currentShader = pRenderer->GetActiveShader();

		const float brightnessThreshold = bUseBRDFLighting ? s.threshold_brdf : s.threshold_phong;

		// bright filter
		pRenderer->BeginEvent("Bloom Bright Filter");
		pRenderer->SetShader(_bloomPass._bloomFilterShader);
		pRenderer->BindRenderTargets(_bloomPass._colorRT, _bloomPass._brightRT);
		pRenderer->UnbindDepthTarget();
		pRenderer->SetBufferObj(EGeometry::QUAD);
		pRenderer->Apply();
		pRenderer->SetTexture("worldRenderTarget", worldTexture);
		pRenderer->SetConstant1f("BrightnessThreshold", brightnessThreshold);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
		pRenderer->EndEvent();

		// blur
		const TextureID brightTexture = pRenderer->GetRenderTargetTexture(_bloomPass._brightRT);
		pRenderer->BeginEvent("Bloom Blur Pass");
		pRenderer->SetShader(_bloomPass._blurShader);
		for (int i = 0; i < s.blurPassCount; ++i)
		{
			const int isHorizontal = i % 2;
			const TextureID pingPong = pRenderer->GetRenderTargetTexture(_bloomPass._blurPingPong[1 - isHorizontal]);
			const int texWidth = pRenderer->GetTextureObject(pingPong)._width;
			const int texHeight = pRenderer->GetTextureObject(pingPong)._height;

			pRenderer->UnbindRenderTargets();
			pRenderer->Apply();
			pRenderer->BindRenderTarget(_bloomPass._blurPingPong[isHorizontal]);
			pRenderer->SetConstant1i("isHorizontal", 1 - isHorizontal);
			pRenderer->SetConstant1i("textureWidth", texWidth);
			pRenderer->SetConstant1i("textureHeight", texHeight);
			pRenderer->SetTexture("InputTexture", i == 0 ? brightTexture : pingPong);
			pRenderer->SetSamplerState("BlurSampler", _bloomPass._blurSampler);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		}
		pRenderer->EndEvent();

		// additive blend combine
		const TextureID colorTex = pRenderer->GetRenderTargetTexture(_bloomPass._colorRT);
		const TextureID bloomTex = pRenderer->GetRenderTargetTexture(_bloomPass._blurPingPong[0]);
		pRenderer->BeginEvent("Bloom Combine");
		pRenderer->SetShader(_bloomPass._bloomCombineShader);
		pRenderer->BindRenderTarget(_bloomPass._finalRT);
		pRenderer->Apply();
		pRenderer->SetTexture("ColorTexture", colorTex);
		pRenderer->SetTexture("BloomTexture", bloomTex);
		pRenderer->SetSamplerState("BlurSampler", _bloomPass._blurSampler);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
		pRenderer->EndEvent();
	}



	// ======================================================================================
	// TONEMAPPING PASS
	// ======================================================================================
	const TextureID colorTex = pRenderer->GetRenderTargetTexture(_bloomPass._isEnabled ? _bloomPass._finalRT : _worldRenderTarget);
	const float isHDR = _settings.HDREnabled ? 1.0f : 0.0f;
	pRenderer->BeginEvent("Tonemapping");
	pRenderer->UnbindDepthTarget();
	pRenderer->SetShader(_tonemappingPass._toneMappingShader);
	pRenderer->SetBufferObj(EGeometry::QUAD);
	pRenderer->SetSamplerState("Sampler", _bloomPass._blurSampler);
	pRenderer->SetConstant1f("exposure", _settings.toneMapping.exposure);
	pRenderer->SetConstant1f("isHDR", isHDR);
	pRenderer->BindRenderTarget(_tonemappingPass._finalRenderTarget);
	pRenderer->SetTexture("ColorTexture", colorTex);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();

	pRenderer->EndEvent();	// Post Process
}



void DeferredRenderingPasses::Initialize(Renderer * pRenderer)
{
	const std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 },
	};

	const std::vector<EShaderType> VS_PS = { EShaderType::VS, EShaderType::PS };

	const std::vector<std::string> Deferred_AmbientLight		= { "deferred_rendering_vs", "deferred_brdf_ambient_ps" };
	const std::vector<std::string> Deferred_AmbientIBL			= { "deferred_rendering_vs", "deferred_brdf_ambientIBL_ps" };
	const std::vector<std::string> DeferredBRDF_LightingFSQ		= { "deferred_rendering_vs", "deferred_brdf_lighting_ps" };
	const std::vector<std::string> DeferredPhong_LightingFSQ	= { "deferred_rendering_vs", "deferred_phong_lighting_ps" };

	const std::vector<std::string> DeferredBRDF_PointLight		= { "MVPTransformationWithUVs_vs", "deferred_brdf_pointLight_ps" };
	const std::vector<std::string> DeferredBRDF_SpotLight		= { "MVPTransformationWithUVs_vs", "deferred_brdf_spotLight_ps" };	// render cone?

	InitializeGBuffer(pRenderer);
	_geometryShader		 = pRenderer->AddShader("Deferred_Geometry", layout);
	_ambientShader		 = pRenderer->AddShader("Deferred_Ambient", Deferred_AmbientLight, VS_PS, layout);
	_ambientIBLShader	 = pRenderer->AddShader("Deferred_AmbientIBL", Deferred_AmbientIBL, VS_PS, layout);
	_BRDFLightingShader  = pRenderer->AddShader("Deferred_BRDF_Lighting", DeferredBRDF_LightingFSQ, VS_PS, layout);
	_phongLightingShader = pRenderer->AddShader("Deferred_Phong_Lighting", DeferredPhong_LightingFSQ, VS_PS, layout);
	_spotLightShader	 = pRenderer->AddShader("Deferred_BRDF_Point", DeferredBRDF_PointLight, VS_PS, layout);
	_pointLightShader	 = pRenderer->AddShader("Deferred_BRDF_Spot", DeferredBRDF_SpotLight, VS_PS, layout);

	// deferred geometry is accessed from elsewhere, needs to be globally defined
	assert(EShaders::DEFERRED_GEOMETRY == _geometryShader);	// this assumption may break, make sure it doesnt...
}

void DeferredRenderingPasses::InitializeGBuffer(Renderer* pRenderer)
{
	Log::Info("Initializing GBuffer...");

	DXGI_SAMPLE_DESC smpDesc;
	smpDesc.Count = 1;
	smpDesc.Quality = 0;

	D3D11_TEXTURE2D_DESC RTDescriptor[2] = { {}, {} };
	constexpr const DXGI_FORMAT Format[2] = { /*DXGI_FORMAT_R11G11B10_FLOAT*/ DXGI_FORMAT_R32G32B32A32_FLOAT , DXGI_FORMAT_R16G16B16A16_FLOAT };

	for (int i = 0; i < 2; ++i)
	{
		RTDescriptor[i].Width = pRenderer->WindowWidth();
		RTDescriptor[i].Height = pRenderer->WindowHeight();
		RTDescriptor[i].MipLevels = 1;
		RTDescriptor[i].ArraySize = 1;
		RTDescriptor[i].Format = Format[i];
		RTDescriptor[i].Usage = D3D11_USAGE_DEFAULT;
		RTDescriptor[i].BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		RTDescriptor[i].CPUAccessFlags = 0;
		RTDescriptor[i].SampleDesc = smpDesc;
		RTDescriptor[i].MiscFlags = 0;
	}

	D3D11_RENDER_TARGET_VIEW_DESC RTVDescriptor[2] = { {},{} };
	for (int i = 0; i < 2; ++i)
	{
		RTVDescriptor[i].Format = Format[i];
		RTVDescriptor[i].ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTVDescriptor[i].Texture2D.MipSlice = 0;
	}
	
	constexpr size_t Float3TypeIndex = 0;		constexpr size_t Float4TypeIndex = 1;
	this->_GBuffer._positionRT		   = pRenderer->AddRenderTarget(RTDescriptor[Float3TypeIndex], RTVDescriptor[Float3TypeIndex]);
	this->_GBuffer._normalRT		   = pRenderer->AddRenderTarget(RTDescriptor[Float3TypeIndex], RTVDescriptor[Float3TypeIndex]);
	this->_GBuffer._diffuseRoughnessRT = pRenderer->AddRenderTarget(RTDescriptor[Float4TypeIndex], RTVDescriptor[Float4TypeIndex]);
	this->_GBuffer._specularMetallicRT = pRenderer->AddRenderTarget(RTDescriptor[Float4TypeIndex], RTVDescriptor[Float4TypeIndex]);
	this->_GBuffer.bInitialized = true;
	// http://download.nvidia.com/developer/presentations/2004/6800_Leagues/6800_Leagues_Deferred_Shading.pdf
	// Option: trade storage for computation
	//  - Store pos.z     and compute xy from z + window.xy
	//	- Store normal.xy and compute z = sqrt(1 - x^2 - y^2)
	Log::Info("Done.");

	{	// Geometry depth stencil state descriptor
		D3D11_DEPTH_STENCILOP_DESC dsOpDesc = {};
		dsOpDesc.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsOpDesc.StencilDepthFailOp = D3D11_STENCIL_OP_INCR_SAT;
		dsOpDesc.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
		dsOpDesc.StencilFunc = D3D11_COMPARISON_ALWAYS;

		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.FrontFace = dsOpDesc;
		
		//dsOpDesc.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
		desc.BackFace = dsOpDesc;

		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		desc.StencilEnable = true;
		desc.StencilReadMask  = 0xFF;
		desc.StencilWriteMask = 0xFF;

		_geometryStencilState = pRenderer->AddDepthStencilState(desc);
	}

	{	// Skybox depth stencil state descriptor
		D3D11_DEPTH_STENCILOP_DESC dsOpDesc = {};
		dsOpDesc.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsOpDesc.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		dsOpDesc.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		dsOpDesc.StencilFunc = D3D11_COMPARISON_EQUAL;

		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.FrontFace = dsOpDesc;

		dsOpDesc.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		desc.BackFace = dsOpDesc;

		desc.DepthEnable = false;

		desc.StencilEnable = true;
		desc.StencilReadMask = 0xFF;
		desc.StencilWriteMask = 0x00;

		_skyboxStencilState = pRenderer->AddDepthStencilState(desc);
	}
}

void DeferredRenderingPasses::ClearGBuffer(Renderer* pRenderer)
{
	const bool bDoClearColor = true;
	const bool bDoClearDepth = false;
	const bool bDoClearStencil = false;
	ClearCommand clearCmd(
		bDoClearColor, bDoClearDepth, bDoClearStencil,
		{ 0, 0, 0, 0 }, 0, 0
	);
	pRenderer->BindRenderTargets(_GBuffer._diffuseRoughnessRT, _GBuffer._specularMetallicRT, _GBuffer._normalRT, _GBuffer._positionRT);
	pRenderer->Begin(clearCmd);
}

void DeferredRenderingPasses::SetGeometryRenderingStates(Renderer* pRenderer) const
{
	const bool bDoClearColor = true;
	const bool bDoClearDepth = true;
	const bool bDoClearStencil = true;
	ClearCommand clearCmd(
		bDoClearColor	, bDoClearDepth	, bDoClearStencil,
		{ 0, 0, 0, 0 }	, 1				, 0
	);

	pRenderer->SetShader(_geometryShader);
	pRenderer->BindRenderTargets(_GBuffer._diffuseRoughnessRT, _GBuffer._specularMetallicRT, _GBuffer._normalRT, _GBuffer._positionRT);
	pRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());
	pRenderer->SetDepthStencilState(_geometryStencilState); 
	pRenderer->SetSamplerState("sNormalSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);
	pRenderer->Begin(clearCmd);
	pRenderer->Apply();
}

void DeferredRenderingPasses::RenderLightingPass(
	Renderer* pRenderer, 
	const RenderTargetID target, 
	const SceneView& sceneView, 
	const SceneLightData& lights, 
	const TextureID tSSAO, 
	bool bUseBRDFLighting) const
{
	const bool bDoClearColor = true;
	const bool bDoClearDepth = false;
	const bool bDoClearStencil = false;
	ClearCommand clearCmd(
		bDoClearColor, bDoClearDepth, bDoClearStencil,
		{ 0, 0, 0, 0 }, 1, 0
	);

	const bool bAmbientOcclusionOn = tSSAO == -1;

	const vec2 screenSize = pRenderer->GetWindowDimensionsAsFloat2();
	const TextureID texNormal = pRenderer->GetRenderTargetTexture(_GBuffer._normalRT);
	const TextureID texDiffuseRoughness = pRenderer->GetRenderTargetTexture(_GBuffer._diffuseRoughnessRT);
	const TextureID texSpecularMetallic = pRenderer->GetRenderTargetTexture(_GBuffer._specularMetallicRT);
	const TextureID texPosition = pRenderer->GetRenderTargetTexture(_GBuffer._positionRT);
	const ShaderID lightingShader = bUseBRDFLighting ? _BRDFLightingShader : _phongLightingShader;
	const TextureID texIrradianceMap = sceneView.environmentMap.irradianceMap;
	const SamplerID smpEnvMap = sceneView.environmentMap.envMapSampler;
	const TextureID texSpecularMap = sceneView.environmentMap.prefilteredEnvironmentMap;
	const TextureID tBRDFLUT = pRenderer->GetRenderTargetTexture(EnvironmentMap::sBRDFIntegrationLUTRT);
	const TextureID tLUTRef = pRenderer->CreateTextureFromFile("DebugTextures/ibl_brdf_lut_reference.png");

	// pRenderer->UnbindRendertargets();	// ignore this for now
	pRenderer->UnbindDepthTarget();
	pRenderer->BindRenderTarget(target);
	pRenderer->Begin(clearCmd);
	pRenderer->Apply();

	// AMBIENT LIGHTING
	if(sceneView.bIsIBLEnabled)
	{
		pRenderer->BeginEvent("Environment Map Lighting Pass");
		pRenderer->SetShader(_ambientIBLShader);
		pRenderer->SetTexture("tPosition", texPosition);
		pRenderer->SetTexture("tDiffuseRoughnessMap", texDiffuseRoughness);
		pRenderer->SetTexture("tNormalMap", texNormal);
		pRenderer->SetTexture("tAmbientOcclusion", tSSAO);
		pRenderer->SetTexture("tIrradianceMap", texIrradianceMap);
		pRenderer->SetTexture("tPreFilteredEnvironmentMap", texSpecularMap);
		pRenderer->SetTexture("tBRDFIntegrationLUT", tLUTRef);
		pRenderer->SetSamplerState("sEnvMapSampler", smpEnvMap);
		pRenderer->SetSamplerState("sNearestSampler", EDefaultSamplerState::POINT_SAMPLER);
		//pRenderer->SetSamplerState("sWrapSampler", EDefaultSamplerState::WRAP_SAMPLER);
		pRenderer->SetConstant1f("ambientFactor", sceneView.sceneAmbientOcclusionFactor);
		pRenderer->SetConstant4x4f("viewToWorld", sceneView.viewToWorld);
	}
	else
	{
		pRenderer->BeginEvent("Ambient Pass");
		pRenderer->SetShader(_ambientShader);
		pRenderer->SetTexture("tDiffuseRoughnessMap", texDiffuseRoughness);
		pRenderer->SetTexture("tAmbientOcclusion", tSSAO);
		pRenderer->SetSamplerState("sNearestSampler", 0);	// todo: nearest sampler
		pRenderer->SetConstant1f("ambientFactor", sceneView.sceneAmbientOcclusionFactor);
	}
	pRenderer->SetBufferObj(EGeometry::QUAD);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();

	// DIFFUSE & SPECULAR LIGHTING
	pRenderer->SetBlendState(EDefaultBlendState::ADDITIVE_COLOR);

	// draw fullscreen quad for lighting for now. Will add light volumes
	// as the scene gets more complex or depending on performance needs.
#ifdef USE_LIGHT_VOLUMES
#if 0

	pRenderer->SetConstant3f("CameraWorldPosition", sceneView.pCamera->GetPositionF());
	pRenderer->SetConstant2f("ScreenSize", screenSize);
	pRenderer->SetTexture("texDiffuseRoughnessMap", texDiffuseRoughness);
	pRenderer->SetTexture("texSpecularMetalnessMap", texSpecularMetallic);
	pRenderer->SetTexture("texNormals", texNormal);
	pRenderer->SetTexture("texPosition", texPosition);

	// POINT LIGHTS
	pRenderer->SetShader(SHADERS::DEFERRED_BRDF_POINT);
	pRenderer->SetBufferObj(GEOMETRY::SPHERE);
	//pRenderer->SetRasterizerState(ERasterizerCullMode::)
	for(const Light& light : lights)
	{
		if (light._type == Light::ELightType::POINT)
		{
			const float& r   = light._range;	//bounding sphere radius
			const vec3&  pos = light._transform._position;
			const XMMATRIX world = {
				r, 0, 0, 0,
				0, r, 0, 0,
				0, 0, r, 0,
				pos.x(), pos.y(), pos.z(), 1,
			};

			const XMMATRIX wvp = world * sceneView.viewProj;
			const LightShaderSignature lightData = light.ShaderSignature();
			pRenderer->SetConstant4x4f("worldViewProj", wvp);
			pRenderer->SetConstantStruct("light", &lightData);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		}
	}
#endif

	// SPOT LIGHTS
#if 0
	pRenderer->SetShader(SHADERS::DEFERRED_BRDF);
	pRenderer->SetConstant3f("cameraPos", m_pCamera->GetPositionF());

	pRenderer->SetBufferObj(GEOMETRY::QUAD);
	
	// for spot lights
	
	pRenderer->Apply();
	pRenderer->DrawIndexed();
#endif

#else
	pRenderer->SetShader(lightingShader);
	ENGINE->SendLightData();

	pRenderer->SetConstant4x4f("matView", sceneView.view);
	pRenderer->SetConstant4x4f("matViewToWorld", sceneView.viewToWorld);
	pRenderer->SetSamplerState("sNearestSampler", 0);	// todo: nearest sampler
	pRenderer->SetTexture("texDiffuseRoughnessMap", texDiffuseRoughness);
	pRenderer->SetTexture("texSpecularMetalnessMap", texSpecularMetallic);
	pRenderer->SetTexture("texNormals", texNormal);
	pRenderer->SetTexture("texPosition", texPosition);
	pRenderer->SetBufferObj(EGeometry::QUAD);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
#endif	// light volumes

	pRenderer->SetBlendState(EDefaultBlendState::DISABLED);
}

void DebugPass::Initialize(Renderer * pRenderer)
{
	_scissorsRasterizer = pRenderer->AddRasterizerState(ERasterizerCullMode::BACK, ERasterizerFillMode::SOLID, false, true);
}

constexpr size_t SSAO_SAMPLE_KERNEL_SIZE = 64;
void AmbientOcclusionPass::Initialize(Renderer * pRenderer)
{
	const std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 },
	};
	const std::vector<EShaderType> VS_PS = { EShaderType::VS, EShaderType::PS };

	const std::vector<std::string> AmbientOcclusionShaders = { "FullscreenQuad_vs", "SSAO_ps" };

	// CREATE SAMPLE KERNEL
	//--------------------------------------------------------------------
	constexpr size_t NOISE_KERNEL_SIZE = 4;

	// src: https://john-chapman-graphics.blogspot.nl/2013/01/ssao-tutorial.html
	for (size_t i = 0; i < SSAO_SAMPLE_KERNEL_SIZE; i++)
	{
		// get a random direction in tangent space - z-up.
		// As the sample kernel will be oriented along the surface normal, 
		// the resulting sample vectors will all end up in the hemisphere.
		vec3 sample(
			RandF(-1, 1),
			RandF(-1, 1),
			RandF(0, 1)	// hemisphere
		);
		sample.normalize();
		sample = sample * RandF(0, 1);	// scale to distribute samples within the hemisphere

		// scale vectors with a power curve based on i to make samples close to center of the
		// hemisphere more significant. think of it as i selects where we sample the hemisphere
		// from, which starts from outer region of the hemisphere and as it increases, we 
		// sample closer to the normal direction. 
		float scale = static_cast<float>(i) / SSAO_SAMPLE_KERNEL_SIZE;
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample = sample * scale;

		this->sampleKernel.push_back(sample);
	}

	// CREATE NOISE TEXTURE & SAMPLER
	//--------------------------------------------------------------------
	for (size_t i = 0; i < NOISE_KERNEL_SIZE * NOISE_KERNEL_SIZE; i++)
	{	// create a square noise texture using random directions
		vec3 noise(
			RandF(-1, 1),
			RandF(-1, 1),
			0 // noise rotates the kernel around z-axis
		);
		this->noiseKernel.push_back(vec4(noise.normalized()));
	}
	TextureDesc texDesc = {};
	texDesc.width = NOISE_KERNEL_SIZE;
	texDesc.height = NOISE_KERNEL_SIZE;
	texDesc.format = EImageFormat::RGBA32F;
	texDesc.texFileName = "noiseKernel";
	texDesc.data = this->noiseKernel.data();
	this->noiseTexture = pRenderer->CreateTexture2D(texDesc);

	const float whiteValue = 1.0f;
	std::vector<vec4> white4x4 = std::vector<vec4>(16, vec4(whiteValue, whiteValue, whiteValue, 1));
	texDesc.width = texDesc.height = 4;
	texDesc.texFileName = "white4x4";
	texDesc.data = white4x4.data();
	this->whiteTexture4x4 = pRenderer->CreateTexture2D(texDesc);

	// The tiling of the texture causes the orientation of the kernel to be repeated and 
	// introduces regularity into the result. By keeping the texture size small we can make 
	// this regularity occur at a high frequency, which can then be removed with a blur step 
	// that preserves the low-frequency detail of the image.
	D3D11_SAMPLER_DESC noiseSamplerDesc = {};
	noiseSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	noiseSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	noiseSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	noiseSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	this->noiseSampler = pRenderer->CreateSamplerState(noiseSamplerDesc);

	// RENDER TARGET
	//--------------------------------------------------------------------
	const DXGI_FORMAT format = DXGI_FORMAT_R32_FLOAT;

	D3D11_TEXTURE2D_DESC rtDesc = {};
	rtDesc.Width = pRenderer->WindowWidth();
	rtDesc.Height = pRenderer->WindowHeight();
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


	this->SSAOShader = pRenderer->AddShader("SSAO", AmbientOcclusionShaders, VS_PS, layout);
	this->occlusionRenderTarget = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->blurRenderTarget		= pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->radius = 6.5f;
	this->intensity = 5.0f;
}

struct SSAOConstants
{
	XMFLOAT4X4 matProjection;
	vec2 screenSize;
	float radius;
	float intensity;
	std::array<float, SSAO_SAMPLE_KERNEL_SIZE * 3> samples;
};
void AmbientOcclusionPass::RenderOcclusion(Renderer* pRenderer, const TextureID texNormals, const TextureID texPositions, const SceneView& sceneView)
{
	const TextureID depthTexture = pRenderer->GetDepthTargetTexture(ENGINE->GetWorldDepthTarget());
	
	XMFLOAT4X4 proj = {};
	XMStoreFloat4x4(&proj, sceneView.projection);
	std::array<float, SSAO_SAMPLE_KERNEL_SIZE * 3> samples;

	size_t idx = 0;
	for (const vec3& v : sampleKernel)
	{
		samples[idx + 0] = v.x();
		samples[idx + 1] = v.y();
		samples[idx + 2] = v.z();
		idx += 3;
	}

	SSAOConstants ssaoConsts = {
		proj,
		pRenderer->GetWindowDimensionsAsFloat2(),
		this->radius,
		this->intensity,
		samples
	};

	pRenderer->BeginEvent("Occlusion Pass");

	pRenderer->SetShader(SSAOShader);
	pRenderer->BindRenderTarget(this->occlusionRenderTarget);
	pRenderer->UnbindDepthTarget();
	pRenderer->SetSamplerState("sNoiseSampler", this->noiseSampler);
	//pRenderer->SetSamplerState("sPointSampler", EDefaultSamplerState::POINT_SAMPLER);
	pRenderer->SetSamplerState("sLinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);
	pRenderer->SetTexture("texViewSpaceNormals", texNormals);
	pRenderer->SetTexture("texViewPositions", texPositions);
	pRenderer->SetTexture("texNoise", this->noiseTexture);
	//pRenderer->SetTexture("texDepth", depthTexture);
	pRenderer->SetConstantStruct("SSAO_constants", &ssaoConsts);
	pRenderer->SetBufferObj(EGeometry::QUAD);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	
	pRenderer->EndEvent();
}

void AmbientOcclusionPass::BilateralBlurPass(Renderer * pRenderer)
{
	pRenderer->UnbindRenderTargets();
	return;
	pRenderer->BeginEvent("Blur Pass");
	pRenderer->SetShader(bilateralBlurShader);
	
	//pRenderer->BindRenderTarget(this->renderTarget);

	pRenderer->SetBufferObj(EGeometry::QUAD);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	pRenderer->EndEvent();
}

void AmbientOcclusionPass::GaussianBlurPass(Renderer * pRenderer)
{
	const TextureID texOcclusion = pRenderer->GetRenderTargetTexture(this->occlusionRenderTarget);
	const vec2 texDimensions = pRenderer->GetWindowDimensionsAsFloat2();

	pRenderer->BeginEvent("Blur Pass");

	pRenderer->SetShader(EShaders::GAUSSIAN_BLUR_4x4);
	pRenderer->BindRenderTarget(this->blurRenderTarget);
	pRenderer->SetTexture("tOcclusion", texOcclusion);
	pRenderer->SetSamplerState("BlurSampler", EDefaultSamplerState::POINT_SAMPLER);
	pRenderer->SetConstant2f("inputTextureDimensions", texDimensions);
	pRenderer->SetBufferObj(EGeometry::QUAD);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	pRenderer->EndEvent();
}
