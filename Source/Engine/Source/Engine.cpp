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

#define OVERRIDE_LEVEL_LOAD 1	// Toggle for overriding level loading
#define OVERRIDE_LEVEL_VALUE 0	// which level to load


// ASYNC / THREADED LOADING SWITCHES
// -------------------------------------------------------
// Uses a thread pool of worker threads to load scene models
// while utilizing a render thread to render loading screen
//
#define LOAD_ASYNC 1
// -------------------------------------------------------
#include "Engine.h"

#include "Application/Input.h"
#include "Application/ThreadPool.h"

#include "Utilities/Log.h"
#include "Utilities/PerfTimer.h"
#include "Utilities/CustomParser.h"
#include "Utilities/Profiler.h"
#include "Camera.h"

#include "Renderer/Renderer.h"
#include "Renderer/TextRenderer.h"
#include "Renderer/GeometryGenerator.h"

#include "Scenes/ObjectsScene.h"
#include "Scenes/SSAOTestScene.h"
#include "Scenes/IBLTestScene.h"
#include "Scenes/StressTestScene.h"
#include "Scenes/SponzaScene.h"

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
	, mUI(mBuiltinMeshes, mEngineConfig)
{}

Engine::~Engine(){}


bool Engine::Initialize(HWND hwnd)
{
	StartRenderThread();
	mpTimer->Start();
	if (!mpRenderer || !mpInput || !mpTimer)
	{
		Log::Error("Nullptr Engine::Init()\n");
		return false;
	}

	// INITIALIZE SYSTEMS
	//--------------------------------------------------------------
	const bool bEnableLogging = true;	// todo: read from settings
	const Settings::Rendering& rendererSettings = sEngineSettings.rendering;
	const Settings::Window& windowSettings = sEngineSettings.window;

	mpInput->Initialize();
	if (!mpRenderer->Initialize(hwnd, windowSettings))
	{
		Log::Error("Cannot initialize Renderer.\n");
		return false;
	}

	// PRIMITIVES
	//--------------------------------------------------------------------
	{
		// cylinder parameters
		const float	 cylHeight = 3.1415f;		const float	 cylTopRadius = 1.0f;
		const float	 cylBottomRadius = 1.0f;	const unsigned cylSliceCount = 120;
		const unsigned cylStackCount = 100;

		// grid parameters
		const float gridWidth = 1.0f;		const float gridDepth = 1.0f;
		const unsigned gridFinenessH = 100;	const unsigned gridFinenessV = 100;

		// sphere parameters
		const float sphRadius = 2.0f;
		const unsigned sphRingCount = 25;	const unsigned sphSliceCount = 25;

		mBuiltinMeshes =	// this should match enum declaration order
		{
			GeometryGenerator::Triangle(1.0f),
			GeometryGenerator::Quad(1.0f),
			GeometryGenerator::FullScreenQuad(),
			GeometryGenerator::Cube(),
			GeometryGenerator::Cylinder(cylHeight, cylTopRadius, cylBottomRadius, cylSliceCount, cylStackCount),
			GeometryGenerator::Sphere(sphRadius, sphRingCount, sphSliceCount),
			GeometryGenerator::Grid(gridWidth, gridDepth, gridFinenessH, gridFinenessV),
			GeometryGenerator::Sphere(sphRadius / 40, 10, 10),
		};
	}

	if (!mpTextRenderer->Initialize(mpRenderer))
	{
		Log::Error("Cannot initialize Text Renderer.\n");
		return false;
	}
	mUI.Initialize(mpRenderer, mpTextRenderer, UI::ProfilerStack{mpCPUProfiler, mpGPUProfiler});
	mpGPUProfiler->Init(mpRenderer->m_deviceContext, mpRenderer->m_device);

	// INITIALIZE RENDERING
	//--------------------------------------------------------------
	// render passes

	mEngineConfig.bDeferredOrForward = rendererSettings.bUseDeferredRendering;
	mEngineConfig.bSSAO = rendererSettings.bAmbientOcclusion;
	mEngineConfig.bBloom = true;	// currently not deserialized
	mEngineConfig.bRenderTargets = true;
	mEngineConfig.bBoundingBoxes = false;
	mSelectedShader = mEngineConfig.bDeferredOrForward ? mDeferredRenderingPasses._geometryShader : EShaders::FORWARD_BRDF;
	mWorldDepthTarget = 0;	// assumes first index in renderer->m_depthTargets[]

	// this is bad... every scene needs to be pushed back.
	mpScenes.push_back(new ObjectsScene(mpRenderer, mpTextRenderer));
	mpScenes.push_back(new SSAOTestScene(mpRenderer, mpTextRenderer));
	mpScenes.push_back(new IBLTestScene(mpRenderer, mpTextRenderer));
	mpScenes.push_back(new StressTestScene(mpRenderer, mpTextRenderer));
	mpScenes.push_back(new SponzaScene(mpRenderer, mpTextRenderer));

	mpTimer->Stop();
	Log::Info("Engine initialized in %.2fs", mpTimer->DeltaTime());
	mbLoading = false;
	return true;
}


