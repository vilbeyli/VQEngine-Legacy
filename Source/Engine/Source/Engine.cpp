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

// skeleton code for transparency, detailed implementation will be done later on.
// disable it for now.
#define ENABLE_TRANSPARENCY 0


#define OVERRIDE_LEVEL_LOAD      1	// Toggle for overriding level loading
// 0: objects 
// 1: ssao test 
// 2: ibl test
// 3: stress test
// 4: sponza
// 5: lights
// 6: lod test
#define OVERRIDE_LEVEL_VALUE     6	// which level to load


#define FULLSCREEN_DEBUG_TEXTURE 1

// ASYNC / THREADED LOADING SWITCHES
// -------------------------------------------------------
// Uses a thread pool of worker threads to load scene models
// while utilizing a render thread to render loading screen
//
#define LOAD_ASYNC 1
// -------------------------------------------------------
#include "Engine.h"

#include "Application/Application.h"
#include "Application/ThreadPool.h"

#include "Utilities/Log.h"
#include "Utilities/CustomParser.h"

#include "Renderer/Renderer.h"
#include "Renderer/Shader.h"

#include "Scenes/ObjectsScene.h"
#include "Scenes/SSAOTestScene.h"
#include "Scenes/PBRScene.h"
#include "Scenes/StressTestScene.h"
#include "Scenes/SponzaScene.h"
#include "Scenes/LightsScene.h"
#include "Scenes/LODTestScene.h"

using namespace VQEngine;


// ====================================================================================
// HELPERS
// ====================================================================================
Settings::Engine Engine::sEngineSettings;
Engine* Engine::sInstance = nullptr;
Engine * Engine::GetEngine()
{
	if (sInstance == nullptr){sInstance = new Engine();}
	return sInstance;
}
void Engine::ToggleRenderingPath()
{
	mEngineConfig.bDeferredOrForward = !mEngineConfig.bDeferredOrForward;

	// initialize GBuffer if its not initialized, i.e., 
	// Renderer started in forward mode and we're toggling deferred for the first time
	if (!mDeferredRenderingPasses._GBuffer.bInitialized && mEngineConfig.bDeferredOrForward)
	{
		mDeferredRenderingPasses.InitializeGBuffer(mpRenderer);
	}
	Log::Info("Toggle Rendering Path: %s Rendering enabled", mEngineConfig.bDeferredOrForward ? "Deferred" : "Forward");

	// if we just turned deferred rendering off, clear the GBuffer textures
	if (!mEngineConfig.bDeferredOrForward)
	{
		mDeferredRenderingPasses.ClearGBuffer(mpRenderer);
		mSelectedShader = sEngineSettings.rendering.bUseBRDFLighting ? EShaders::FORWARD_BRDF : EShaders::FORWARD_PHONG;
	}
}
void Engine::ToggleAmbientOcclusion()
{
	mEngineConfig.bSSAO = !mEngineConfig.bSSAO;
	if (!mEngineConfig.bSSAO)
	{
		mDeferredRenderingPasses.ClearGBuffer(mpRenderer);
	}
	Log::Info("Toggle Ambient Occlusion: %s", mEngineConfig.bSSAO ? "On" : "Off");
}

void Engine::ToggleBloom()
{
	mEngineConfig.bBloom = !mEngineConfig.bBloom;
	Log::Info("Toggle Bloom: %s", mEngineConfig.bBloom ? "On" : "Off");
}

int Engine::GetFPS() const
{
	const float frametimeCPU = mpCPUProfiler->GetRootEntryAvg();
	const float frameTimeGPU = mpGPUProfiler->GetEntryAvg("GPU");
	return static_cast<int>(1.0f / std::max(frameTimeGPU, frametimeCPU));
}

float Engine::GetTotalTime() const { return mpTimer->TotalTime(); }



