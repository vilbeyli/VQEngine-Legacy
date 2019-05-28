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

// #TODO: base class pass / better naming

#pragma once

#include "Engine/Settings.h"
#include "Engine/Skybox.h"

#include "Renderer/RenderingEnums.h"

#include "Utilities/vectormath.h"

#include <array>
#include <vector>
#include <memory>
#include <unordered_map>


// ===========================================================================================
// DEBUGGING
// ===========================================================================================
//
// Engine will be updating the values during Engine::HandleInput() and output values
// to the console / file log.

#define SSAO_DEBUGGING 0
//
// wheel up/down             : radius +/-
// shift+ wheel up/down      : intensity +/-
// ctrl + wheel up/down      : ssao quality
// ctrl+shift+ wheel up/down : AO Technique selection +/-
// k/j                       : AO quality (# taps) +/-

#define BLOOM_DEBUGGING 0
//
// wheel up/down             : brightness threshold +/-
// shift+ wheel up/down      : blur strength +/-
// ctrl + wheel up/down      : shader selection +/-

#define GBUFFER_DEBUGGING 0
//
// wheel press               : toggle depth pre-pass

// ===========================================================================================


// current implementation employs the same logic for the
// game-object level object batching. however, we're using
// mesh-level instancing: this results in a very large number
// of transformation matrix constant buffer updates per mesh.
//
#define SHADOW_PASS_USE_INSTANCED_DRAW_DATA 1



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
struct SceneLightingConstantBuffer;
struct ShadowView;
struct SceneView;
struct RenderTargetDesc;

// BASE CLASS
//
struct RenderPass
{
	enum ECommonShaders
	{
		TRANSPOZE = 0,

		NUM_COMMON_SHADERS
	};
	static ShaderID sShaderTranspoze;
	static void InitializeCommonSaders(Renderer* pRenderer);

	RenderPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_)
		: pCPU(pCPU_)
		, pGPU(pGPU_)
	{}
	
	CPUProfiler*& pCPU;
	GPUProfiler*& pGPU;
};



#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
struct MeshDrawData
{
#define INCLUDE_OBJECT_POINTER_TO_DRAW_DATA 0
#if !INCLUDE_OBJECT_POINTER_TO_DRAW_DATA
	void AddMeshTransformation(MeshID meshID, const XMMATRIX& matTransform)
	{
		if (meshTransformListLookup.find(meshID) != meshTransformListLookup.end())
		{
			meshTransformListLookup.at(meshID).push_back(matTransform);
		}
		else
		{
			meshTransformListLookup[meshID].push_back(matTransform);
		}
	}

	// Note:
	//
	// may result in a lot of XMMATRIX copies, we can use ID's to keep
	// track of matrices instead of storing one per mesh.
	std::unordered_map<MeshID, std::vector<XMMATRIX>> meshTransformListLookup;
#else
	// Want to eliminate redundant matrix copies with the above design.
	// each mesh should be mapped to draw data which should contain:
	// - the game object we're drawing (to be used as the LOD lookup key)
	// - matrix data index. # game objs = # matrices.
	//   remember to copy matrices here to an array so we can access with 
	//   higher cache coherency. (in the long run, game objects should be
	//   separated from transforms and transforms should all be kept in
	//   an array so only one copy of contiguous transforms are stored
	//   and iterated over.
	//
	struct ObjectDrawLookupData
	{
		const GameObject* pObj = nullptr;
		int martixID = -1;
	};
	std::vector< ObjectDrawLookupData> mObjectdrawData;
	std::vector<const XMMATRIX*> mpTransformationMatrices;
	std::unordered_map<MeshID, std::vector< ObjectDrawLookupData>> mMeshDrawDataLookup;

	void AddMeshTransformation(MeshID meshID, const XMMATRIX& matTransform, const GameObject* pObj = nullptr)
	{
		assert(pObj);
		const bool bMeshExistsInLookup = mMeshDrawDataLookup.find(meshID) != mMeshDrawDataLookup.end();
		if (bMeshExistsInLookup)
		{
			//mpTransformationMatrices
			mMeshDrawDataLookup.at(meshID).push_back(ObjectDrawLookupData{pObj, -1});
		}

		// add to lookup
		else
		{
			 mMeshDrawDataLookup[meshID].push_back(ObjectDrawLookupData{pObj,-1 });
		}
	}

#endif
};
#else
struct MeshDrawData
{
	MeshDrawData(const GameObject* pObj);
	MeshDrawData();