bool Engine::Load(ThreadPool* pThreadPool)
{
	mpCPUProfiler->BeginProfile();
	mpThreadPool = pThreadPool;
	const Settings::Rendering& rendererSettings = sEngineSettings.rendering;
	
	mLoadingScreenTextures.push_back(mpRenderer->CreateTextureFromFile("LoadingScreen0.png"));

#if LOAD_ASYNC

	// quickly render one frame of loading screen
	mbLoading = true;
	mAccumulator = 1.0f;	// start rendering loading screen on update loop
	RenderLoadingScreen(true);

	// set up a parallel task to load everything.
	auto AsyncEngineLoad = [&]() -> bool
	{
		// LOAD ENVIRONMENT MAPS
		//
		// mpCPUProfiler->BeginProfile();
		// mpCPUProfiler->BeginEntry("EngineLoad");
		Log::Info("-------------------- LOADING ENVIRONMENT MAPS --------------------- ");
		//mpTimer->Start();
#if 1
		Skybox::InitializePresets_Async(mpRenderer, rendererSettings);
#else
		{
			std::unique_lock<std::mutex> lck(Engine::mLoadRenderingMutex);
			Skybox::InitializePresets(mpRenderer, rendererSettings);
		}
#endif
		//mpTimer->Stop();
		//Log::Info("-------------------- ENVIRONMENT MAPS LOADED IN %.2fs. --------------------", mpTimer->DeltaTime());
		//mpTimer->Reset();

		// SCENE INITIALIZATION
		//
		Log::Info("-------------------- LOADING SCENE --------------------- ");
		//mpTimer->Start();
		const size_t numScenes = sEngineSettings.sceneNames.size();
		assert(sEngineSettings.levelToLoad < numScenes);
#if defined(_DEBUG) && (OVERRIDE_LEVEL_LOAD > 0)
		sEngineSettings.levelToLoad = OVERRIDE_LEVEL_VALUE;
#endif
		
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
		//mpTimer->Start();
		{	// #AsyncLoad: Mutex DEVICE
			//Log::Info("---------------- INITIALIZING RENDER PASSES ---------------- ");
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mShadowMapPass.InitializeSpotLightShadowMaps(mpRenderer, rendererSettings.shadowMap);
			}
			//renderer->m_Direct3D->ReportLiveObjects();
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mDeferredRenderingPasses.Initialize(mpRenderer);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mPostProcessPass.Initialize(mpRenderer, rendererSettings.postProcess);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mDebugPass.Initialize(mpRenderer);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mSSAOPass.Initialize(mpRenderer);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mZPrePass.Initialize(mpRenderer);
			}
			{
				std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
				mForwardLightingPass.Initialize(mpRenderer);
			}
		}
		//mpTimer->Stop();
		//Log::Info("---------------- INITIALIZING RENDER PASSES DONE IN %.2fs ---------------- ", mpTimer->DeltaTime());
		//mpCPUProfiler->EndEntry();
		//mpCPUProfiler->EndProfile();

		mbLoading = false;
		return true;
	};

	mpThreadPool->AddTask(AsyncEngineLoad);
	return true;

