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

#include "SceneManager.h"	
#include "SceneParser.h"
#include "Engine.h"			
#include "Input.h"
#include "Renderer.h"
#include "Camera.h"
#include "utils.h"
#include "Light.h"
#include "BaseSystem.h"	// only for renderer settings... maybe store in renderer?

#define KEY_TRIG(k) ENGINE->INP()->IsKeyTriggered(k)


constexpr int		MAX_LIGHTS = 20;
constexpr int		MAX_SPOTS = 10;


SceneManager::SceneManager()
	:
	m_pCamera(new Camera()),
	m_roomScene(*this),
	m_bUsePaniniProjection(false)
{}

SceneManager::~SceneManager()
{}


void SceneManager::ReloadLevel()
{
	const SerializedScene&		scene            =  m_serializedScenes.back();	// load last level
	const Settings::Renderer&	rendererSettings = BaseSystem::s_rendererSettings;
	Log::Info("Reloading Level...");

	SetCameraSettings(scene.cameraSettings, rendererSettings.window);
	// only camera reset for now
	// todo: unload and reload scene, initialize depth pass...
}

void SceneManager::Load(Renderer* renderer, PathManager* pathMan, const Settings::Renderer& rendererSettings)
{
#if 0
	m_pPathManager		= pathMan;
#endif
	m_useDeferredRendering	= rendererSettings.bUseDeferredRendering;
	m_isAmbientOcclusionOn	= rendererSettings.bAmbientOcclusion;
	m_pRenderer				= renderer;
	m_selectedShader		= m_useDeferredRendering ? SHADERS::DEFERRED_GEOMETRY : SHADERS::FORWARD_BRDF;
	m_debugRender			= false;
	
	if (m_useDeferredRendering)
	{
		m_deferredRenderingPasses.InitializeGBuffer(renderer);
	}
	
	Skybox::InitializePresets(m_pRenderer);
	
	m_serializedScenes.push_back(SceneParser::ReadScene());
	
	SerializedScene& scene = m_serializedScenes.back();	// non-const because lights are std::move()d for roomscene loading
	m_roomScene.Load(m_pRenderer, scene);
	SetCameraSettings(scene.cameraSettings, rendererSettings.window);
	m_pRenderer->SetCamera(m_pCamera.get());	// set camera of the renderer (perhaps multiple views/cameras?)


	{	// RENDER PASS INITIALIZATION
		// todo: static game object array in gameobj.h 
		m_shadowMapPass.Initialize(m_pRenderer, m_pRenderer->m_device, rendererSettings.shadowMap);
		//renderer->m_Direct3D->ReportLiveObjects();
		m_ZPassObjects.push_back(&m_roomScene.m_room.floor);
		m_ZPassObjects.push_back(&m_roomScene.m_room.wallL);
		m_ZPassObjects.push_back(&m_roomScene.m_room.wallR);
		m_ZPassObjects.push_back(&m_roomScene.m_room.wallF);
		m_ZPassObjects.push_back(&m_roomScene.m_room.ceiling);
		m_ZPassObjects.push_back(&m_roomScene.quad);
		m_ZPassObjects.push_back(&m_roomScene.grid);
		m_ZPassObjects.push_back(&m_roomScene.cylinder);
		m_ZPassObjects.push_back(&m_roomScene.triangle);
		m_ZPassObjects.push_back(&m_roomScene.cube);
		m_ZPassObjects.push_back(&m_roomScene.sphere);
		for (GameObject& obj : m_roomScene.cubes)	m_ZPassObjects.push_back(&obj);
		for (GameObject& obj : m_roomScene.spheres)	m_ZPassObjects.push_back(&obj);

		m_postProcessPass.Initialize(m_pRenderer, rendererSettings.postProcess);

		// Samplers
		D3D11_SAMPLER_DESC normalSamplerDesc = {};
		normalSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		normalSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		normalSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		normalSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		normalSamplerDesc.MinLOD = 0.f;
		normalSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		normalSamplerDesc.MipLODBias = 0.f;
		normalSamplerDesc.MaxAnisotropy = 0;
		normalSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		m_normalSampler = renderer->CreateSamplerState(normalSamplerDesc);
	}
}

void SceneManager::SetCameraSettings(const Settings::Camera& cameraSettings, const Settings::Window& windowSettings)
{
	const auto& NEAR_PLANE = cameraSettings.nearPlane;
	const auto& FAR_PLANE = cameraSettings.farPlane;
	const float AspectRatio = static_cast<float>(windowSettings.width) / windowSettings.height;
	const float VerticalFoV = cameraSettings.fovV * DEG2RAD;

	m_pCamera->m_settings = cameraSettings;
	m_pCamera->m_settings.aspect = AspectRatio;

	m_pCamera->SetOthoMatrix(windowSettings.width, windowSettings.height, NEAR_PLANE, FAR_PLANE);
	m_pCamera->SetProjectionMatrix(VerticalFoV, AspectRatio, NEAR_PLANE, FAR_PLANE);

	m_pCamera->SetPosition(cameraSettings.x, cameraSettings.y, cameraSettings.z);

	m_pCamera->m_yaw = m_pCamera->m_pitch = 0.0f;
	m_pCamera->Rotate(cameraSettings.yaw, cameraSettings.pitch * DEG2RAD, 1.0f);
}


