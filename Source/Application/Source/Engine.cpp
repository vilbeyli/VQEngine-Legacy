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
#include "Input.h"
#include "Utilities/Log.h"
#include "Utilities/PerfTimer.h"
#include "Utilities/CustomParser.h"
#include "SceneManager.h"
#include "WorkerPool.h"
#include "Settings.h"

#include "Renderer/Renderer.h"
#include "Camera.h"

#include <sstream>

Engine* Engine::sInstance = nullptr;

Engine::Engine()
	:
	mpRenderer(new Renderer()),
	mpInput(new Input()),
	mpTimer(new PerfTimer()),
	mpSceneManager(new SceneManager(mLights)),
	mActiveSkybox(ESkyboxPreset::SKYBOX_PRESET_COUNT),	// default: none
	mbUsePaniniProjection(false)
	//,mObjectPool(1024)
{}

Engine::~Engine(){}

void Engine::CalcFrameStats()
{
	static long  frameCount = 0;
	static float timeElaped = -0.9f;
	constexpr float UpdateInterval = 0.5f;

	++frameCount;
	if (mpTimer->TotalTime() - timeElaped >= UpdateInterval)
	{
		float fps = static_cast<float>(frameCount);	// #frames / 1.0f
		float frameTime = 1000.0f / fps;			// milliseconds

		std::ostringstream stats;
		stats.precision(2);
		stats << "VDemo | "
			<< "dt: " << frameTime << "ms ";
		stats.precision(4);
		stats << "FPS: " << fps;
		SetWindowText(mpRenderer->GetWindow(), stats.str().c_str());
		frameCount = 0;
		timeElaped += UpdateInterval;
	}
}


void Engine::Exit()
{
	mpRenderer->Exit();

	mWorkerPool.Terminate();

	Log::Exit();
	if (sInstance)
	{
		delete sInstance;
		sInstance = nullptr;
	}
}

Engine * Engine::GetEngine()
{
	if (sInstance == nullptr)
	{
		sInstance = new Engine();
	}

	return sInstance;
}

void Engine::ToggleLightingModel()
{
	// enable toggling only on forward rendering for now
	const bool isBRDF = sEngineSettings.rendering.bUseBRDFLighting = !sEngineSettings.rendering.bUseBRDFLighting;
	mSelectedShader = mbUseDeferredRendering 
		? mDeferredRenderingPasses._geometryShader
		: (isBRDF ? EShaders::FORWARD_BRDF : EShaders::FORWARD_PHONG);
	Log::Info("Toggle Lighting Model: %s Ligting", isBRDF ? "PBR - BRDF" : "Blinn-Phong");
}

void Engine::ToggleRenderingPath()
{
	mbUseDeferredRendering = !mbUseDeferredRendering;

	// initialize GBuffer if its not initialized, i.e., 
	// Renderer started in forward mode and we're toggling deferred for the first time
	if (!mDeferredRenderingPasses._GBuffer.bInitialized && mbUseDeferredRendering)
	{	
		mDeferredRenderingPasses.InitializeGBuffer(mpRenderer);
	}
	Log::Info("Toggle Rendering Path: %s Rendering enabled", mbUseDeferredRendering ? "Deferred" : "Forward");

	// if we just turned deferred rendering off, clear the gbuffer textures
	if (!mbUseDeferredRendering)
	{
		mDeferredRenderingPasses.ClearGBuffer(mpRenderer);
		mSelectedShader = sEngineSettings.rendering.bUseBRDFLighting ? EShaders::FORWARD_BRDF : EShaders::FORWARD_PHONG;
	}
}

void Engine::ToggleAmbientOcclusion()
{
	mbIsAmbientOcclusionOn = !mbIsAmbientOcclusionOn;
	if (!mbIsAmbientOcclusionOn)
	{
		mDeferredRenderingPasses.ClearGBuffer(mpRenderer);
	}
	Log::Info("Toggle Ambient Occlusion: %s", mbIsAmbientOcclusionOn ? "On" : "Off");
}

void Engine::Pause()
{
	mbIsPaused = true;
}

void Engine::Unpause()
{
	mbIsPaused = false;
}

const Settings::Engine& Engine::ReadSettingsFromFile()
{
	sEngineSettings = Parser::ReadSettings("EngineSettings.ini");
	return sEngineSettings;
}

