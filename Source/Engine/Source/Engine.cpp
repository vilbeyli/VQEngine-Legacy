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

#define FULLSCREEN_DEBUG_TEXTURE 1

#include "Engine.h"

#include "Application/Application.h"
#include "Application/ThreadPool.h"

#include "Utilities/Log.h"
#include "Utilities/CustomParser.h"

#include "Renderer/Renderer.h"
#include "Renderer/Shader.h"

// required for initializing (pointers to scene classes)
#include "Scenes/ObjectsScene.h"
#include "Scenes/SSAOTestScene.h"
#include "Scenes/PBRScene.h"
#include "Scenes/StressTestScene.h"
#include "Scenes/SponzaScene.h"
#include "Scenes/LightsScene.h"
#include "Scenes/LODTestScene.h"

using namespace VQEngine;

Settings::Engine Engine::sEngineSettings;
Engine* Engine::sInstance = nullptr;



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
	, mSimulationWorkers(std::max(1ull, (VQEngine::ThreadPool::sHardwareThreadCount >> 1) - 1))
	, mRenderWorkers    (std::max(1ull, (VQEngine::ThreadPool::sHardwareThreadCount >> 1) - 1))
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

void Engine::Exit()
{
#if USE_DX12
	StopThreads();
#endif

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



// ====================================================================================
// TOGGLES & UTILITIES
// ====================================================================================
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


Engine * Engine::GetEngine()
{
	if (sInstance == nullptr) { sInstance = new Engine(); }
	return sInstance;
}
const Settings::Engine& Engine::ReadSettingsFromFile()
{
	sEngineSettings = Parser::ReadSettings("Data/EngineSettings.ini");
	return sEngineSettings;
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

