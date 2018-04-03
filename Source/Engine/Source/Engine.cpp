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
#include "Application/Input.h"
#include "Utilities/Log.h"
#include "Utilities/PerfTimer.h"
#include "Utilities/CustomParser.h"
#include "SceneManager.h"
#include "Application/WorkerPool.h"
#include "Engine/Settings.h"

#include "Utilities/Profiler.h"

#include "Renderer/Renderer.h"
#include "Renderer/TextRenderer.h"
#include "Utilities/Camera.h"

#include <sstream>

Settings::Engine Engine::sEngineSettings;
Engine* Engine::sInstance = nullptr;

Engine::Engine()
	:
	mpRenderer(new Renderer()),
	mpTextRenderer(new TextRenderer()),
	mpInput(new Input()),
	mpTimer(new PerfTimer()),
	mpSceneManager(new SceneManager(mLights)),
	mProfiler(new Profiler()),
	mbUsePaniniProjection(false),
	mbShowControls(true)
	//,mObjectPool(1024)
{}

Engine::~Engine(){}

void Engine::Exit()
{
	mpTextRenderer->Exit();
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
	Log::Info("Toggle Lighting Model: %s Lighting", isBRDF ? "PBR - BRDF" : "Blinn-Phong");
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

	mWorkerPool.Initialize(workerCount);
	mpInput->Initialize();
	if (!mpRenderer->Initialize(hwnd, windowSettings))
	{
		Log::Error("Cannot initialize Renderer.\n");
		return false;
	}
	if (!mpTextRenderer->Initialize(mpRenderer))
	{
		Log::Error("Cannot initialize Text Renderer.\n");
		return false;
	}


	// INITIALIZE RENDERING
	//--------------------------------------------------------------
	// render passes
	mbUseDeferredRendering = rendererSettings.bUseDeferredRendering;
	mbIsAmbientOcclusionOn = rendererSettings.bAmbientOcclusion;
	mDisplayRenderTargets = true;
	mSelectedShader = mbUseDeferredRendering ? mDeferredRenderingPasses._geometryShader : EShaders::FORWARD_BRDF;
	mWorldDepthTarget = 0;	// assumes first index in renderer->m_depthTargets[]
	return true;
}

