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



#define USE_COMPUTE_PASS_UNIT_TEST 0
#define USE_COMPUTE_SSAO           0
#define USE_COMPUTE_BLUR           1

constexpr int DRAW_INSTANCED_COUNT_DEPTH_PASS = 256;



void ShadowMapPass::InitializeSpotLightShadowMaps(Renderer* pRenderer, const Settings::ShadowMap& shadowMapSettings)
{
	this->mShadowMapDimension_Spot = static_cast<int>(shadowMapSettings.dimension);

#if _DEBUG
	//pRenderer->m_Direct3D->ReportLiveObjects("--------SHADOW_PASS_INIT");
#endif

	// check feature support & error handle:
	// https://msdn.microsoft.com/en-us/library/windows/apps/dn263150
	const bool bDepthOnly = true;
	const EImageFormat format = bDepthOnly ? R32 : R24G8;

	DepthTargetDesc depthDesc;
	depthDesc.format = bDepthOnly ? D32F : D24UNORM_S8U;
	TextureDesc& texDesc = depthDesc.textureDesc;
	texDesc.format = format;
	texDesc.usage = static_cast<ETextureUsage>(DEPTH_TARGET | RESOURCE);

	// CREATE DEPTH TARGETS: SPOT LIGHTS
	//
	texDesc.height = texDesc.width = mShadowMapDimension_Spot;
	texDesc.arraySize = NUM_SPOT_LIGHT_SHADOW;
	auto DepthTargetIDs = pRenderer->AddDepthTarget(depthDesc);
	this->mDepthTargets_Spot.resize(NUM_SPOT_LIGHT_SHADOW);
	std::copy(RANGE(DepthTargetIDs), this->mDepthTargets_Spot.begin());

	this->mShadowMapTextures_Spot = this->mDepthTargets_Spot.size() > 0
		? pRenderer->GetDepthTargetTexture(this->mDepthTargets_Spot[0])
		: -1;

	this->mShadowMapShader = EShaders::SHADOWMAP_DEPTH;


	ShaderDesc instancedShaderDesc = { "DepthShader",
		ShaderStageDesc{"DepthShader_vs.hlsl", { ShaderMacro{ "INSTANCED", "1"}, ShaderMacro{"INSTANCE_COUNT", std::to_string(DRAW_INSTANCED_COUNT_DEPTH_PASS) } } },
		ShaderStageDesc{"DepthShader_ps.hlsl" , {} }
	};
	this->mShadowMapShaderInstanced = pRenderer->CreateShader(instancedShaderDesc);


	ZeroMemory(&mShadowViewPort_Spot, sizeof(D3D11_VIEWPORT));
	this->mShadowViewPort_Spot.Height = static_cast<float>(mShadowMapDimension_Spot);
	this->mShadowViewPort_Spot.Width = static_cast<float>(mShadowMapDimension_Spot);
	this->mShadowViewPort_Spot.MinDepth = 0.f;
	this->mShadowViewPort_Spot.MaxDepth = 1.f;

}

void ShadowMapPass::InitializeDirectionalLightShadowMap(Renderer * pRenderer, const Settings::ShadowMap & shadowMapSettings)
{
	const int textureDimension = static_cast<int>(shadowMapSettings.dimension);
	const bool bDepthOnly = true;
	const EImageFormat format = bDepthOnly ? R32 : R24G8;

	DepthTargetDesc depthDesc;
	depthDesc.format = bDepthOnly ? D32F : D24UNORM_S8U;
	TextureDesc& texDesc = depthDesc.textureDesc;
	texDesc.format = format;
	texDesc.usage = static_cast<ETextureUsage>(DEPTH_TARGET | RESOURCE);

	texDesc.height = texDesc.width = textureDimension;
	texDesc.arraySize = 1;
	
#if 0
	// first time - add target
	if (this->mDepthTarget_Directional == -1)
	{
		this->mDepthTarget_Directional = pRenderer->AddDepthTarget(depthDesc)[0];
		this->mShadowMapTexture_Directional = pRenderer->GetDepthTargetTexture(this->mDepthTarget_Directional);
	}
	else // other times - resize target
	{
		pRenderer->ResizeDepthTarget(this->mDepthTarget_Directional, depthDesc);
	}
#else
	// always add depth target when loading a new scene
	this->mDepthTarget_Directional = pRenderer->AddDepthTarget(depthDesc)[0];
	this->mShadowMapTexture_Directional = pRenderer->GetDepthTargetTexture(this->mDepthTarget_Directional);
#endif

	mShadowViewPort_Directional.Width  = static_cast<FLOAT>(textureDimension);
	mShadowViewPort_Directional.Height = static_cast<FLOAT>(textureDimension);
	mShadowViewPort_Directional.MinDepth = 0.f;
	mShadowViewPort_Directional.MaxDepth = 1.f;
}