// ====================================================================================
// INTERFACE
// ====================================================================================
Engine::Engine()
	: mpRenderer(new Renderer())
	, mpTextRenderer(new TextRenderer())
	, mpInput(new Input())
	, mpTimer(new PerfTimer()) 
	, mpCPUProfiler(new CPUProfiler())
	, mpGPUProfiler(new GPUProfiler())
	, mbUsePaniniProjection(false)
	, mFrameCount(0)
	, mAccumulator(0.0f)
	, mUI(mEngineConfig)
	, mShadowMapPass(mpCPUProfiler, mpGPUProfiler)
	, mDeferredRenderingPasses(mpCPUProfiler, mpGPUProfiler)
	, mAOPass(mpCPUProfiler, mpGPUProfiler)
	, mPostProcessPass(mpCPUProfiler, mpGPUProfiler)
	, mDebugPass(mpCPUProfiler, mpGPUProfiler)
	, mZPrePass(mpCPUProfiler, mpGPUProfiler)
	, mForwardLightingPass(mpCPUProfiler, mpGPUProfiler)
	, mAAResolvePass(mpCPUProfiler, mpGPUProfiler)
	, mbMouseCaptured(false)
	, mbIsPaused(false)
	, mbStartEngineShutdown(false)
	, mpApp(nullptr)
{}


Engine::~Engine(){}


bool Engine::Initialize(Application* pApplication)
{
	mbLoading = true;
	this->mpApp = pApplication;
	this->mpApp->CaptureMouse(true);
	this->mbMouseCaptured = true; 

	Log::Info("[ENGINE]: Initializing --------------------");
	if (!mpRenderer || !mpInput || !mpTimer)
	{
		Log::Error("Nullptr Engine::Init()\n");
		return false;
	}

	mpTimer->Start();


	// INITIALIZE SYSTEMS
	//--------------------------------------------------------------
	const bool bEnableLogging = true;	// todo: read from settings
	const Settings::Window& windowSettings = sEngineSettings.window;

	mpInput->Initialize();
	if (!mpRenderer->Initialize(mpApp->m_hwnd, windowSettings, sEngineSettings.rendering))
	{
		Log::Error("Cannot initialize Renderer.\n");
		return false;
	}

#if USE_DX12
	StartThreads();


	// LOADING
	//
	/// TODO
	Scene::InitializeBuiltinMeshes();

#else
	StartRenderThread();
	Scene::InitializeBuiltinMeshes();
	LoadShaders();
	
	if (!mpTextRenderer->Initialize(mpRenderer))
	{
		Log::Error("Cannot initialize Text Renderer.\n");
		//return false;
	}

	mUI.Initialize(mpRenderer, mpTextRenderer, UI::ProfilerStack{mpCPUProfiler, mpGPUProfiler});
	mpGPUProfiler->Init(mpRenderer->m_deviceContext, mpRenderer->m_device);
#endif
	// INITIALIZE RENDER PASSES & SCENES
	//--------------------------------------------------------------
	mEngineConfig.bDeferredOrForward = sEngineSettings.rendering.bUseDeferredRendering;
	mEngineConfig.bSSAO = sEngineSettings.rendering.bAmbientOcclusion;
	mEngineConfig.bBloom = true;	// currently not deserialized
	mEngineConfig.bRenderTargets = false;
	mEngineConfig.bBoundingBoxes = false;
	mSelectedShader = mEngineConfig.bDeferredOrForward ? mDeferredRenderingPasses._geometryShader : EShaders::FORWARD_BRDF;
	mWorldDepthTarget = 0;	// assumes first index in renderer->m_depthTargets[]

	// this is bad... every scene needs to be pushed back.
	Scene::BaseSceneParams baseSceneParams;
	baseSceneParams.pRenderer = mpRenderer;
	baseSceneParams.pCPUProfiler = mpCPUProfiler;
	baseSceneParams.pTextRenderer = mpTextRenderer;
	mpScenes.push_back(new ObjectsScene    (baseSceneParams));
	mpScenes.push_back(new SSAOTestScene   (baseSceneParams));
	mpScenes.push_back(new PBRScene        (baseSceneParams));
	mpScenes.push_back(new StressTestScene (baseSceneParams));
	mpScenes.push_back(new SponzaScene     (baseSceneParams));
	mpScenes.push_back(new LightsScene     (baseSceneParams));
	mpScenes.push_back(new LODTestScene    (baseSceneParams));

	mpTimer->Stop();
	Log::Info("[ENGINE]: Initialized in %.2fs --------------", mpTimer->DeltaTime());
	return true;
}