bool Engine::Load()
{
	const Settings::Rendering& rendererSettings = sEngineSettings.rendering;

	mProfiler->BeginProfile();
	mProfiler->BeginEntry("EngineLoad");
	Log::Info("-------------------- LOADING ENVIRONMENT MAPS --------------------- ");
	mpTimer->Start();
	Skybox::InitializePresets(mpRenderer, rendererSettings.bEnableEnvironmentLighting, rendererSettings.bPreLoadEnvironmentMaps);
	mpTimer->Stop();
	Log::Info("--------------------- ENVIRONMENT MAPS LOADED IN %.2fs.", mpTimer->DeltaTime());


	const bool bLoadSuccess = mpSceneManager->Load(mpRenderer, nullptr, sEngineSettings, mZPassObjects);
	if (!bLoadSuccess)
	{
		Log::Error("Engine couldn't load scene.");
		return false;
	}

	mpTimer->Reset();
	mpTimer->Start();
	{	// RENDER PASS INITIALIZATION
		Log::Info("---------------- INITIALIZING RENDER PASSES ---------------- ");
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
	mpTimer->Stop();
	Log::Info("---------------- INITIALIZING RENDER PASSES DONE IN %.2fs ---------------- ", mpTimer->DeltaTime());
	mProfiler->EndEntry();
	mProfiler->EndProfile();
	return true;
}


bool Engine::HandleInput()
{
	if (mpInput->IsKeyTriggered("Backspace"))	TogglePause();

	if (mpInput->IsKeyTriggered("F1")) ToggleLightingModel();
	if (mpInput->IsKeyTriggered("F2")) ToggleAmbientOcclusion();
	if (mpInput->IsKeyTriggered("F3")) mPostProcessPass._bloomPass.ToggleBloomPass();
	if (mpInput->IsKeyTriggered("F4")) mDisplayRenderTargets = !mDisplayRenderTargets;
	if (mpInput->IsKeyTriggered("F5")) ToggleRenderingPath();


	if (mpInput->IsKeyTriggered(";"))
	{
		if (mpInput->IsKeyDown("Shift"))	;
		else								mbShowProfiler = !mbShowProfiler; 
	}
	if (mpInput->IsKeyTriggered("'"))
	{
		if (mpInput->IsKeyDown("Shift"))	mbShowControls = !mbShowControls;
		else								mFrameStats.bShow = !mFrameStats.bShow;
	}


	if (mpInput->IsKeyTriggered("\\")) mpRenderer->ReloadShaders();

	// todo: wire this to some UI text/control
	if (mbIsAmbientOcclusionOn)
	{
		if (mpInput->IsKeyDown("Shift"))
		{
			const float step = 0.1f;
			if (mpInput->IsScrollUp()) { mSSAOPass.intensity += step; Log::Info("SSAO Intensity: %.2f", mSSAOPass.intensity); }
			if (mpInput->IsScrollDown()) { mSSAOPass.intensity -= step; if (mSSAOPass.intensity < 0.301) mSSAOPass.intensity = 1.0f; Log::Info("SSAO Intensity: %.2f", mSSAOPass.intensity); }
		}
		else
		{
			const float step = 0.5f;
			if (mpInput->IsScrollUp()) { mSSAOPass.radius += step; Log::Info("SSAO Radius: %.2f", mSSAOPass.radius); }
			if (mpInput->IsScrollDown()) { mSSAOPass.radius -= step; if (mSSAOPass.radius < 0.301) mSSAOPass.radius = 1.0f; Log::Info("SSAO Radius: %.2f", mSSAOPass.radius); }
		}
	}

	return true;
}


void Engine::CalcFrameStats(float dt)
{
	static unsigned long long frameCount = 0;
	constexpr size_t RefreshRate = 5;	// refresh every 5 frames
	constexpr size_t SampleCount = 50;	// over 50 dt samples

	static std::vector<float> dtSamples(SampleCount, 0.0f);

	dtSamples[frameCount % SampleCount] = dt;

	float dtSampleSum = 0.0f;
	std::for_each(dtSamples.begin(), dtSamples.end(), [&dtSampleSum](float dt) { dtSampleSum += dt; });
	const float frameTime = dtSampleSum / SampleCount;
	const int fps = static_cast<int>(1.0f / frameTime);


	if (frameCount % RefreshRate == 0)
	{
		mCurrentFrameTime = frameTime;

		std::ostringstream stats;
		stats.precision(2);
		stats << std::fixed;
		stats << "VQEngine Demo | " << "CPU Frame: " << frameTime * 1000.0f << " ms  FPS: ";
		stats.precision(4);
		stats << fps;
		SetWindowText(mpRenderer->GetWindow(), stats.str().c_str());
	}

	++frameCount;
}


bool Engine::UpdateAndRender()
{
	const float dt = mpTimer->Tick();
	mProfiler->BeginEntry("CPU");

	const bool bExitApp = !HandleInput();
	if (!mbIsPaused)
	{
		CalcFrameStats(dt);

		mProfiler->BeginEntry("Update()");
		mpSceneManager->Update(dt);
		mProfiler->EndEntry();

		PreRender();
		Render();
	}


	mProfiler->EndEntry();

	mProfiler->StateCheck();
	return bExitApp;
}

// can't use std::array<T&, 2>, hence std::array<T*, 2> 
// array of 2: light data for non-shadowing and shadowing lights
constexpr size_t NON_SHADOWING_LIGHT_INDEX	= 0;
constexpr size_t SHADOWING_LIGHT_INDEX		= 1;
using pPointLightDataArray		 = std::array<PointLightDataArray*, 2>;
using pSpotLightDataArray		 = std::array<SpotLightDataArray*, 2>;
using pDirectionalLightDataArray = std::array<DirectionalLightDataArray*, 2>;

// stores the number of lights per light type
using pNumArray = std::array<int*, Light::ELightType::LIGHT_TYPE_COUNT>;

void Engine::PreRender()
{
	mProfiler->BeginEntry("PreRender()");
	
	// set scene view
	const Camera& viewCamera = mpSceneManager->GetMainCamera();
	const Scene* scene = mpSceneManager->mpActiveScene;
	const XMMATRIX view = viewCamera.GetViewMatrix();
	const XMMATRIX viewInverse = viewCamera.GetViewInverseMatrix();
	const XMMATRIX proj = viewCamera.GetProjectionMatrix();
	XMVECTOR det = XMMatrixDeterminant(proj);
	const XMMATRIX projInv = XMMatrixInverse(&det, proj);

	mSceneView.viewProj = view * proj;
	mSceneView.view = view;
	mSceneView.viewInverse = viewInverse;
	mSceneView.projection = proj;
	mSceneView.projectionInverse = projInv;

	mSceneView.cameraPosition = viewCamera.GetPositionF();

	mSceneView.sceneRenderSettings = scene->GetSceneRenderSettings();
	mSceneView.bIsPBRLightingUsed = IsLightingModelPBR();
	mSceneView.bIsDeferredRendering = mbUseDeferredRendering;
	mSceneView.bIsIBLEnabled = scene->mSceneRenderSettings.bSkylightEnabled && mSceneView.bIsPBRLightingUsed;

	mSceneView.environmentMap = scene->GetEnvironmentMap();

	// gather scene lights
	mSceneLightData.ResetCounts();
	mShadowView.spots.clear();
	mShadowView.directionals.clear();
	mShadowView.points.clear();
	pNumArray lightCounts
	{ 
		&mSceneLightData._cb.pointLightCount,
		&mSceneLightData._cb.spotLightCount,
		&mSceneLightData._cb.directionalLightCount
	};
	pNumArray casterCounts
	{ 
		&mSceneLightData._cb.pointLightCount_shadow,
		&mSceneLightData._cb.spotLightCount_shadow,
		&mSceneLightData._cb.directionalLightCount_shadow
	};

	for (const Light& l : mLights)
	{
		// index in p*LightDataArray to differentiate between shadow casters and non-shadow casters
		const size_t shadowIndex = l._castsShadow ? SHADOWING_LIGHT_INDEX : NON_SHADOWING_LIGHT_INDEX;

		// add to the count of the current light type & whether its shadow casting or not
		pNumArray& refLightCounts = l._castsShadow ? casterCounts : lightCounts;
		const size_t lightIndex = (*refLightCounts[l._type])++;

		switch (l._type)
		{
		case Light::ELightType::POINT:
			mSceneLightData._cb.pointLights[lightIndex] = l.GetPointLightData();
			mSceneLightData._cb.pointLightsShadowing[lightIndex] = l.GetPointLightData();
			break;
		case Light::ELightType::SPOT:
			mSceneLightData._cb.spotLights[lightIndex] = l.GetSpotLightData();
			mSceneLightData._cb.spotLightsShadowing[lightIndex] = l.GetSpotLightData();
			break;
		case Light::ELightType::DIRECTIONAL:
			//mSceneLightData._cb.pointLights[lightIndex] = l.GetPointLightData();
			//mSceneLightData._cb.pointLightsShadowing[lightIndex] = l.GetPointLightData();
			break;
		default:
			Log::Error("Engine::PreRender(): UNKNOWN LIGHT TYPE");
			continue;
		}
	}

	unsigned numShd = 0;	// only for spot lights for now
	for (const Light& l : mLights)
	{
		// shadowing lights
		if (l._castsShadow)
		{
			switch (l._type)
			{
			case Light::ELightType::SPOT:
				mShadowView.spots.push_back(&l);
				mSceneLightData._cb.shadowViews[numShd++] = l.GetLightSpaceMatrix();
				break;
			case Light::ELightType::POINT:
				mShadowView.points.push_back(&l);
				break;
			case Light::ELightType::DIRECTIONAL:
				mShadowView.directionals.push_back(&l);
				break;
			}
		}

	}

	mTBNDrawObjects.clear();
	std::vector<const GameObject*> objects;
	mpSceneManager->mpActiveScene->GetSceneObjects(objects);
	for (const GameObject* obj : objects)
	{
		if (obj->mRenderSettings.bRenderTBN)
			mTBNDrawObjects.push_back(obj);
	}

	mProfiler->EndEntry();
}

void Engine::RenderLights() const
{
	mpRenderer->BeginEvent("Render Lights Pass");
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
	const float shadowDimension = static_cast<float>(mShadowMapPass._shadowMapDimension);
	// SPOT & POINT LIGHTS
	//--------------------------------------------------------------
	mpRenderer->SetConstantStruct("sceneLightData", &mSceneLightData._cb);
	mpRenderer->SetConstant2f("spotShadowMapDimensions", vec2(shadowDimension, shadowDimension));

	// SHADOW MAPS
	//--------------------------------------------------------------
	mpRenderer->SetTextureArray("texSpotShadowMaps", mShadowMapPass._spotShadowMaps);

#ifdef _DEBUG
	const SceneLightingData::cb& cb = mSceneLightData._cb;	// constant buffer shorthand
	if (cb.pointLightCount > cb.pointLights.size())	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (cb.spotLightCount  > cb.spotLights.size())	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
}

void Engine::Render()
{
	mProfiler->BeginEntry("Render()");
	mpRenderer->BeginFrame();

	const XMMATRIX& viewProj = mSceneView.viewProj;
	const Scene* pScene = mpSceneManager->mpActiveScene;

	// SHADOW MAPS
	//------------------------------------------------------------------------
	mProfiler->BeginEntry("Shadow Pass");
	mpRenderer->BeginEvent("Shadow Pass");
	
	mpRenderer->UnbindRenderTargets();	// unbind the back render target | every pass has their own render targets
	mShadowMapPass.RenderShadowMaps(mpRenderer, mZPassObjects, mShadowView);
	
	mpRenderer->EndEvent();
	mProfiler->EndEntry();

	// LIGHTING PASS
	//------------------------------------------------------------------------
	mpRenderer->ResetPipelineState();
	mpRenderer->SetRasterizerState(static_cast<int>(EDefaultRasterizerState::CULL_NONE));
	mpRenderer->SetViewport(mpRenderer->WindowWidth(), mpRenderer->WindowHeight());

	//==========================================================================
	// DEFERRED RENDERER
	//==========================================================================
	if (mbUseDeferredRendering)
	{
		const GBuffer& gBuffer = mDeferredRenderingPasses._GBuffer;
		const TextureID texNormal = mpRenderer->GetRenderTargetTexture(gBuffer._normalRT);
		const TextureID texDiffuseRoughness = mpRenderer->GetRenderTargetTexture(gBuffer._diffuseRoughnessRT);
		const TextureID texSpecularMetallic = mpRenderer->GetRenderTargetTexture(gBuffer._specularMetallicRT);
		const TextureID texDepthTexture = mpRenderer->mDefaultDepthBufferTexture;
		const TextureID tSSAO = mbIsAmbientOcclusionOn && mSceneView.sceneRenderSettings.bAmbientOcclusionEnabled
			? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget)
			: mSSAOPass.whiteTexture4x4;

		// GEOMETRY - DEPTH PASS
		mProfiler->BeginEntry("Geometry Pass");
		mpRenderer->BeginEvent("Geometry Pass");
		mDeferredRenderingPasses.SetGeometryRenderingStates(mpRenderer);
		mFrameStats.numSceneObjects = mpSceneManager->Render(mpRenderer, mSceneView);
		mpRenderer->EndEvent();	mProfiler->EndEntry();

		// AMBIENT OCCLUSION  PASS
		mProfiler->BeginEntry("Ambient Occlusion Pass");
		if (mbIsAmbientOcclusionOn && mSceneView.sceneRenderSettings.bAmbientOcclusionEnabled)
		{
			// TODO: if BeginEntry() is inside, it's never reset to 0 if ambiend occl is turned off
			mpRenderer->BeginEvent("Ambient Occlusion Pass");
			mSSAOPass.RenderOcclusion(mpRenderer, texNormal, mSceneView);
			//m_SSAOPass.BilateralBlurPass(m_pRenderer);	// todo
			mSSAOPass.GaussianBlurPass(mpRenderer);
			mpRenderer->EndEvent();	
		}
		mProfiler->EndEntry();

		// DEFERRED LIGHTING PASS
		mProfiler->BeginEntry("Lighting Pass");
		mpRenderer->BeginEvent("Lighting Pass");
		mDeferredRenderingPasses.RenderLightingPass(mpRenderer, mPostProcessPass._worldRenderTarget, mSceneView, mSceneLightData, tSSAO, sEngineSettings.rendering.bUseBRDFLighting);
		mpRenderer->EndEvent(); mProfiler->EndEntry();

		mProfiler->BeginEntry("Skybox & Lights");
		// LIGHT SOURCES
		mpRenderer->BindDepthTarget(mWorldDepthTarget);
		
		// SKYBOX
		mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_TEST_ONLY);
		if (pScene->HasSkybox())
		{
			pScene->RenderSkybox(mSceneView.viewProj);
		}

		RenderLights();
		mProfiler->EndEntry();
	}

	//==========================================================================
	// FORWARD RENDERER
	//==========================================================================
	else
	{
		const bool bZPrePass = mbIsAmbientOcclusionOn && mSceneView.sceneRenderSettings.bAmbientOcclusionEnabled;
		const TextureID tSSAO = bZPrePass ? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget) : mSSAOPass.whiteTexture4x4;
		const TextureID texIrradianceMap = mSceneView.environmentMap.irradianceMap;
		const SamplerID smpEnvMap = mSceneView.environmentMap.envMapSampler < 0 ? EDefaultSamplerState::POINT_SAMPLER : mSceneView.environmentMap.envMapSampler;
		const TextureID prefilteredEnvMap = mSceneView.environmentMap.prefilteredEnvironmentMap;
		const TextureID tBRDFLUT = mpRenderer->GetRenderTargetTexture(EnvironmentMap::sBRDFIntegrationLUTRT);

		// AMBIENT OCCLUSION - Z-PREPASS
		if (bZPrePass)
		{
			const RenderTargetID normals	= mDeferredRenderingPasses._GBuffer._normalRT;
			const TextureID texNormal		= mpRenderer->GetRenderTargetTexture(normals);
			
			const bool bDoClearColor = true;
			const bool bDoClearDepth = true;
			const bool bDoClearStencil = false;
			ClearCommand clearCmd(
				bDoClearColor, bDoClearDepth, bDoClearStencil,
				{ 0, 0, 0, 0 }, 1, 0
			);


			mpRenderer->BeginEvent("Z-PrePass");
			mpRenderer->SetShader(EShaders::Z_PREPRASS);
			mpRenderer->SetSamplerState("sNormalSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);
			mpRenderer->BindRenderTarget(normals);
			mpRenderer->BindDepthTarget(mWorldDepthTarget);
			mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_WRITE);
			mpRenderer->BeginRender(clearCmd);
			mpSceneManager->Render(mpRenderer, mSceneView);
			mpRenderer->EndEvent();

			mpRenderer->BeginEvent("Ambient Occlusion Pass");
			mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
			mpRenderer->UnbindRenderTargets();
			mpRenderer->Apply();
			mSSAOPass.RenderOcclusion(mpRenderer, texNormal, mSceneView);
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
		mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_WRITE);
		mpRenderer->BeginRender(clearCmd);

		// SKYBOX
		// if we're not rendering the skybox, call apply() to unbind
		// shadow light depth target so we can bind it in the lighting pass
		// otherwise, skybox render pass will take care of it
		if (pScene->HasSkybox())	pScene->RenderSkybox(mSceneView.viewProj);
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

			// todo: shader defines -> have a PBR shader with and without environment lighting through preprocessor
			const bool bSkylight = mSceneView.bIsIBLEnabled && texIrradianceMap != -1;
			if (bSkylight)
			{
				mpRenderer->SetTexture("tIrradianceMap", texIrradianceMap);
				mpRenderer->SetTexture("tPreFilteredEnvironmentMap", prefilteredEnvMap);
				mpRenderer->SetTexture("tBRDFIntegrationLUT", tBRDFLUT);
				mpRenderer->SetSamplerState("sEnvMapSampler", smpEnvMap);
			}
			
			if (mSelectedShader == EShaders::FORWARD_BRDF)
			{
				mpRenderer->SetConstant1f("isEnvironmentLightingOn", bSkylight ? 1.0f : 0.0f);
				mpRenderer->SetSamplerState("sWrapSampler", EDefaultSamplerState::WRAP_SAMPLER);
				mpRenderer->SetSamplerState("sNearestSampler", EDefaultSamplerState::POINT_SAMPLER);
			}
			else
				mpRenderer->SetSamplerState("sNormalSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);
			// todo: shader defines -> have a PBR shader with and without environment lighting through preprocessor

			mpRenderer->SetConstant1f("ambientFactor", mSceneView.sceneRenderSettings.ambientFactor);
			mpRenderer->SetConstant3f("cameraPos", mSceneView.cameraPosition);
			mpRenderer->SetConstant2f("screenDimensions", mpRenderer->GetWindowDimensionsAsFloat2());
			mpRenderer->SetSamplerState("sLinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);

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
	mProfiler->BeginEntry("Post Process");
	mPostProcessPass.Render(mpRenderer, sEngineSettings.rendering.bUseBRDFLighting);
	mProfiler->EndEntry();

	// DEBUG PASS
	//------------------------------------------------------------------------
	if (mDisplayRenderTargets)
	{
		mProfiler->BeginEntry("Debug Textures");
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
		const TextureID white4x4	 = mSSAOPass.whiteTexture4x4;
		//TextureID tShadowMap		 = mpRenderer->GetDepthTargetTexture(mShadowMapPass._spotShadowDepthTargets);
		TextureID tBlurredBloom		 = mpRenderer->GetRenderTargetTexture(mPostProcessPass._bloomPass._blurPingPong[0]);
		TextureID tDiffuseRoughness  = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer._diffuseRoughnessRT);
		//TextureID tSceneDepth		 = m_pRenderer->m_state._depthBufferTexture._id;
		TextureID tSceneDepth		 = mpRenderer->GetDepthTargetTexture(0);
		TextureID tNormals			 = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer._normalRT);
		TextureID tAO				 = mbIsAmbientOcclusionOn ? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget) : mSSAOPass.whiteTexture4x4;
		TextureID tBRDF				 = mpRenderer->GetRenderTargetTexture(EnvironmentMap::sBRDFIntegrationLUTRT);
		TextureID preFilteredEnvMap  = pScene->GetEnvironmentMap().prefilteredEnvironmentMap;
		preFilteredEnvMap = preFilteredEnvMap < 0 ? white4x4 : preFilteredEnvMap; 

		const std::vector<DrawQuadOnScreenCommand> quadCmds = [&]() {
			const vec2 screenPosition(0.0f, (float)bottomPaddingPx);
			std::vector<DrawQuadOnScreenCommand> c
			{	//		Pixel Dimensions	 Screen Position (offset below)	  Texture			DepthTexture?
				{ fullscreenTextureScaledDownSize,	screenPosition,			tSceneDepth			, true},
				{ fullscreenTextureScaledDownSize,	screenPosition,			tDiffuseRoughness	, false},
				{ fullscreenTextureScaledDownSize,	screenPosition,			tNormals			, false},
				//{ squareTextureScaledDownSize    ,	screenPosition,			tShadowMap			, true},
				{ fullscreenTextureScaledDownSize,	screenPosition,			tBlurredBloom		, false},
				{ fullscreenTextureScaledDownSize,	screenPosition,			tAO					, false},
				{ squareTextureScaledDownSize,		screenPosition,			tBRDF				, false },
			};
			for (size_t i = 1; i < c.size(); i++)	// offset textures accordingly (using previous' x-dimension)
				c[i].bottomLeftCornerScreenCoordinates.x() = c[i-1].bottomLeftCornerScreenCoordinates.x() + c[i - 1].dimensionsInPixels.x() + paddingPx;
			return c;
		}();


		mpRenderer->BeginEvent("Debug Textures");
		mpRenderer->SetShader(EShaders::DEBUG);
		for (const DrawQuadOnScreenCommand& cmd : quadCmds)
		{
			mpRenderer->DrawQuadOnScreen(cmd);
		}
		mpRenderer->EndEvent();
		mProfiler->EndEntry();
	}

	// UI TEXT
	mProfiler->BeginEntry("UI");
	const int fps = static_cast<int>(1.0f / mCurrentFrameTime);

	std::ostringstream stats;
	std::string entry;

	TextDrawDescription drawDesc;
	drawDesc.color = LinearColor::green;
	drawDesc.scale = 0.45f;

	// Performance stats
	if (mbShowProfiler)
	{
		const bool bSortStats = true;
		const float X_POS = sEngineSettings.window.width * 0.81f;
		const float Y_POS = 30.0f;
		mProfiler->RenderPerformanceStats(mpTextRenderer, vec2(X_POS, Y_POS), drawDesc, bSortStats);
	}
	if (mFrameStats.bShow)
	{
		const float X_POS = sEngineSettings.window.width * 0.81f;
		const float Y_POS = 600.0f;
		mFrameStats.rstats = mpRenderer->GetRenderStats();
		mFrameStats.Render(mpTextRenderer, vec2(X_POS, Y_POS), drawDesc);
	}
	if (mbShowControls)
	{
		TextDrawDescription _drawDesc(drawDesc);
		_drawDesc.color = vec3(1, 1, 0.1f) * 0.65f;
		_drawDesc.scale = 0.40f;
		int numLine = FrameStats::numStat + 1;

		const float X_POS = sEngineSettings.window.width * 0.81f;
		const float Y_POS = 0.75f * sEngineSettings.window.height;
		const float LINE_HEIGHT = 25.0f;
		vec2 screenPosition(X_POS, Y_POS);
		
		_drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		_drawDesc.text = std::string("F1 - Lighting Model: ") + (!sEngineSettings.rendering.bUseBRDFLighting ? "Blinn-Phong" : "PBR - BRDF");
		mpTextRenderer->RenderText(_drawDesc);

		_drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		_drawDesc.text = std::string("F2 - SSAO: ") + (mbIsAmbientOcclusionOn ? "On" : "Off");
		mpTextRenderer->RenderText(_drawDesc);

		_drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		_drawDesc.text = std::string("F3 - Bloom: ") + (mPostProcessPass._bloomPass._isEnabled ? "On" : "Off");
		mpTextRenderer->RenderText(_drawDesc);

		_drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		_drawDesc.text = std::string("F4 - Display Render Targets: ") + (mDisplayRenderTargets ? "On" : "Off");
		mpTextRenderer->RenderText(_drawDesc);

		_drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		_drawDesc.text = std::string("F5 - Render Mode: ") + (!mbUseDeferredRendering ? "Forward" : "Deferred");
		mpTextRenderer->RenderText(_drawDesc);
	}

	mProfiler->EndEntry();

#endif
	mProfiler->BeginEntry("Present");
	mpRenderer->EndFrame();
	mProfiler->EndEntry();
	mProfiler->EndEntry();
}

const char* FrameStats::statNames[FrameStats::numStat] =
{
	"Objects: ",
	"Culled Objects: ",
	"Vertices: ",
	"Indices: ",
	"Draw Calls: ",
};
void FrameStats::Render(TextRenderer* pTextRenderer, const vec2& screenPosition, const TextDrawDescription& drawDesc)
{
	TextDrawDescription _drawDesc(drawDesc);
	const float LINE_HEIGHT = 25.0f;
	constexpr size_t RENDER_ORDER[] = { 4, 2, 3, 0, 1 };

	pTextRenderer->RenderText(_drawDesc);
	for (size_t i = 0; i < FrameStats::numStat; ++i)
	{
		_drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + i * LINE_HEIGHT);
		_drawDesc.text = FrameStats::statNames[RENDER_ORDER[i]] + std::to_string(FrameStats::stats[RENDER_ORDER[i]]);
		pTextRenderer->RenderText(_drawDesc);
	}
}