	std::vector<MeshID> meshIDs;
	XMMATRIX matWorld;
#ifdef _DEBUG
	const GameObject* pObj;
#endif
};
#endif // SHADOW_PASS_USE_INSTANCED_DRAW_DATA

using DepthTargetIDArray = std::vector<DepthTargetID>;

constexpr int MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS = 64;
struct DepthOnlyPass_PerObjectMatrices             { XMMATRIX wvp; };
struct DepthOnlyPass_PerObjectMatricesCubemap      { XMMATRIX matWorld; XMMATRIX wvp; };
struct DepthOnlyPass_InstancedObjectCBuffer        { DepthOnlyPass_PerObjectMatrices        objMatrices[MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS]; };
struct DepthOnlyPass_InstancedObjectCubemapCBuffer { DepthOnlyPass_PerObjectMatricesCubemap objMatrices[MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS]; };
struct ShadowMapPass : public RenderPass
{
	ShadowMapPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	void Initialize(Renderer* pRenderer, const Settings::ShadowMap& shadowMapSettings);
	void RenderShadowMaps(Renderer* pRenderer, const ShadowView& shadowView, GPUProfiler* pGPUProfiler) const;
	
	vec2 GetDirectionalShadowMapDimensions(Renderer* pRenderer) const;


	void InitializeSpotLightShadowMaps(const Settings::ShadowMap& shadowMapSettings);
	void InitializePointLightShadowMaps(const Settings::ShadowMap& shadowMapSettings);
	void InitializeDirectionalLightShadowMap(const Settings::ShadowMap& shadowMapSettings);

	Renderer*			mpRenderer = nullptr;
	ShaderID			mShadowMapShader = -1;
	ShaderID			mShadowMapShaderInstanced = -1;
	ShaderID			mShadowCubeMapShader = -1;

	unsigned			mShadowMapDimension_Spot = 0;
	unsigned			mShadowMapDimension_Point = 0;
	
	TextureID			mShadowMapTextures_Spot = -1;		// tex2D array
	TextureID			mShadowMapTexture_Directional = -1;	// tex2D array
	TextureID			mShadowMapTextures_Point = -1;		// cubemap array

	DepthTargetIDArray	mDepthTargets_Spot;
	DepthTargetID		mDepthTarget_Directional = -1;
	DepthTargetIDArray	mDepthTargets_Point;
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
	void Render(Renderer* pRenderer, bool bBloomOn, TextureID texOverride = -1) const;

	RenderTargetID			_worldRenderTarget;
	BloomPass				_bloomPass;
	TonemappingCombinePass	_tonemappingPass;
	Settings::PostProcess	_settings;
};

struct GBuffer
{
	bool			bInitialized = false;
	RenderTargetID	mRTDiffuseRoughness;
	RenderTargetID	mRTSpecularMetallic;
	RenderTargetID	mRTNormals;
	RenderTargetID  mRTEmissive;;
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
	enum ENormalMapCompressionLevel
	{
		NO_COMPRESSION = 0
		, FULL_PRECISION = NO_COMPRESSION // RGBA_32F    : 16B / px
		, HALF_PRECISION                  // RGBA_16F    :  8B / px
		, LOW_PRECISION                   // R11G11B10_F :  4B / px (buggy, doesn't work well)
	};

