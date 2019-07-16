//	VQEngine | DirectX11 Renderer
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

#include "RenderPasses.h"
#include "Engine/Engine.h"
#include "Engine/GameObject.h"
#include "Engine/Light.h"
#include "Engine/SceneResourceView.h"
#include "Engine/Camera.h"
#include "Engine/SceneView.h"

#include "Renderer/Renderer.h"
#include "Renderer/D3DManager.h"

#include "Utilities/Log.h"


#include <array>



ShaderID RenderPass::sShaderTranspoze = -1;

void RenderPass::InitializeCommonSaders(Renderer* pRenderer)
{
	const ShaderDesc CSDescTranspose =
	{
		"Transpose_Compute",
		ShaderStageDesc { "Transpose_cs.hlsl", {} }
	};
	sShaderTranspoze = pRenderer->CreateShader(CSDescTranspose);
}


constexpr const EImageFormat HDR_Format = RGBA16F;
constexpr const EImageFormat LDR_Format = RGBA8UN;
static bool bFirstInitialization = true;

void PostProcessPass::UpdateSettings(const Settings::PostProcess& newSettings, Renderer* pRenderer)
{
	_settings = newSettings;
	_bloomPass.UpdateSettings(pRenderer, newSettings.bloom);
}

void PostProcessPass::Initialize(Renderer* pRenderer, const Settings::PostProcess& postProcessSettings)
{
#if USE_DX12

#else
	_settings = postProcessSettings;

	const EImageFormat imageFormat = _settings.HDREnabled ? HDR_Format : LDR_Format;

	RenderTargetDesc rtDesc = {};
	rtDesc.textureDesc.width  = pRenderer->WindowWidth();
	rtDesc.textureDesc.height = pRenderer->WindowHeight();
	rtDesc.textureDesc.mipCount = 1;
	rtDesc.textureDesc.arraySize = 1;
	rtDesc.textureDesc.format = imageFormat;
	rtDesc.textureDesc.usage = ETextureUsage::RENDER_TARGET_RW;
	rtDesc.format = imageFormat;
	this->_bloomPass.Initialize(pRenderer, _settings.bloom, rtDesc);
	
	// Tonemapping
	this->_tonemappingPass._finalRenderTarget = pRenderer->GetBackBufferRenderTarget();

	const char* pFSQ_VS = "FullScreenquad_vs.hlsl";

	// -- HDR Tonemapper --------------------------------------------------------------------------
	ShaderDesc tonemappingShaderDesc = ShaderDesc{"Tonemapping_HDR",
		ShaderStageDesc{ pFSQ_VS              , {} },
		ShaderStageDesc{ "Tonemapping_ps.hlsl", { {"HDR_TONEMAPPER", "1"} } }
	};
	this->_tonemappingPass._toneMappingShaderHDR = pRenderer->CreateShader(tonemappingShaderDesc);

	// -- LDR Tonemapper --------------------------------------------------------------------------
	tonemappingShaderDesc = ShaderDesc{ "Tonemapping_LDR",
		ShaderStageDesc{ pFSQ_VS              , {} },
		ShaderStageDesc{ "Tonemapping_ps.hlsl", { {"LDR_TONEMAPPER", "1"} } }
	};
	this->_tonemappingPass._toneMappingShaderLDR = pRenderer->CreateShader(tonemappingShaderDesc);

	// -- HDR Tonemapper (Single Channel)----------------------------------------------------------
	tonemappingShaderDesc = ShaderDesc{ "Tonemapping_HDR_SingleChannel",
		ShaderStageDesc{ pFSQ_VS              , {} },
		ShaderStageDesc{ "Tonemapping_ps.hlsl", { {"HDR_TONEMAPPER", "1"}, {"SINGLE_CHANNEL", "1"} } }
	};
	this->_tonemappingPass._toneMappingShaderHDR_SingleChannelColor = pRenderer->CreateShader(tonemappingShaderDesc);

	// -- LDR Tonemapper (Single Channel)----------------------------------------------------------
	tonemappingShaderDesc = ShaderDesc{ "Tonemapping_LDR_SingleChannel",
		ShaderStageDesc{ pFSQ_VS              , {} },
		ShaderStageDesc{ "Tonemapping_ps.hlsl", { {"LDR_TONEMAPPER", "1"}, {"SINGLE_CHANNEL", "1"} } }
	};
	this->_tonemappingPass._toneMappingShaderLDR_SingleChannelColor = pRenderer->CreateShader(tonemappingShaderDesc);

	// World Render Target
	this->_worldRenderTarget = pRenderer->AddRenderTarget(rtDesc);
#endif
}




