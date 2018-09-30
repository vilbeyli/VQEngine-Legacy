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
#include "SceneResources.h"
#include "Engine.h"
#include "GameObject.h"
#include "Light.h"
#include "SceneView.h"

#include "Utilities/Log.h"

#include "Renderer/Renderer.h"


#include <algorithm>
#ifdef min
#undef min
#endif

#define ENABLE_COMPUTE_BLUR             1
#define ENABLE_COMPUTE_BLUR_TRANSPOZE   1
constexpr bool USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH = false;

#define USE_COMPUTE_BLUR                ENABLE_COMPUTE_BLUR || ENABLE_COMPUTE_BLUR_TRANSPOZE



void BloomPass::Initialize(Renderer* pRenderer, const Settings::Bloom& bloomSettings, const RenderTargetDesc& rtDesc)
{
	this->_colorRT = pRenderer->AddRenderTarget(rtDesc);
	this->_brightRT = pRenderer->AddRenderTarget(rtDesc);
	this->_finalRT = pRenderer->AddRenderTarget(rtDesc);
	this->_blurPingPong[0] = pRenderer->AddRenderTarget(rtDesc);
	this->_blurPingPong[1] = pRenderer->AddRenderTarget(rtDesc);

	const int BLUR_KERNEL_DIMENSION = 21;	// should be odd
	const char* pFSQ_VS = "FullScreenquad_vs.hlsl";
	const ShaderDesc bloomPipelineShaders[] =
	{
		ShaderDesc{"Bloom",
			ShaderStageDesc{pFSQ_VS   , {} },
			ShaderStageDesc{"Bloom_ps.hlsl", {} },
		},
		ShaderDesc{"Blur",
			ShaderStageDesc{pFSQ_VS  , {} },
			ShaderStageDesc{"Blur_ps.hlsl", { { "KERNEL_DIMENSION", std::to_string(BLUR_KERNEL_DIMENSION) } } }
		},
		ShaderDesc{"BloomCombine",
			ShaderStageDesc{pFSQ_VS          , {} },
			ShaderStageDesc{"BloomCombine_ps.hlsl", {} },
		},
	};
	this->_bloomFilterShader = pRenderer->CreateShader(bloomPipelineShaders[0]);
	this->_blurShader = pRenderer->CreateShader(bloomPipelineShaders[1]);
	this->_bloomCombineShader = pRenderer->CreateShader(bloomPipelineShaders[2]);

	D3D11_SAMPLER_DESC blurSamplerDesc = {};
	blurSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	this->_blurSampler = pRenderer->CreateSamplerState(blurSamplerDesc);

#if USE_COMPUTE_BLUR
	// Blur Compute Resources
	TextureDesc texDesc = TextureDesc();
	texDesc.usage = ETextureUsage::COMPUTE_RW_TEXTURE;
	texDesc.height = pRenderer->WindowHeight();
	texDesc.width = pRenderer->WindowWidth();
	texDesc.format = rtDesc.textureDesc.format;

	// Dispatch() spawns a thread group among X or Y dimensions.
	// The compute shader kernel describes the thread group size
	// essentially the other dimension (of X or Y) to cover the entire image.
	//
	// In short, Dispatch() spawns a thread group, which corresponds to a 
	// a row or a column of pixels of the image so that each thread covers
	// one pixel.
	//
	const char* pCBufferPreProcessorValue = USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH ? "1" : "0";
	const int COMPUTE_KERNEL_DIMENSION = 1024;
	const ShaderDesc CSDescV =
	{
		"Blur_Compute_Vertical",
		ShaderStageDesc { "Blur_cs.hlsl", {
			{ "VERTICAL"    , "1" },
			{ "HORIZONTAL"  , "0" },
			{ "IMAGE_SIZE_X", std::to_string(texDesc.width) },
			{ "IMAGE_SIZE_Y", std::to_string(texDesc.height) },
			{ "THREAD_GROUP_SIZE_X", "1" },
			{ "THREAD_GROUP_SIZE_Y", std::to_string(COMPUTE_KERNEL_DIMENSION) },
			{ "THREAD_GROUP_SIZE_Z", "1"},
			{ "USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH", pCBufferPreProcessorValue}, // use this w/ "1" to make blur strength dynamic
			{ "PASS_COUNT", std::to_string(bloomSettings.blurStrength) },          // lets us unroll the blur strength loop (faster)
			{ "KERNEL_DIMENSION", std::to_string(BLUR_KERNEL_DIMENSION) } }
		}
	};
	const ShaderDesc CSDescH =
	{
		"Blur_Compute_Horizontal",
		ShaderStageDesc { "Blur_cs.hlsl", {
			{ "VERTICAL"   , "0" },
			{ "HORIZONTAL" , "1" },
			{ "IMAGE_SIZE_X", std::to_string(texDesc.width) },
			{ "IMAGE_SIZE_Y", std::to_string(texDesc.height) },
			{ "THREAD_GROUP_SIZE_X", std::to_string(COMPUTE_KERNEL_DIMENSION) },
			{ "THREAD_GROUP_SIZE_Y", "1" },
			{ "THREAD_GROUP_SIZE_Z", "1"},
			{ "USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH", pCBufferPreProcessorValue}, // use this w/ "1" to make blur strength dynamic
			{ "PASS_COUNT", std::to_string(bloomSettings.blurStrength) },          // lets us unroll the blur strength loop (faster)
			{ "KERNEL_DIMENSION", std::to_string(BLUR_KERNEL_DIMENSION) } }
		}
	};
#endif
#if ENABLE_COMPUTE_BLUR

	this->blurComputeOutputPingPong[0] = pRenderer->CreateTexture2D(texDesc);
	this->blurComputeOutputPingPong[1] = pRenderer->CreateTexture2D(texDesc);
	this->blurComputeShaderPingPong[0] = pRenderer->CreateShader(CSDescH);
	this->blurComputeShaderPingPong[1] = pRenderer->CreateShader(CSDescV);
	mSelectedBloomShader = BloomShader::CS_1D_Kernels;
#endif

#if ENABLE_COMPUTE_BLUR_TRANSPOZE
#if !ENABLE_COMPUTE_BLUR
	// Compute Blur Transpoze needs the regular compute horizontal blur shader
	this->blurComputeOutputPingPong[0] = pRenderer->CreateTexture2D(texDesc);
	// as well as one output buffer to store the results in
	this->blurComputeShaderPingPong[0] = pRenderer->CreateShader(CSDescH);
#endif
	const ShaderDesc CSDescHTz =
	{
		"Blur_Compute_Transpoze",
		ShaderStageDesc { "BlurTranspoze_cs.hlsl", {
			{ "IMAGE_SIZE_X", std::to_string(texDesc.height) },
			{ "IMAGE_SIZE_Y", std::to_string(texDesc.width) },
			{ "THREAD_GROUP_SIZE_X", std::to_string(COMPUTE_KERNEL_DIMENSION) },
			{ "THREAD_GROUP_SIZE_Y", "1" },
			{ "THREAD_GROUP_SIZE_Z", "1" },
			{ "USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH", pCBufferPreProcessorValue}, // use this w/ "1" to make blur strength dynamic
			{ "PASS_COUNT", std::to_string(bloomSettings.blurStrength) },          // lets us unroll the blur strength loop (faster)
			{ "KERNEL_DIMENSION", std::to_string(BLUR_KERNEL_DIMENSION) } }
		}
	};
	this->blurHorizontalTranspozeComputeShader = pRenderer->CreateShader(CSDescHTz);

	// Transpose Image Shader and Resource
	const ShaderDesc CSDescTranspose =
	{
		"Transpose_Compute",
		ShaderStageDesc { "Transpose_cs.hlsl", {} }
	};
	this->transpozeCompute = pRenderer->CreateShader(CSDescTranspose);

	texDesc = TextureDesc();
	texDesc.usage = ETextureUsage::COMPUTE_RW_TEXTURE;
	texDesc.height = pRenderer->WindowWidth();
	texDesc.width = pRenderer->WindowHeight();
	texDesc.format = rtDesc.textureDesc.format;
	this->texTransposedImage = pRenderer->CreateTexture2D(texDesc);

	mSelectedBloomShader = BloomShader::CS_1D_Kernels_Transpoze_Out;
#endif
}

