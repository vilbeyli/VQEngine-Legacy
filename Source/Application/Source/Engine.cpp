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

#include "Engine.h"
#include "Log.h"
#include "Input.h"
#include "PerfTimer.h"
#include "SceneParser.h"
#include "SceneManager.h"
#include "WorkerPool.h"
#include "Settings.h"

#include "Renderer.h"
#include "Camera.h"

#include <sstream>

Engine* Engine::s_instance = nullptr;

Engine::Engine()
	:
	m_pRenderer(new Renderer()),
	m_input(new Input()),
	m_timer(new PerfTimer()),
	m_sceneManager(new SceneManager(m_pCamera, m_lights)),
	m_pCamera(new Camera()),
	m_activeSkybox(ESkyboxPresets::SKYBOX_PRESET_COUNT),	// default: none
	m_bUsePaniniProjection(false)
{}

Engine::~Engine(){}


void Engine::TogglePause()
{
	m_isPaused = !m_isPaused;
}


void Engine::Exit()
{
	m_pRenderer->Exit();

	m_workerPool.Terminate();

	Log::Exit();
	if (s_instance)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

const Input* Engine::INP() const
{
	return m_input;
}

Engine * Engine::GetEngine()
{
	if (s_instance == nullptr)
	{
		s_instance = new Engine();
	}

	return s_instance;
}

void Engine::ToggleLightingModel()
{
	//const bool bIsPhong = m_selectedShader == SHADERS::FORWARD_PHONG || 0;// SHADERS::DEFERRED_PHONG;
	//const bool bIsBRDF = !bIsPhong;	// assumes only 2 shading models
	if (!m_useDeferredRendering)
	{
		m_selectedShader = m_selectedShader == EShaders::FORWARD_PHONG ? EShaders::FORWARD_BRDF : EShaders::FORWARD_PHONG;
	}
}

void Engine::Pause()
{
	m_isPaused = true;
}

void Engine::Unpause()
{
	m_isPaused = false;
}

float Engine::TotalTime() const
{
	return m_timer->TotalTime();
}

const Settings::Renderer& Engine::InitializeRendererSettingsFromFile()
{
	s_rendererSettings = SceneParser::ReadRendererSettings();
	
	return s_rendererSettings;
}

bool Engine::Initialize(HWND hwnd)
{
	if (!m_pRenderer || !m_input || !m_sceneManager || !m_timer)
	{
		Log::Error("Nullptr Engine::Init()\n");
		return false;
	}

	// INITIALIZE SYSTEMS
	//--------------------------------------------------------------
	const bool bEnableLogging = true;	// todo: read from settings
	Log::Initialize(bEnableLogging);
	
	constexpr size_t workerCount = 1;
	m_workerPool.Initialize(workerCount);
		
	m_input->Initialize();
	

	// INITIALIZE RENDERING
	//--------------------------------------------------------------
	if (!m_pRenderer->Initialize(hwnd, s_rendererSettings))
	{
		Log::Error("Cannot initialize Renderer.\n");
		return false;
	}

	m_useDeferredRendering = s_rendererSettings.bUseDeferredRendering;
	if (m_useDeferredRendering)
	{
		m_deferredRenderingPasses.InitializeGBuffer(m_pRenderer);
	}
	m_selectedShader = m_useDeferredRendering ? EShaders::DEFERRED_GEOMETRY : EShaders::FORWARD_BRDF;

	m_isAmbientOcclusionOn = s_rendererSettings.bAmbientOcclusion;
	m_debugRender = false;

	Skybox::InitializePresets(m_pRenderer);
	return true;
}

bool Engine::Load()
{
	bool bLoadSuccess = false;
	m_sceneManager->Load(m_pRenderer, nullptr, s_rendererSettings, m_pCamera);
	{	// RENDER PASS INITIALIZATION
		// todo: static game object array in gameobj.h or engine.h
		m_shadowMapPass.Initialize(m_pRenderer, m_pRenderer->m_device, s_rendererSettings.shadowMap);
		//renderer->m_Direct3D->ReportLiveObjects();
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.m_room.floor);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.m_room.wallL);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.m_room.wallR);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.m_room.wallF);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.m_room.ceiling);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.quad);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.grid);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.cylinder);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.triangle);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.cube);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.sphere);
		for (GameObject& obj : m_sceneManager->m_roomScene.cubes)	m_ZPassObjects.push_back(&obj);
		for (GameObject& obj : m_sceneManager->m_roomScene.spheres)	m_ZPassObjects.push_back(&obj);

		m_postProcessPass.Initialize(m_pRenderer, s_rendererSettings.postProcess);

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
		m_normalSampler = m_pRenderer->CreateSamplerState(normalSamplerDesc);
	}

	m_activeSkybox = m_sceneManager->GetSceneSkybox();

	bLoadSuccess = true;
	return bLoadSuccess;
}