#else

	RenderLoadingScreen(true); // quickly render one frame of loading screen

	// LOAD ENVIRONMENT MAPS
	//
	mpTimer->Start();
	mpCPUProfiler->BeginEntry("EngineLoad");
	Log::Info("-------------------- LOADING ENVIRONMENT MAPS --------------------- ");
	Skybox::InitializePresets(mpRenderer, rendererSettings);
	Log::Info("-------------------- ENVIRONMENT MAPS LOADED IN %.2fs. --------------------", mpTimer->StopGetDeltaTimeAndReset());

	// SCENE INITIALIZATION
	//
	Log::Info("-------------------- LOADING SCENE --------------------- ");
	mpTimer->Start();
	const size_t numScenes = sEngineSettings.sceneNames.size();
	assert(sEngineSettings.levelToLoad < numScenes);
#if defined(_DEBUG) && (OVERRIDE_LEVEL_LOAD > 0)
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
		mShadowMapPass.InitializeSpotLightShadowMaps(mpRenderer, rendererSettings.shadowMap);
		//renderer->m_Direct3D->ReportLiveObjects();
		
		mDeferredRenderingPasses.Initialize(mpRenderer);
		mPostProcessPass.Initialize(mpRenderer, rendererSettings.postProcess);
		mDebugPass.Initialize(mpRenderer);
		mSSAOPass.Initialize(mpRenderer);
	}
	Log::Info("---------------- INITIALIZING RENDER PASSES DONE IN %.2fs ---------------- ", mpTimer->StopGetDeltaTimeAndReset());
	mpCPUProfiler->EndEntry();
	mpCPUProfiler->EndProfile();
	mpCPUProfiler->BeginProfile();

	return true;
#endif
}


bool Engine::LoadSceneFromFile()
{
	mCurrentLevel = sEngineSettings.levelToLoad;
	SerializedScene mSerializedScene = Parser::ReadScene(mpRenderer, sEngineSettings.sceneNames[mCurrentLevel]);
	if (mSerializedScene.loadSuccess == '0')
	{
		Log::Error("Scene[%d] did not load.", mCurrentLevel);
		return false;
	}

	assert(mCurrentLevel < mpScenes.size());
	mpActiveScene = mpScenes[mCurrentLevel];
	mpActiveScene->mpThreadPool = mpThreadPool;

	mpActiveScene->LoadScene(mSerializedScene, sEngineSettings.window, mBuiltinMeshes);

	// update engine settings and post process settings
	// todo: multiple data - inconsistent state -> sort out ownership
	sEngineSettings.rendering.postProcess.bloom = mSerializedScene.settings.bloom;
	mPostProcessPass._settings = sEngineSettings.rendering.postProcess;
	if (mpActiveScene->mDirectionalLight.enabled)
	{
		// #AsyncLoad: Mutex DEVICE
		std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
		mShadowMapPass.InitializeDirectionalLightShadowMap(mpRenderer, mpActiveScene->mDirectionalLight.GetSettings());
	}
	return true;
}

