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

#include "Renderer/Renderer.h"


#define ENABLE_COMPUTE_PASS_UNIT_TEST   0

#define ENABLE_BILATERAL_BLUR_PASS      1

constexpr size_t SSAO_SAMPLE_KERNEL_SIZE = 64;
TextureID AmbientOcclusionPass::whiteTexture4x4 = -1;

//constexpr size_t sz = sizeof(SSAOConstants);
constexpr size_t VEC_SZ = 4;
struct SSAOConstants
{
	XMFLOAT4X4 matProjection;
	//-----------------------------
	XMFLOAT4X4 matProjectionInverse;
	//-----------------------------
	vec2 screenSize;
	float radius;
	float intensity;
	//-----------------------------
	std::array<float, SSAO_SAMPLE_KERNEL_SIZE * VEC_SZ> samples;
};

void AmbientOcclusionPass::Initialize(Renderer * pRenderer)
{
	// CREATE SAMPLE KERNEL
	//--------------------------------------------------------------------
	constexpr size_t NOISE_KERNEL_SIZE = 4;

	// src: https://john-chapman-graphics.blogspot.nl/2013/01/ssao-tutorial.html
	for (size_t i = 0; i < SSAO_SAMPLE_KERNEL_SIZE; i++)
	{
		// get a random direction in tangent space, Z-up.
		// As the sample kernel will be oriented along the surface normal, 
		// the resulting sample vectors will all end up in the hemisphere.
		vec3 sample
		(
			RandF(-1, 1),
			RandF(-1, 1),
			RandF(0, 1)	// hemisphere normal direction (up)
		);
		sample.normalize();				// bring the sample to the hemisphere surface
		sample = sample * RandF(0.1f, 1);	// scale to distribute samples within the hemisphere

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
		vec3 noise
		(
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
	texDesc.pData = this->noiseKernel.data();
	texDesc.dataPitch = sizeof(vec4) * NOISE_KERNEL_SIZE;
	this->noiseTexture = pRenderer->CreateTexture2D(texDesc);

	const float whiteValue = 1.0f;
	std::vector<vec4> white4x4 = std::vector<vec4>(16, vec4(whiteValue, whiteValue, whiteValue, 1));
	texDesc.width = texDesc.height = 4;
	texDesc.texFileName = "white4x4";
	texDesc.pData = white4x4.data();
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
	const EImageFormat imageFormat = R32F;
	RenderTargetDesc rtDesc = {};
	rtDesc.textureDesc.width  = pRenderer->WindowWidth();
	rtDesc.textureDesc.height = pRenderer->WindowHeight();
	rtDesc.textureDesc.mipCount = 1;
	rtDesc.textureDesc.arraySize = 1;
	rtDesc.textureDesc.format = imageFormat;
	rtDesc.textureDesc.usage = ETextureUsage::RENDER_TARGET_RW;
	rtDesc.format = imageFormat;

	this->occlusionRenderTarget = pRenderer->AddRenderTarget(rtDesc);
	this->blurRenderTarget = pRenderer->AddRenderTarget(rtDesc);

	// SHADER
	//--------------------------------------------------------------------
	const ShaderDesc ssaoShaderDesc =
	{ "SSAO_Shader",
		ShaderStageDesc{ "FullscreenQuad_vs.hlsl", {} },
		ShaderStageDesc{ "SSAO_ps.hlsl", {} },
	};
	this->SSAOShader = pRenderer->CreateShader(ssaoShaderDesc);
#if SSAO_DEBUGGING
	this->radius = 6.5f;
	this->intensity = 1.0f;
#endif

#if USE_COMPUTE_PASS_UNIT_TEST
	const ShaderDesc testComputeShaderDesc =
	{
		"TestCompute",
		ShaderStageDesc { "testCompute_cs.hlsl", {} }
	};
	this->testComputeShader = pRenderer->CreateShader(testComputeShaderDesc);

#if 0 // this leaks GPU memory...
	BufferDesc uaBufferDesc;
	uaBufferDesc.mType = EBufferType::COMPUTE_RW_TEXTURE;
	uaBufferDesc.mUsage = EBufferUsage::GPU_READ_WRITE;
	uaBufferDesc.mStride = 1920 * sizeof(vec4);
	uaBufferDesc.mElementCount = 1080;
	this->UABuffer = pRenderer->CreateBuffer(uaBufferDesc);
#endif

	texDesc = TextureDesc();
	texDesc.usage = ETextureUsage::COMPUTE_RW_TEXTURE;
	texDesc.height = 1080;
	texDesc.width = 1920;
	texDesc.format = EImageFormat::RGBA32F;
	this->RWTex2D = pRenderer->CreateTexture2D(texDesc);
#endif
#ifdef USE_COMPUTE_SSAO
	const ShaderDesc CSDesc =
	{
		"SSAO_Compute",
		ShaderStageDesc { "SSAO_cs.hlsl", {} }
	};
	this->ssaoComputeShader = pRenderer->CreateShader(CSDesc);

	texDesc = TextureDesc();
	texDesc.usage = ETextureUsage::COMPUTE_RW_TEXTURE;
	texDesc.height = 1080;
	texDesc.width = 1920;
	texDesc.format = EImageFormat::RGBA32F;
	this->texSSAOComputeOutput = pRenderer->CreateTexture2D(texDesc);
#endif

#ifdef ENABLE_BILATERAL_BLUR_PASS
	texDesc = TextureDesc();
	texDesc.usage = ETextureUsage::COMPUTE_RW_TEXTURE;
	texDesc.width = pRenderer->WindowWidth();
	texDesc.height = pRenderer->WindowHeight();
	texDesc.format = EImageFormat::RGBA32F;

	bilateralBlurUAV = pRenderer->CreateTexture2D(texDesc);

#endif
}



void AmbientOcclusionPass::RenderAmbientOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView)
{
	// SIMPLE SSAO
	//
	pGPU->BeginEntry("SSAO");
	// early out if we are not rendering anything into the G-Buffer
	if (sceneView.culledOpaqueList.empty() && sceneView.culluedOpaqueInstancedRenderListLookup.empty())
	{
		pRenderer->BindRenderTarget(this->occlusionRenderTarget);
		pRenderer->UnbindDepthTarget();
		pRenderer->BeginRender(ClearCommand::Color(1, 1, 1, 1));
	}
	else
	{
		pRenderer->BeginEvent("Occlusion Pass");
		pGPU->BeginEntry("Occl");
		RenderOcclusion(pRenderer, texNormals, sceneView);
		pGPU->EndEntry();
		pRenderer->EndEvent(); // Occlusion Pass

		pRenderer->BeginEvent("Blur Simple 4x4");
		pGPU->BeginEntry("Simple Blur");
		GaussianBlurPass(pRenderer);
		pGPU->EndEntry();
		pRenderer->EndEvent();
	}
	pGPU->EndEntry(); // SSAO


	// INTERLEAVED SSAO
	//
	pGPU->BeginEntry("SSAO_Interleaved");
	// early out if we are not rendering anything into the G-Buffer
	if (sceneView.culledOpaqueList.empty() && sceneView.culluedOpaqueInstancedRenderListLookup.empty())
	{
		//pRenderer->BindRenderTarget(this->occlusionRenderTarget);
		//pRenderer->UnbindDepthTarget();
		//pRenderer->BeginRender(ClearCommand::Color(1, 1, 1, 1));
	}
	else
	{
		pRenderer->BeginEvent("Deinterleave Pass");
		pGPU->BeginEntry("Deinterleave");

		pGPU->EndEntry();
		pRenderer->EndEvent(); // Deinterleave Pass

		pRenderer->BeginEvent("Interleaved Occlusion Pass");
		pGPU->BeginEntry("Occl<Intlrv>");
		RenderOcclusionInterleaved(pRenderer, texNormals, sceneView);
		pGPU->EndEntry();
		pRenderer->EndEvent(); // Interleaved Occlusion Pass


		pRenderer->BeginEvent("Blur Bilateral");
		pGPU->BeginEntry("Bilateral Blur");
		BilateralBlurPass(pRenderer);
		pGPU->EndEntry();
		pRenderer->EndEvent();


		pRenderer->BeginEvent("Interleave Pass");
		pGPU->BeginEntry("Interleave");

		pGPU->EndEntry();
		pRenderer->EndEvent(); // Interleave Pass
	}
	pGPU->EndEntry();
}



void AmbientOcclusionPass::RenderOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView)
{
	pRenderer->BeginEvent("Occlusion Pass");

	const TextureID depthTexture = pRenderer->GetDepthTargetTexture(ENGINE->GetWorldDepthTarget());
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);

	XMFLOAT4X4 proj = {};
	XMStoreFloat4x4(&proj, sceneView.proj);

	XMFLOAT4X4 projInv = {};
	XMStoreFloat4x4(&projInv, sceneView.projInverse);

	std::array<float, SSAO_SAMPLE_KERNEL_SIZE * VEC_SZ> samples;
	size_t idx = 0;
	for (const vec3& v : sampleKernel)
	{
		samples[idx + 0] = v.x();
		samples[idx + 1] = v.y();
		samples[idx + 2] = v.z();
		samples[idx + 3] = -1.0f;
		idx += VEC_SZ;
	}

	SSAOConstants ssaoConsts = {
		proj,
		projInv,
		pRenderer->GetWindowDimensionsAsFloat2(),
#if SSAO_DEBUGGING
		this->radius,
		this->intensity,
#else
		sceneView.sceneRenderSettings.ssao.radius,
		sceneView.sceneRenderSettings.ssao.intensity,
#endif
		samples
	};

	pRenderer->SetShader(SSAOShader, true);
	pRenderer->BindRenderTarget(this->occlusionRenderTarget);
	pRenderer->UnbindDepthTarget();
	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
	pRenderer->SetSamplerState("sNoiseSampler", this->noiseSampler);
	pRenderer->SetSamplerState("sPointSampler", EDefaultSamplerState::POINT_SAMPLER);
	//pRenderer->SetSamplerState("sLinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);
	pRenderer->SetTexture("texViewSpaceNormals", texNormals);
	pRenderer->SetTexture("texNoise", this->noiseTexture);
	pRenderer->SetTexture("texDepth", depthTexture);
	pRenderer->SetConstantStruct("SSAO_constants", &ssaoConsts);
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	pRenderer->EndEvent(); // Occlusion Pass
}

