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
#include "Camera.h"
#include "SceneResourceView.h"

#include "Application/Application.h"
#include "Application/Input.h"
#include "Application/ThreadPool.h"

#include "Utilities/Log.h"
#include "Utilities/PerfTimer.h"
#include "Utilities/CustomParser.h"
#include "Renderer/Profiler.h"

#include "Renderer/Renderer.h"
#include "Renderer/TextRenderer.h"
#include "Renderer/GeometryGenerator.h"

#include "Scenes/ObjectsScene.h"
#include "Scenes/SSAOTestScene.h"
#include "Scenes/PBRScene.h"
#include "Scenes/StressTestScene.h"
#include "Scenes/SponzaScene.h"
#include "Scenes/LightsScene.h"
#include "Scenes/LODTestScene.h"

#include <sstream>
#include <DirectXMath.h>

using namespace VQEngine;

void Engine::StartRenderThread()
{
	mbStopRenderThread = false;
	mRenderThread = std::thread(&Engine::RenderThread, this);
}

void Engine::StopRenderThreadAndWait()
{
	mbStopRenderThread = true;
	mSignalRender.notify_all();
	mRenderThread.join();
}

std::mutex Engine::mLoadRenderingMutex;

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
{}


Engine::~Engine(){}


bool Engine::Initialize(HWND hwnd)
{
	mbIsPaused = false;
	Log::Info("[ENGINE]: Initializing --------------------");
	if (!mpRenderer || !mpInput || !mpTimer)
	{
		Log::Error("Nullptr Engine::Init()\n");
		return false;
	}

	mpTimer->Start();
	StartRenderThread();


	// INITIALIZE SYSTEMS
	//--------------------------------------------------------------
	const bool bEnableLogging = true;	// todo: read from settings
	const Settings::Window& windowSettings = sEngineSettings.window;

	mpInput->Initialize();
	if (!mpRenderer->Initialize(hwnd, windowSettings, sEngineSettings.rendering))
	{
		Log::Error("Cannot initialize Renderer.\n");
		return false;
	}

	Scene::InitializeBuiltinMeshes();
	LoadShaders();
	
	if (!mpTextRenderer->Initialize(mpRenderer))
	{
		Log::Error("Cannot initialize Text Renderer.\n");
		//return false;
	}

	mUI.Initialize(mpRenderer, mpTextRenderer, UI::ProfilerStack{mpCPUProfiler, mpGPUProfiler});
	mpGPUProfiler->Init(mpRenderer->m_deviceContext, mpRenderer->m_device);

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
	mbLoading = false;
	return true;
}


bool Engine::Load(ThreadPool* pThreadPool)
{
	Log::Info("[ENGINE]: Loading -------------------------");
	mpCPUProfiler->BeginProfile();
	mpThreadPool = pThreadPool;
	
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
				mAAResolvePass.Initialize(mpRenderer, mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._shadeTarget));
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

	return true;
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

			mpRenderer->UnbindDepthTarget();
			mpRenderer->UnbindRenderTargets();
			mpRenderer->Apply();

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
	
	// create the ShaderCache folder if it doesn't exist
	Application::s_ShaderCacheDirectory = Application::s_WorkspaceDirectory + "\\ShaderCache";
	DirectoryUtil::CreateFolderIfItDoesntExist(Application::s_ShaderCacheDirectory);

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
			Render();
		}

		// PRESENT THE FRAME
		//
		mpCPUProfiler->BeginEntry("Present");
		mpRenderer->EndFrame();
		mpCPUProfiler->EndEntry();


		mpCPUProfiler->EndEntry();	// CPU
		mpCPUProfiler->StateCheck();


		++mFrameCount;

#if LOAD_ASYNC
	}
#endif



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
}