bool Engine::Load()
{
#if 0//USE_DX12
	// TODO: proper threaaded laoding
	///mEngineLoadingTaskQueue.queue.push()
	return true;
#else

	Log::Info("[ENGINE]: Loading -------------------------");
	mpCPUProfiler->BeginProfile();

	mpThreadPool = new ThreadPool(4);
	mpThreadPool->StartThreads();

	// prepare loading screen resources
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/0.png"));
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/1.png"));
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/2.png"));
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/3.png"));
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/4.png"));
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/5.png"));
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/6.png"));
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/7.png"));
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen/8.png"));
	mActiveLoadingScreen = mLoadingScreenTextures[MathUtil::RandU(0, mLoadingScreenTextures.size())];

#if LOAD_ASYNC

	// quickly render one frame of loading screen
	mbLoading = true;
	mAccumulator = 1.0f;	// start rendering loading screen on update loop
	RenderLoadingScreen(true);

	// set up a parallel task to load everything.
	auto AsyncEngineLoad = [&]() -> bool
	{
		PerfTimer timer;
		timer.Start();
		// LOAD ENVIRONMENT MAPS
		//
		Log::Info("\tLOADING ENVIRONMENT MAPS ===");
#if 1
		Skybox::InitializePresets_Async(mpRenderer, sEngineSettings.rendering);
#else
		{
			std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
			Skybox::InitializePresets(mpRenderer, sEngineSettings.rendering);
		}
#endif


		// SCENE INITIALIZATION
		//
		Log::Info("\tLOADING SCENE ===");
		//mpTimer->Start();
		const size_t numScenes = sEngineSettings.sceneNames.size();
		assert(sEngineSettings.levelToLoad < numScenes);
#if /*defined(_DEBUG) &&*/ (OVERRIDE_LEVEL_LOAD > 0)
		sEngineSettings.levelToLoad = OVERRIDE_LEVEL_VALUE;
#endif
		{
			std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
			mShadowMapPass.Initialize(mpRenderer, sEngineSettings.rendering.shadowMap);
		}
		
		if (!LoadSceneFromFile())
		{
			Log::Error("Engine couldn't load scene.");
			return false;
		}
		
		//mpTimer->Stop();
		//Log::Info("-------------------- SCENE LOADED IN %.2fs. --------------------", mpTimer->DeltaTime());
		//mpTimer->Reset();

		// RENDER PASS INITIALIZATION
		//
		const bool bAAResolve = sEngineSettings.rendering.antiAliasing.IsAAEnabled();
		Log::Info("\tINITIALIZE RENDER PASSES ===");
		//mpTimer->Start();
		{	// #AsyncLoad: Mutex DEVICE
			//Log::Info("---------------- INITIALIZING RENDER PASSES ---------------- ");
			//renderer->m_Direct3D->ReportLiveObjects();
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mPostProcessPass.Initialize(mpRenderer, sEngineSettings.rendering.postProcess);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mDeferredRenderingPasses.Initialize(mpRenderer, bAAResolve);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mDebugPass.Initialize(mpRenderer);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mAOPass.Initialize(mpRenderer);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mZPrePass.Initialize(mpRenderer);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mForwardLightingPass.Initialize(mpRenderer);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
#if USE_DX12
				// TODO-DX12: mAAResolvePass.Initialize(mpRenderer, mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._shadeTarget));
#else
				mAAResolvePass.Initialize(mpRenderer, mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._shadeTarget));
#endif
			}
		}
		//mpTimer->Stop();
		//Log::Info("---------------- INITIALIZING RENDER PASSES DONE IN %.2fs ---------------- ", mpTimer->DeltaTime());
		//mpCPUProfiler->EndEntry();
		//mpCPUProfiler->EndProfile();

		mbLoading = false;
		const float loadTime = timer.StopGetDeltaTimeAndReset();
		Log::Info("[ENGINE]: Loaded (Async)  in %.2fs ---------", loadTime);
		return true;
	};

	mpThreadPool->AddTask(AsyncEngineLoad);
#else

	RenderLoadingScreen(true); // quickly render one frame of loading screen

	// LOAD ENVIRONMENT MAPS
	//
	mpTimer->Start();
	mpCPUProfiler->BeginEntry("EngineLoad");
	Log::Info("-------------------- LOADING ENVIRONMENT MAPS --------------------- ");
	Skybox::InitializePresets(mpRenderer, sEngineSettings.rendering);
	Log::Info("-------------------- ENVIRONMENT MAPS LOADED IN %.2fs. --------------------", mpTimer->StopGetDeltaTimeAndReset());

	// SCENE INITIALIZATION
	//
	Log::Info("-------------------- LOADING SCENE --------------------- ");
	mpTimer->Start();
	const size_t numScenes = sEngineSettings.sceneNames.size();
	assert(sEngineSettings.levelToLoad < numScenes);