#define INSTANCED_DRAW 1
void ShadowMapPass::RenderShadowMaps(Renderer* pRenderer, const ShadowView& shadowView, GPUProfiler* pGPUProfiler) const
{
	//-----------------------------------------------------------------------------------------------
	struct PerObjectMatrices { XMMATRIX wvp; };
	struct InstancedObjectCBuffer { PerObjectMatrices objMatrices[DRAW_INSTANCED_COUNT_DEPTH_PASS]; };
	auto Is2DGeometry = [](MeshID mesh)
	{
		return mesh == EGeometry::TRIANGLE || mesh == EGeometry::QUAD || mesh == EGeometry::GRID;
	};
	auto RenderDepth = [&](const GameObject* pObj, const XMMATRIX& viewProj)
	{
		const ModelData& model = pObj->GetModelData();
		const PerObjectMatrices objMats = PerObjectMatrices({ pObj->GetTransform().WorldTransformationMatrix() * viewProj });

		pRenderer->SetConstantStruct("ObjMats", &objMats);
		std::for_each(model.mMeshIDs.begin(), model.mMeshIDs.end(), [&](MeshID id)
		{
			const RasterizerStateID rasterizerState = Is2DGeometry(id) ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(ENGINE->mpActiveScene, id);

			pRenderer->SetRasterizerState(rasterizerState);
			pRenderer->SetVertexBuffer(IABuffer.first);
			pRenderer->SetIndexBuffer(IABuffer.second);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		});
	};
	//-----------------------------------------------------------------------------------------------


	const bool bNoShadowingLights = shadowView.spots.empty() && shadowView.points.empty() && shadowView.pDirectional == nullptr;
	if (bNoShadowingLights) return;

	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_WRITE);
	pRenderer->SetShader(mShadowMapShader);					// shader for rendering z buffer

	// SPOT LIGHT SHADOW MAPS
	//
	pGPUProfiler->BeginEntry("Spots");
	pRenderer->SetViewport(mShadowViewPort_Spot);
	for (size_t i = 0; i < shadowView.spots.size(); i++)
	{
		const XMMATRIX viewProj = shadowView.spots[i]->GetLightSpaceMatrix();
		pRenderer->BeginEvent("Spot[" + std::to_string(i)  + "]: DrawSceneZ()");
#if _DEBUG
		if (shadowView.shadowMapRenderListLookUp.find(shadowView.spots[i]) == shadowView.shadowMapRenderListLookUp.end())
		{
			Log::Error("Spot light not found in shadowmap render list lookup");
			continue;;
		}
#endif
		pRenderer->BindDepthTarget(mDepthTargets_Spot[i]);	// only depth stencil buffer
		pRenderer->BeginRender(ClearCommand::Depth(1.0f));
		pRenderer->Apply();

		for (const GameObject* pObj : shadowView.shadowMapRenderListLookUp.at(shadowView.spots[i]))
		{
			RenderDepth(pObj, viewProj);
		}
		pRenderer->EndEvent();
	}
	pGPUProfiler->EndEntry();	// spots


	// DIRECTIONAL SHADOW MAP
	//
	if (shadowView.pDirectional != nullptr)
	{
		const XMMATRIX viewProj = shadowView.pDirectional->GetLightSpaceMatrix();

		pGPUProfiler->BeginEntry("Directional");
		pRenderer->BeginEvent("Directional: DrawSceneZ()");

		// RENDER NON-INSTANCED SCENE OBJECTS
		//
		pRenderer->SetViewport(mShadowViewPort_Directional);
		pRenderer->BindDepthTarget(mDepthTarget_Directional);
		pRenderer->Apply();
		pRenderer->BeginRender(ClearCommand::Depth(1.0f));
		for (const GameObject* pObj : shadowView.casters)
		{
			RenderDepth(pObj, viewProj);
		}
		

		// RENDER INSTANCED SCENE OBJECTS
		//
		pRenderer->SetShader(mShadowMapShaderInstanced);
		pRenderer->BindDepthTarget(mDepthTarget_Directional);

		InstancedObjectCBuffer cbuffer;
		for(const RenderListLookupEntry& MeshID_RenderList : shadowView.RenderListsPerMeshType)
		{
			const MeshID& mesh = MeshID_RenderList.first;
			const std::vector<const GameObject*>& renderList = MeshID_RenderList.second;
		
			const RasterizerStateID rasterizerState = EDefaultRasterizerState::CULL_NONE;// Is2DGeometry(mesh) ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(ENGINE->mpActiveScene, mesh);
		
			pRenderer->SetRasterizerState(rasterizerState);
			pRenderer->SetVertexBuffer(IABuffer.first);
			pRenderer->SetIndexBuffer(IABuffer.second);
		
			int batchCount = 0;
			do
			{
				int instanceID = 0;
				for (; instanceID < DRAW_INSTANCED_COUNT_DEPTH_PASS; ++instanceID)
				{
					const int renderListIndex = DRAW_INSTANCED_COUNT_DEPTH_PASS * batchCount + instanceID;
					if (renderListIndex == renderList.size())
						break;
		
					cbuffer.objMatrices[instanceID] =
					{
						renderList[renderListIndex]->GetTransform().WorldTransformationMatrix() * viewProj
					};
				}
		
				pRenderer->SetConstantStruct("ObjMats", &cbuffer);
				pRenderer->Apply();
				pRenderer->DrawIndexedInstanced(instanceID);
			} while (batchCount++ < renderList.size() / DRAW_INSTANCED_COUNT_DEPTH_PASS);
		}

		pRenderer->EndEvent();
		pGPUProfiler->EndEntry();
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

	const EImageFormat imageFormat = _settings.HDREnabled ? HDR_Format : LDR_Format;

	RenderTargetDesc rtDesc = {};
	rtDesc.textureDesc.width  = pRenderer->WindowWidth();
	rtDesc.textureDesc.height = pRenderer->WindowHeight();
	rtDesc.textureDesc.mipCount = 1;
	rtDesc.textureDesc.arraySize = 1;
	rtDesc.textureDesc.format = imageFormat;
	rtDesc.textureDesc.usage = ETextureUsage::RENDER_TARGET_RW;
	rtDesc.format = imageFormat;

	// Bloom effect
	this->_bloomPass._colorRT = pRenderer->AddRenderTarget(rtDesc);
	this->_bloomPass._brightRT = pRenderer->AddRenderTarget(rtDesc);
	this->_bloomPass._finalRT = pRenderer->AddRenderTarget(rtDesc);
	this->_bloomPass._blurPingPong[0] = pRenderer->AddRenderTarget(rtDesc);
	this->_bloomPass._blurPingPong[1] = pRenderer->AddRenderTarget(rtDesc);

	const char* pFSQ_VS = "FullScreenquad_vs.hlsl";
	const ShaderDesc bloomPipelineShaders[] = 
	{
		ShaderDesc{"Bloom", 
			ShaderStageDesc{pFSQ_VS   , {} },
			ShaderStageDesc{"Bloom_ps.hlsl", {} },
		},
		ShaderDesc{"Blur",
			ShaderStageDesc{pFSQ_VS  , {} },
			ShaderStageDesc{"Blur_ps.hlsl", {} },
		},
		ShaderDesc{"BloomCombine",
			ShaderStageDesc{pFSQ_VS          , {} },
			ShaderStageDesc{"BloomCombine_ps.hlsl", {} },
		},
	};
	this->_bloomPass._bloomFilterShader  = pRenderer->CreateShader(bloomPipelineShaders[0]);
	this->_bloomPass._blurShader		 = pRenderer->CreateShader(bloomPipelineShaders[1]);
	this->_bloomPass._bloomCombineShader = pRenderer->CreateShader(bloomPipelineShaders[2]);

	D3D11_SAMPLER_DESC blurSamplerDesc = {};
	blurSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	blurSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	this->_bloomPass._blurSampler = pRenderer->CreateSamplerState(blurSamplerDesc);
	
	// Tonemapping
	this->_tonemappingPass._finalRenderTarget = pRenderer->GetDefaultRenderTarget();

	const ShaderDesc tonemappingShaderDesc = ShaderDesc{"Tonemapping",
		ShaderStageDesc{ pFSQ_VS         , {} },
		ShaderStageDesc{ "Tonemapping_ps.hlsl", {} }
	};
	this->_tonemappingPass._toneMappingShader = pRenderer->CreateShader(tonemappingShaderDesc);

	// World Render Target
	this->_worldRenderTarget = pRenderer->AddRenderTarget(rtDesc);

#if USE_COMPUTE_BLUR
	// Blur Compute Resources
	TextureDesc texDesc = TextureDesc();
	texDesc.usage = ETextureUsage::COMPUTE_RW_TEXTURE;
	texDesc.height = pRenderer->WindowHeight(); 
	texDesc.width = pRenderer->WindowWidth();
	texDesc.format = imageFormat;
	this->_bloomPass.blurComputeOutputPingPong[0] = pRenderer->CreateTexture2D(texDesc);
	this->_bloomPass.blurComputeOutputPingPong[1] = pRenderer->CreateTexture2D(texDesc);

	
	// Dispatch() spawns a thread group among X or Y dimensions.
	// The compute shader kernel describes the thread group size
	// essentially the other dimension (of X or Y) to cover the entire image.
	//
	// In short, Dispatch() spawns a thread group, which corresponds to a 
	// a row or a column of pixels of the image so that each thread covers
	// one pixel.
	//
	const int KERNEL_DIMENSION = 1024;
	const ShaderDesc CSDescV =
	{
		"Blur_Compute_Vertical",
		ShaderStageDesc { "Blur_cs.hlsl", {
			{ "VERTICAL"    , "1" },
			{ "IMAGE_SIZE_X", std::to_string(texDesc.width) },
			{ "IMAGE_SIZE_Y", std::to_string(texDesc.height) },
			{ "THREAD_GROUP_SIZE_X", "1" }, // max=1024
			{ "THREAD_GROUP_SIZE_Y", std::to_string(KERNEL_DIMENSION) },
			{ "THREAD_GROUP_SIZE_Z", "1"},
			{ "PASS_COUNT", std::to_string(_settings.bloom.blurStrength) } }
		}
	};
	const ShaderDesc CSDescH =
	{
		"Blur_Compute_Horizontal",
		ShaderStageDesc { "Blur_cs.hlsl", {
			{ "HORIZONTAL" , "1" },
			{ "IMAGE_SIZE_X", std::to_string(texDesc.width) },
			{ "IMAGE_SIZE_Y", std::to_string(texDesc.height) },
			{ "THREAD_GROUP_SIZE_X", std::to_string(KERNEL_DIMENSION) },
			{ "THREAD_GROUP_SIZE_Y", "1" }, 
			{ "THREAD_GROUP_SIZE_Z", "1"},
			{ "PASS_COUNT", std::to_string(_settings.bloom.blurStrength) } }
		}
	};
	this->_bloomPass.blurComputeShaderPingPong[0] = pRenderer->CreateShader(CSDescH);
	this->_bloomPass.blurComputeShaderPingPong[1] = pRenderer->CreateShader(CSDescV);
#endif
}