void Engine::RenderThread()	// This thread is currently only used during loading.
{
	constexpr bool bOneTimeLoadingScreenRender = false; // We're looping;
	while (!mbStopRenderThread)
	{
		std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
#if 0 // solution attempt. #thread-issue
		mSignalRender.wait(lck, [=]() { return !mbStopRenderThread; });
#else
		// deadlock: render thread waits on signal,
		//           StopRenderThreadAndWait() waits on join
		mSignalRender.wait(lck);
#endif
		mpCPUProfiler->BeginEntry("CPU");

		mpGPUProfiler->BeginProfile(mFrameCount);
		mpGPUProfiler->BeginEntry("GPU");

		mpRenderer->BeginFrame();
		RenderLoadingScreen(bOneTimeLoadingScreenRender);
		RenderUI();	
		
		mpGPUProfiler->EndEntry();	// GPU
		mpGPUProfiler->EndProfile(mFrameCount);


		mpCPUProfiler->BeginEntry("Present");
		mpRenderer->EndFrame();
		mpCPUProfiler->EndEntry(); // Present


		mpCPUProfiler->EndEntry();	// CPU
		mpCPUProfiler->StateCheck();
		mAccumulator = 0.0f;
		++mFrameCount;
	}

	// when loading is finished, we use the same signal variable to signal
	// the update thread (which woke this thread up in the first place)
	mpCPUProfiler->Clear();
	mpGPUProfiler->Clear();
	mSignalRender.notify_all();
}