bool Engine::Initialize(HWND hwnd)
{
	if (!mpRenderer || !mpInput || !mpSceneManager || !mpTimer)
	{
		Log::Error("Nullptr Engine::Init()\n");
		return false;
	}

	// INITIALIZE SYSTEMS
	//--------------------------------------------------------------
	const bool bEnableLogging = true;	// todo: read from settings
	constexpr size_t workerCount = 1;
	const Settings::Rendering& rendererSettings = sEngineSettings.rendering;
	const Settings::Window& windowSettings = sEngineSettings.window;

	Log::Initialize(bEnableLogging);
	mWorkerPool.Initialize(workerCount);
	mpInput->Initialize();
	if (!mpRenderer->Initialize(hwnd, windowSettings))
	{
		Log::Error("Cannot initialize Renderer.\n");
		return false;
	}


	// INITIALIZE RENDERING
	//--------------------------------------------------------------
	// render passes
	mbUseDeferredRendering = rendererSettings.bUseDeferredRendering;
	mbIsAmbientOcclusionOn = rendererSettings.bAmbientOcclusion;
	mDebugRender = true;
	mSelectedShader = mbUseDeferredRendering ? mDeferredRenderingPasses._geometryShader : EShaders::FORWARD_BRDF;
	mWorldDepthTarget = 0;	// assumes first index in renderer->m_depthTargets[]

	Skybox::InitializePresets(mpRenderer);

	return true;
}

bool Engine::Load()
{
	const Settings::Rendering& rendererSettings = sEngineSettings.rendering;
	const bool bLoadSuccess = mpSceneManager->Load(mpRenderer, nullptr, sEngineSettings, mZPassObjects);
	if (!bLoadSuccess)
	{
		Log::Error("Engine couldn't load scene.");
		return false;
	}

	{	// RENDER PASS INITIALIZATION
		mShadowMapPass.Initialize(mpRenderer, mpRenderer->m_device, rendererSettings.shadowMap);
		//renderer->m_Direct3D->ReportLiveObjects();
		
		mDeferredRenderingPasses.Initialize(mpRenderer);
		mPostProcessPass.Initialize(mpRenderer, rendererSettings.postProcess);
		mDebugPass.Initialize(mpRenderer);
		mSSAOPass.Initialize(mpRenderer);

		// Samplers
		D3D11_SAMPLER_DESC normalSamplerDesc = {};
		normalSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		normalSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		normalSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		normalSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		normalSamplerDesc.MinLOD = 0.f;
		normalSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		normalSamplerDesc.MipLODBias = 0.f;
		normalSamplerDesc.MaxAnisotropy = 0;
		normalSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		mNormalSampler = mpRenderer->CreateSamplerState(normalSamplerDesc);
	}
	return true;
}