#if /*defined(_DEBUG) &&*/ (OVERRIDE_LEVEL_LOAD > 0)
	sEngineSettings.levelToLoad = OVERRIDE_LEVEL_VALUE;
#endif
	if (!LoadSceneFromFile())
	{
		Log::Error("Engine couldn't load scene.");
		return false;
	}
	Log::Info("-------------------- SCENE LOADED IN %.2fs. --------------------", mpTimer->StopGetDeltaTimeAndReset());

	// RENDER PASS INITIALIZATION
	//
	mpTimer->Start();
	{	
		Log::Info("---------------- INITIALIZING RENDER PASSES ---------------- ");
		mShadowMapPass.Initialize(mpRenderer, sEngineSettings.rendering.shadowMap);
		//renderer->m_Direct3D->ReportLiveObjects();
		
		mDeferredRenderingPasses.Initialize(mpRenderer);
		mPostProcessPass.Initialize(mpRenderer, sEngineSettings.rendering.postProcess);
		mDebugPass.Initialize(mpRenderer);
		mAOPass.Initialize(mpRenderer);
		mAAResolvePass.Initialize(mpRenderer, mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._shadeTarget));
	}
	Log::Info("---------------- INITIALIZING RENDER PASSES DONE IN %.2fs ---------------- ", mpTimer->StopGetDeltaTimeAndReset());
	mpCPUProfiler->EndEntry();
	mpCPUProfiler->EndProfile();
	mpCPUProfiler->BeginProfile();
	Log::Info("[ENGINE]: Loaded (Async) ------------------");
#endif

	ENGINE->mpTimer->Reset();
	ENGINE->mpTimer->Start();
	return true;

#endif // USE_DX12
}



bool Engine::LoadSceneFromFile()
{
	mCurrentLevel = sEngineSettings.levelToLoad;
	SerializedScene mSerializedScene;
	{
		std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
		mSerializedScene = Parser::ReadScene(mpRenderer, sEngineSettings.sceneNames[mCurrentLevel]);
	}
	if (mSerializedScene.loadSuccess == '0')
	{
		Log::Error("Scene[%d] did not load.", mCurrentLevel);
		return false;
	}

	assert(mCurrentLevel < mpScenes.size());
	mpActiveScene = mpScenes[mCurrentLevel];
	mpActiveScene->mpThreadPool = mpThreadPool;

	mpActiveScene->LoadScene(mSerializedScene, sEngineSettings.window);

	// update engine settings and post process settings
	// todo: multiple data - inconsistent state -> sort out ownership
	sEngineSettings.rendering.postProcess.bloom = mSerializedScene.settings.bloom;
	mPostProcessPass._settings = sEngineSettings.rendering.postProcess;
	//sEngineSettings.rendering.shadowMap.directionalShadowMapDimensions = mpActiveScene->mDirectionalLight.GetSettings().directionalShadowMapDimensions;
	if (mpActiveScene->mDirectionalLight.mbEnabled)
	{
		// #AsyncLoad: Mutex DEVICE
		std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
		mShadowMapPass.InitializeDirectionalLightShadowMap(sEngineSettings.rendering.shadowMap);
	}
	return true;
}

bool Engine::LoadScene(int level)
{
#if LOAD_ASYNC
	mActiveLoadingScreen = mLoadingScreenTextures[MathUtil::RandU(0, mLoadingScreenTextures.size())];
	mbLoading = true;
	Log::Info("LoadScene: %d", level);
	auto loadFn = [&, level]()
	{
		{
			std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
#if USE_DX12
			// TODO-DX12: Draw Render Screen;
#else
			mpRenderer->UnbindDepthTarget();
			mpRenderer->UnbindRenderTargets();
			mpRenderer->Apply();
#endif
			mpActiveScene->UnloadScene();
		}
		Engine::sEngineSettings.levelToLoad = level;
		bool bLoadSuccess = LoadSceneFromFile();

		sEngineSettings.rendering.postProcess.bloom = mpActiveScene->mSceneRenderSettings.bloom;
		mPostProcessPass.UpdateSettings(sEngineSettings.rendering.postProcess, mpRenderer);

		mbLoading = false;
		return bLoadSuccess;
	};
	mpThreadPool->AddTask(loadFn);
	return true;
#else
	mpActiveScene->UnloadScene();
	Engine::sEngineSettings.levelToLoad = level;
	return LoadSceneFromFile();
#endif
}