struct BlurParameters { unsigned blurStrength; };
void BloomPass::Render(Renderer* pRenderer, CPUProfiler* pCPU, GPUProfiler* pGPU, RenderTargetID _worldRenderTarget, const Settings::Bloom& settings) const
{
	const TextureID worldTexture = pRenderer->GetRenderTargetTexture(_worldRenderTarget);
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);
	const ShaderID currentShader = pRenderer->GetActiveShader();

	//pCPU->BeginEntry("Bloom");
	pRenderer->BeginEvent("Bloom");
	pGPU->BeginEntry("Bloom Filter");

	// bright filter
	pRenderer->BeginEvent("Bright Filter");
	pRenderer->SetShader(this->_bloomFilterShader, true);
	pRenderer->BindRenderTargets(_colorRT, _brightRT);
	pRenderer->UnbindDepthTarget();
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->Apply();
	pRenderer->SetTexture("worldRenderTarget", worldTexture);
	pRenderer->SetConstant1f("BrightnessThreshold", settings.brightnessThreshold);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();

	pGPU->EndEntry();

	// blur
	const TextureID brightTexture = pRenderer->GetRenderTargetTexture(_brightRT);
	const int BlurPassCount = settings.blurStrength * 2;	// 1 pass for Horizontal and Vertical each

	BlurParameters params;
	params.blurStrength = settings.blurStrength;
	switch (mSelectedBloomShader)
	{
	case BloomPass::PS_1D_Kernels:	// PIXEL_SHADER_BLUR : 1.78 ms 1080p
	{
		pGPU->BeginEntry("Bloom Blur<PS>");
		pRenderer->BeginEvent("Blur Pass");
		pRenderer->SetShader(_blurShader, true);
		for (int i = 0; i < BlurPassCount; ++i)
		{
			const int isHorizontal = i % 2;
			const TextureID pingPong = pRenderer->GetRenderTargetTexture(_blurPingPong[1 - isHorizontal]);
			const int texWidth = pRenderer->GetTextureObject(pingPong)._width;
			const int texHeight = pRenderer->GetTextureObject(pingPong)._height;

			pRenderer->UnbindRenderTargets();
			pRenderer->Apply();
			pRenderer->BindRenderTarget(_blurPingPong[isHorizontal]);
			pRenderer->SetConstant1i("isHorizontal", 1 - isHorizontal);
			pRenderer->SetConstant1i("textureWidth", texWidth);
			pRenderer->SetConstant1i("textureHeight", texHeight);
			pRenderer->SetTexture("InputTexture", i == 0 ? brightTexture : pingPong);
			pRenderer->SetSamplerState("BlurSampler", _blurSampler);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		}
		pRenderer->EndEvent(); // PIXEL_SHADER_BLUR
		pGPU->EndEntry();
	}	break;

#if ENABLE_COMPUTE_BLUR
	case BloomPass::CS_1D_Kernels:
	{
		pGPU->BeginEntry("Bloom Blur<CS>");
		pRenderer->BeginEvent("Blur Compute Pass");
		for (int i = 0; i < 2; ++i)
		{
			const int INDEX_PING_PONG = i % 2;
			const int INDEX_PING_PONG_OTHER = (i + 1) % 2;

			const int DISPATCH_GROUP_Y = i % 2 == 0		// HORIZONTAL BLUR
				? static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().y())
				: 1;
			const int DISPATCH_GROUP_X = i % 2 != 0		// VERTICAL BLUR
				? static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().x())
				: 1;
			const int DISPATCH_GROUP_Z = 1;

			pRenderer->SetShader(this->blurComputeShaderPingPong[INDEX_PING_PONG], true);
			pRenderer->SetTexture("texColorIn", i == 0 ? brightTexture : this->blurComputeOutputPingPong[INDEX_PING_PONG_OTHER]);
			pRenderer->SetUnorderedAccessTexture("texColorOut", this->blurComputeOutputPingPong[INDEX_PING_PONG]);
			pRenderer->SetSamplerState("sSampler", EDefaultSamplerState::POINT_SAMPLER);
			if(USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH)
				pRenderer->SetConstantStruct("cBlurParameters", &params);
			pRenderer->Apply();
			pRenderer->Dispatch(DISPATCH_GROUP_X, DISPATCH_GROUP_Y, DISPATCH_GROUP_Z);
		}
		pRenderer->EndEvent();
		pGPU->EndEntry();
	}	break;