bool Engine::LoadScene(int level)
{
#if LOAD_ASYNC
	mbLoading = true;
	Log::Info("LoadScene: %d", level);
	auto loadFn = [&, level]()
	{
		{
			std::unique_lock<std::mutex> lck(mLoadRenderingMutex);
			mpActiveScene->UnloadScene();
		}
		Engine::sEngineSettings.levelToLoad = level;
		bool bLoadSuccess = LoadSceneFromFile();
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

void Engine::Exit()
{
	mpCPUProfiler->EndProfile();
	mpGPUProfiler->Exit();
	mUI.Exit();
	mpTextRenderer->Exit();
	mpRenderer->Exit();
	mBuiltinMeshes.clear();

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
		
		mpGPUProfiler->EndEntry();
		mpGPUProfiler->EndProfile(mFrameCount);

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

	if (mpInput->IsKeyTriggered("F1")) ToggleRenderingPath();
	if (mpInput->IsKeyTriggered("F2")) ToggleAmbientOcclusion();
	if (mpInput->IsKeyTriggered("F3")) ToggleBloom();
	if (mpInput->IsKeyTriggered("F4")) mEngineConfig.bRenderTargets = !mEngineConfig.bRenderTargets;
	if (mpInput->IsKeyTriggered("F5")) mEngineConfig.bBoundingBoxes = !mEngineConfig.bBoundingBoxes;

	if (mpInput->IsKeyTriggered("'")) ToggleControlsTextRendering();
	if (mpInput->IsKeyTriggered("F"))// && mpInput->AreKeysDown(2, "ctrl", "shift"))
	{
		if(mpInput->IsKeyDown("ctrl") && mpInput->IsKeyDown("shift"))
			ToggleProfilerRendering();
	}

	// The following input will not be handled if the engine is currently loading a level
	//
	if (!mbLoading)
	{
		if (mpInput->IsKeyTriggered("\\")) mpRenderer->ReloadShaders();

#if SSAO_DEBUGGING
		// todo: wire this to some UI text/control
		if (mEngineConfig.bSSAO)
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
#endif

#if BLOOM_DEBUGGING
		if (mEngineConfig.bBloom)
		{
			Settings::Bloom& bloom = mPostProcessPass._settings.bloom;
			float& threshold = bloom.brightnessThreshold;
			const float step = 0.05f;
			const float threshold_hi = 3.0f;
			const float threshold_lo = 0.05f;
			if (mpInput->IsScrollUp()) { threshold += step; if (threshold > threshold_hi) threshold = threshold_hi; Log::Info("Bloom Brightness Cutoff Threshold: %.2f", threshold); }
			if (mpInput->IsScrollDown()) { threshold -= step; if (threshold < threshold_lo) threshold = threshold_lo; Log::Info("Bloom Brightness Cutoff Threshold: %.2f", threshold); }
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


		// index using enums. first element of environment map presets starts with cubemap preset count, as if both lists were concatenated.
		const EEnvironmentMapPresets firstPreset = static_cast<EEnvironmentMapPresets>(CUBEMAP_PRESET_COUNT);
		const EEnvironmentMapPresets lastPreset = static_cast<EEnvironmentMapPresets>(
			static_cast<EEnvironmentMapPresets>(CUBEMAP_PRESET_COUNT) + ENVIRONMENT_MAP_PRESET_COUNT - 1
			);

		EEnvironmentMapPresets selectedEnvironmentMap = mpActiveScene->GetActiveEnvironmentMapPreset();
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
	const int fps = static_cast<int>(1.0f / frameTime);


	if (frameCount % RefreshRate == 0)
	{
		mCurrentFrameTime = frameTime;
		float frameTimeGPU = mpGPUProfiler->GetEntryAvg("GPU");

		std::ostringstream stats;
		stats.precision(2);
		stats << std::fixed;
		stats << "VQEngine | " << "CPU: " << frameTime * 1000.0f << " ms  GPU: " << frameTimeGPU * 1000.f << " ms | FPS: ";
		stats.precision(4);
		stats << fps;
		SetWindowText(mpRenderer->GetWindow(), stats.str().c_str());
	}

	++frameCount;
}

const Settings::Engine& Engine::ReadSettingsFromFile()
{
	sEngineSettings = Parser::ReadSettings("EngineSettings.ini");
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
	const vec2 directionalShadowMapDimensions
		= vec2(mShadowMapPass.mShadowViewPort_Directional.Width, mShadowMapPass.mShadowViewPort_Directional.Height);
	mpRenderer->SetConstantStruct("Lights", &mSceneLightData._cb);
	mpRenderer->SetConstant2f("spotShadowMapDimensions", vec2(shadowDimension, shadowDimension));
	//mpRenderer->SetConstant2f("directionalShadowMapDimensions", directionalShadowMapDimensions);
	mpRenderer->SetConstant1f("directionalShadowMapDimension", directionalShadowMapDimensions.x());
	mpRenderer->SetConstant1f("directionalDepthBias", mpActiveScene->mDirectionalLight.depthBias);

	// SHADOW MAPS
	//
	mpRenderer->SetTextureArray("texSpotShadowMaps", mShadowMapPass.mShadowMapTextures_Spot);
	if (mShadowMapPass.mShadowMapTexture_Directional != -1)
		mpRenderer->SetTextureArray("texDirectionalShadowMaps", mShadowMapPass.mShadowMapTexture_Directional);

#ifdef _DEBUG
	const SceneLightingData::cb& cb = mSceneLightData._cb;	// constant buffer shorthand
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

	// gather scene lights
	//mpCPUProfiler->BeginEntry("Gather Lights");
	mpActiveScene->GatherLightData(mSceneLightData);
	//mpCPUProfiler->EndEntry();

	// TODO: #RenderPass or Scene should manage this.
	// mTBNDrawObjects.clear();
	// std::vector<const GameObject*> objects;
	// mpActiveScene->GetSceneObjects(objects);
	// for (const GameObject* obj : objects)
	// {
	// 	if (obj->mRenderSettings.bRenderTBN)
	// 		mTBNDrawObjects.push_back(obj);
	// }


	mpActiveScene->PreRender(mpCPUProfiler, mFrameStats);
	mFrameStats.rstats = mpRenderer->GetRenderStats();
	mFrameStats.fps = static_cast<int>(1.0f / mpGPUProfiler->GetRootEntryAvg());

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

	// SHADOW MAPS
	//------------------------------------------------------------------------
	mpCPUProfiler->BeginEntry("Shadow Pass");
	mpGPUProfiler->BeginEntry("Shadow Pass");
	mpRenderer->BeginEvent("Shadow Pass");
	
	mpRenderer->UnbindRenderTargets();	// unbind the back render target | every pass has their own render targets
	mShadowMapPass.RenderShadowMaps(mpRenderer, mpActiveScene->mShadowView, mpGPUProfiler);
	
	mpRenderer->EndEvent();
	mpCPUProfiler->EndEntry();
	mpGPUProfiler->EndEntry();

	// LIGHTING PASS
	//------------------------------------------------------------------------
	mpRenderer->ResetPipelineState();
	mpRenderer->SetRasterizerState(static_cast<int>(EDefaultRasterizerState::CULL_NONE));
	mpRenderer->SetViewport(mpRenderer->WindowWidth(), mpRenderer->WindowHeight());

	//==========================================================================
	// DEFERRED RENDERER
	//==========================================================================
	if (mEngineConfig.bDeferredOrForward)
	{
		const GBuffer& gBuffer = mDeferredRenderingPasses._GBuffer;
		const TextureID texNormal = mpRenderer->GetRenderTargetTexture(gBuffer._normalRT);
		const TextureID texDiffuseRoughness = mpRenderer->GetRenderTargetTexture(gBuffer._diffuseRoughnessRT);
		const TextureID texSpecularMetallic = mpRenderer->GetRenderTargetTexture(gBuffer._specularMetallicRT);
		const TextureID texDepthTexture = mpRenderer->mDefaultDepthBufferTexture;
		const TextureID tSSAO = mEngineConfig.bSSAO && bSceneSSAO
			? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget)
			: mSSAOPass.whiteTexture4x4;
		const DeferredRenderingPasses::RenderParams deferredLightingParams = 
		{
			mpRenderer
			, mPostProcessPass._worldRenderTarget
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
		mpCPUProfiler->BeginEntry("SSAO Pass");
		mpGPUProfiler->BeginEntry("SSAO");
		if (mEngineConfig.bSSAO && bSceneSSAO)
		{
			// TODO: if BeginEntry() is inside, it's never reset to 0 if ambient occl is turned off
			mpGPUProfiler->BeginEntry("Occlusion");
			mpRenderer->BeginEvent("Ambient Occlusion Pass");
			mSSAOPass.RenderOcclusion(mpRenderer, texNormal, mpActiveScene->mSceneView);
			//m_SSAOPass.BilateralBlurPass(m_pRenderer);	// todo
			mpGPUProfiler->EndEntry();

			mpGPUProfiler->BeginEntry("Blur");
			mSSAOPass.GaussianBlurPass(mpRenderer);
			mpRenderer->EndEvent();	
			mpGPUProfiler->EndEntry();
		}
		mpCPUProfiler->EndEntry();
		mpGPUProfiler->EndEntry();

		// DEFERRED LIGHTING PASS
		mpCPUProfiler->BeginEntry("Lighting Pass");
		mpGPUProfiler->BeginEntry("Lighting Pass");
		mpRenderer->BeginEvent("Lighting Pass");
#if ENABLE_TRANSPARENCY
		mpGPUProfiler->BeginEntry("Opaque Pass (ScreenSpace)");
		mpCPUProfiler->BeginEntry("Opaque Pass (ScreenSpace)");
#endif
		{
			mDeferredRenderingPasses.RenderLightingPass(deferredLightingParams);
		}
#if ENABLE_TRANSPARENCY
		mpCPUProfiler->EndEntry();
		mpGPUProfiler->EndEntry();
#endif

#if ENABLE_TRANSPARENCY
		// TRANSPARENT OBJECTS - FORWARD RENDER
		mpGPUProfiler->BeginEntry("Alpha Pass (Forward)");
		mpCPUProfiler->BeginEntry("Alpha Pass (Forward)");
		{
			mpRenderer->BindDepthTarget(GetWorldDepthTarget());
			mpRenderer->SetShader(EShaders::FORWARD_BRDF);
			mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_WRITE);
			//mpRenderer->SetBlendState(EDefaultBlendState::ADDITIVE_COLOR);


			mpRenderer->SetConstant1f("ambientFactor", mSceneView.sceneRenderSettings.ambientFactor);
			mpRenderer->SetConstant3f("cameraPos", mSceneView.cameraPosition);
			mpRenderer->SetConstant2f("screenDimensions", mpRenderer->GetWindowDimensionsAsFloat2());
			const TextureID texIrradianceMap = mSceneView.environmentMap.irradianceMap;
			const SamplerID smpEnvMap = mSceneView.environmentMap.envMapSampler < 0 ? EDefaultSamplerState::POINT_SAMPLER : mSceneView.environmentMap.envMapSampler;
			const TextureID prefilteredEnvMap = mSceneView.environmentMap.prefilteredEnvironmentMap;
			const TextureID tBRDFLUT = mpRenderer->GetRenderTargetTexture(EnvironmentMap::sBRDFIntegrationLUTRT);
			const bool bSkylight = mSceneView.bIsIBLEnabled && texIrradianceMap != -1;
			if (bSkylight)
			{
				mpRenderer->SetTexture("tIrradianceMap", texIrradianceMap);
				mpRenderer->SetTexture("tPreFilteredEnvironmentMap", prefilteredEnvMap);
				mpRenderer->SetTexture("tBRDFIntegrationLUT", tBRDFLUT);
				mpRenderer->SetSamplerState("sEnvMapSampler", smpEnvMap);
			}

			mpRenderer->SetConstant1f("isEnvironmentLightingOn", bSkylight ? 1.0f : 0.0f);
			mpRenderer->SetSamplerState("sWrapSampler", EDefaultSamplerState::WRAP_SAMPLER);
			mpRenderer->SetSamplerState("sNearestSampler", EDefaultSamplerState::POINT_SAMPLER);
			mpRenderer->SetSamplerState("sLinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);

			mpRenderer->Apply();
			mFrameStats.numSceneObjects += mpActiveScene->RenderAlpha(mSceneView);
			
		}
		mpCPUProfiler->EndEntry();
		mpGPUProfiler->EndEntry();
#endif
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
			mpActiveScene->RenderSkybox(mpActiveScene->mSceneView.viewProj);
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
			? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget) 
			: mSSAOPass.whiteTexture4x4;
		const TextureID texIrradianceMap = mpActiveScene->mSceneView.environmentMap.irradianceMap;
		const SamplerID smpEnvMap = mpActiveScene->mSceneView.environmentMap.envMapSampler < 0 
			? EDefaultSamplerState::POINT_SAMPLER 
			: mpActiveScene->mSceneView.environmentMap.envMapSampler;
		const TextureID prefilteredEnvMap = mpActiveScene->mSceneView.environmentMap.prefilteredEnvironmentMap;
		const TextureID tBRDFLUT = EnvironmentMap::sBRDFIntegrationLUTTexture;
		const RenderTargetID renderTarget = mPostProcessPass._worldRenderTarget;

		// AMBIENT OCCLUSION - Z-PREPASS
		if (bZPrePass)
		{
			const RenderTargetID normals = mDeferredRenderingPasses._GBuffer._normalRT;
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
				mpGPUProfiler->BeginEntry("SSAO");

				mpGPUProfiler->BeginEntry("Occlusion");
				mSSAOPass.RenderOcclusion(mpRenderer, texNormal, mpActiveScene->mSceneView);
				mpGPUProfiler->EndEntry();	// Occlusion

				mpGPUProfiler->BeginEntry("Blur");
				//m_SSAOPass.BilateralBlurPass(m_pRenderer);	// todo
				mSSAOPass.GaussianBlurPass(mpRenderer);
				mpGPUProfiler->EndEntry(); // Blur
				mpGPUProfiler->EndEntry(); // SSAO
			}
			mpRenderer->EndEvent(); // Ambient Occlusion Pass
		}

		const bool bDoClearColor = true;
		const bool bDoClearDepth = !bZPrePass;
		const bool bDoClearStencil = true;
		ClearCommand clearCmd(
			bDoClearColor, bDoClearDepth, bDoClearStencil,
			{ 0, 0, 0, 0 }, 1, 0
		);

		mpRenderer->BindRenderTarget(renderTarget);
		mpRenderer->BindDepthTarget(mWorldDepthTarget);
		mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_WRITE);
		mpRenderer->BeginRender(clearCmd);

		// SKYBOX
		// if we're not rendering the skybox, call apply() to unbind
		// shadow light depth target so we can bind it in the lighting pass
		// otherwise, skybox render pass will take care of it
		if (mpActiveScene->HasSkybox())	mpActiveScene->RenderSkybox(mpActiveScene->mSceneView.viewProj);
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
			, renderTarget
		};
		mForwardLightingPass.RenderLightingPass(forwardLightingParams);
	}

	RenderLights();
	mpRenderer->SetBlendState(EDefaultBlendState::DISABLED);

	// POST PROCESS PASS | DEBUG PASS | UI PASS
	//------------------------------------------------------------------------
	mpCPUProfiler->BeginEntry("Post Process");
	mpGPUProfiler->BeginEntry("Post Process"); 
	mPostProcessPass.Render(mpRenderer, mEngineConfig.bBloom);
	mpCPUProfiler->EndEntry();
	mpGPUProfiler->EndEntry();
	//------------------------------------------------------------------------
	RenderDebug(viewProj);
	RenderUI();
	//------------------------------------------------------------------------
	mpCPUProfiler->EndEntry();	// Render() call

}

