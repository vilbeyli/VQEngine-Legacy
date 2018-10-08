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
#define ENABLE_INTERLEAVED_SSAO_PASS    0

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
	const ShaderDesc gaussianBlurShaderdesc = 
	{ "GaussianBlur4x4", 
		ShaderStageDesc{"FullscreenQuad_vs.hlsl" , {} },
		ShaderStageDesc{"GaussianBlur4x4_ps.hlsl", {} }
	};
	this->SSAOShader = pRenderer->CreateShader(ssaoShaderDesc);
	this->gaussianBlurShader = pRenderer->CreateShader(gaussianBlurShaderdesc);
#if SSAO_DEBUGGING
	this->radius = 17.5f;
	this->intensity = 3.40f;
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

#if ENABLE_BILATERAL_BLUR_PASS
	texDesc = TextureDesc();
	texDesc.usage = ETextureUsage::COMPUTE_RW_TEXTURE;
	texDesc.width = pRenderer->WindowWidth();
	texDesc.height = pRenderer->WindowHeight();
	texDesc.format = EImageFormat::RGBA32F;
	bilateralBlurUAVs[0] = pRenderer->CreateTexture2D(texDesc);
	bilateralBlurUAVs[1] = pRenderer->CreateTexture2D(texDesc);

	constexpr int BILATERAL_BLUR_PASS_COUNT = 1;
	constexpr int COMPUTE_KERNEL_DIMENSION = 1024;
	constexpr int BLUR_KERNEL_DIMENSION = 15;
	const ShaderDesc CSDescV =
	{
		"BilateralBlur<CS>_Vertical",
		ShaderStageDesc { "BilateralBlur_cs.hlsl", {
			{ "VERTICAL"    , "1" },
			{ "HORIZONTAL"  , "0" },
			{ "IMAGE_SIZE_X", std::to_string(texDesc.width) },
			{ "IMAGE_SIZE_Y", std::to_string(texDesc.height) },
			{ "THREAD_GROUP_SIZE_X", "1" },
			{ "THREAD_GROUP_SIZE_Y", std::to_string(COMPUTE_KERNEL_DIMENSION) },
			{ "THREAD_GROUP_SIZE_Z", "1"},
			{ "PASS_COUNT", std::to_string(BILATERAL_BLUR_PASS_COUNT) },          // lets us unroll the blur strength loop (faster)
			{ "KERNEL_DIMENSION", std::to_string(BLUR_KERNEL_DIMENSION) } }
		}
	};
	const ShaderDesc CSDescH =
	{
		"BilateralBlur<CS>_Horizontal",
		ShaderStageDesc { "BilateralBlur_cs.hlsl", {
			{ "VERTICAL"   , "0" },
			{ "HORIZONTAL" , "1" },
			{ "IMAGE_SIZE_X", std::to_string(texDesc.width) },
			{ "IMAGE_SIZE_Y", std::to_string(texDesc.height) },
			{ "THREAD_GROUP_SIZE_X", std::to_string(COMPUTE_KERNEL_DIMENSION) },
			{ "THREAD_GROUP_SIZE_Y", "1" },
			{ "THREAD_GROUP_SIZE_Z", "1"},
			{ "PASS_COUNT", std::to_string(BILATERAL_BLUR_PASS_COUNT) },          // lets us unroll the blur strength loop (faster)
			{ "KERNEL_DIMENSION", std::to_string(BLUR_KERNEL_DIMENSION) } }
		}
	};
	this->bilateralBlurShaderH = pRenderer->CreateShader(CSDescH);
	this->bilateralBlurShaderV = pRenderer->CreateShader(CSDescV);
#endif


#if ENABLE_INTERLEAVED_SSAO_PASS
	// INTERLEAVED SSAO RESOURCES
	const ShaderDesc deinterleaveShaderDesc = 
	{
		"Deinterleave_Shader",
		ShaderStageDesc{ "Deinterleave_cs.hlsl", {} },
	};
	this->deinterleaveShader = pRenderer->CreateShader(deinterleaveShaderDesc);

	TextureDesc deinterleavedDepthTextureArrayDesc = {};
	deinterleavedDepthTextureArrayDesc.arraySize     = 4;
	deinterleavedDepthTextureArrayDesc.bGenerateMips = false;
	deinterleavedDepthTextureArrayDesc.format        = imageFormat;
	deinterleavedDepthTextureArrayDesc.mipCount      = 1;
	deinterleavedDepthTextureArrayDesc.width         = static_cast<int>(pRenderer->WindowWidth()  / 2.0f);
	deinterleavedDepthTextureArrayDesc.height        = static_cast<int>(pRenderer->WindowHeight() / 2.0f);
	deinterleavedDepthTextureArrayDesc.usage         = ETextureUsage::COMPUTE_RW_TEXTURE;
	this->deinterleavedDepthTextures = pRenderer->CreateTexture2D(deinterleavedDepthTextureArrayDesc);