#endif

#if ENABLE_COMPUTE_BLUR_TRANSPOZE
	case BloomPass::CS_1D_Kernels_Transpoze_Out:
	{
		pGPU->BeginEntry("Bloom Blur<CS_T>");
		pRenderer->BeginEvent("Blur Compute_Transpoze Pass");

		// Horizontal Blur
		//
		int       DISPATCH_GROUP_X = 1;
		int       DISPATCH_GROUP_Y = static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().y());
		const int DISPATCH_GROUP_Z = 1;
		
		pRenderer->SetShader(this->blurComputeShaderPingPong[0], true, true);
		pRenderer->SetTexture("texColorIn", brightTexture);
		pRenderer->SetSamplerState("sSampler", EDefaultSamplerState::POINT_SAMPLER);
		if (USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH)
			pRenderer->SetConstantStruct("cBlurParameters", &params);
		pRenderer->SetUnorderedAccessTexture("texColorOut", this->blurComputeOutputPingPong[0]);
		pRenderer->Apply();
		pRenderer->Dispatch(DISPATCH_GROUP_X, DISPATCH_GROUP_Y, DISPATCH_GROUP_Z);


		// Transpoze
		//
		DISPATCH_GROUP_X = static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().x() / 16);
		DISPATCH_GROUP_Y = static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().y() / 16);
		pRenderer->SetShader(this->transpozeCompute, true, true);
		pRenderer->SetUnorderedAccessTexture("texImageIn", this->blurComputeOutputPingPong[0]);
		pRenderer->SetUnorderedAccessTexture("texTranspozeOut", this->texTransposedImage);
		pRenderer->Apply();
		pRenderer->Dispatch(DISPATCH_GROUP_X, DISPATCH_GROUP_Y, DISPATCH_GROUP_Z);


		// Horizontal Blur on Transpozed Image In ==> Transpoze Out
		//
		DISPATCH_GROUP_X = 1;
		DISPATCH_GROUP_Y = static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().x());
		pRenderer->SetShader(this->blurHorizontalTranspozeComputeShader, true, true);
		pRenderer->SetTexture("texColorIn", this->texTransposedImage);
		pRenderer->SetSamplerState("sSampler", EDefaultSamplerState::POINT_SAMPLER);
		if (USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH)
			pRenderer->SetConstantStruct("cBlurParameters", &params);
		pRenderer->SetUnorderedAccessTexture("texColorOut", this->blurComputeOutputPingPong[0]);
		pRenderer->Apply();
		pRenderer->Dispatch(DISPATCH_GROUP_X, DISPATCH_GROUP_Y, DISPATCH_GROUP_Z);

		pRenderer->EndEvent();
		pGPU->EndEntry();
	}	break;
