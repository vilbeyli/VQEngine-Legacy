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
	// enable toggling only on forward rendering for now
	if (!m_useDeferredRendering)
	{
		m_selectedShader = m_selectedShader == EShaders::FORWARD_PHONG ? EShaders::FORWARD_BRDF : EShaders::FORWARD_PHONG;
	}
	else
	{
		Log::Info("Deferred mode only supports BRDF Lighting model...");
	}
}

void Engine::ToggleRenderingPath()
{
	m_useDeferredRendering = !m_useDeferredRendering;

	// initialize GBuffer if its not initialized, i.e., 
	// Renderer started in forward mode and we're toggling deferred for the first time
	if (!m_deferredRenderingPasses._GBuffer.bInitialized && m_useDeferredRendering)
	{	
		m_deferredRenderingPasses.InitializeGBuffer(m_pRenderer);
	}
	Log::Info("Toggle Rendering Path: %s Rendering enabled", m_useDeferredRendering ? "Deferred" : "Forward");

	// if we just turned deferred rendering off, clear the gbuffer textures
	if(!m_useDeferredRendering)
		m_deferredRenderingPasses.ClearGBuffer(m_pRenderer);
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

	// render passes
	m_useDeferredRendering = s_rendererSettings.bUseDeferredRendering;
	if (m_useDeferredRendering || true)	// initialize G buffer nevertheless
	{
		m_deferredRenderingPasses.InitializeGBuffer(m_pRenderer);
	}
	m_isAmbientOcclusionOn = s_rendererSettings.bAmbientOcclusion;
	m_debugRender = true;
	m_selectedShader = m_useDeferredRendering ? EShaders::DEFERRED_GEOMETRY : EShaders::FORWARD_BRDF;
	
	// default states
	const bool bDepthWrite = true;
	const bool bStencilWrite = false;
	m_defaultDepthStencilState = m_pRenderer->AddDepthStencilState(bDepthWrite, bStencilWrite);
	m_worldDepthTarget = 0;	// assumes first index in renderer->m_depthTargets[]

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
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.obj2);
		m_ZPassObjects.push_back(&m_sceneManager->m_roomScene.Plane2);

		for (GameObject& obj : m_sceneManager->m_roomScene.cubes)	m_ZPassObjects.push_back(&obj);
		for (GameObject& obj : m_sceneManager->m_roomScene.spheres)	m_ZPassObjects.push_back(&obj);

		m_postProcessPass.Initialize(m_pRenderer, s_rendererSettings.postProcess);
		m_debugPass.Initialize(m_pRenderer);
		m_SSAOPass.Initialize(m_pRenderer);

		// Samplers
		D3D11_SAMPLER_DESC normalSamplerDesc = {};
		normalSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		normalSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		normalSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
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

	if (m_input->IsKeyTriggered("F1")) m_selectedShader = EShaders::TEXTURE_COORDINATES;
	if (m_input->IsKeyTriggered("F2")) m_selectedShader = EShaders::NORMAL;
	if (m_input->IsKeyTriggered("F3")) m_selectedShader = EShaders::UNLIT;
	if (m_input->IsKeyTriggered("F4")) m_selectedShader = m_selectedShader == EShaders::TBN ? EShaders::FORWARD_BRDF : EShaders::TBN;

	if (m_input->IsKeyTriggered("F5")) m_pRenderer->sEnableBlend = !m_pRenderer->sEnableBlend;
	if (m_input->IsKeyTriggered("F6")) ToggleLightingModel();
	if (m_input->IsKeyTriggered("F7")) m_debugRender = !m_debugRender;
	if (m_input->IsKeyTriggered("F8")) ToggleRenderingPath();

	if (m_input->IsKeyTriggered("F9")) m_postProcessPass._bloomPass.ToggleBloomPass();

	if (m_input->IsKeyTriggered("R")) m_sceneManager->ReloadLevel();
	if (m_input->IsKeyTriggered("\\")) m_pRenderer->ReloadShaders();
	if (m_input->IsKeyTriggered(";")) m_bUsePaniniProjection = !m_bUsePaniniProjection;

	if (m_input->IsKeyDown("Shift"))
	{
		const float step = 0.1f;
		if (m_input->IsScrollUp())   { m_SSAOPass.intensity += step; Log::Info("SSAO Intensity: %.2f", m_SSAOPass.intensity); }
		if (m_input->IsScrollDown()) { m_SSAOPass.intensity -= step; if (m_SSAOPass.intensity < 1.001) m_SSAOPass.intensity = 1.0f; Log::Info("SSAO Intensity: %.2f", m_SSAOPass.intensity); }
	}
	else
	{
		const float step = 0.5f;
		if (m_input->IsScrollUp())   { m_SSAOPass.radius += step; Log::Info("SSAO Radius: %.2f", m_SSAOPass.radius); }
		if (m_input->IsScrollDown()) { m_SSAOPass.radius -= step; if (m_SSAOPass.radius < 1.001) m_SSAOPass.radius = 1.0f; Log::Info("SSAO Radius: %.2f", m_SSAOPass.radius); }
	}

	return true;
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
	const XMMATRIX view			= m_pCamera->GetViewMatrix();
	const XMMATRIX viewInverse	= m_pCamera->GetViewInverseMatrix();
	const XMMATRIX proj			= m_pCamera->GetProjectionMatrix();
	m_sceneView.viewProj = view * proj;
	m_sceneView.view = view;
	m_sceneView.viewToWorld = viewInverse;
	m_sceneView.projection = proj;

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