void Engine::HandleInput()
{
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
	if (mpInput->IsKeyTriggered("R"))
	{
		if (mpInput->IsKeyDown("Shift")) ReloadScene();
		else							 mpActiveScene->ResetActiveCamera();
	}


	if (mpInput->IsKeyTriggered("1"))	mLevelLoadQueue.push(0);
	if (mpInput->IsKeyTriggered("2"))	mLevelLoadQueue.push(1);
	if (mpInput->IsKeyTriggered("3"))	mLevelLoadQueue.push(2);
	if (mpInput->IsKeyTriggered("4"))	mLevelLoadQueue.push(3);
	if (mpInput->IsKeyTriggered("5"))	mLevelLoadQueue.push(4);
	if (mpInput->IsKeyTriggered("6"))	mLevelLoadQueue.push(5);
	if (mpInput->IsKeyTriggered("7"))	mLevelLoadQueue.push(6);


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

void Engine::SendLightData() const
{
	const float shadowDimension = static_cast<float>(mShadowMapPass.mShadowMapDimension_Spot);

	// LIGHTS ( POINT | SPOT | DIRECTIONAL )
	//
	const vec2 directionalShadowMapDimensions = mShadowMapPass.GetDirectionalShadowMapDimensions(mpRenderer);
	mpRenderer->SetConstantStruct("Lights", &mSceneLightData._cb);
	mpRenderer->SetConstant2f("spotShadowMapDimensions", vec2(shadowDimension, shadowDimension));
	mpRenderer->SetConstant1f("directionalShadowMapDimension", directionalShadowMapDimensions.x());
	//mpRenderer->SetConstant2f("directionalShadowMapDimensions", directionalShadowMapDimensions);
	

	// SHADOW MAPS
	//
	mpRenderer->SetTextureArray("texSpotShadowMaps", mShadowMapPass.mShadowMapTextures_Spot);
	mpRenderer->SetTextureArray("texPointShadowMaps", mShadowMapPass.mShadowMapTextures_Point);
	if (mShadowMapPass.mShadowMapTexture_Directional != -1)
		mpRenderer->SetTextureArray("texDirectionalShadowMaps", mShadowMapPass.mShadowMapTexture_Directional);

#ifdef _DEBUG
	const SceneLightingConstantBuffer::cb& cb = mSceneLightData._cb;	// constant buffer shorthand
	if (cb.pointLightCount > cb.pointLights.size())	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (cb.spotLightCount > cb.spotLights.size())	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
}

void Engine::PreRender()
{
#if LOAD_ASYNC
	if (mbLoading) return;
#endif
	mpCPUProfiler->BeginEntry("PreRender()");

	mpActiveScene->mSceneView.bIsPBRLightingUsed = IsLightingModelPBR();
	mpActiveScene->mSceneView.bIsDeferredRendering = mEngineConfig.bDeferredOrForward;

	// TODO: #RenderPass or Scene should manage this.
	// mTBNDrawObjects.clear();
	// std::vector<const GameObject*> objects;
	// mpActiveScene->GetSceneObjects(objects);
	// for (const GameObject* obj : objects)
	// {
	// 	if (obj->mRenderSettings.bRenderTBN)
	// 		mTBNDrawObjects.push_back(obj);
	// }

	mpActiveScene->PreRender(mFrameStats, mSceneLightData);
	mFrameStats.rstats = mpRenderer->GetRenderStats();
	mFrameStats.fps = GetFPS();

	mpCPUProfiler->EndEntry();
}

// ====================================================================================
// RENDER FUNCTIONS
// ====================================================================================
void Engine::Render()
{
	mpCPUProfiler->BeginEntry("Render()");

	mpGPUProfiler->BeginProfile(mFrameCount);
	mpGPUProfiler->BeginEntry("GPU");

	mpRenderer->BeginFrame();

	const XMMATRIX& viewProj = mpActiveScene->mSceneView.viewProj;
	const bool bSceneSSAO = mpActiveScene->mSceneView.sceneRenderSettings.ssao.bEnabled;
	const bool bAAResolve = sEngineSettings.rendering.antiAliasing.IsAAEnabled();

	// SHADOW MAPS
	//------------------------------------------------------------------------
	mpCPUProfiler->BeginEntry("Shadow Pass");
	mpGPUProfiler->BeginEntry("Shadow Pass");
	mpRenderer->BeginEvent("Shadow Pass");
	
	mpRenderer->UnbindRenderTargets();	// unbind the back render target | every pass should have their own render targets
	mShadowMapPass.RenderShadowMaps(mpRenderer, mpActiveScene->mShadowView, mpGPUProfiler);
	
	mpRenderer->EndEvent();
	mpCPUProfiler->EndEntry();
	mpGPUProfiler->EndEntry();

	// LIGHTING PASS
	//------------------------------------------------------------------------
	mpRenderer->ResetPipelineState();

	//==========================================================================
	// DEFERRED RENDERER
	//==========================================================================
	if (mEngineConfig.bDeferredOrForward)
	{
		const GBuffer& gBuffer = mDeferredRenderingPasses._GBuffer;
		const TextureID texNormal = mpRenderer->GetRenderTargetTexture(gBuffer.mRTNormals);
		const TextureID texDiffuseRoughness = mpRenderer->GetRenderTargetTexture(gBuffer.mRTDiffuseRoughness);
		const TextureID texSpecularMetallic = mpRenderer->GetRenderTargetTexture(gBuffer.mRTSpecularMetallic);
		const TextureID texDepthTexture = mpRenderer->mDefaultDepthBufferTexture;
		const TextureID tSSAO = mEngineConfig.bSSAO && bSceneSSAO
			? mAOPass.GetBlurredAOTexture(mpRenderer)
			: mAOPass.whiteTexture4x4;
		const DeferredRenderingPasses::RenderParams deferredLightingParams = 
		{
			mpRenderer
			, mpActiveScene->mSceneView
			, mSceneLightData
			, tSSAO
			, sEngineSettings.rendering.bUseBRDFLighting
		};

		// GEOMETRY - DEPTH PASS
		mpGPUProfiler->BeginEntry("Geometry Pass");
		mpCPUProfiler->BeginEntry("Geometry Pass");
		mpRenderer->BeginEvent("Geometry Pass");
		mDeferredRenderingPasses.RenderGBuffer(mpRenderer, mpActiveScene, mpActiveScene->mSceneView);
		mpRenderer->EndEvent();	
		mpCPUProfiler->EndEntry();
		mpGPUProfiler->EndEntry();

		// AMBIENT OCCLUSION  PASS
		mpCPUProfiler->BeginEntry("AO Pass");
		if (mEngineConfig.bSSAO && bSceneSSAO)
		{
			mAOPass.RenderAmbientOcclusion(mpRenderer, texNormal, mpActiveScene->mSceneView);
		}
		mpCPUProfiler->EndEntry(); // AO Pass

		// DEFERRED LIGHTING PASS
		mpCPUProfiler->BeginEntry("Lighting Pass");
		mpGPUProfiler->BeginEntry("Lighting Pass");
		mpRenderer->BeginEvent("Lighting Pass");
		mDeferredRenderingPasses.RenderLightingPass(deferredLightingParams);
		mpRenderer->EndEvent();
		mpCPUProfiler->EndEntry();
		mpGPUProfiler->EndEntry();

		mpCPUProfiler->BeginEntry("Skybox & Lights");
		
		// LIGHT SOURCES
		mpRenderer->BindDepthTarget(mWorldDepthTarget);
		
		// SKYBOX
		//
		if (mpActiveScene->HasSkybox())
		{
			mpGPUProfiler->BeginEntry("Skybox Pass");
			mpActiveScene->RenderSkybox(mpActiveScene->mSceneView.viewProj);
			mpGPUProfiler->EndEntry();
		}

		mpCPUProfiler->EndEntry();
	}

	//==========================================================================
	// FORWARD RENDERER
	//==========================================================================
	else
	{
		const bool bZPrePass = mEngineConfig.bSSAO && bSceneSSAO;
		const TextureID tSSAO = bZPrePass
			? mAOPass.GetBlurredAOTexture(mpRenderer)
			: mAOPass.whiteTexture4x4;
		const TextureID texIrradianceMap = mpActiveScene->mSceneView.environmentMap.irradianceMap;
		const SamplerID smpEnvMap = mpActiveScene->mSceneView.environmentMap.envMapSampler < 0 
			? EDefaultSamplerState::POINT_SAMPLER 
			: mpActiveScene->mSceneView.environmentMap.envMapSampler;
		const TextureID prefilteredEnvMap = mpActiveScene->mSceneView.environmentMap.prefilteredEnvironmentMap;
		const TextureID tBRDFLUT = EnvironmentMap::sBRDFIntegrationLUTTexture;
		
		const RenderTargetID lightingRenderTarget = bAAResolve 
			? mDeferredRenderingPasses._shadeTarget // resource reusage
			: mPostProcessPass._worldRenderTarget;

		// AMBIENT OCCLUSION - Z-PREPASS
		if (bZPrePass)
		{
			mpRenderer->SetViewport(mpRenderer->FrameRenderTargetWidth(), mpRenderer->FrameRenderTargetHeight());
			const RenderTargetID normals = mDeferredRenderingPasses._GBuffer.mRTNormals; // resource reusage
			const TextureID texNormal = mpRenderer->GetRenderTargetTexture(normals);
			const ZPrePass::RenderParams zPrePassParams =
			{
				mpRenderer,
				mpActiveScene,
				mpActiveScene->mSceneView,
				normals
			};

			mpGPUProfiler->BeginEntry("Z-PrePass");
			mZPrePass.RenderDepth(zPrePassParams);
			mpGPUProfiler->EndEntry();

			mpRenderer->BeginEvent("Ambient Occlusion Pass");
			{
				mAOPass.RenderAmbientOcclusion(mpRenderer, texNormal, mpActiveScene->mSceneView);
			}
			mpRenderer->EndEvent(); // Ambient Occlusion Pass
		}

		const bool bDoClearColor = true;
		const bool bDoClearDepth = !bZPrePass;
		const bool bDoClearStencil = false;
		ClearCommand clearCmd(
			bDoClearColor, bDoClearDepth, bDoClearStencil,
			{ 0, 0, 0, 0 }, 1, 0
		);

		mpRenderer->BindRenderTarget(lightingRenderTarget);
		mpRenderer->BindDepthTarget(mWorldDepthTarget);
		mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_TEST_ONLY);
		mpRenderer->BeginRender(clearCmd);

		// SKYBOX
		// if we're not rendering the skybox, call apply() to unbind
		// shadow light depth target so we can bind it in the lighting pass
		// otherwise, skybox render pass will take care of it
		if (mpActiveScene->HasSkybox())
		{
			mpGPUProfiler->BeginEntry("Skybox Pass");
			mpActiveScene->RenderSkybox(mpActiveScene->mSceneView.viewProj);
			mpGPUProfiler->EndEntry();
		}
		else
		{
			// todo: this might be costly, profile this
			mpRenderer->SetShader(mSelectedShader);	// set shader so apply won't complain 
			mpRenderer->Apply();					// apply to bind depth stencil
		}

		// LIGHTING
		const ForwardLightingPass::RenderParams forwardLightingParams = 
		{
			  mpRenderer
			, mpActiveScene
			, mpActiveScene->mSceneView
			, mSceneLightData
			, tSSAO
			, lightingRenderTarget
			, mAOPass.blackTexture4x4
			, bZPrePass
		};

		mpGPUProfiler->BeginEntry("Lighting<Forward> Pass");
		mForwardLightingPass.RenderLightingPass(forwardLightingParams);
		mpGPUProfiler->EndEntry();
	}

	mpActiveScene->RenderLights(); // when skymaps are disabled, there's an error here that needs fixing.

	RenderDebug(viewProj);

	// AA RESOLVE
	//------------------------------------------------------------------------
	if (bAAResolve)
	{
		// currently only SSAA is supported
		mpRenderer->BeginEvent("Resolve AA");
		mpGPUProfiler->BeginEntry("Resolve AA");
		mpCPUProfiler->BeginEntry("Resolve AA");

		mAAResolvePass.SetInputTexture(mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._shadeTarget));
		mAAResolvePass.Render(mpRenderer);

		mpCPUProfiler->EndEntry();	// Resolve AA
		mpGPUProfiler->EndEntry();	// Resolve AA
		mpRenderer->EndEvent(); // Resolve AA
	}

	// Determine the output texture for Lighting Pass
	// which is also the input texture for post processing
	//
	const TextureID postProcessInputTextureID = mpRenderer->GetRenderTargetTexture(bAAResolve 
		? mAAResolvePass.mResolveTarget     // SSAA (shares the same render target for deferred and forward)
		: (mEngineConfig.bDeferredOrForward 
			? mDeferredRenderingPasses._shadeTarget // deferred / no SSAA
			: mPostProcessPass._worldRenderTarget)  // forward  / no SSAA
	);


	mpRenderer->SetBlendState(EDefaultBlendState::DISABLED);

	// POST PROCESS PASS | DEBUG PASS | UI PASS
	//------------------------------------------------------------------------
	mpCPUProfiler->BeginEntry("Post Process");
	mpGPUProfiler->BeginEntry("Post Process"); 