#endif // ENABLE_INTERLEAVED_SSAO_PASS
}



void AmbientOcclusionPass::RenderAmbientOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView)
{
	// SIMPLE SSAO
	//
	pRenderer->BeginEvent("SSAO");
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

		switch (quality)
		{
		case AmbientOcclusionPass::LOW:
			pGPU->BeginEntry("Simple Blur");
			GaussianBlurPass(pRenderer);
			pGPU->EndEntry();
			break;
		case AmbientOcclusionPass::HIGH:
			pGPU->BeginEntry("Bilateral Blur");
			BilateralBlurPass(pRenderer, texNormals);
			pGPU->EndEntry();
			break;
		}
	}
	pGPU->EndEntry(); // SSAO
	pRenderer->EndEvent(); // SSAO


#if ENABLE_INTERLEAVED_SSAO_PASS
	// INTERLEAVED SSAO
	//
	pRenderer->BeginEvent("SSAO_Interleaved");
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
		DeinterleaveDepth(pRenderer);
		pGPU->EndEntry();
		pRenderer->EndEvent(); // Deinterleave Pass

		pRenderer->BeginEvent("Interleaved Occlusion Pass");
		pGPU->BeginEntry("Occl<Intlrv>");
		RenderOcclusionInterleaved(pRenderer, texNormals, sceneView);
		pGPU->EndEntry();
		pRenderer->EndEvent(); // Interleaved Occlusion Pass


		pRenderer->BeginEvent("Blur Bilateral");
		pGPU->BeginEntry("Bilateral Blur");
		BilateralBlurPass(pRenderer, texNormals);
		pGPU->EndEntry();
		pRenderer->EndEvent();


		pRenderer->BeginEvent("Interleave Pass");
		pGPU->BeginEntry("Interleave");
		// TODO
		pGPU->EndEntry();
		pRenderer->EndEvent(); // Interleave Pass
	}
	pGPU->EndEntry();
	pRenderer->EndEvent(); // SSAO_Interleaved
#endif
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



void AmbientOcclusionPass::DeinterleaveDepth(Renderer* pRenderer)
{
	const TextureID texDepth = pRenderer->GetDepthTargetTexture(ENGINE->GetWorldDepthTarget());
	const vec2 imageDimensions = pRenderer->GetWindowDimensionsAsFloat2();

	pRenderer->BeginEvent("Deinterleave");
	pRenderer->SetShader(deinterleaveShader, true);
	pRenderer->SetTexture("texInput", texDepth);
	pRenderer->SetUnorderedAccessTexture("texOutputs", deinterleavedDepthTextures);
	pRenderer->Apply();
	pRenderer->Dispatch((int)imageDimensions.x() / 2, (int)imageDimensions.y() / 2, 1);
	pRenderer->EndEvent();
}

// TODO: impl
void AmbientOcclusionPass::RenderOcclusionInterleaved(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView)
{
	// https://developer.nvidia.com/sites/default/files/akamai/gameworks/samples/DeinterleavedTexturing.pdf

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

#if 0
	pRenderer->BeginEvent("Occlusion Pass");

	pRenderer->BindRenderTarget(this->occlusionRenderTarget);
	pRenderer->UnbindDepthTarget();
	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);

	//pRenderer->SetSamplerState("sLinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);


#endif


#if 0
	// OCCLUSION x4
	//
	pRenderer->BeginEvent("Deinterleaved Occlusion Pass");
	pRenderer->SetShader(deinterleavedSSAOShader, true);
	pRenderer->SetTexture("texViewSpaceNormals", texNormals);
	pRenderer->SetTexture("texNoise", this->noiseTexture);
	pRenderer->SetTexture("texDepth", texDepth);
	pRenderer->SetConstantStruct("SSAO_constants", &ssaoConsts);
	pRenderer->SetSamplerState("sNoiseSampler", this->noiseSampler);
	pRenderer->SetSamplerState("sPointSampler", EDefaultSamplerState::POINT_SAMPLER);
	for (int i = 0; i < 4; ++i)
	{
		//pRenderer->SetUnorderedAccessTexture("texOut")
		//pRenderer->Apply();
		//pRenderer->
	}
	pRenderer->EndEvent();