#endif
	default:
		Log::Warning("Unsupported Bloom Shader = %d", mSelectedBloomShader);
		break;
	}


	// additive blend combine
	const TextureID colorTex = pRenderer->GetRenderTargetTexture(_colorRT);
	const TextureID bloomTex = GetBloomTexture(pRenderer);

	pGPU->BeginEntry("Bloom Combine");
	pRenderer->BeginEvent("Combine");
	pRenderer->SetShader(_bloomCombineShader, true, true);
	pRenderer->BindRenderTarget(_finalRT);
	pRenderer->SetTexture("ColorTexture", colorTex);
	pRenderer->SetTexture("BloomTexture", bloomTex);
	pRenderer->SetSamplerState("BlurSampler", _blurSampler);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();

	//pCPU->EndEntry(); // bloom
	pRenderer->EndEvent(); // bloom
	pGPU->EndEntry(); // bloom
}


TextureID BloomPass::GetBloomTexture(const Renderer* pRenderer) const
{
	switch (mSelectedBloomShader)
	{
	case BloomPass::PS_1D_Kernels: return pRenderer->GetRenderTargetTexture(_blurPingPong[0]);
	case BloomPass::CS_1D_Kernels: return blurComputeOutputPingPong[1];
	case BloomPass::CS_1D_Kernels_Transpoze_Out: return blurComputeOutputPingPong[0];
	case BloomPass::NUM_BLOOM_SHADERS:
	default:
		return -1;
	}
}