#if FULLSCREEN_DEBUG_TEXTURE
	const TextureID texDebug = mAOPass.GetBlurredAOTexture(mpRenderer);
	mPostProcessPass.Render(mpRenderer, mEngineConfig.bBloom, mbOutputDebugTexture 
		? texDebug 
		: postProcessInputTextureID
	);
#else
	mPostProcessPass.Render(mpRenderer, mEngineConfig.bBloom);
#endif
	mpCPUProfiler->EndEntry();
	mpGPUProfiler->EndEntry();

	//------------------------------------------------------------------------
	RenderUI();
	//------------------------------------------------------------------------
	
	mpCPUProfiler->EndEntry();	// Render() call

	mpGPUProfiler->EndEntry(); // GPU
	mpGPUProfiler->EndProfile(mFrameCount);
}

void Engine::RenderDebug(const XMMATRIX& viewProj)
{
	mpGPUProfiler->BeginEntry("Debug Pass");
	mpCPUProfiler->BeginEntry("Debug Pass");
	if (mEngineConfig.bBoundingBoxes)	// BOUNDING BOXES
	{
		mpActiveScene->RenderDebug(viewProj);
	}

	if (mEngineConfig.bRenderTargets)	// RENDER TARGETS
	{
		mpCPUProfiler->BeginEntry("Debug Textures");
		const int screenWidth = sEngineSettings.window.width;
		const int screenHeight = sEngineSettings.window.height;
		const float aspectRatio = static_cast<float>(screenWidth) / screenHeight;

		// debug texture strip draw settings
		const int bottomPaddingPx = 0;	 // offset from bottom of the screen
		const int heightPx = 128; // height for every texture
		const int paddingPx = 0;  // padding between debug textures
		const vec2 fullscreenTextureScaledDownSize((float)heightPx * aspectRatio, (float)heightPx);
		const vec2 squareTextureScaledDownSize((float)heightPx, (float)heightPx);

		// Textures to draw
		const TextureID white4x4 = mAOPass.whiteTexture4x4;
		//const TextureID tShadowMap		 = mpRenderer->GetDepthTargetTexture(mShadowMapPass._spotShadowDepthTargets);
		const TextureID tBlurredBloom = mPostProcessPass._bloomPass.GetBloomTexture(mpRenderer);
		const TextureID tDiffuseRoughness = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer.mRTDiffuseRoughness);
		//const TextureID tSceneDepth		 = m_pRenderer->m_state._depthBufferTexture._id;
		const TextureID tSceneDepth = mpRenderer->GetDepthTargetTexture(0);
		const TextureID tNormals = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer.mRTNormals);
		const TextureID tAO = mEngineConfig.bSSAO ? mAOPass.GetBlurredAOTexture(mpRenderer) : mAOPass.whiteTexture4x4;
		const TextureID tBRDF = EnvironmentMap::sBRDFIntegrationLUTTexture;
		TextureID preFilteredEnvMap = mpActiveScene->GetEnvironmentMap().prefilteredEnvironmentMap;
		preFilteredEnvMap = preFilteredEnvMap < 0 ? white4x4 : preFilteredEnvMap;
		TextureID tDirectionalShadowMap = (mShadowMapPass.mDepthTarget_Directional == -1 || mpActiveScene->mDirectionalLight.mbEnabled == 0)
			? white4x4 
			: mpRenderer->GetDepthTargetTexture(mShadowMapPass.mDepthTarget_Directional);

		const std::vector<DrawQuadOnScreenCommand> quadCmds = [&]() 
		{
			// first row -----------------------------------------
			vec2 screenPosition(0.0f, static_cast<float>(bottomPaddingPx + heightPx * 0));
			std::vector<DrawQuadOnScreenCommand> c
			{	//		Pixel Dimensions	 Screen Position (offset below)	  Texture			DepthTexture?
				{ fullscreenTextureScaledDownSize,	screenPosition,			tSceneDepth			, true },
			{ fullscreenTextureScaledDownSize,	screenPosition,			tDiffuseRoughness	, false },
			{ fullscreenTextureScaledDownSize,	screenPosition,			tNormals			, false },
			//{ squareTextureScaledDownSize    ,	screenPosition,			tShadowMap			, true},
			{ fullscreenTextureScaledDownSize,	screenPosition,			tBlurredBloom		, false },
			{ fullscreenTextureScaledDownSize,	screenPosition,			tAO					, false, 1 },
			//{ squareTextureScaledDownSize,		screenPosition,			tBRDF				, false },
			};
			for (size_t i = 1; i < c.size(); i++)	// offset textures accordingly (using previous' x-dimension)
				c[i].bottomLeftCornerScreenCoordinates.x() = c[i - 1].bottomLeftCornerScreenCoordinates.x() + c[i - 1].dimensionsInPixels.x() + paddingPx;

			// second row -----------------------------------------
			screenPosition = vec2(0.0f, static_cast<float>(bottomPaddingPx + heightPx * 1));
			const size_t shadowMapCount = mSceneLightData._cb.pointLightCount_shadow;
			const size_t row_offset = c.size();
			size_t currShadowMap = 0;

			// the directional light
			{
				c.push_back({ squareTextureScaledDownSize, screenPosition, tDirectionalShadowMap, false });

				if (currShadowMap > 0)
				{
					c[row_offset].bottomLeftCornerScreenCoordinates.x()
						= c[row_offset - 1].bottomLeftCornerScreenCoordinates.x()
						+ c[row_offset - 1].dimensionsInPixels.x() + paddingPx;
				}
				++currShadowMap;
			}

			// spot lights
			// TODO: spot light depth texture is stored as a texture array and the
			//       current debug shader doesn't support texture array -> extend
			//       the debug shader in v0.5.0
			//
			//for (size_t i = 0; i < mSceneLightData._cb.spotLightCount_shadow; ++i)
			//{
			//	TextureID tex = mpRenderer->GetDepthTargetTexture(mShadowMapPass.mDepthTargets_Spot[i]);
			//	c.push_back({
			//		squareTextureScaledDownSize, screenPosition, tex, true
			//		});
			//
			//	if (currShadowMap > 0)
			//	{
			//		c[row_offset + i].bottomLeftCornerScreenCoordinates.x()
			//			= c[row_offset + i - 1].bottomLeftCornerScreenCoordinates.x()
			//			+ c[row_offset + i - 1].dimensionsInPixels.x() + paddingPx;
			//	}
			//	++currShadowMap;
			//}

			return c;
		}();


		mpRenderer->BeginEvent("Debug Textures");
		mpRenderer->SetShader(EShaders::DEBUG);
		for (const DrawQuadOnScreenCommand& cmd : quadCmds)
		{
			mpRenderer->DrawQuadOnScreen(cmd);
		}
		mpRenderer->EndEvent();
		mpCPUProfiler->EndEntry();
	}

	// Render TBN vectors (INACTIVE)
	//