void Engine::RenderDebug(const XMMATRIX& viewProj)
{
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
		const int paddingPx = 0;	 // padding between debug textures
		const vec2 fullscreenTextureScaledDownSize((float)heightPx * aspectRatio, (float)heightPx);
		const vec2 squareTextureScaledDownSize((float)heightPx, (float)heightPx);

		// Textures to draw
		const TextureID white4x4 = mSSAOPass.whiteTexture4x4;
		//TextureID tShadowMap		 = mpRenderer->GetDepthTargetTexture(mShadowMapPass._spotShadowDepthTargets);
		TextureID tBlurredBloom = mpRenderer->GetRenderTargetTexture(mPostProcessPass._bloomPass._blurPingPong[0]);
		TextureID tDiffuseRoughness = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer._diffuseRoughnessRT);
		//TextureID tSceneDepth		 = m_pRenderer->m_state._depthBufferTexture._id;
		TextureID tSceneDepth = mpRenderer->GetDepthTargetTexture(0);
		TextureID tNormals = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer._normalRT);
		TextureID tAO = mEngineConfig.bSSAO ? mpRenderer->GetRenderTargetTexture(mSSAOPass.blurRenderTarget) : mSSAOPass.whiteTexture4x4;
		TextureID tBRDF = EnvironmentMap::sBRDFIntegrationLUTTexture;
		TextureID preFilteredEnvMap = mpActiveScene->GetEnvironmentMap().prefilteredEnvironmentMap;
		preFilteredEnvMap = preFilteredEnvMap < 0 ? white4x4 : preFilteredEnvMap;
		TextureID tDirectionalShadowMap = (mShadowMapPass.mDepthTarget_Directional == -1 || mpActiveScene->mDirectionalLight.enabled == 0)
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
			{ fullscreenTextureScaledDownSize,	screenPosition,			tAO					, false },
			{ squareTextureScaledDownSize,		screenPosition,			tBRDF				, false },
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
}