void SceneManager::ToggleLightingModel()
{
	//const bool bIsPhong = m_selectedShader == SHADERS::FORWARD_PHONG || 0;// SHADERS::DEFERRED_PHONG;
	//const bool bIsBRDF = !bIsPhong;	// assumes only 2 shading models
	if (!m_useDeferredRendering)
	{
		m_selectedShader = m_selectedShader == SHADERS::FORWARD_PHONG ? SHADERS::FORWARD_BRDF : SHADERS::FORWARD_PHONG;
	}
}


void SceneManager::HandleInput()
{
	//-------------------------------------------------------------------------------- SHADER CONFIGURATION ----------------------------------------------------------
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------
	if (ENGINE->INP()->IsKeyTriggered("F1")) m_selectedShader = SHADERS::TEXTURE_COORDINATES;
	if (ENGINE->INP()->IsKeyTriggered("F2")) m_selectedShader = SHADERS::NORMAL;
	if (ENGINE->INP()->IsKeyTriggered("F3")) m_selectedShader = SHADERS::UNLIT;
	if (ENGINE->INP()->IsKeyTriggered("F4")) m_selectedShader = m_selectedShader == SHADERS::TBN ? SHADERS::FORWARD_BRDF : SHADERS::TBN;

	if (ENGINE->INP()->IsKeyTriggered("F5")) m_pRenderer->sEnableBlend = !m_pRenderer->sEnableBlend;
	if (ENGINE->INP()->IsKeyTriggered("F6")) ToggleLightingModel();
	if (ENGINE->INP()->IsKeyTriggered("F7")) m_debugRender = !m_debugRender;

	if (ENGINE->INP()->IsKeyTriggered("F9")) m_postProcessPass._bloomPass.ToggleBloomPass();

	if (ENGINE->INP()->IsKeyTriggered("R")) ReloadLevel();
	if (ENGINE->INP()->IsKeyTriggered("\\")) m_pRenderer->ReloadShaders();
	if (ENGINE->INP()->IsKeyTriggered(";")) m_bUsePaniniProjection = !m_bUsePaniniProjection;
	if (ENGINE->INP()->IsKeyTriggered("'")) m_roomScene.ToggleFloorNormalMap();

}

void SceneManager::Update(float dt)
{
	HandleInput();
	m_pCamera->Update(dt);
	m_roomScene.Update(dt);
}
//-----------------------------------------------------------------------------------------------------------------------------------------