#if 0
	const bool bIsShaderTBN = !mTBNDrawObjects.empty();
	if (bIsShaderTBN)
	{
		constexpr bool bSendMaterial = false;

		mpRenderer->BeginEvent("Draw TBN Vectors");
		if (mEngineConfig.bForwardOrDeferred)
			mpRenderer->BindDepthTarget(mWorldDepthTarget);

		mpRenderer->SetShader(EShaders::TBN);

		for (const GameObject* obj : mTBNDrawObjects)
			obj->RenderOpaque(mpRenderer, mSceneView, bSendMaterial, mpActiveScene->mMaterials);


		if (mEngineConfig.bForwardOrDeferred)
			mpRenderer->UnbindDepthTarget();

		mpRenderer->SetShader(mSelectedShader);
		mpRenderer->EndEvent();
	}
#endif
	mpCPUProfiler->EndEntry();	// Debug
	mpGPUProfiler->EndEntry();
}


void Engine::RenderUI() const
{
	mpRenderer->BeginEvent("UI");
	mpGPUProfiler->BeginEntry("UI");
	mpCPUProfiler->BeginEntry("UI");
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_NONE);	
	if (mEngineConfig.mbShowProfiler) { mUI.RenderPerfStats(mFrameStats); }
	if (mEngineConfig.mbShowControls) { mUI.RenderEngineControls(); }
	if (!mbLoading)
	{
		if (mpActiveScene)
		{
			mpActiveScene->RenderUI();
		}
#if DEBUG
		else
		{
			Log::Warning("Engine::mpActiveScene = nullptr when !mbLoading ");
		}
#endif
	}

	mpCPUProfiler->EndEntry();	// UI
	mpGPUProfiler->EndEntry();
	mpRenderer->EndEvent(); // UI
}