bool Engine::HandleInput()
{
	if (mpInput->IsKeyDown("Escape"))			return false;
	if (mpInput->IsKeyTriggered("Backspace"))	TogglePause();

	if (!mbUseDeferredRendering)
	{	// forward only keys
		if (mpInput->IsKeyTriggered("F1")) mSelectedShader = EShaders::TEXTURE_COORDINATES;
		if (mpInput->IsKeyTriggered("F2")) mSelectedShader = EShaders::NORMAL;
		if (mpInput->IsKeyTriggered("F3")) mSelectedShader = EShaders::UNLIT;
		if (mpInput->IsKeyTriggered("F4")) mSelectedShader = mSelectedShader == EShaders::TBN ? (sEngineSettings.rendering.bUseBRDFLighting ? EShaders::FORWARD_BRDF : EShaders::FORWARD_PHONG) : EShaders::TBN;

		if (mpInput->IsKeyTriggered("F5")) mSelectedShader = IsLightingModelPBR() ? EShaders::FORWARD_BRDF : EShaders::FORWARD_PHONG;
	}

	//if (mpInput->IsKeyTriggered("F5")) mpRenderer->sEnableBlend = !mpRenderer->sEnableBlend;
	if (mpInput->IsKeyTriggered("F6")) ToggleLightingModel();
	if (mpInput->IsKeyTriggered("F7")) mDebugRender = !mDebugRender;
	if (mpInput->IsKeyTriggered("F8")) ToggleRenderingPath();

	if (mpInput->IsKeyTriggered("F9")) mPostProcessPass._bloomPass.ToggleBloomPass();
	if (mpInput->IsKeyTriggered(";")) ToggleAmbientOcclusion();

	if (mpInput->IsKeyTriggered("\\")) mpRenderer->ReloadShaders();
	//if (m_input->IsKeyTriggered(";")) m_bUsePaniniProjection = !m_bUsePaniniProjection;

	if (mbIsAmbientOcclusionOn)
	{
		if (mpInput->IsKeyDown("Shift"))
		{
			const float step = 0.1f;
			if (mpInput->IsScrollUp()) { mSSAOPass.intensity += step; Log::Info("SSAO Intensity: %.2f", mSSAOPass.intensity); }
			if (mpInput->IsScrollDown()) { mSSAOPass.intensity -= step; if (mSSAOPass.intensity < 1.001) mSSAOPass.intensity = 1.0f; Log::Info("SSAO Intensity: %.2f", mSSAOPass.intensity); }
		}
		else
		{
			const float step = 0.5f;
			if (mpInput->IsScrollUp()) { mSSAOPass.radius += step; Log::Info("SSAO Radius: %.2f", mSSAOPass.radius); }
			if (mpInput->IsScrollDown()) { mSSAOPass.radius -= step; if (mSSAOPass.radius < 1.001) mSSAOPass.radius = 1.0f; Log::Info("SSAO Radius: %.2f", mSSAOPass.radius); }
		}
	}

	return true;
}

bool Engine::UpdateAndRender()
{
	const float dt = mpTimer->Tick();
	const bool bExitApp = !HandleInput();
	if (!mbIsPaused)
	{
		CalcFrameStats();

		mpSceneManager->Update(dt);

		PreRender();
		Render();
	}

	mpInput->Update();	// update previous state after frame;
	return bExitApp;
}

// prepares rendering context: gets data from scene and sets up data structures ready to be sent to GPU
void Engine::PreRender()
{
	// set scene view
	const Camera& viewCamera = mpSceneManager->GetMainCamera();
	const XMMATRIX view			= viewCamera.GetViewMatrix();
	const XMMATRIX viewInverse	= viewCamera.GetViewInverseMatrix();
	const XMMATRIX proj			= viewCamera.GetProjectionMatrix();
	mSceneView.viewProj = view * proj;
	mSceneView.view = view;
	mSceneView.viewToWorld = viewInverse;
	mSceneView.projection = proj;
	mSceneView.cameraPosition = viewCamera.GetPositionF();
	mSceneView.bIsPBRLightingUsed = IsLightingModelPBR();
	mSceneView.bIsDeferredRendering = mbUseDeferredRendering;

	// gather scene lights
	mSceneLightData.ResetCounts();
	std::array<size_t*        , Light::ELightType::LIGHT_TYPE_COUNT> lightCounts 
	{ // can't use size_t& as template instantiation won't allow size_t&*. using size_t* instead.
		&mSceneLightData.pointLightCount,
		&mSceneLightData.spotLightCount,
		&/*TODO: add directional lights*/mSceneLightData.spotLightCount,
	};
	std::array<LightDataArray*, Light::ELightType::LIGHT_TYPE_COUNT> lightData
	{	// can't use LightDataArray& as template instantiation won't allow LightDataArray&*. using LightDataArray* instead.
		&mSceneLightData.pointLights, 
		&mSceneLightData.spotLights, 
		&/*TODO: add directional lights*/mSceneLightData.spotLights, 
	};

	for (const Light& l : mLights)
	{
		const size_t lightIndex = (*lightCounts[l._type])++;
		(*lightData[l._type])[lightIndex] = l.ShaderSignature();
	}

	if (mSceneLightData.spotLightCount > 0)	// temp hack: assume 1 spot light casting shadows
	{
		mSceneLightData.shadowCasterData[0].shadowMap = mShadowMapPass._shadowMap;
		mSceneLightData.shadowCasterData[0].shadowSampler = mShadowMapPass._shadowSampler;
		mSceneLightData.shadowCasterData[0].lightSpaceMatrix = mLights[0].GetLightSpaceMatrix();
	}

	mTBNDrawObjects.clear();
	std::vector<const GameObject*> objects;
	mpSceneManager->mpActiveScene->GetSceneObjects(objects);
	for (const GameObject* obj : objects)
	{
		if (obj->mRenderSettings.bRenderTBN)
			mTBNDrawObjects.push_back(obj);
	}

	mActiveSkybox = mpSceneManager->GetSceneSkybox();
}

