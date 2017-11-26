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

// #TODO: baseclass pass / better naming

#pragma once

#include "Engine/Settings.h"
#include "Skybox.h"

#include "Renderer/RenderingEnums.h"

#include "Utilities/utils.h"

#include <array>
#include <vector>
#include <memory>

using std::shared_ptr;

class Camera;
class Renderer;
class GameObject;
struct Light;
struct ID3D11Device;

struct SceneLightingData;
struct SceneView
{
	XMMATRIX		viewProj;
	XMMATRIX		view;
	XMMATRIX		viewToWorld;
	XMMATRIX		projection;
	vec3			cameraPosition;
	bool			bIsPBRLightingUsed;
	bool			bIsDeferredRendering;
	bool			bIsIBLEnabled;
	Settings::SceneRender sceneRenderSettings;
	EnvironmentMap	environmentMap;
};

struct ShadowView
{	// shadowing Lights
	std::vector<const Light*> spots;
	std::vector<const Light*> points;
	std::vector<const Light*> directionals;
};

// todo: shader defines
// don't forget to update GPU define too (LightingCommon.hlsl)
using DepthTargetIDArray = std::vector<DepthTargetID>;
struct ShadowMapPass
{
	void Initialize(Renderer* pRenderer, ID3D11Device* device, const Settings::ShadowMap& shadowMapSettings);
	void RenderShadowMaps(Renderer* pRenderer, const std::vector<const GameObject*> ZPassObjects, const ShadowView& shadowView) const;
	
	unsigned				_shadowMapDimension;
	SamplerID				_shadowSampler;
	ShaderID				_shadowShader;
	RasterizerStateID		_drawRenderState;
	RasterizerStateID		_shadowRenderState;
	D3D11_VIEWPORT			_shadowViewport;
	
	TextureID				_pointShadowMaps;			// tex2D array
	TextureID				_spotShadowMaps;			// tex2D array
	TextureID				_directionalShadowMaps;		// cubemap array

	DepthTargetIDArray		_spotShadowDepthTargets;
	DepthTargetID			_directionalShadowDepthTargets;
	DepthTargetID			_pointShadowDepthTargets;
};

struct BloomPass
{
	inline void ToggleBloomPass() { _isEnabled = !_isEnabled; }
	BloomPass() : 
		_blurSampler(-1), 
		_colorRT(-1), 
		_brightRT(-1), 
		_blurPingPong({ -1, -1 }), 
		_isEnabled(true) 
	{}

	SamplerID						_blurSampler;
	RenderTargetID					_colorRT;
	RenderTargetID					_brightRT;
	RenderTargetID					_finalRT;
	std::array<RenderTargetID, 2>	_blurPingPong;
	bool							_isEnabled;
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
	void Initialize(
		Renderer*						pRenderer, 
		const Settings::PostProcess&	postProcessSettings
	);
	void Render(Renderer* pRenderer, bool bDeferredRendering) const;

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
	RenderTargetID	_positionRT;	// todo: construct position from viewProj^-1 
};

struct DeferredRenderingPasses
{
	void Initialize(Renderer* pRenderer);
	void InitializeGBuffer(Renderer* pRenderer);
	void ClearGBuffer(Renderer* pRenderer);
	void SetGeometryRenderingStates(Renderer* pRenderer) const;
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
	DepthStencilStateID _skyboxStencilState;
	ShaderID			_geometryShader;
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

// learnopengl:			https://learnopengl.com/#!Advanced-Lighting/SSAO
// Blizzard Dev Paper:	http://developer.amd.com/wordpress/media/2012/10/S2008-Filion-McNaughton-StarCraftII.pdf
struct AmbientOcclusionPass
{
	static TextureID whiteTexture4x4;
	
	void Initialize(Renderer* pRenderer);
	void RenderOcclusion(Renderer* pRenderer, const TextureID texNormals, const TextureID texPositions, const SceneView& sceneView);
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

	float radius;
	float intensity;
};