bool Engine::LoadShaders()
{
	PerfTimer timer;
	timer.Start();

	constexpr unsigned VS_PS = SHADER_STAGE_VS | SHADER_STAGE_PS;
	const std::vector<ShaderDesc> shaderDescs =
	{
		ShaderDesc{ "PhongLighting<forward>",{
			ShaderStageDesc{ "Forward_Phong_vs.hlsl", {} },
			ShaderStageDesc{ "Forward_Phong_ps.hlsl", {} }
		}},
		ShaderDesc{ "UnlitTextureColor" , ShaderDesc::CreateStageDescsFromShaderName("UnlitTextureColor", VS_PS) },
		ShaderDesc{ "TextureCoordinates", {
			ShaderStageDesc{"MVPTransformationWithUVs_vs.hlsl", {} },
			ShaderStageDesc{"TextureCoordinates_ps.hlsl"      , {} }
		}},
		ShaderDesc{ "Normal"            , ShaderDesc::CreateStageDescsFromShaderName("Normal"  , VS_PS) },
		ShaderDesc{ "Tangent"           , ShaderDesc::CreateStageDescsFromShaderName("Tangent" , VS_PS) },
		ShaderDesc{ "Binormal"          , ShaderDesc::CreateStageDescsFromShaderName("Binormal", VS_PS) },
		ShaderDesc{ "Line"              , ShaderDesc::CreateStageDescsFromShaderName("Line"    , VS_PS) },
		ShaderDesc{ "TNB"               , ShaderDesc::CreateStageDescsFromShaderName("TNB"     , VS_PS) },
		ShaderDesc{ "Debug"             , ShaderDesc::CreateStageDescsFromShaderName("Debug"   , VS_PS) },
		ShaderDesc{ "Skybox"            , ShaderDesc::CreateStageDescsFromShaderName("Skybox"  , VS_PS) },
		ShaderDesc{ "SkyboxEquirectangular", {
			ShaderStageDesc{"Skybox_vs.hlsl"               , {} },
			ShaderStageDesc{"SkyboxEquirectangular_ps.hlsl", {} }
		}},
		//ShaderDesc{ "Forward_BRDF"      , ShaderDesc::CreateStageDescsFromShaderName("Forward_BRDF", VS_PS)},
		ShaderDesc{ "Forward_BRDF"      , {
		ShaderStageDesc{"Forward_BRDF_vs.hlsl", {}},
		ShaderStageDesc{"Forward_BRDF_ps.hlsl", {
#if ENABLE_PARALLAX_MAPPING
		ShaderMacro{"ENABLE_PARALLAX_MAPPING", "1"}
#endif
	}}
	}},
		ShaderDesc{ "DepthShader"           , ShaderDesc::CreateStageDescsFromShaderName("DepthShader", SHADER_STAGE_VS)},
		ShaderDesc{ "DepthShader_Instanced" , ShaderStageDesc{"DepthShader_vs.hlsl", {
			ShaderMacro{ "INSTANCED"     , "1" },
			ShaderMacro{ "INSTANCE_COUNT", std::to_string(MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS) }
		}}
	},
	};
		
	// todo: do not depend on array index, use a lookup, remove s_shaders[]
	for (int i = 0; i < shaderDescs.size(); ++i)
	{
		mBuiltinShaders.push_back(mpRenderer->CreateShader(shaderDescs[i]));
	}
	RenderPass::InitializeCommonSaders(mpRenderer);
	timer.Stop();
	//Log::Info("[ENGINE]: Shaders loaded in %.2fs", timer.DeltaTime());
	
	return true;
}

void Engine::Exit()
{
#if USE_DX12
	StopThreads();
#endif

	delete mpThreadPool;

	mpCPUProfiler->EndProfile();
	mpGPUProfiler->Exit();
	mUI.Exit();
	mpTextRenderer->Exit();
	mpRenderer->Exit();

	Log::Exit();
	if (sInstance)
	{
		delete sInstance;
		sInstance = nullptr;
	}
}


