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

// #TODO: baseclass pass / better naming

#pragma once

#include "Engine/Settings.h"
#include "Skybox.h"

#include "Renderer/RenderingEnums.h"

#include "Utilities/vectormath.h"

#include <array>
#include <vector>
#include <memory>
#include <unordered_map>

using std::shared_ptr;

class Camera;
class Renderer;
class GameObject;
class Scene;
class GPUProfiler;
class CPUProfiler;

struct Light;
struct DirectionalLight;
struct ID3D11Device;
struct SceneLightingData;
struct ShadowView;
struct SceneView;
struct RenderTargetDesc;


struct RenderPass
{
	RenderPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_)
		: pCPU(pCPU_)
		, pGPU(pGPU_)
	{}
	
	CPUProfiler*& pCPU;
	GPUProfiler*& pGPU;
};


using DepthTargetIDArray = std::vector<DepthTargetID>;
struct ShadowMapPass : public RenderPass
{
	ShadowMapPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	void InitializeSpotLightShadowMaps(Renderer* pRenderer, const Settings::ShadowMap& shadowMapSettings);
	void InitializeDirectionalLightShadowMap(Renderer* pRenderer, const Settings::ShadowMap& shadowMapSettings);
	void RenderShadowMaps(Renderer* pRenderer, const ShadowView& shadowView, GPUProfiler* pGPUProfiler) const;
	
	Renderer*			mpRenderer = nullptr;
	ShaderID			mShadowMapShader = -1;
	ShaderID			mShadowMapShaderInstanced = -1;

	unsigned			mShadowMapDimension_Spot = 0;
	D3D11_VIEWPORT		mShadowViewPort_Spot;	// spot light viewport
	D3D11_VIEWPORT		mShadowViewPort_Directional;
	
	TextureID			mShadowMapTextures_Spot = -1;			// tex2D array
	TextureID			mShadowMapTexture_Directional = -1;		// tex2D array
	TextureID			mShadowMapTextures_Point = -1;			// cubemap array

	DepthTargetIDArray	mDepthTargets_Spot;
	DepthTargetID		mDepthTarget_Directional = -1;
	DepthTargetID		mDepthTargets_Point = -1;
};

struct BloomPass : public RenderPass
{
	enum BloomShader : int
	{
		PS_1D_Kernels = 0,
		CS_1D_Kernels,
		CS_1D_Kernels_Transpoze_Out,
		NUM_BLOOM_SHADERS
	};
	
	BloomPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_)
		: RenderPass(pCPU_, pGPU_)
		, _blurSampler(-1)
		, _colorRT(-1)
		, _brightRT(-1)
		, _blurPingPong({ -1, -1 }
		) {}

	SamplerID						_blurSampler;
	RenderTargetID					_colorRT;
	RenderTargetID					_brightRT;
	RenderTargetID					_finalRT;
	std::array<RenderTargetID, 2>	_blurPingPong;
	ShaderID						_blurShader;
	ShaderID						_bloomFilterShader;
	ShaderID						_bloomCombineShader;


	std::array<ShaderID, 2>  blurComputeShaderPingPong;
	std::array<TextureID, 2> blurComputeOutputPingPong;

	ShaderID  transpozeCompute;
	ShaderID  blurHorizontalTranspozeComputeShader;
	TextureID texTransposedImage;

	BloomShader mSelectedBloomShader;


	void Initialize(Renderer* pRenderer, const Settings::Bloom& bloomSettings, const RenderTargetDesc& rtDesc);
	void Render(Renderer* pRenderer, RenderTargetID rtDestination, const Settings::Bloom& settings) const;
	void UpdateSettings(Renderer* pRenderer, const Settings::Bloom& bloomSettings);

	TextureID GetBloomTexture(const Renderer* pRenderer) const;
};

struct TonemappingCombinePass : public RenderPass
{
	TonemappingCombinePass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	RenderTargetID	_finalRenderTarget;
	ShaderID		_toneMappingShader;
};

struct PostProcessPass : public RenderPass
{
	PostProcessPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) 
		: RenderPass(pCPU_, pGPU_) 
		, _bloomPass(pCPU_, pGPU_)
		, _tonemappingPass(pCPU_, pGPU_) 
	{}
	void UpdateSettings(const Settings::PostProcess& newSettings, Renderer* pRenderer);
	void Initialize(Renderer* pRenderer, const Settings::PostProcess& postProcessSettings);
	void Render(Renderer* pRenderer, bool bBloomOn) const;

	RenderTargetID			_worldRenderTarget;
	BloomPass				_bloomPass;
	TonemappingCombinePass	_tonemappingPass;
	Settings::PostProcess	_settings;
};

struct GBuffer
{
	bool			bInitialized = false;
	RenderTargetID	_diffuseRoughnessRT;
	RenderTargetID	_specularMetallicRT;
	RenderTargetID	_normalRT;
};
struct ObjectMatrices
{
	XMMATRIX world;
	XMMATRIX normal;
	XMMATRIX worldViewProj;
};
template<unsigned NUM_INSTANCES>
struct InstancedObjectMatrices
{
	ObjectMatrices objMatrices[NUM_INSTANCES];
};