void Engine::RenderLights() const
{
	mpRenderer->BeginEvent("Render Lights Pass");
	mpRenderer->Reset();	// is reset necessary?
	mpRenderer->SetShader(EShaders::UNLIT);
	for (const Light& light : mLights)
	{
		mpRenderer->SetBufferObj(light._renderMesh);
		const XMMATRIX world = light._transform.WorldTransformationMatrix();
		const XMMATRIX worldViewProj = world  * mSceneView.viewProj;
		const vec3 color = light._color.Value();
		mpRenderer->SetConstant4x4f("worldViewProj", worldViewProj);
		mpRenderer->SetConstant3f("diffuse", color);
		mpRenderer->SetConstant1f("isDiffuseMap", 0.0f);
		mpRenderer->Apply();
		mpRenderer->DrawIndexed();
	}
	mpRenderer->EndEvent();
}

void Engine::SendLightData() const
{
	// SPOT & POINT LIGHTS
	//--------------------------------------------------------------
	mpRenderer->SetConstant1f("lightCount", static_cast<float>(mSceneLightData.pointLightCount));
	mpRenderer->SetConstant1f("spotCount", static_cast<float>(mSceneLightData.spotLightCount));
	mpRenderer->SetConstantStruct("lights", static_cast<const void*>(mSceneLightData.pointLights.data()));
	mpRenderer->SetConstantStruct("spots", static_cast<const void*>(mSceneLightData.spotLights.data()));

	// SHADOW MAPS
	//--------------------------------------------------------------
	// first light is spot: single shadow map support for now
	if (mSceneLightData.spotLightCount > 0)
	{
		const ShadowCasterData& caster = mSceneLightData.shadowCasterData[0];
		mpRenderer->SetConstant4x4f("lightSpaceMat", caster.lightSpaceMatrix);
		mpRenderer->SetTexture("texShadowMap", caster.shadowMap);
		mpRenderer->SetSamplerState("sShadowSampler", caster.shadowSampler);
	}

#ifdef _DEBUG
	if (mSceneLightData.pointLightCount > mSceneLightData.pointLights.size())	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (mSceneLightData.spotLightCount > mSceneLightData.spotLights.size())	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
}