#endif
}


void AmbientOcclusionPass::BilateralBlurPass(Renderer * pRenderer, const TextureID texNormals)
{
#if ENABLE_BILATERAL_BLUR_PASS
	const TextureID texOcclusion = pRenderer->GetRenderTargetTexture(this->occlusionRenderTarget);
	const TextureID texDepth = pRenderer->GetDepthTargetTexture(ENGINE->GetWorldDepthTarget());
	const vec2 texDimensions = pRenderer->GetWindowDimensionsAsFloat2();
	const int DISPATCH_GROUP_Z = 1;

	bilateralBlurParameters.depthThreshold = 0.0001f;
	bilateralBlurParameters.normalDotThreshold = 0.995f;

	pRenderer->BeginEvent("Blur Pass <BiL>");

	// HORIZONTAL BLUR
	//
	int DISPATCH_GROUP_X = 1;
	int DISPATCH_GROUP_Y = static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().y());

	pRenderer->SetShader(this->bilateralBlurShaderH, true, true);
	pRenderer->SetSamplerState("sSampler", EDefaultSamplerState::POINT_SAMPLER);
	pRenderer->SetTexture("texOcclusion", texOcclusion);
	pRenderer->SetTexture("texNormals", texNormals);
	pRenderer->SetTexture("texDepth", texDepth);
	pRenderer->SetConstantStruct("cParameters", &bilateralBlurParameters);
	pRenderer->SetUnorderedAccessTexture("texBlurredOcclusionOut", this->bilateralBlurUAVs[0]);
	pRenderer->Apply();
	pRenderer->Dispatch(DISPATCH_GROUP_X, DISPATCH_GROUP_Y, DISPATCH_GROUP_Z);

	// VERTICAL BLUR
	//
	DISPATCH_GROUP_X = static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().x());
	DISPATCH_GROUP_Y = 1;
	
#if 0
	pRenderer->SetShader(this->bilateralBlurShaderV);
	pRenderer->SetTexture("texOcclusion", this->bilateralBlurUAVs[0]);
	pRenderer->SetConstantStruct("cParameters", &bilateralBlurParameters);
	pRenderer->SetUnorderedAccessTexture("texBlurredOcclusionOut", this->bilateralBlurUAVs[1]);
	pRenderer->Apply();
	pRenderer->Dispatch(DISPATCH_GROUP_X, DISPATCH_GROUP_Y, DISPATCH_GROUP_Z);
#endif

	pRenderer->EndEvent();	// Blur Pass <BiL>
	pRenderer->SetShader(this->bilateralBlurShaderV, true, true);

#endif
}

void AmbientOcclusionPass::GaussianBlurPass(Renderer * pRenderer)
{
	const TextureID texOcclusion = pRenderer->GetRenderTargetTexture(this->occlusionRenderTarget);
	const vec2 texDimensions = pRenderer->GetWindowDimensionsAsFloat2();
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);

	pRenderer->BeginEvent("Blur Pass <Gau>");

	pRenderer->SetShader(this->gaussianBlurShader, true);
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
void AmbientOcclusionPass::ChangeQualityLevel(int upOrDown)
{
	int desiredQuality = this->quality + upOrDown;
	if (upOrDown > 0)	desiredQuality = desiredQuality >= SSAO_QUALITY_LEVEL_COUNT ? SSAO_QUALITY_LEVEL_COUNT-1 : desiredQuality;
	else				desiredQuality = desiredQuality < 0 ? 0 : desiredQuality;
	this->quality = static_cast<SSAOQuality>(desiredQuality);
	//Log::Info("SSAO Blur Quality: %d", this->quality);
}
TextureID AmbientOcclusionPass::GetBlurredAOTexture(Renderer* pRenderer) const
{
	TextureID ret = 0;
	switch (quality)
	{
	case AmbientOcclusionPass::LOW:
		ret = pRenderer->GetRenderTargetTexture(this->blurRenderTarget);	// gaussian blur RT
		break;
	case AmbientOcclusionPass::HIGH:
		ret = this->bilateralBlurUAVs[0];
		break;
	default:
		assert(false);
		break;
	}
	return ret;
}