void Engine::RenderLights() const
{
	m_pRenderer->BeginEvent("Render Lights Pass");
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
	m_pRenderer->EndEvent();
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
	m_pRenderer->SetTexture("texShadowMap", caster.shadowMap);
	m_pRenderer->SetSamplerState("sShadowSampler", caster.shadowSampler);

#ifdef _DEBUG
	if (m_sceneLightData.pointLightCount > m_sceneLightData.pointLights.size())	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (m_sceneLightData.spotLightCount > m_sceneLightData.spotLights.size())	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
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
	m_pRenderer->BeginEvent("Shadow Pass");
	m_pRenderer->UnbindRenderTarget();	// unbind the back render target | every pass has their own render targets
	m_shadowMapPass.RenderShadowMaps(m_pRenderer, _shadowCasters, m_ZPassObjects);
	m_pRenderer->EndEvent();

	// LIGHTING PASS
	//------------------------------------------------------------------------
	m_pRenderer->Reset();
	m_pRenderer->SetRasterizerState(static_cast<int>(EDefaultRasterizerState::CULL_NONE));
	m_pRenderer->SetViewport(m_pRenderer->WindowWidth(), m_pRenderer->WindowHeight());

	if (m_useDeferredRendering)
	{	// DEFERRED
		const GBuffer& gBuffer = m_deferredRenderingPasses._GBuffer;
		const TextureID texNormal = m_pRenderer->GetRenderTargetTexture(gBuffer._normalRT);
		const TextureID texDiffuseRoughness = m_pRenderer->GetRenderTargetTexture(gBuffer._diffuseRoughnessRT);
		const TextureID texSpecularMetallic = m_pRenderer->GetRenderTargetTexture(gBuffer._specularMetallicRT);
		const TextureID texPosition = m_pRenderer->GetRenderTargetTexture(gBuffer._positionRT);
		const TextureID texDepthTexture = m_pRenderer->m_state._depthBufferTexture;

		// GEOMETRY - DEPTH PASS
		m_pRenderer->BeginEvent("Geometry Pass");
		m_deferredRenderingPasses.SetGeometryRenderingStates(m_pRenderer);
		m_sceneManager->Render(m_pRenderer, m_sceneView);
		m_pRenderer->EndEvent();

		// AMBIENT OCCLUSION  PASS
		if (m_isAmbientOcclusionOn)
		{
			m_pRenderer->BeginEvent("Ambient Occlusion Pass");
			m_SSAOPass.RenderOcclusion(m_pRenderer, texNormal, texPosition, m_sceneView);
			//m_SSAOPass.BilateralBlurPass(m_pRenderer);	// todo
			m_SSAOPass.GaussianBlurPass(m_pRenderer);
			m_pRenderer->EndEvent();
		}

		// DEFERRED LIGHTING PASS
		const TextureID tSSAO = m_isAmbientOcclusionOn ? m_pRenderer->GetRenderTargetTexture(m_SSAOPass.blurRenderTarget) : -1;
		m_pRenderer->BeginEvent("Lighting Pass");
		m_deferredRenderingPasses.RenderLightingPass(m_pRenderer, m_postProcessPass._worldRenderTarget, m_sceneView, m_sceneLightData, tSSAO);
		m_pRenderer->EndEvent();

		// LIGHT SOURCES
		m_pRenderer->BindDepthTarget(m_worldDepthTarget);
		RenderLights();

		// SKYBOX
		if (m_activeSkybox != ESkyboxPresets::SKYBOX_PRESET_COUNT)
		{
			m_pRenderer->SetDepthStencilState(m_deferredRenderingPasses._skyboxStencilState);
			Skybox::s_Presets[m_activeSkybox].Render(m_sceneView.viewProj);
			m_pRenderer->SetDepthStencilState(m_defaultDepthStencilState);
			m_pRenderer->UnbindDepthTarget();
		}
	}


	else
	{	// FORWARD
		m_selectedShader = m_selectedShader == EShaders::DEFERRED_GEOMETRY ? EShaders::FORWARD_BRDF : m_selectedShader;
		const float clearColor[4] = { 0.2f, 0.4f, 0.3f, 1.0f };
		const float clearDepth = 1.0f;

		m_pRenderer->BindRenderTarget(m_postProcessPass._worldRenderTarget);
		m_pRenderer->BindDepthTarget(0);
		m_pRenderer->SetDepthStencilState(m_defaultDepthStencilState);
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

		// LIGHTING
		m_pRenderer->BeginEvent("Lighting Pass");
		m_pRenderer->SetShader(m_selectedShader);	// forward brdf/phong
		m_pRenderer->SetConstant3f("cameraPos", m_pCamera->GetPositionF());
		m_pRenderer->SetSamplerState("sNormalSampler", m_normalSampler);

		SendLightData();
		m_sceneManager->Render(m_pRenderer, m_sceneView);
		m_pRenderer->EndEvent();

		RenderLights();
	}

	// Tangent-Bitangent-Normal drawing
	//------------------------------------------------------------------------
	const bool bIsShaderTBN = true;
	if (bIsShaderTBN)
	{
		m_pRenderer->BeginEvent("Draw TBN Vectors");
		if (m_useDeferredRendering)
			m_pRenderer->BindDepthTarget(m_worldDepthTarget);

		m_pRenderer->SetShader(EShaders::TBN);
		m_sceneManager->m_roomScene.cube.Render(m_pRenderer, m_sceneView, false);
		m_sceneManager->m_roomScene.sphere.Render(m_pRenderer, m_sceneView, false);

		if (m_useDeferredRendering)
			m_pRenderer->UnbindDepthTarget();

		m_pRenderer->SetShader(m_selectedShader);
		m_pRenderer->EndEvent();
	}

#if 1
	// POST PROCESS PASS
	//------------------------------------------------------------------------
	m_postProcessPass.Render(m_pRenderer);


	// DEBUG PASS
	//------------------------------------------------------------------------
	if (m_debugRender)
	{
		const int screenWidth  = s_rendererSettings.window.width;
		const int screenHeight = s_rendererSettings.window.height;
		const float aspectRatio = static_cast<float>(screenWidth) / screenHeight;
#if 0
		// TODO: calculate scales and transform each quad into appropriate position in NDC space [0,1]
		// y coordinate is upside-down, region covering the 70% <-> 90% of screen height 
		const float topPct = 0.7f;	const float botPct = 0.9f;
		const int rectTop = 0;// * topPct;
		const int rectBot = screenHeight;// * botPct;
		//m_pRenderer->SetRasterizerState(m_debugPass._scissorsRasterizer);
		//m_pRenderer->SetScissorsRect(0, screenWidth, rectTop, rectBot);
#endif
		// debug texture strip draw settings
		const int bottomPaddingPx = 0;	 // offset from bottom of the screen
		const int heightPx        = 128; // height for every texture
		const int paddingPx       = 0;	 // padding between debug textures
		const vec2 fullscreenTextureScaledDownSize((float)heightPx * aspectRatio, (float)heightPx);
		const vec2 squareTextureScaledDownSize    ((float)heightPx              , (float)heightPx);

		// Textures to draw
		TextureID tShadowMap		 = m_pRenderer->GetDepthTargetTexture(m_shadowMapPass._shadowDepthTarget);
		TextureID tBlurredBloom		 = m_pRenderer->GetRenderTargetTexture(m_postProcessPass._bloomPass._blurPingPong[0]);
		TextureID tDiffuseRoughness  = m_pRenderer->GetRenderTargetTexture(m_deferredRenderingPasses._GBuffer._diffuseRoughnessRT);
		//TextureID tSceneDepth		 = m_pRenderer->m_state._depthBufferTexture._id;
		TextureID tSceneDepth		 = m_pRenderer->GetDepthTargetTexture(0);
		TextureID tNormals			 = m_pRenderer->GetRenderTargetTexture(m_deferredRenderingPasses._GBuffer._normalRT);
		TextureID tAO				 = m_pRenderer->GetRenderTargetTexture(m_SSAOPass.blurRenderTarget);

		const std::vector<DrawQuadOnScreenCommand> quadCmds = [&]() {
			const vec2 screenPosition(0.0f, (float)bottomPaddingPx);
			std::vector<DrawQuadOnScreenCommand> c
			{	//		Pixel Dimensions	 Screen Position (offset below)	  Texture			DepthTexture?
				{ fullscreenTextureScaledDownSize,	screenPosition,			tSceneDepth			, true},
				{ fullscreenTextureScaledDownSize,	screenPosition,			tDiffuseRoughness	, false},
				{ fullscreenTextureScaledDownSize,	screenPosition,			tNormals			, false},
				{ squareTextureScaledDownSize    ,	screenPosition,			tShadowMap			, true},
				{ fullscreenTextureScaledDownSize,	screenPosition,			tBlurredBloom		, false},
				{ fullscreenTextureScaledDownSize,	screenPosition,			tAO					, false},
				//{ vec2(1600, 900),	vec2(0,0),			tAO					, false},
			};
			for (size_t i = 1; i < c.size(); i++)	// offset textures accordingly (using previous' x-dimension)
				c[i].bottomLeftCornerScreenCoordinates.x() = c[i-1].bottomLeftCornerScreenCoordinates.x() + c[i - 1].dimensionsInPixels.x() + paddingPx;
			return c;
		}();

		m_pRenderer->BeginEvent("Debug Pass");
		m_pRenderer->SetShader(EShaders::DEBUG);
		for (const DrawQuadOnScreenCommand& cmd : quadCmds)
		{
			m_pRenderer->DrawQuadOnScreen(cmd);
		}
		m_pRenderer->EndEvent();
	}
#endif
	m_pRenderer->End();
}