// ====================================================================================
// UPDATE FUNCTIONS
// ====================================================================================
void Engine::SimulateAndRenderFrame()
{
	const float dt = mpTimer->Tick();

	HandleInput();

#if USE_DX12
	mpCPUProfiler->BeginEntry("CPU");
	if (!mbIsPaused && !mbLoading)
	{
		CalcFrameStats(dt);

		// THREADIFY THIS
		mpCPUProfiler->BeginEntry("Update()");
		mpActiveScene->UpdateScene(dt);
		mpCPUProfiler->EndEntry();	// Update

		// THREADIFY THIS
		PreRender(); 
		// TODO...

		// THREADIFY THIS
		// PROCESS A LOAD LEVEL EVENT
		//
		if (!mLevelLoadQueue.empty())
		{
			int lvl = mLevelLoadQueue.back();
			mLevelLoadQueue.pop();
			LoadScene(lvl);
		}
	}

	mpInput->PostUpdate();

	mpCPUProfiler->EndEntry();	// CPU

	mpCPUProfiler->StateCheck();
	///++mFrameCount;
#else // USE_DX12

#if LOAD_ASYNC

	if (mbLoading)
	{
		CalcFrameStats(dt);
		//std::atomic_fetch_add(&mAccumulator, dt);	// not supported by ryzen?
		mAccumulator = mAccumulator.load() + dt;

		const int refreshRate = 30;	// limit frequency to 30Hz
		bool bRender = mAccumulator > (1.0f / refreshRate);
		if (bRender)
		{	// Signal rendering loading screen at the refresh rate
			mSignalRender.notify_all();
		}
	}
	else
	{
		if (mRenderThread.joinable())
		{	// Transition to single threaded rendering
			StopRenderThreadAndWait();		// blocks execution
			mpCPUProfiler->EndProfile();	// reset the cpu profiler entries
			mpCPUProfiler->BeginProfile();
		}
#endif

		mpCPUProfiler->BeginEntry("CPU");
		if (!mbIsPaused)
		{
			CalcFrameStats(dt);

			mpCPUProfiler->BeginEntry("Update()");
			mpActiveScene->UpdateScene(dt);
			mpCPUProfiler->EndEntry();	// Update

			PreRender();

			mpRenderer->BeginFrame();
			Render();
		}
		
		mpGPUProfiler->EndEntry();
		mpGPUProfiler->EndProfile(mFrameCount);

		// PRESENT THE FRAME
		//
		mpCPUProfiler->BeginEntry("Present");
		mpRenderer->EndFrame();

		mpCPUProfiler->EndEntry();// Present
		mpCPUProfiler->EndEntry();	// CPU

		mpCPUProfiler->StateCheck();
		++mFrameCount;

#if LOAD_ASYNC
	}
#endif // LOAD_ASYNC



	// PROCESS A LOAD LEVEL EVENT
	//
	if (!mLevelLoadQueue.empty())
	{
		int lvl = mLevelLoadQueue.back();
		mLevelLoadQueue.pop();
		mpCPUProfiler->Clear();
		mpGPUProfiler->Clear();
#if LOAD_ASYNC
		StartRenderThread();
		mbLoading = true;
		mSignalRender.notify_all();
#endif
		LoadScene(lvl);
	}

	mpInput->PostUpdate();
#endif // USE_DX12

	if (mbStartEngineShutdown)
	{
		mpApp->m_bAppWantsExit = true;
	}
}

