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
struct Light;
struct DirectionalLight;
struct ID3D11Device;
class GPUProfiler;
struct SceneLightingData;
struct ShadowView;
struct SceneView;



using DepthTargetIDArray = std::vector<DepthTargetID>;
struct ShadowMapPass
{
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

struct BloomPass
{
	BloomPass() : 
		_blurSampler(-1), 
		_colorRT(-1), 
		_brightRT(-1), 
		_blurPingPong({ -1, -1 })
	{}

	SamplerID						_blurSampler;
	RenderTargetID					_colorRT;
	RenderTargetID					_brightRT;
	RenderTargetID					_finalRT;
	std::array<RenderTargetID, 2>	_blurPingPong;
	ShaderID						_blurShader;
	ShaderID						_bilateralBlurShader;
	ShaderID						_bloomFilterShader;
	ShaderID						_bloomCombineShader;
};

struct TonemappingCombinePass
{
	RenderTargetID	_finalRenderTarget;
	ShaderID		_toneMappingShader;
};

struct PostProcessPass
{
	void Initialize(Renderer* pRenderer, const Settings::PostProcess& postProcessSettings);
	void Render(Renderer* pRenderer, bool bBloomOn) const;

	RenderTargetID				_worldRenderTarget;
	BloomPass					_bloomPass;
	TonemappingCombinePass		_tonemappingPass;
	Settings::PostProcess		_settings;
};

struct GBuffer
{
	bool			bInitialized = false;
	RenderTargetID	_diffuseRoughnessRT;
	RenderTargetID	_specularMetallicRT;
	RenderTargetID	_normalRT;
};
struct ObjectMatrices_ViewSpace
{
	XMMATRIX wv;
	XMMATRIX nv;
	XMMATRIX wvp;
};
struct DeferredRenderingPasses
{
	void Initialize(Renderer* pRenderer);
	void InitializeGBuffer(Renderer* pRenderer);

	void ClearGBuffer(Renderer* pRenderer);
	void RenderGBuffer(Renderer* pRenderer, const Scene* pScene, const SceneView& sceneView) const;
	
	void RenderLightingPass(
		Renderer* pRenderer, 
		const RenderTargetID target, 
		const SceneView& sceneView, 
		const SceneLightingData& lights, 
		const TextureID tSSAO, 
		bool bUseBRDFLighting
	) const;

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

struct DebugPass
{
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
//

//--------------------------------------------------------------------------------------------

// learnopengl:			https://learnopengl.com/#!Advanced-Lighting/SSAO
// Blizzard Dev Paper:	http://developer.amd.com/wordpress/media/2012/10/S2008-Filion-McNaughton-StarCraftII.pdf
struct AmbientOcclusionPass
{
	static TextureID whiteTexture4x4;
	
	void Initialize(Renderer* pRenderer);
	void RenderOcclusion(Renderer* pRenderer, const TextureID texNormals, const SceneView& sceneView);
	void BilateralBlurPass(Renderer* pRenderer);
	void GaussianBlurPass(Renderer* pRenderer);	// Gaussian 4x4 kernel

	std::vector<vec3>	sampleKernel;
	std::vector<vec4>	noiseKernel;
	TextureID			noiseTexture;
	SamplerID			noiseSampler;
	RenderTargetID		occlusionRenderTarget;
	RenderTargetID		blurRenderTarget;
	ShaderID			SSAOShader;
	ShaderID			bilateralBlurShader;
	ShaderID			blurShader;

#if SSAO_DEBUGGING
	float radius;		
	float intensity;	
#endif
};