void PostProcessPass::Render(Renderer* pRenderer, bool bBloomOn, TextureID inputTextureID) const
{
#if USE_DX12

#else
	const bool bBloom = bBloomOn && _settings.bloom.bEnabled;
	const auto IABuffersQuad = SceneResourceView::GetBuiltinMeshVertexAndIndexBufferID(EGeometry::FULLSCREENQUAD);

	pRenderer->BeginEvent("Post Processing");
	// ======================================================================================
	// BLOOM  PASS
	// ======================================================================================
	if (bBloom)
	{
		this->_bloomPass.Render(pRenderer, inputTextureID, _settings.bloom);
	}

	const TextureID toneMappingInputTex = bBloom
		? pRenderer->GetRenderTargetTexture(this->_bloomPass._finalRT)
		: inputTextureID;

	// ======================================================================================
	// TONEMAPPING PASS
	// ======================================================================================
	const bool bIsSingleChannel = toneMappingInputTex == -1 ? 1 : 0;

	pRenderer->BeginEvent("Tonemapping");
	pGPU->BeginEntry("Tonemapping");

	pRenderer->SetShader(_settings.HDREnabled 
		? (bIsSingleChannel ? _tonemappingPass._toneMappingShaderHDR_SingleChannelColor : _tonemappingPass._toneMappingShaderHDR)
		: (bIsSingleChannel ? _tonemappingPass._toneMappingShaderLDR_SingleChannelColor : _tonemappingPass._toneMappingShaderLDR)
	, true);

	pRenderer->UnbindDepthTarget();
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->SetSamplerState("Sampler", _bloomPass._blurSampler);
	pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);
	pRenderer->SetConstant1f("exposure", _settings.toneMapping.exposure);
	pRenderer->BindRenderTarget(_tonemappingPass._finalRenderTarget);
	pRenderer->SetTexture("ColorTexture", toneMappingInputTex);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	pRenderer->EndEvent();	// Tonemapping
	pGPU->EndEntry();		// Tonemapping
	pRenderer->EndEvent();	// Post Process
#endif
}





void DebugPass::Initialize(Renderer * pRenderer)
{
#if USE_DX12

#else
	_scissorsRasterizer = pRenderer->AddRasterizerState(ERasterizerCullMode::BACK, ERasterizerFillMode::SOLID, false, true);
#endif
}

void AAResolvePass::Initialize(Renderer* pRenderer, TextureID inputTextureID)
{
#if USE_DX12

#else
	// todo shader and others

	ShaderDesc shdDesc;
	shdDesc.shaderName = "AAResolveSahder";
	shdDesc.stages = {
		ShaderStageDesc{ "FullScreenQuad_vs.hlsl", {} },
		ShaderStageDesc{ "AAResolve_ps.hlsl", {} } 
	};

	const Texture& inputTex = pRenderer->GetTextureObject(inputTextureID);
	const EImageFormat format = pRenderer->GetTextureImageFormat(inputTextureID);

	const float AAScalingInverse = 1.0f / pRenderer->mAntiAliasing.fUpscaleFactor;

	RenderTargetDesc rtDesc;
	rtDesc.format = format;
	rtDesc.textureDesc.format = format;
	rtDesc.textureDesc.arraySize = 1;
	rtDesc.textureDesc.bGenerateMips = false;
	rtDesc.textureDesc.bIsCubeMap = false;
	rtDesc.textureDesc.height = static_cast<int>(inputTex._height * AAScalingInverse);
	rtDesc.textureDesc.width  = static_cast<int>(inputTex._width  * AAScalingInverse);
	rtDesc.textureDesc.mipCount = 1;
	rtDesc.textureDesc.usage = ETextureUsage::RENDER_TARGET_RW;
	
	this->mResolveShaderID = pRenderer->CreateShader(shdDesc);
	this->mResolveTarget = pRenderer->AddRenderTarget(rtDesc);
	this->mResolveInputTextureID = inputTextureID; // To be set by the engine, this set here is useless.
#endif
}

void AAResolvePass::Render(Renderer* pRenderer) const
{
#if USE_DX12

#else
	const auto IABuffersQuad = SceneResourceView::GetBuiltinMeshVertexAndIndexBufferID(EGeometry::FULLSCREENQUAD);
	pRenderer->SetShader(this->mResolveShaderID, true, true);
	pRenderer->UnbindDepthTarget();
	pRenderer->BindRenderTarget(this->mResolveTarget);
	pRenderer->SetViewport(pRenderer->GetWindowDimensionsAsFloat2());
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->SetSamplerState("LinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);
	pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);
	pRenderer->SetTexture("ColorTexture", this->mResolveInputTextureID);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
#endif
}