// struct GBufferPass : public RenderPass
// {
// 
// };
// 
// struct LightingPass : public RenderPass
// {
// 	// virtual void RenderObject(const GameObject* pObj) = 0;
// };
// 
// struct DeferredLightingPass : public LightingPass
// {
// 
// };

struct DeferredRenderingPasses : public RenderPass
{
	DeferredRenderingPasses(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	struct RenderParams
	{
		Renderer* pRenderer;
		const RenderTargetID target;
		const SceneView& sceneView;
		const SceneLightingData& lights;
		const TextureID tSSAO;
		bool bUseBRDFLighting;
	};
	void Initialize(Renderer* pRenderer);
	void InitializeGBuffer(Renderer* pRenderer);

	void ClearGBuffer(Renderer* pRenderer);
	void RenderGBuffer(Renderer* pRenderer, const Scene* pScene, const SceneView& sceneView) const;
	
	void RenderLightingPass(const RenderParams& args) const;

	GBuffer _GBuffer;
	DepthStencilStateID _geometryStencilState;
	ShaderID			_geometryShader;
	ShaderID			_geometryInstancedShader;
	ShaderID			_ambientShader;
	ShaderID			_ambientIBLShader;
	//ShaderID			_environmentMapSpecularShader;
	ShaderID			_phongLightingShader;
	ShaderID			_BRDFLightingShader;

	// todo
	ShaderID			_spotLightShader;
	ShaderID			_pointLightShader;
};

struct ForwardLightingPass : public RenderPass
{
	ForwardLightingPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	struct RenderParams
	{
		Renderer* pRenderer;
		const Scene* pScene;
		const SceneView& sceneView;
		const SceneLightingData& lights;
		const TextureID tSSAO;
		const RenderTargetID targetRT;
	};

	void Initialize(Renderer* pRenderer);
	void RenderLightingPass(const RenderParams& args) const;


	ShaderID fwdPhong;
	ShaderID fwdBRDF;
	ShaderID fwdBRDFInstanced;
};

struct DebugPass : public RenderPass
{
	DebugPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	void Initialize(Renderer* pRenderer);
	RasterizerStateID _scissorsRasterizer;
};

// Engine will be updating the values in Engine::HandleInput()
//--------------------------------------------------------------------------------------------
//
#define SSAO_DEBUGGING 0
//
// wheel up/down		:	radius +/-
// shift+ wheel up/down	:	intensity +/-
//
//
#define BLOOM_DEBUGGING 0
//
// wheel up/down		:	brightness threshold +/-
// shift+ wheel up/down	:	blur strength +/-
// ctrl + wheel up/down :	shader selection +/-
//

//--------------------------------------------------------------------------------------------

// learnopengl:			https://learnopengl.com/#!Advanced-Lighting/SSAO
// Blizzard Dev Paper:	http://developer.amd.com/wordpress/media/2012/10/S2008-Filion-McNaughton-StarCraftII.pdf
struct AmbientOcclusionPass : public RenderPass
{
	AmbientOcclusionPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	static TextureID whiteTexture4x4;
	
	void Initialize(Renderer* pRenderer);
	void RenderAmbientOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView);

	void RenderOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView);
	void RenderOcclusionInterleaved(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView);
	void BilateralBlurPass(Renderer* pRenderer);
	void GaussianBlurPass(Renderer* pRenderer);	// Gaussian 4x4 kernel

	// SSAO resources
	std::vector<vec3>	sampleKernel;
	std::vector<vec4>	noiseKernel;
	TextureID			noiseTexture;
	SamplerID			noiseSampler;

	// Regular SSAO resources -------------------------------
	RenderTargetID		occlusionRenderTarget;
	RenderTargetID		blurRenderTarget;
	TextureID			bilateralBlurUAV;

	ShaderID			SSAOShader;
	ShaderID			blurShader; // gaussian
	// Regular SSAO resources -------------------------------


	// Interleaved SSAO resources -------------------------------
	std::array<RenderTargetID, 4> interleavedDepthTextures;
	std::array<RenderTargetID, 4> interleavedNormalTextures;
	//RenderTargetID			 
	// Interleaved SSAO resources -------------------------------


	// Compute Shader Unit Test ---------------------------
	ShaderID testComputeShader;
	BufferID UABuffer;
	TextureID RWTex2D;
	// Compute Shader Unit Test ---------------------------

#if SSAO_DEBUGGING
	float radius;		
	float intensity;	
#endif
};


struct ZPrePass : public RenderPass
{
	ZPrePass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	struct RenderParams
	{
		Renderer* pRenderer;
		const Scene* pScene; 
		const SceneView& sceneView;
		const RenderTargetID normalRT;
	};

	void Initialize(Renderer* pRenderer);
	void RenderDepth(const RenderParams& args) const;

	SamplerID normalMapSampler;
	ShaderID objShader;
	ShaderID objShaderInstanced;
};