void Engine::Render()
{	
	const XMMATRIX& viewProj = mSceneView.viewProj;
	
	// get shadow casters (todo: static/dynamic lights)
	std::vector<const Light*> _shadowCasters;
	for (const Light& light : mpSceneManager->mRoomScene.mLights)
	{
		if (light._castsShadow)
			_shadowCasters.push_back(&light);
	}


	// SHADOW MAPS
	//------------------------------------------------------------------------
	mpRenderer->BeginEvent("Shadow Pass");
	mpRenderer->UnbindRenderTargets();	// unbind the back render target | every pass has their own render targets
	mShadowMapPass.RenderShadowMaps(mpRenderer, _shadowCasters, mZPassObjects);
	mpRenderer->EndEvent();

	// LIGHTING PASS
	//------------------------------------------------------------------------
	mpRenderer->Reset();
	mpRenderer->SetRasterizerState(static_cast<int>(EDefaultRasterizerState::CULL_NONE));
	mpRenderer->SetViewport(mpRenderer->WindowWidth(), mpRenderer->WindowHeight());

	if (mbUseDeferredRendering)
	{	// DEFERRED
		const GBuffer& gBuffer = mDeferredRenderingPasses._GBuffer;
		const TextureID texNormal = mpRenderer->GetRenderTargetTexture(gBuffer._normalRT);
		const TextureID texDiffuseRoughness = mpRenderer->GetRenderTargetTexture(gBuffer._diffuseRoughnessRT);
		const TextureID texSpecularMetallic = mpRenderer->GetRenderTargetTexture(gBuffer._specularMetallicRT);
		const TextureID texPosition = mpRenderer->GetRenderTargetTexture(gBuffer._positionRT);
		const TextureID texDepthTexture = mpRenderer->m_state._depthBufferTexture;
		const TextureID tSSAO = mbIsAmbientOcclusionOn ? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget) : mSSAOPass.whiteTexture4x4;

		// GEOMETRY - DEPTH PASS
		mpRenderer->BeginEvent("Geometry Pass");
		mDeferredRenderingPasses.SetGeometryRenderingStates(mpRenderer);
		mpSceneManager->Render(mpRenderer, mSceneView);
		mpRenderer->EndEvent();

		// AMBIENT OCCLUSION  PASS
		if (mbIsAmbientOcclusionOn)
		{
			mpRenderer->BeginEvent("Ambient Occlusion Pass");
			mSSAOPass.RenderOcclusion(mpRenderer, texNormal, texPosition, mSceneView);
			//m_SSAOPass.BilateralBlurPass(m_pRenderer);	// todo
			mSSAOPass.GaussianBlurPass(mpRenderer);
			mpRenderer->EndEvent();
		}

		// DEFERRED LIGHTING PASS
		mpRenderer->BeginEvent("Lighting Pass");
		mDeferredRenderingPasses.RenderLightingPass(mpRenderer, mPostProcessPass._worldRenderTarget, mSceneView, mSceneLightData, tSSAO, sEngineSettings.rendering.bUseBRDFLighting);
		mpRenderer->EndEvent();

		// LIGHT SOURCES
		mpRenderer->BindDepthTarget(mWorldDepthTarget);
		RenderLights();

		// SKYBOX
		if (mActiveSkybox != ESkyboxPreset::SKYBOX_PRESET_COUNT)
		{
			// Note: this can be done without stencil read/write/masking. set depth test to equals1 
			mpRenderer->SetDepthStencilState(mDeferredRenderingPasses._skyboxStencilState);
			Skybox::s_Presets[mActiveSkybox].Render(mSceneView.viewProj);
			mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_WRITE);
			mpRenderer->UnbindDepthTarget();
		}
	}


	else
	{	// FORWARD
		const bool bZPrePass = mbIsAmbientOcclusionOn;
		const TextureID tSSAO = mbIsAmbientOcclusionOn ? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget) : mSSAOPass.whiteTexture4x4;

		// AMBIENT OCCLUSION - Z-PREPASS
		if (bZPrePass)
		{
			const RenderTargetID normals	= mDeferredRenderingPasses._GBuffer._normalRT;
			const RenderTargetID positions	= mDeferredRenderingPasses._GBuffer._positionRT;
			const TextureID texNormal		= mpRenderer->GetRenderTargetTexture(normals);
			const TextureID texPosition		= mpRenderer->GetRenderTargetTexture(positions);
			
			const bool bDoClearColor = true;
			const bool bDoClearDepth = true;
			const bool bDoClearStencil = false;
			ClearCommand clearCmd(
				bDoClearColor, bDoClearDepth, bDoClearStencil,
				{ 0, 0, 0, 0 }, 1, 0
			);


			mpRenderer->BeginEvent("Z-PrePass");
			mpRenderer->SetShader(EShaders::Z_PREPRASS);
			mpRenderer->BindDepthTarget(mWorldDepthTarget);
			mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_WRITE);
			mpRenderer->BindRenderTargets(normals, positions);
			mpRenderer->Begin(clearCmd);
			mpSceneManager->Render(mpRenderer, mSceneView);
			mpRenderer->EndEvent();

			mpRenderer->BeginEvent("Ambient Occlusion Pass");
			mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
			mpRenderer->UnbindRenderTargets();
			mpRenderer->Apply();
			mSSAOPass.RenderOcclusion(mpRenderer, texNormal, texPosition, mSceneView);
			//m_SSAOPass.BilateralBlurPass(m_pRenderer);	// todo
			mSSAOPass.GaussianBlurPass(mpRenderer);
			mpRenderer->EndEvent();
		}

		const bool bDoClearColor = true;
		const bool bDoClearDepth = !bZPrePass;
		const bool bDoClearStencil = true;
		ClearCommand clearCmd(
			bDoClearColor, bDoClearDepth, bDoClearStencil,
			{ 0, 0, 0, 0 }, 1, 0
		);

		mpRenderer->BindRenderTarget(mPostProcessPass._worldRenderTarget);
		mpRenderer->BindDepthTarget(mWorldDepthTarget);
		mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_WRITE);
		mpRenderer->Begin(clearCmd);

		// SKYBOX
		// if we're not rendering the skybox, call apply() to unbind
		// shadow light depth target so we can bind it in the lighting pass
		// otherwise, skybox render pass will take care of it
		if (mActiveSkybox != ESkyboxPreset::SKYBOX_PRESET_COUNT)	Skybox::s_Presets[mActiveSkybox].Render(mSceneView.viewProj);
		else
		{
			// todo: this might be costly, profile this
			mpRenderer->SetShader(mSelectedShader);	// set shader so apply won't complain 
			mpRenderer->Apply();					// apply to bind depth stencil
		}

		// LIGHTING
		mpRenderer->BeginEvent("Lighting Pass");
		mpRenderer->SetShader(mSelectedShader);
		if (mSelectedShader == EShaders::FORWARD_BRDF || mSelectedShader == EShaders::FORWARD_PHONG)
		{
			mpRenderer->SetTexture("texAmbientOcclusion", tSSAO);
			mpRenderer->SetConstant3f("cameraPos", mSceneView.cameraPosition);
			mpRenderer->SetConstant2f("screenDimensions", mpRenderer->GetWindowDimensionsAsFloat2());
			mpRenderer->SetSamplerState("sNormalSampler", mNormalSampler);
			SendLightData();
		}

		mpSceneManager->Render(mpRenderer, mSceneView);
		mpRenderer->EndEvent();

		RenderLights();
	}

	// Tangent-Bitangent-Normal drawing
	//------------------------------------------------------------------------
	const bool bIsShaderTBN = !mTBNDrawObjects.empty();
	if (bIsShaderTBN)
	{
		constexpr bool bSendMaterial = false;

		mpRenderer->BeginEvent("Draw TBN Vectors");
		if (mbUseDeferredRendering)
			mpRenderer->BindDepthTarget(mWorldDepthTarget);

		mpRenderer->SetShader(EShaders::TBN);

		for (const GameObject* obj : mTBNDrawObjects)
			obj->Render(mpRenderer, mSceneView, bSendMaterial);
		

		if (mbUseDeferredRendering)
			mpRenderer->UnbindDepthTarget();

		mpRenderer->SetShader(mSelectedShader);
		mpRenderer->EndEvent();
	}