void AmbientOcclusionPass::RenderOcclusionInterleaved(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView)
{
#if 0
	const TextureID depthTexture = pRenderer->GetDepthTargetTexture(ENGINE->GetWorldDepthTarget());
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);

	XMFLOAT4X4 proj = {};
	XMStoreFloat4x4(&proj, sceneView.proj);

	XMFLOAT4X4 projInv = {};
	XMStoreFloat4x4(&projInv, sceneView.projInverse);

	std::array<float, SSAO_SAMPLE_KERNEL_SIZE * VEC_SZ> samples;
	size_t idx = 0;
	for (const vec3& v : sampleKernel)
	{
		samples[idx + 0] = v.x();
		samples[idx + 1] = v.y();
		samples[idx + 2] = v.z();
		samples[idx + 3] = -1.0f;
		idx += VEC_SZ;
	}

	SSAOConstants ssaoConsts = {
		proj,
		projInv,
		pRenderer->GetWindowDimensionsAsFloat2(),
#if SSAO_DEBUGGING
		this->radius,
		this->intensity,
#else
		sceneView.sceneRenderSettings.ssao.radius,
		sceneView.sceneRenderSettings.ssao.intensity,
#endif
		samples
	};

	pRenderer->BeginEvent("Occlusion Pass");

	pRenderer->SetShader(SSAOShader, true);
	pRenderer->BindRenderTarget(this->occlusionRenderTarget);
	pRenderer->UnbindDepthTarget();
	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
	pRenderer->SetSamplerState("sNoiseSampler", this->noiseSampler);
	pRenderer->SetSamplerState("sPointSampler", EDefaultSamplerState::POINT_SAMPLER);
	//pRenderer->SetSamplerState("sLinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);
	pRenderer->SetTexture("texViewSpaceNormals", texNormals);
	pRenderer->SetTexture("texNoise", this->noiseTexture);
	pRenderer->SetTexture("texDepth", depthTexture);
	pRenderer->SetConstantStruct("SSAO_constants", &ssaoConsts);
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	pRenderer->EndEvent();
#endif

	// INTERLEAVE
	//

	// OCCLUSION x4
	//

	// DEINTERLEAVE
	//
}