void Engine::RenderLoadingScreen(bool bOneTimeRender) const
{
	const XMMATRIX matTransformation = XMMatrixIdentity();
	const auto IABuffers = SceneResourceView::GetBuiltinMeshVertexAndIndexBufferID(EGeometry::FULLSCREENQUAD);

	if (bOneTimeRender)
	{
		mpRenderer->BeginFrame();
	}
	else
	{	
		mpGPUProfiler->BeginEntry("Loading Screen");
	}

	mpRenderer->SetShader(EShaders::UNLIT);
	mpRenderer->UnbindDepthTarget();
	mpRenderer->BindRenderTarget(0);
	mpRenderer->SetVertexBuffer(IABuffers.first);
	mpRenderer->SetIndexBuffer(IABuffers.second);
	mpRenderer->SetTexture("texDiffuseMap", mActiveLoadingScreen);
	mpRenderer->SetConstant1f("isDiffuseMap", 1.0f);
	mpRenderer->SetConstant3f("diffuse", vec3(1.0f, 1, 1));
	mpRenderer->SetConstant4x4f("worldViewProj", matTransformation);
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);
	mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
	mpRenderer->SetViewport(mpRenderer->WindowWidth(), mpRenderer->WindowHeight());
	mpRenderer->Apply();
	mpRenderer->DrawIndexed();

	if (bOneTimeRender)
	{
		mpRenderer->EndFrame();
		mpRenderer->UnbindRenderTargets();
	}
	else
	{
		mpGPUProfiler->EndEntry();
	}
}