void Engine::CalcFrameStats()
{
	static long  frameCount = 0;
	static float timeElaped = -0.9f;
	constexpr float UpdateInterval = 0.5f;

	++frameCount;
	if (m_timer->TotalTime() - timeElaped >= UpdateInterval)
	{
		float fps = static_cast<float>(frameCount);	// #frames / 1.0f
		float frameTime = 1000.0f / fps;			// milliseconds

		std::ostringstream stats;
		stats.precision(2);
		stats	<< "VDemo | "
				<< "dt: " << frameTime << "ms "; 
		stats.precision(4);
		stats	<< "FPS: " << fps;
		SetWindowText(m_pRenderer->GetWindow(), stats.str().c_str());
		frameCount = 0;
		timeElaped += UpdateInterval;
	}
}

bool Engine::HandleInput()
{
	if (m_input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	if (m_input->IsKeyTriggered(0x08)) // Key backspace
		TogglePause();

	if (ENGINE->INP()->IsKeyTriggered("F1")) m_selectedShader = EShaders::TEXTURE_COORDINATES;
	if (ENGINE->INP()->IsKeyTriggered("F2")) m_selectedShader = EShaders::NORMAL;
	if (ENGINE->INP()->IsKeyTriggered("F3")) m_selectedShader = EShaders::UNLIT;
	if (ENGINE->INP()->IsKeyTriggered("F4")) m_selectedShader = m_selectedShader == EShaders::TBN ? EShaders::FORWARD_BRDF : EShaders::TBN;
	m_pRenderer->SetShader(0);
	if (ENGINE->INP()->IsKeyTriggered("F5")) m_pRenderer->sEnableBlend = !m_pRenderer->sEnableBlend;
	if (ENGINE->INP()->IsKeyTriggered("F6")) ToggleLightingModel();
	if (ENGINE->INP()->IsKeyTriggered("F7")) m_debugRender = !m_debugRender;

	if (ENGINE->INP()->IsKeyTriggered("F9")) m_postProcessPass._bloomPass.ToggleBloomPass();

	if (ENGINE->INP()->IsKeyTriggered("R")) m_sceneManager->ReloadLevel();
	if (ENGINE->INP()->IsKeyTriggered("\\")) m_pRenderer->ReloadShaders();
	if (ENGINE->INP()->IsKeyTriggered(";")) m_bUsePaniniProjection = !m_bUsePaniniProjection;

	return true;
}

void Engine::RenderLights() const
{
	m_pRenderer->Reset();	// is reset necessary?
	m_pRenderer->SetShader(EShaders::UNLIT);
	for (const Light& light : m_lights)
	{
		m_pRenderer->SetBufferObj(light._renderMesh);
		const XMMATRIX world = light._transform.WorldTransformationMatrix();
		const XMMATRIX worldViewProj = world  * m_sceneView.viewProj;
		const vec3 color = light._color.Value();
		m_pRenderer->SetConstant4x4f("worldViewProj", worldViewProj);
		m_pRenderer->SetConstant3f("diffuse", color);
		m_pRenderer->SetConstant1f("isDiffuseMap", 0.0f);
		m_pRenderer->Apply();
		m_pRenderer->DrawIndexed();
	}
}


void Engine::SendLightData() const
{
	// SPOT & POINT LIGHTS
	//--------------------------------------------------------------
	m_pRenderer->SetConstant1f("lightCount", static_cast<float>(m_sceneLightData.pointLightCount));
	m_pRenderer->SetConstant1f("spotCount", static_cast<float>(m_sceneLightData.spotLightCount));
	m_pRenderer->SetConstantStruct("lights", static_cast<const void*>(m_sceneLightData.pointLights.data()));
	m_pRenderer->SetConstantStruct("spots", static_cast<const void*>(m_sceneLightData.spotLights.data()));

	// SHADOW MAPS
	//--------------------------------------------------------------
	// first light is spot: single shadow map support for now
	const ShadowCasterData& caster = m_sceneLightData.shadowCasterData[0];
	m_pRenderer->SetConstant4x4f("lightSpaceMat", caster.lightSpaceMatrix);
	m_pRenderer->SetTexture("gShadowMap", caster.shadowMap);
	m_pRenderer->SetSamplerState("sShadowSampler", caster.shadowSampler);

#ifdef _DEBUG
	if (m_sceneLightData.pointLightCount > m_sceneLightData.pointLights.size())	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (m_sceneLightData.spotLightCount > m_sceneLightData.spotLights.size())	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
}

bool Engine::UpdateAndRender()
{
	const float dt = m_timer->Tick();
	const bool bExitApp = !HandleInput();
	if (!m_isPaused)
	{
		CalcFrameStats();

		m_pCamera->Update(dt);
		m_sceneManager->Update(dt);

		PreRender();
		Render();
	}

	m_input->Update();	// update previous state after frame;
	return bExitApp;
}

// prepares rendering context: gets data from scene and sets up data structures ready to be sent to GPU
void Engine::PreRender()
{
	// set scene view
	const XMMATRIX view = m_pCamera->GetViewMatrix();
	const XMMATRIX proj = m_pCamera->GetProjectionMatrix();
	m_sceneView.viewProj = view * proj;
	m_sceneView.pCamera = m_pCamera;

	// gather scene lights
	size_t spotCount = 0;	size_t lightCount = 0;
	for (const Light& l : m_lights)
	{
		switch (l._type)
		{
		case Light::ELightType::POINT:
			m_sceneLightData.pointLights[lightCount] = l.ShaderSignature();
			++lightCount;
			break;
		case Light::ELightType::SPOT:
			m_sceneLightData.spotLights[spotCount] = l.ShaderSignature();
			++spotCount;
			break;
		default:
			OutputDebugString("SceneManager::RenderBuilding(): ERROR: UNKOWN LIGHT TYPE\n");
			break;
		}
	}
	m_sceneLightData.spotLightCount = spotCount;
	m_sceneLightData.pointLightCount = lightCount;

	m_sceneLightData.shadowCasterData[0].shadowMap = m_shadowMapPass._shadowMap;
	m_sceneLightData.shadowCasterData[0].shadowSampler = m_shadowMapPass._shadowSampler;
	m_sceneLightData.shadowCasterData[0].lightSpaceMatrix = m_lights[0].GetLightSpaceMatrix();
}


void Engine::Render()
{	
	const XMMATRIX& viewProj = m_sceneView.viewProj;
	
	// get shadow casters (todo: static/dynamic lights)
	std::vector<const Light*> _shadowCasters;
	for (const Light& light : m_sceneManager->m_roomScene.m_lights)
	{
		if (light._castsShadow)
			_shadowCasters.push_back(&light);
	}


	// SHADOW MAPS
	//------------------------------------------------------------------------
	m_pRenderer->UnbindRenderTarget();	// unbind the back render target | every pass has their own render targets
	m_shadowMapPass.RenderShadowMaps(m_pRenderer, _shadowCasters, m_ZPassObjects);

	// LIGHTING PASS
	//------------------------------------------------------------------------
	m_pRenderer->Reset();
	m_pRenderer->BindDepthTarget(0);
	m_pRenderer->SetDepthStencilState(0);
	m_pRenderer->SetRasterizerState(static_cast<int>(EDefaultRasterizerState::CULL_NONE));
	m_pRenderer->SetViewport(m_pRenderer->WindowWidth(), m_pRenderer->WindowHeight());

	if (m_useDeferredRendering)
	{	// DEFERRED
		const GBuffer& gBuffer = m_deferredRenderingPasses._GBuffer;
		const TextureID texNormal = m_pRenderer->GetRenderTargetTexture(gBuffer._normalRT);
		const TextureID texDiffuseRoughness = m_pRenderer->GetRenderTargetTexture(gBuffer._diffuseRoughnessRT);
		const TextureID texSpecularMetallic = m_pRenderer->GetRenderTargetTexture(gBuffer._specularMetallicRT);
		const TextureID texPosition = m_pRenderer->GetRenderTargetTexture(gBuffer._positionRT);
		const TextureID texDepthTexture = m_pRenderer->m_state._depthBufferTexture._id;

		// GEOMETRY - DEPTH PASS
		m_deferredRenderingPasses.SetGeometryRenderingStates(m_pRenderer);
		m_sceneManager->Render(m_pRenderer, m_sceneView);

		// AMBIENT OCCLUSION  PASS
		if (m_isAmbientOcclusionOn)
		{
			// TODO: implement
		}

		// DEFERRED LIGHTING PASS
		m_deferredRenderingPasses.RenderLightingPass(m_pRenderer, m_postProcessPass._worldRenderTarget, m_sceneView, m_sceneLightData);

		// TODO: skybox
	}
	else
	{	// FORWARD
		const float clearColor[4] = { 0.2f, 0.4f, 0.3f, 1.0f };
		const float clearDepth = 1.0f;

		// MAIN PASS
		m_pRenderer->BindRenderTarget(m_postProcessPass._worldRenderTarget);
		m_pRenderer->Begin(clearColor, clearDepth);

		// SKYBOX
		// if we're not rendering the skybox, call apply() to unbind
		// shadow light depth target so we can bind it in the lighting pass
		// otherwise, skybox render pass will take care of it
		if (m_activeSkybox != ESkyboxPresets::SKYBOX_PRESET_COUNT)	Skybox::s_Presets[m_activeSkybox].Render(m_sceneView.viewProj);
		else
		{
			// todo: this might be costly, profile this
			m_pRenderer->SetShader(m_selectedShader);	// set shader so apply won't complain 
			m_pRenderer->Apply();						// apply to bind depth stencil
		}

		m_pRenderer->SetShader(m_selectedShader);	// forward brdf/phong
		m_pRenderer->SetConstant3f("cameraPos", m_pCamera->GetPositionF());
		m_pRenderer->SetSamplerState("sNormalSampler", 0);
		m_pRenderer->SetConstant1f("fovH", m_pCamera->m_settings.fovH * DEG2RAD);
		m_pRenderer->SetConstant1f("panini", m_bUsePaniniProjection ? 1.0f : 0.0f);

		SendLightData();
		m_sceneManager->Render(m_pRenderer, m_sceneView);

		// Tangent-Bitangent-Normal drawing
		//------------------------------------------------------------------------
		const bool bIsShaderTBN = true;
		if (bIsShaderTBN)
		{
			m_pRenderer->SetShader(EShaders::TBN);
			m_sceneManager->m_roomScene.cube.Render(m_pRenderer, viewProj, false);
			m_sceneManager->m_roomScene.sphere.Render(m_pRenderer, viewProj, false);

			m_pRenderer->SetShader(m_selectedShader);
		}
	}

	RenderLights();


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

		m_pRenderer->SetShader(EShaders::DEBUG);
		m_pRenderer->SetTexture("t_shadowMap", m_shadowMapPass._shadowMap);	// todo: decide shader naming 
		m_pRenderer->SetBufferObj(EGeometry::QUAD);
		m_pRenderer->Apply();
		m_pRenderer->DrawIndexed();
	}
#endif
	m_pRenderer->End();
}