#if 1
	// POST PROCESS PASS
	//------------------------------------------------------------------------
	mPostProcessPass.Render(mpRenderer, sEngineSettings.rendering.bUseBRDFLighting);


	// DEBUG PASS
	//------------------------------------------------------------------------
	if (mDebugRender)
	{
		const int screenWidth  = sEngineSettings.window.width;
		const int screenHeight = sEngineSettings.window.height;
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
		TextureID tShadowMap		 = mpRenderer->GetDepthTargetTexture(mShadowMapPass._shadowDepthTarget);
		TextureID tBlurredBloom		 = mpRenderer->GetRenderTargetTexture(mPostProcessPass._bloomPass._blurPingPong[0]);
		TextureID tDiffuseRoughness  = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer._diffuseRoughnessRT);
		//TextureID tSceneDepth		 = m_pRenderer->m_state._depthBufferTexture._id;
		TextureID tSceneDepth		 = mpRenderer->GetDepthTargetTexture(0);
		TextureID tNormals			 = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer._normalRT);
		TextureID tAO				 = mbIsAmbientOcclusionOn ? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget) : mSSAOPass.whiteTexture4x4;

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
			};
			for (size_t i = 1; i < c.size(); i++)	// offset textures accordingly (using previous' x-dimension)
				c[i].bottomLeftCornerScreenCoordinates.x() = c[i-1].bottomLeftCornerScreenCoordinates.x() + c[i - 1].dimensionsInPixels.x() + paddingPx;
			return c;
		}();

		mpRenderer->BeginEvent("Debug Pass");
		mpRenderer->SetShader(EShaders::DEBUG);
		for (const DrawQuadOnScreenCommand& cmd : quadCmds)
		{
			mpRenderer->DrawQuadOnScreen(cmd);
		}
		mpRenderer->EndEvent();
	}
#endif
	mpRenderer->End();
}

