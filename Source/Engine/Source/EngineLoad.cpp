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


#include "Engine.h"
#include "Scene.h"

#include "Application/Application.h"

#include "Renderer/Renderer.h"

#include "Utilities/Log.h"
#include "Utilities/CustomParser.h"


#define OVERRIDE_LEVEL_LOAD      1	// Toggle for overriding level loading
// 0: objects 
// 1: ssao test 
// 2: ibl test
// 3: stress test
// 4: sponza
// 5: lights
// 6: lod test
#define OVERRIDE_LEVEL_VALUE     6	// which level to load



bool Engine::Load()
{
	Log::Info("[ENGINE]: Loading -------------------------");
	mbLoading = true;
	mpCPUProfiler->BeginProfile();

#if USE_DX12
	
	mSimulationWorkers.AddTask([this]()
	{
		Scene::InitializeBuiltinMeshes();
		this->mpRenderer->GPUFlush_UploadHeap();
	});
	mSimulationWorkers.AddTask([&]()
	{
		// TODO: this could be further refined to better
		//       utilize the workers.
		mpRenderer->LoadDefaultResources();
	});


#if 0 // TEST GROUNDS
	//mSimulationWorkers.AddTask([=](){ LoadLoadingScreenTextures(); });

	mSimulationWorkers.AddTask([]()
	{
		std::this_thread::sleep_for(std::chrono::milliseconds::duration(3000));
		Log::Info("Worker done.");
		return;
	}, mEvent_LoadFinished);

	Log::Info("Started worker task. Waiting...");
	{
		std::unique_lock<std::mutex> lock(mEventMutex_LoadFinished);
		mEvent_LoadFinished.wait(lock);
	}
	Log::Info("Worker task wait has finished on main thread.");
	mEvent_FrameUpdateFinished.notify_one();
#endif

	// TODO: proper threaded loading
	mSimulationWorkers.AddTask([&]() { this->LoadLoadingScreenTextures(); }, mEvent_LoadingScreenReady);
	mSimulationWorkers.AddTask([&]()
	{

#if 0 /// TODO: rewrite the proper multithreaded loading code for the code below
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

			mDeferredRenderingPasses.Initialize(mpRenderer, sEngineSettings.rendering.antiAliasing.IsAAEnabled());
			mPostProcessPass.Initialize(mpRenderer, sEngineSettings.rendering.postProcess);
			mDebugPass.Initialize(mpRenderer);
			mAOPass.Initialize(mpRenderer);
			mAAResolvePass.Initialize(mpRenderer, mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._shadeTarget));
		}
		Log::Info("---------------- INITIALIZING RENDER PASSES DONE IN %.2fs ---------------- ", mpTimer->StopGetDeltaTimeAndReset());
		mpCPUProfiler->EndEntry();
		mpCPUProfiler->EndProfile();
		mpCPUProfiler->BeginProfile();
		Log::Info("[ENGINE]: Loaded (Serial) ------------------");
		return true;
#endif

	});

	return true;

#else // USE_DX12
	mpThreadPool.StartThreads();

	LoadLoadingScreenTextures();

#if LOAD_ASYNC
	mbLoading = true;
	mAccumulator = 1.0f;		// start rendering loading screen on update loop
	RenderLoadingScreen(true);	// quickly render one frame of loading screen

	// set up a parallel task to load everything.
	mpThreadPool->AddTask([=]() { this->Load_Async(); });
#else // LOAD_ASYNC
	RenderLoadingScreen(true);
	this->Load_Serial();
#endif // LOAD_ASYNC

	ENGINE->mpTimer->Reset();
	ENGINE->mpTimer->Start();
	return true;

#endif // USE_DX12
}

#if !USE_DX12
bool Engine::Load_Async()
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
}

bool Engine::Load_Serial()
{
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

		mDeferredRenderingPasses.Initialize(mpRenderer, sEngineSettings.rendering.antiAliasing.IsAAEnabled());
		mPostProcessPass.Initialize(mpRenderer, sEngineSettings.rendering.postProcess);
		mDebugPass.Initialize(mpRenderer);
		mAOPass.Initialize(mpRenderer);
		mAAResolvePass.Initialize(mpRenderer, mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._shadeTarget));
	}
	Log::Info("---------------- INITIALIZING RENDER PASSES DONE IN %.2fs ---------------- ", mpTimer->StopGetDeltaTimeAndReset());
	mpCPUProfiler->EndEntry();
	mpCPUProfiler->EndProfile();
	mpCPUProfiler->BeginProfile();
	Log::Info("[ENGINE]: Loaded (Serial) ------------------");
	return true;
}
#endif

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

void Engine::LoadLoadingScreenTextures()
{
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
	mpActiveScene->mpThreadPool = &mSimulationWorkers;

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
	mSimulationWorkers.AddTask(loadFn);
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