void PostProcessPass::Render(Renderer* pRenderer, bool bBloomOn, CPUProfiler* pCPU, GPUProfiler* pGPU) const
{
	pRenderer->BeginEvent("Post Processing");

	const TextureID worldTexture = pRenderer->GetRenderTargetTexture(_worldRenderTarget);
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);

	// ======================================================================================
	// BLOOM  PASS
	// ======================================================================================
	const Settings::Bloom& sBloom = _settings.bloom;
	const bool bBloom = bBloomOn && _settings.bloom.bEnabled;
	if (bBloom)
	{
		const ShaderID currentShader = pRenderer->GetActiveShader();

		//pCPU->BeginEntry("Bloom");
		pRenderer->BeginEvent("Bloom");
		pGPU->BeginEntry("Bloom Filter");

		// bright filter
		pRenderer->BeginEvent("Bright Filter");
		pRenderer->SetShader(_bloomPass._bloomFilterShader, true);
		pRenderer->BindRenderTargets(_bloomPass._colorRT, _bloomPass._brightRT);
		pRenderer->UnbindDepthTarget();
		pRenderer->SetVertexBuffer(IABuffersQuad.first);
		pRenderer->SetIndexBuffer(IABuffersQuad.second);
		pRenderer->Apply();
		pRenderer->SetTexture("worldRenderTarget", worldTexture);
		pRenderer->SetConstant1f("BrightnessThreshold", sBloom.brightnessThreshold);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
		pRenderer->EndEvent();
		
		pGPU->EndEntry();

		// blur
		const TextureID brightTexture = pRenderer->GetRenderTargetTexture(_bloomPass._brightRT);
		const int BlurPassCount = sBloom.blurStrength * 2;	// 1 pass for Horizontal and Vertical each

		pGPU->BeginEntry("Bloom Blur<PS>");
		pRenderer->BeginEvent("Blur Pass");
		pRenderer->SetShader(_bloomPass._blurShader, true);
		for (int i = 0; i < BlurPassCount; ++i)
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

#if USE_COMPUTE_BLUR

		pGPU->EndEntry();
		pGPU->BeginEntry("Bloom Blur<CS>");
		pRenderer->BeginEvent("Blur Compute Pass");
		for (int i = 0; i < 2; ++i)
		{
			const int INDEX_PING_PONG = i % 2;
			const int INDEX_PING_PONG_OTHER = (i + 1) % 2;

			//const int NUM_DIPATCH_CALLS = std::max();
			const int DISPATCH_GROUP_Y = i % 2 == 0		// HORIZONTAL BLUR
				? static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().y())
				: 1;
			const int DISPATCH_GROUP_X = i % 2 != 0 
				? static_cast<int>(pRenderer->GetWindowDimensionsAsFloat2().x())
				: 1;
			const int DISPATCH_GROUP_Z = 1;

			pRenderer->SetShader(this->_bloomPass.blurComputeShaderPingPong[INDEX_PING_PONG]);
			pRenderer->SetUnorderedAccessTexture("texColorOut", this->_bloomPass.blurComputeOutputPingPong[INDEX_PING_PONG]);
			pRenderer->SetTexture("texColorIn", i == 0 
				? brightTexture 
				: this->_bloomPass.blurComputeOutputPingPong[INDEX_PING_PONG_OTHER]
			);
			pRenderer->SetSamplerState("sSampler", EDefaultSamplerState::POINT_SAMPLER);
			pRenderer->Apply();
			pRenderer->Dispatch(DISPATCH_GROUP_X, DISPATCH_GROUP_Y, DISPATCH_GROUP_Z);
		}
		pRenderer->EndEvent();
		pGPU->EndEntry();