void Engine::HandleInput()
{
	if (mpInput->IsKeyUp("ESC"))
	{
		if (mbMouseCaptured)
		{ 
			mbMouseCaptured = false;
			mpApp->CaptureMouse(mbMouseCaptured);
		}
		else if (!this->IsLoading())
		{
			mbStartEngineShutdown = true;
		}
	}

	if (mpInput->IsKeyTriggered("Backspace"))	TogglePause();

	if (mpInput->IsKeyTriggered("F1")) ToggleControlsTextRendering();
	if (mpInput->IsKeyTriggered("F2")) ToggleAmbientOcclusion();
	if (mpInput->IsKeyTriggered("F3")) ToggleBloom();
	if (mpInput->IsKeyTriggered("F4")) mEngineConfig.bRenderTargets = !mEngineConfig.bRenderTargets;
	if (mpInput->IsKeyTriggered("F5")) mEngineConfig.bBoundingBoxes = !mEngineConfig.bBoundingBoxes;
	if (mpInput->IsKeyTriggered("F6")) ToggleRenderingPath();

	//if (mpInput->IsKeyTriggered("'")) 
	if (mpInput->IsKeyTriggered("F"))// && mpInput->AreKeysDown(2, "ctrl", "shift"))
	{
		if(mpInput->IsKeyDown("ctrl") && mpInput->IsKeyDown("shift"))
			ToggleProfilerRendering();
	}

	// The following input will not be handled if the engine is currently loading a level
	if (mbLoading)
		return;

	// ----------------------------------------------------------------------------------------------

	if (mpInput->IsKeyTriggered("\\")) mpRenderer->ReloadShaders();

#if SSAO_DEBUGGING
	// todo: wire this to some UI text/control
	if (mEngineConfig.bSSAO)
	{
		if (mpInput->IsKeyDown("Shift") && mpInput->IsKeyDown("Ctrl"))
		{
			if (mpInput->IsWheelUp())		mAOPass.ChangeAOTechnique(+1);
			if (mpInput->IsWheelDown())	mAOPass.ChangeAOTechnique(-1);
		}
		else if (mpInput->IsKeyDown("Shift"))
		{
			const float step = 0.1f;
			if (mpInput->IsWheelUp()) { mAOPass.intensity += step; Log::Info("SSAO Intensity: %.2f", mAOPass.intensity); }
			if (mpInput->IsWheelDown()) { mAOPass.intensity -= step; if (mAOPass.intensity < 0.301) mAOPass.intensity = 1.0f; Log::Info("SSAO Intensity: %.2f", mAOPass.intensity); }
			//if (mpInput->IsMouseDown(Input::EMouseButtons::MOUSE_BUTTON_MIDDLE)) mAOPass.ChangeAOQuality(-1);
		}
		else if (mpInput->IsKeyDown("Ctrl"))
		{
			if (mpInput->IsWheelUp())		mAOPass.ChangeBlurQualityLevel(+1);
			if (mpInput->IsWheelDown())	mAOPass.ChangeBlurQualityLevel(-1);
		}
		else
		{
			const float step = 0.5f;
			if (mpInput->IsWheelUp()) { mAOPass.radius += step; Log::Info("SSAO Radius: %.2f", mAOPass.radius); }
			if (mpInput->IsWheelDown()) { mAOPass.radius -= step; if (mAOPass.radius < 0.301) mAOPass.radius = 1.0f; Log::Info("SSAO Radius: %.2f", mAOPass.radius); }
			//if (mpInput->IsMouseDown(Input::EMouseButtons::MOUSE_BUTTON_MIDDLE)) mAOPass.ChangeAOQuality(+1);
			if (mpInput->IsKeyTriggered("K")) mAOPass.ChangeAOQuality(+1);
			if (mpInput->IsKeyTriggered("J")) mAOPass.ChangeAOQuality(-1);
		}
	}
#endif

#if BLOOM_DEBUGGING
	if (mEngineConfig.bBloom)
	{
		Settings::Bloom& bloom = mPostProcessPass._settings.bloom;
		float& threshold = bloom.brightnessThreshold;
		int& blurStrength = bloom.blurStrength;
		const float step = 0.05f;
		const float threshold_hi = 3.0f;
		const float threshold_lo = 0.05f;
		if (mpInput->IsWheelUp()   && !mpInput->IsKeyDown("Shift") && !mpInput->IsKeyDown("Ctrl")) { threshold += step; if (threshold > threshold_hi) threshold = threshold_hi; Log::Info("Bloom Brightness Cutoff Threshold: %.2f", threshold); }
		if (mpInput->IsWheelDown() && !mpInput->IsKeyDown("Shift") && !mpInput->IsKeyDown("Ctrl")) { threshold -= step; if (threshold < threshold_lo) threshold = threshold_lo; Log::Info("Bloom Brightness Cutoff Threshold: %.2f", threshold); }
		if (mpInput->IsWheelUp()   && mpInput->IsKeyDown("Shift"))  { blurStrength += 1; Log::Info("Bloom Blur Strength = %d", blurStrength);}
		if (mpInput->IsWheelDown() && mpInput->IsKeyDown("Shift"))  { blurStrength -= 1; if (blurStrength == 0) blurStrength = 1; Log::Info("Bloom Blur Strength = %d", blurStrength); }
		if ((mpInput->IsWheelDown() || mpInput->IsWheelUp()) && mpInput->IsKeyDown("Ctrl"))   
		{ 
			const int direction = mpInput->IsWheelDown() ? -1 : +1;
			int nextShader = mPostProcessPass._bloomPass.mSelectedBloomShader + direction;
			if (nextShader < 0)
				nextShader = BloomPass::BloomShader::NUM_BLOOM_SHADERS - 1;
			if (nextShader == BloomPass::BloomShader::NUM_BLOOM_SHADERS)
				nextShader = 0;
			mPostProcessPass._bloomPass.mSelectedBloomShader = static_cast<BloomPass::BloomShader>(nextShader);
		}
	}
#endif

#if FULLSCREEN_DEBUG_TEXTURE
	if (mpInput->IsKeyTriggered("Space")) mbOutputDebugTexture = !mbOutputDebugTexture;
#endif

#if GBUFFER_DEBUGGING
	if (mpInput->IsWheelPressed())
	{
		mDeferredRenderingPasses.mbUseDepthPrepass = !mDeferredRenderingPasses.mbUseDepthPrepass;
		Log::Info("GBuffer Depth PrePass: ", (mDeferredRenderingPasses.mbUseDepthPrepass ? "Enabled" : "Disabled"));
	}
#endif

	// SCENE -------------------------------------------------------------
	if (mpInput->IsKeyTriggered("1"))	mLevelLoadQueue.push(0);
	if (mpInput->IsKeyTriggered("2"))	mLevelLoadQueue.push(1);
	if (mpInput->IsKeyTriggered("3"))	mLevelLoadQueue.push(2);
	if (mpInput->IsKeyTriggered("4"))	mLevelLoadQueue.push(3);
	if (mpInput->IsKeyTriggered("5"))	mLevelLoadQueue.push(4);
	if (mpInput->IsKeyTriggered("6"))	mLevelLoadQueue.push(5);
	if (mpInput->IsKeyTriggered("7"))	mLevelLoadQueue.push(6);
	if (mpActiveScene)
	{
		if (mpInput->IsKeyTriggered("R"))
		{
			if (mpInput->IsKeyDown("Shift")) ReloadScene();
			else							 mpActiveScene->ResetActiveCamera();
		}

		// index using enums. first element of environment map presets starts with cubemap preset count, as if both lists were concatenated.
		constexpr EEnvironmentMapPresets firstPreset = static_cast<EEnvironmentMapPresets>(CUBEMAP_PRESET_COUNT);
		constexpr EEnvironmentMapPresets lastPreset = static_cast<EEnvironmentMapPresets>(
			static_cast<EEnvironmentMapPresets>(CUBEMAP_PRESET_COUNT) + ENVIRONMENT_MAP_PRESET_COUNT - 1
			);

		EEnvironmentMapPresets selectedEnvironmentMap = mpActiveScene->GetActiveEnvironmentMapPreset();
		if (selectedEnvironmentMap == -1)
			return; // if no skymap is selected, ignore input to change it
		if (ENGINE->INP()->IsKeyTriggered("PageUp"))	selectedEnvironmentMap = selectedEnvironmentMap == lastPreset ? firstPreset : static_cast<EEnvironmentMapPresets>(selectedEnvironmentMap + 1);
		if (ENGINE->INP()->IsKeyTriggered("PageDown"))	selectedEnvironmentMap = selectedEnvironmentMap == firstPreset ? lastPreset : static_cast<EEnvironmentMapPresets>(selectedEnvironmentMap - 1);
		if (ENGINE->INP()->IsKeyTriggered("PageUp") || ENGINE->INP()->IsKeyTriggered("PageDown"))
			mpActiveScene->SetEnvironmentMap(selectedEnvironmentMap);
	}
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

#if 0
	// TODO: this has to be moved from here, Renderer::GetWindow() is deprecated
	if (frameCount % RefreshRate == 0)
	{
		mCurrentFrameTime = frameTime;
		const float frameTimeGPU = mpGPUProfiler->GetEntryAvg("GPU");
		
		std::ostringstream stats;
		stats.precision(2);
		stats << std::fixed;
		stats << "VQEngine | " << "CPU: " << frameTime * 1000.0f << " ms  GPU: " << frameTimeGPU * 1000.f << " ms | FPS: ";
		stats.precision(4);
		stats << GetFPS();
		SetWindowText(mpRenderer->GetWindow(), stats.str().c_str());
	}
#endif
	++frameCount;
}

const Settings::Engine& Engine::ReadSettingsFromFile()
{
	sEngineSettings = Parser::ReadSettings("Data/EngineSettings.ini");
	return sEngineSettings;
}

bool Engine::ReloadScene()
{
#if LOAD_ASYNC
	const auto& settings = Engine::GetSettings();
	mLevelLoadQueue.push(settings.levelToLoad);
	return true;
#else
	const auto& settings = Engine::GetSettings();
	Log::Info("Reloading Scene (%d)...", settings.levelToLoad);

	mpActiveScene->UnloadScene();
	bool bLoadSuccess = LoadSceneFromFile();
	return bLoadSuccess;
#endif
}

