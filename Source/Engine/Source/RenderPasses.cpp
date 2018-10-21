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
#include "Engine.h"
#include "GameObject.h"
#include "Light.h"
#include "SceneResources.h"
#include "Camera.h"
#include "SceneView.h"

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
	this->_tonemappingPass._finalRenderTarget = pRenderer->GetDefaultRenderTarget();

	const char* pFSQ_VS = "FullScreenquad_vs.hlsl";
	const ShaderDesc tonemappingShaderDesc = ShaderDesc{"Tonemapping",
		ShaderStageDesc{ pFSQ_VS              , {} },
		ShaderStageDesc{ "Tonemapping_ps.hlsl", {} }
	};
	this->_tonemappingPass._toneMappingShader = pRenderer->CreateShader(tonemappingShaderDesc);

	// World Render Target
	this->_worldRenderTarget = pRenderer->AddRenderTarget(rtDesc);
}




void PostProcessPass::Render(Renderer* pRenderer, bool bBloomOn, TextureID texOverride) const
{
	const bool bBloom = bBloomOn && _settings.bloom.bEnabled && (texOverride == -1);
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);

	const TextureID worldTexture = pRenderer->GetRenderTargetTexture(_worldRenderTarget);
	const TextureID colorTex = texOverride == -1
		? ( pRenderer->GetRenderTargetTexture(bBloom ? _bloomPass._finalRT : _worldRenderTarget) )
		: ( texOverride );


	pRenderer->BeginEvent("Post Processing");
	// ======================================================================================
	// BLOOM  PASS
	// ======================================================================================
	if (bBloom) this->_bloomPass.Render(pRenderer, _worldRenderTarget, _settings.bloom);


	// ======================================================================================
	// TONEMAPPING PASS
	// ======================================================================================
	pRenderer->BeginEvent("Tonemapping");
	pGPU->BeginEntry("Tonemapping");

	pRenderer->UnbindDepthTarget();
	pRenderer->SetShader(_tonemappingPass._toneMappingShader, true);
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->SetSamplerState("Sampler", _bloomPass._blurSampler);
	pRenderer->SetConstant1f("exposure", _settings.toneMapping.exposure);
	pRenderer->SetConstant1f("isHDR", _settings.HDREnabled ? 1.0f : 0.0f);
	pRenderer->BindRenderTarget(_tonemappingPass._finalRenderTarget);
	pRenderer->SetTexture("ColorTexture", colorTex);

	// quick hack for outputting white texture for fullscreen AO debugging
	pRenderer->SetConstant1i("isSingleChannel", texOverride != -1 ? 1 : 0);
	// quick hack for outputting white texture for fullscreen AO debugging

	pRenderer->Apply();
	pRenderer->DrawIndexed();

	pRenderer->EndEvent();	// Tonemapping
	pGPU->EndEntry();		// Tonemapping
	pRenderer->EndEvent();	// Post Process
}





void DebugPass::Initialize(Renderer * pRenderer)
{
	_scissorsRasterizer = pRenderer->AddRasterizerState(ERasterizerCullMode::BACK, ERasterizerFillMode::SOLID, false, true);
}