#endif

		// additive blend combine
		const TextureID colorTex = pRenderer->GetRenderTargetTexture(_bloomPass._colorRT);
		const TextureID bloomTex = pRenderer->GetRenderTargetTexture(_bloomPass._blurPingPong[0]);

		pGPU->BeginEntry("Bloom Combine");
		pRenderer->BeginEvent("Combine");
		pRenderer->SetShader(_bloomPass._bloomCombineShader, true);
		pRenderer->BindRenderTarget(_bloomPass._finalRT);
		pRenderer->SetTexture("ColorTexture", colorTex);
		pRenderer->SetTexture("BloomTexture", bloomTex);
		pRenderer->SetSamplerState("BlurSampler", _bloomPass._blurSampler);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
		pRenderer->EndEvent();
		
		//pCPU->EndEntry(); // bloom
		pRenderer->EndEvent(); // bloom
		pGPU->EndEntry(); // bloom
	}



	// ======================================================================================
	// TONEMAPPING PASS
	// ======================================================================================
	const TextureID colorTex = pRenderer->GetRenderTargetTexture(bBloom ? _bloomPass._finalRT : _worldRenderTarget);
	const float isHDR = _settings.HDREnabled ? 1.0f : 0.0f;
	pRenderer->BeginEvent("Tonemapping");
	//pCPU->BeginEntry("Tonemapping");
	pGPU->BeginEntry("Tonemapping");

	pRenderer->UnbindDepthTarget();
	pRenderer->SetShader(_tonemappingPass._toneMappingShader, true);
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->SetSamplerState("Sampler", _bloomPass._blurSampler);
	pRenderer->SetConstant1f("exposure", _settings.toneMapping.exposure);
	pRenderer->SetConstant1f("isHDR", isHDR);
	pRenderer->BindRenderTarget(_tonemappingPass._finalRenderTarget);
	pRenderer->SetTexture("ColorTexture", colorTex);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();

	//pCPU->EndEntry(); // tonemapping
	pGPU->EndEntry(); // tonemapping
	pRenderer->EndEvent();	// Post Process
}



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
	rtDesc.textureDesc.width = pRenderer->WindowWidth();
	rtDesc.textureDesc.height = pRenderer->WindowHeight();
	rtDesc.textureDesc.mipCount = 1;
	rtDesc.textureDesc.arraySize = 1;
	rtDesc.textureDesc.format = imageFormat;
	rtDesc.textureDesc.usage = ETextureUsage::RENDER_TARGET_RW;
	rtDesc.format = imageFormat;

	this->occlusionRenderTarget = pRenderer->AddRenderTarget(rtDesc);
	this->blurRenderTarget		= pRenderer->AddRenderTarget(rtDesc);

	// SHADER
	//--------------------------------------------------------------------
	const ShaderDesc ssaoShaderDesc = 
	{	"SSAO_Shader",
		ShaderStageDesc{ "FullscreenQuad_vs.hlsl", {} },
		ShaderStageDesc{ "SSAO_ps.hlsl", {} },
	};
	this->SSAOShader = pRenderer->CreateShader(ssaoShaderDesc);