void AmbientOcclusionPass::BilateralBlurPass(Renderer * pRenderer)
{
#ifdef ENABLE_BILATERAL_BLUR_PASS
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);
	const TextureID texOcclusion = pRenderer->GetRenderTargetTexture(this->occlusionRenderTarget);
	const vec2 texDimensions = pRenderer->GetWindowDimensionsAsFloat2();

	pRenderer->BeginEvent("Blur Pass <BiL>");
	pRenderer->SetShader(EShaders::BILATERAL_BLUR, true);
	//pRenderer->SetTexture("texNormals", 0);
	//pRenderer->SetTexture("texDepth", 0);
	//pRenderer->SetConstant2f("inputTextureDimensions", texDimensions);
	//pRenderer->SetTexture("texOcclusion", texOcclusion);
	//pRenderer->SetTexture("texBlurredOcclusionOut", this->bilateralBlurUAV);
	pRenderer->Apply();
	pRenderer->Dispatch(1, 1, 1);
	pRenderer->EndEvent();	// Blur Pass <BiL>
#endif
}

void AmbientOcclusionPass::GaussianBlurPass(Renderer * pRenderer)
{
	const TextureID texOcclusion = pRenderer->GetRenderTargetTexture(this->occlusionRenderTarget);
	const vec2 texDimensions = pRenderer->GetWindowDimensionsAsFloat2();
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);

	pRenderer->BeginEvent("Blur Pass <Gau>");

	pRenderer->SetShader(EShaders::GAUSSIAN_BLUR_4x4, true);
	pRenderer->BindRenderTarget(this->blurRenderTarget);
	pRenderer->SetTexture("tOcclusion", texOcclusion);
	pRenderer->SetSamplerState("BlurSampler", EDefaultSamplerState::POINT_SAMPLER);
	pRenderer->SetConstant2f("inputTextureDimensions", texDimensions);
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	pRenderer->EndEvent();


#if USE_COMPUTE_PASS_UNIT_TEST
	pRenderer->BeginEvent("Test Compute");
	pRenderer->SetShader(this->testComputeShader);
	////pRenderer->SetUABuffer(this->UABuffer);
	pRenderer->SetUnorderedAccessTexture("outColor", this->RWTex2D);
	pRenderer->Apply();
	pRenderer->Dispatch(120, 68, 1);
	pRenderer->EndEvent();
#endif

}