	DeferredRenderingPasses(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	struct RenderParams
	{
		Renderer* pRenderer;
		const RenderTargetID target;
		const SceneView& sceneView;
		const SceneLightingConstantBuffer& lights;
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
	DepthStencilStateID _geometryStencilStatePreZ;
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

	// implemented depth prepass for testing purposes.
	// enabling this will make GBuffer pass almost twice as slow as the bottleneck 
	// is not the PS calculations, but rather the pass looks memory bandwidth-limited.
	bool mbUseDepthPrepass = false;

	ENormalMapCompressionLevel mNormalRTCompressionLevel = HALF_PRECISION;
};

struct ForwardLightingPass : public RenderPass
{
	ForwardLightingPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	struct RenderParams
	{
		Renderer* pRenderer;
		const Scene* pScene;
		const SceneView& sceneView;
		const SceneLightingConstantBuffer& lights;
		const TextureID tSSAO;
		const RenderTargetID targetRT;
		const TextureID tEmptyTex;
		const bool bZPrePass;
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


// learnopengl:			https://learnopengl.com/#!Advanced-Lighting/SSAO
// Blizzard Dev Paper:	http://developer.amd.com/wordpress/media/2012/10/S2008-Filion-McNaughton-StarCraftII.pdf
struct AmbientOcclusionPass : public RenderPass
{
	struct BilateralBlurConstants
	{
		float normalDotThreshold;
		float depthThreshold;
	};
	enum EAOQuality
	{
		AO_QUALITY_LOW = 0, // 16 taps / px
		AO_QUALITY_MEDIUM,  // 32 taps / px
		AO_QUALITY_HIGH,    // 64 taps / px

		AO_QUALITY_NUM_OPTIONS
	};
	enum EBlurQuality
	{
		BLUR_QUALITY_LOW = 0,	// Gaussian Blur
		BLUR_QUALITY_HIGH,		// Bilateral Blur (currently disabled)
		BLUR_QUALITY_NUM_OPTIONS
	};
	enum EAOTechnique
	{
		HBAO = 0, // Modified Crytek - Normal-aligned hemisphere sampling
		HBAO_DT,  // De-interleaved Texturing (currently disabled)

		NUM_AO_TECHNIQUES
	};

	AmbientOcclusionPass(CPUProfiler*& pCPU_, GPUProfiler*& pGPU_) : RenderPass(pCPU_, pGPU_) {}
	static TextureID whiteTexture4x4;
	static TextureID blackTexture4x4;
	
	// Pass interface
	void Initialize(Renderer* pRenderer);
	void RenderAmbientOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView) const;
	void ChangeBlurQualityLevel(int upOrDown);  // -1 or 1 as input
	void ChangeAOTechnique(int upOrDown);       // -1 or 1 as input
	void ChangeAOQuality(int upOrDown);         // -1 or 1 as input
	TextureID GetBlurredAOTexture(Renderer* pRenderer) const;

	// draw functions
	void RenderOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView) const;
	void RenderOcclusionInterleaved(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView) const;
	void DeinterleaveDepth(Renderer* pRenderer) const;
	void InterleaveAOTexture(Renderer* pRenderer) const;
	void BilateralBlurPass(Renderer* pRenderer, const TextureID texNormals, const TextureID texAO, const SceneView& sceneView) const;
	void GaussianBlurPass(Renderer* pRenderer, const TextureID texAO) const;	// Gaussian 4x4 kernel

	EBlurQuality blurQuality;
	EAOTechnique aoTech;
	EAOQuality aoQuality;

	// SSAO resources
	std::vector<vec3>	sampleKernel[AO_QUALITY_NUM_OPTIONS];
	std::vector<vec2>	noiseKernel;
	TextureID			noiseTexture;
	SamplerID			noiseSampler;

	// Regular SSAO resources -------------------------------
	RenderTargetID		occlusionRenderTarget;
	ShaderID			SSAOShader[AO_QUALITY_NUM_OPTIONS];
	// Regular SSAO resources -------------------------------

	// Blur resources --------------------------------------
	RenderTargetID			 gaussianBlurRenderTarget;
	std::array<TextureID, 2> bilateralBlurUAVs;
	TextureID				 bilateralBlurTranspozeUAV;
	BilateralBlurConstants	 bilateralBlurParameters;

	ShaderID gaussianBlurShader;
	ShaderID transposeAOShader;
	ShaderID bilateralBlurShaderH;
	ShaderID bilateralBlurShaderV;
	// Blur resources --------------------------------------

	// Interleaved SSAO resources -------------------------------
	ShaderID	deinterleaveShader;
	ShaderID	deinterleavedSSAOShader[AO_QUALITY_NUM_OPTIONS];
	ShaderID	interleaveShader;

	TextureID	deinterleavedDepthTextures;
	TextureID	interleavedAOTexture;
	RenderTargetID	deinterleavedAORenderTargets[4]; // TODO: RTArray
	// Interleaved SSAO resources -------------------------------


	// Compute Shader Unit Test ---------------------------
	ShaderID  testComputeShader;
	BufferID  UABuffer;
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