void SceneManager::Render() const
{
	const XMMATRIX view = m_pCamera->GetViewMatrix();
	const XMMATRIX proj = m_pCamera->GetProjectionMatrix();
	const XMMATRIX viewProj = view * proj;
	
	// get shadow casters (todo: static/dynamic lights)
	std::vector<const Light*> _shadowCasters;
	for (const Light& light : m_roomScene.m_lights)
	{
		if (light._castsShadow)
			_shadowCasters.push_back(&light);
	}

	
	// SHADOW MAPS
	//------------------------------------------------------------------------
	m_pRenderer->UnbindRenderTarget();	// unbind the back render target | every pass has their own render targets
	m_shadowMapPass.RenderShadowMaps(m_pRenderer, _shadowCasters, m_ZPassObjects);

	// LIGHTING pass
	//------------------------------------------------------------------------
	m_pRenderer->Reset();
	m_pRenderer->BindDepthStencil(0);
	m_pRenderer->SetDepthStencilState(0);
	m_pRenderer->SetRasterizerState(static_cast<int>(EDefaultRasterizerState::CULL_NONE));
	m_pRenderer->SetViewport(m_pRenderer->WindowWidth(), m_pRenderer->WindowHeight());

	if (m_useDeferredRendering)
	{
		const GBuffer& gBuffer = m_deferredRenderingPasses._GBuffer;
		const TextureID texNormal			= m_pRenderer->GetRenderTargetTexture(gBuffer._normalRT);
		const TextureID texDiffuseRoughness = m_pRenderer->GetRenderTargetTexture(gBuffer._diffuseRoughnessRT);
		const TextureID texSpecularMetallic = m_pRenderer->GetRenderTargetTexture(gBuffer._specularMetallicRT);
		const TextureID texPosition			= m_pRenderer->GetRenderTargetTexture(gBuffer._positionRT);
		const TextureID texDepthTexture		= m_pRenderer->m_state._depthBufferTexture._id;
		
		// GEOMETRY - DEPTH PASS
		m_deferredRenderingPasses.SetGeometryRenderingStates(m_pRenderer);
		m_roomScene.Render(m_pRenderer, viewProj);

		// AMBIENT OCCLUSION  PASS
		if (m_isAmbientOcclusionOn)
		{
			// TODO: implement
		}

		// DEFERRED LIGHTING PASS
		m_deferredRenderingPasses.RenderLightingPass(m_pRenderer, m_postProcessPass._worldRenderTarget, m_pCamera.get(), m_roomScene.m_lights);
	}
	else
	{
		const float clearColor[4] = { 0.2f, 0.4f, 0.7f, 1.0f };
		const float clearDepth = 1.0f;

		// MAIN PASS
		//------------------------------------------------------------------------
		m_pRenderer->SetShader(m_selectedShader);	// forward brdf/phong
		m_pRenderer->BindRenderTarget(m_postProcessPass._worldRenderTarget);
		m_pRenderer->Begin(clearColor, clearDepth);
		m_pRenderer->SetConstant3f("cameraPos", m_pCamera->GetPositionF());
		m_pRenderer->SetSamplerState("sNormalSampler", 0);
		m_pRenderer->SetConstant1f("fovH", m_pCamera->m_settings.fovH * DEG2RAD);
		m_pRenderer->SetConstant1f("panini", m_bUsePaniniProjection ? 1.0f : 0.0f);

		SendLightData();
		m_roomScene.Render(m_pRenderer, viewProj);

		// Tangent-Bitangent-Normal drawing
		//------------------------------------------------------------------------
		const bool bIsShaderTBN = true;
		if (bIsShaderTBN)
		{
			m_pRenderer->SetShader(SHADERS::TBN);
			m_roomScene.cube.Render(m_pRenderer, viewProj, false);
			m_roomScene.sphere.Render(m_pRenderer, viewProj, false);

			m_pRenderer->SetShader(m_selectedShader);
		}
	}


#if 1
	// POST PROCESS PASS
	//------------------------------------------------------------------------
	m_postProcessPass.Render(m_pRenderer);


	// DEBUG PASS
	//------------------------------------------------------------------------
	if (m_debugRender)
	{
		// TBN test
		//auto prevShader = m_selectedShader;
		//m_selectedShader = m_renderData->TNBShader;
		//RenderCentralObjects(view, proj);
		//m_selectedShader = prevShader;

		m_pRenderer->SetShader(SHADERS::DEBUG);
		m_pRenderer->SetTexture("t_shadowMap", m_shadowMapPass._shadowMap);	// todo: decide shader naming 
		m_pRenderer->SetBufferObj(GEOMETRY::QUAD);
		m_pRenderer->Apply();
		m_pRenderer->DrawIndexed();
	}
#endif
	m_pRenderer->End();
}


void SceneManager::SendLightData() const
{
	// SPOT & POINT LIGHTS
	//--------------------------------------------------------------
	const LightShaderSignature defaultLight = LightShaderSignature();
	std::vector<LightShaderSignature> lights(MAX_LIGHTS, defaultLight);
	std::vector<LightShaderSignature> spots(MAX_SPOTS, defaultLight);
	unsigned spotCount = 0;
	unsigned lightCount = 0;
	for (const Light& l : m_roomScene.m_lights)
	{
		switch (l._type)
		{
		case Light::LightType::POINT:
			lights[lightCount] = l.ShaderSignature();
			++lightCount;
			break;
		case Light::LightType::SPOT:
			spots[spotCount] = l.ShaderSignature();
			++spotCount;
			break;
		default:
			OutputDebugString("SceneManager::RenderBuilding(): ERROR: UNKOWN LIGHT TYPE\n");
			break;
		}
	}
	m_pRenderer->SetConstant1f("lightCount", static_cast<float>(lightCount));
	m_pRenderer->SetConstant1f("spotCount" , static_cast<float>(spotCount));
	m_pRenderer->SetConstantStruct("lights", static_cast<void*>(lights.data()));
	m_pRenderer->SetConstantStruct("spots" , static_cast<void*>(spots.data()));

	// SHADOW MAPS
	//--------------------------------------------------------------
	// first light is spot: single shadaw map support for now
	const Light& caster = m_roomScene.m_lights[0];
	TextureID shadowMap = m_shadowMapPass._shadowMap;
	SamplerID shadowSampler = m_shadowMapPass._shadowSampler;

	m_pRenderer->SetConstant4x4f("lightSpaceMat", caster.GetLightSpaceMatrix());
	m_pRenderer->SetTexture("gShadowMap", shadowMap);
	m_pRenderer->SetSamplerState("sShadowSampler", shadowSampler);

#ifdef _DEBUG
	if (lights.size() > MAX_LIGHTS)	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (spots.size() > MAX_SPOTS)	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
}