#if SSAO_DEBUGGING
	this->radius = 6.5f;
	this->intensity = 1.0f;
#endif

	// Compute Shader Unit Test -----------------------------------
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
	// Compute Shader Unit Test -----------------------------------


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
}

void AmbientOcclusionPass::RenderOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView)
{
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
}

void AmbientOcclusionPass::BilateralBlurPass(Renderer * pRenderer)
{
	pRenderer->UnbindRenderTargets();
	return;
	pRenderer->BeginEvent("Blur Pass");
	pRenderer->SetShader(bilateralBlurShader);
	
	//pRenderer->BindRenderTarget(this->renderTarget);

	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	pRenderer->EndEvent();	// Blur Pass


}

void AmbientOcclusionPass::GaussianBlurPass(Renderer * pRenderer)
{
	const TextureID texOcclusion = pRenderer->GetRenderTargetTexture(this->occlusionRenderTarget);
	const vec2 texDimensions = pRenderer->GetWindowDimensionsAsFloat2();
	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);

	pRenderer->BeginEvent("Blur Pass");

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

#if USE_COMPUTE_SSAO
	const TextureID depthTexture = pRenderer->GetDepthTargetTexture(ENGINE->GetWorldDepthTarget());
	pRenderer->BeginEvent("SSAO Compute");
	pRenderer->SetShader(this->ssaoComputeShader, true);
	pRenderer->SetTexture("texDepth", depthTexture);
	pRenderer->SetSamplerState("sSampler", EDefaultSamplerState::POINT_SAMPLER);
	pRenderer->SetUnorderedAccessTexture("texSSAOOutput", texSSAOComputeOutput);
	//pRenderer->SetTexture("texViewSpaceNormals", 0);
	//pRenderer->SetTexture("texNoise", this->noiseTexture);
	pRenderer->Apply();
	pRenderer->Dispatch(120, 68, 1);
	pRenderer->EndEvent();
#endif
}




void DebugPass::Initialize(Renderer * pRenderer)
{
	_scissorsRasterizer = pRenderer->AddRasterizerState(ERasterizerCullMode::BACK, ERasterizerFillMode::SOLID, false, true);
}