void Engine::RenderUI() const
{
	mpGPUProfiler->BeginEntry("UI");
	mpCPUProfiler->BeginEntry("UI");
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_NONE);	
	if (mEngineConfig.mbShowProfiler) { mUI.RenderPerfStats(mFrameStats); }
	if (mEngineConfig.mbShowControls) { mUI.RenderEngineControls(); }
	if (!mbLoading)
	{
		if (!mpActiveScene)
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
}

void Engine::RenderLights() const
{
	mpRenderer->BeginEvent("Render Lights Pass");
	mpRenderer->SetShader(EShaders::UNLIT);
	mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_TEST_ONLY);
	for (const Light& light : mpActiveScene->mLights)
	{
		//if (!light._bEnabled) continue; // #BreaksRelease
		
		if (light.type == Light::ELightType::DIRECTIONAL)
			continue;	// do not render directional lights

		const auto IABuffers = mBuiltinMeshes[light.renderMesh].GetIABuffers();
		const XMMATRIX world = light.transform.WorldTransformationMatrix();
		const XMMATRIX worldViewProj = world * mpActiveScene->mSceneView.viewProj;
		const vec3 color = light.color.Value() * 2.5f;

		mpRenderer->SetVertexBuffer(IABuffers.first);
		mpRenderer->SetIndexBuffer(IABuffers.second);
		mpRenderer->SetConstant4x4f("worldViewProj", worldViewProj);
		mpRenderer->SetConstant3f("diffuse", color);
		mpRenderer->SetConstant1f("isDiffuseMap", 0.0f);
		mpRenderer->Apply();
		mpRenderer->DrawIndexed();
	}
	mpRenderer->EndEvent();
}

void Engine::RenderLoadingScreen(bool bOneTimeRender) const
{
	const TextureID texLoadingScreen = mLoadingScreenTextures.back();
	const XMMATRIX matTransformation = XMMatrixIdentity();
	const auto IABuffers = mBuiltinMeshes[EGeometry::FULLSCREENQUAD].GetIABuffers();

	if (bOneTimeRender)
	{
		mpRenderer->BeginFrame();
	}
	else
	{	
		mpGPUProfiler->BeginEntry("Loading Screen");
	}

	mpRenderer->SetShader(EShaders::UNLIT);
	mpRenderer->BindRenderTarget(0);
	mpRenderer->SetVertexBuffer(IABuffers.first);
	mpRenderer->SetIndexBuffer(IABuffers.second);
	mpRenderer->SetTexture("texDiffuseMap", texLoadingScreen);
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
