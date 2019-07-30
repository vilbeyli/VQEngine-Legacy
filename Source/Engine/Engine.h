//	VQEngine | DirectX11 Renderer
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

#pragma once


// Currently only forward_brdf shader has the implementation. The output
// requires more tweaking and has artifacts. Switching this off until
// its fixed.
//
#define ENABLE_PARALLAX_MAPPING 0

// Uses a thread pool of worker threads to load scene models
// while utilizing a render thread to render loading screen
//
#define LOAD_ASYNC 1


#include "Utilities/PerfTimer.h"
#include "Utilities/Profiler.h"

#include "RenderPasses/RenderPasses.h"

#include "Light.h"
#include "Mesh.h"
#include "DataStructures.h"
#include "Skybox.h"
#include "Settings.h"
#include "UI.h"

#include "Application/Input.h"

#include <memory>
#include <atomic>
#include <mutex>
#include <queue>

using std::shared_ptr;
using std::unique_ptr;

#if USE_DX12
#include "Application/ThreadPool.h"
#else
namespace VQEngine { class ThreadPool; }
#endif

class Renderer;
class TextRenderer;
class Input;
class Parser;

class CPUProfiler;
class GPUProfiler;
class PerfTimer;

class Scene;


//
// TODO: CleanUp
//--------------------------------------------------------------
#if 0
	class OS
	{
		Input mInput;
		ThreadPool mThreadSystem; // ?
		Window mWindow;
	};
}
#endif
#ifndef _WIN64
// usage of XMMATRIX in Engine class causes alignment warning: 
// > Engine might not be on 16-byte boundary. 
// To fix this, we declare that we want to align the Engine class to 16-byte boundary.
// We also override new/delete functions to allocate and free aligned memory
#define ALIGNMENT __declspec(align(16))
#else
#define ALIGNMENT
#endif
//--------------------------------------------------------------

ALIGNMENT class Engine
{
	friend class Application;
	friend struct ShadowMapPass;	//temporary
	friend class SceneManager;	// read/write ZPassObjects


public:	
	//----------------------------------------------------------------------------------------------------------------
	// STATIC INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	static const Settings::Engine&	ReadSettingsFromFile();
	static const Settings::Engine&	GetSettings() { return sEngineSettings; }
	static Engine*					GetEngine();

	~Engine();

#ifndef _WIN64
	void*					operator new(size_t size) { return _mm_malloc(size, 16); }
	void					operator delete(void* p)  { _mm_free(p); }
#endif
	//----------------------------------------------------------------------------------------------------------------
	// CORE INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	bool			Initialize(Application* pApplication);
	void			Exit();
	
	bool			Load();
	void			SimulateAndRenderFrame();

	void			SendLightData() const; // TODO: remove this function
	inline void		Pause()  { mbIsPaused = true; }
	inline void		Unpause(){ mbIsPaused = false; }
	

	//----------------------------------------------------------------------------------------------------------------
	// OS EVENTS
	//----------------------------------------------------------------------------------------------------------------
	void OnKeyDown(KeyCode key);
	void OnKeyUp(KeyCode key);
	void OnMouseMove(long x, long y, short scroll);
	void OnMouseDown(const Input::EMouseButtons& btn);
	void OnMouseUp(const Input::EMouseButtons& btn);
	void OnWindowGainFocus();
	void OnWindowLoseFocus();
	void OnWindowMove();
	void OnWindowResize();


	//----------------------------------------------------------------------------------------------------------------
	// GETTERS
	//----------------------------------------------------------------------------------------------------------------
	inline const Input*		INP() const { return mpInput; }
	inline bool				IsMouseCaptured() const { return mbMouseCaptured; }

	int						GetFPS() const;
	float					GetTotalTime() const;
	inline ShaderID			GetSelectedShader() const { return mSelectedShader; }
	inline DepthTargetID	GetWorldDepthTarget() const { return mWorldDepthTarget; }
	inline bool				GetSettingShowControls() const { return mEngineConfig.mbShowControls; }

	bool					IsLightingModelPBR() const { return sEngineSettings.rendering.bUseBRDFLighting; }
	bool inline				IsProfileRenderingOn() const { return mEngineConfig.mbShowProfiler; }
	bool inline				IsLoading() const { return mbLoading; }
	
	//----------------------------------------------------------------------------------------------------------------
	// TOGGLES
	//----------------------------------------------------------------------------------------------------------------
	void					ToggleRenderingPath();	// Forward / Deferred
	void					ToggleAmbientOcclusion();
	void					ToggleBloom(); 
	void inline				ToggleProfilerRendering() { mEngineConfig.mbShowProfiler = !mEngineConfig.mbShowProfiler; }
	void inline				ToggleControlsTextRendering() { mEngineConfig.mbShowControls = !mEngineConfig.mbShowControls; }
	void inline				TogglePause() { mbIsPaused = !mbIsPaused; }

private:
	Engine();
	void CalcFrameStats(float dt);
	void HandleInput();

	//----------------------------------------------------------------------------------------------------------------
	// THREADING
	//----------------------------------------------------------------------------------------------------------------
#if USE_DX12
	// Starts the Rendering, Loading and Update threads.
	//
	void StartThreads();

	// Signals Rendering, Loading and Update threads to finalize and calls join on them.
	// Blocks the calling thread until the three threads are all finished.
	//
	void StopThreads();
#else  // USE_DX12
	// Starts the rendering thread when loading scenes
	//
	void StartRenderThread();

	// Signals render thread to stop and blocks the current thread until the render thread joins
	//
	void StopRenderThreadAndWait();
#endif // USE_DX12

	//----------------------------------------------------------------------------------------------------------------
	// LOADING
	//----------------------------------------------------------------------------------------------------------------
#if USE_DX12
#else
	bool Load_Async();
	bool Load_Serial();
#endif
	bool LoadSceneFromFile();
	bool LoadScene(int level);
	bool LoadShaders();
	bool ReloadScene();
	void LoadLoadingScreenTextures();
	//void Load

	//----------------------------------------------------------------------------------------------------------------
	// RENDERING
	//----------------------------------------------------------------------------------------------------------------
	// prepares rendering context: gets data from scene and sets up data structures ready to be sent to GPU
	void PreRender();
	void Render();
	void RenderDebug(const XMMATRIX& viewProj);
	void RenderUI() const;
	void RenderLoadingScreen(bool bOneTimeRender) const;

//==============================================================================================================

private:
	//----------------------------------------------------------------------------------------------------------------
	// STATIC
	//----------------------------------------------------------------------------------------------------------------
	static Engine*					sInstance;
	static Settings::Engine			sEngineSettings;

	//----------------------------------------------------------------------------------------------------------------
	// SYSTEMS
	//----------------------------------------------------------------------------------------------------------------
	Application*					mpApp; // capture/release mouse, window handle etc
	Input*							mpInput;
	Renderer*						mpRenderer;
	TextRenderer*					mpTextRenderer;
	PerfTimer*						mpTimer;
	CPUProfiler*					mpCPUProfiler;
	GPUProfiler*					mpGPUProfiler;
	float mCurrentFrameTime;

private:
	//----------------------------------------------------------------------------------------------------------------
	// SCENE
	//----------------------------------------------------------------------------------------------------------------
	std::vector<Scene*>				mpScenes;
	Scene*							mpActiveScene;

	SceneLightingConstantBuffer		mSceneLightData;	// more memory than required?

	// #SceneRefactoring
	// current design for adding new scenes is as follows (and is horrible...):
	// - add the .scn scene file to Data/Levels directory
	// - Create a class for your scene and inherit from Scene base class 
	//    - use the .BAT file with scene name as an argument in the Scenes directory
	// - edit settings.ini to start with your scene
	// - remember to add scene name in engine.cpp::LoadScene
	int								mCurrentLevel;

	bool							mbUsePaniniProjection;//(UNUSED)
	std::vector<ShaderID>			mBuiltinShaders;
	std::vector<TextureID>			mLoadingScreenTextures;

	//----------------------------------------------------------------------------------------------------------------
	// RENDERING 
	//----------------------------------------------------------------------------------------------------------------
	ShaderID						mSelectedShader;
	TextureID						mActiveLoadingScreen;

	DepthTargetID					mWorldDepthTarget;

	// #RenderPassRefactoring
	// keeping passes around like this doesn't make sense...
	ShadowMapPass					mShadowMapPass;
	DeferredRenderingPasses			mDeferredRenderingPasses;
	AmbientOcclusionPass			mAOPass;
	PostProcessPass					mPostProcessPass;
	DebugPass						mDebugPass;
	ZPrePass						mZPrePass;
	ForwardLightingPass				mForwardLightingPass;
	AAResolvePass					mAAResolvePass;

	std::vector<const GameObject*>	mTBNDrawObjects;
	
	VQEngine::UI					mUI;

	//----------------------------------------------------------------------------------------------------------------
	// ENGINE STATE
	//----------------------------------------------------------------------------------------------------------------
	FrameStats			mFrameStats;
	bool				mbIsPaused;
	bool				mbMouseCaptured;
	bool				mbStartEngineShutdown;
	bool				mbOutputDebugTexture;

	EngineConfig		mEngineConfig;

	unsigned long long	mFrameCount;

	//----------------------------------------------------------------------------------------------------------------
	// THREADING
	//---------------------------------------------------------------------------------------------------------------- 
	//struct Threading
	//{
		VQEngine::ThreadPool mSimulationWorkers;
		VQEngine::ThreadPool mRenderWorkers;
	
		std::atomic<bool>	mbLoading;
		std::atomic<float>	mAccumulator;	// frame time accumulator
		std::queue<int>		mLevelLoadQueue;

#if USE_DX12

#if 0
		VQEngine::Thread mRenderThread;
		VQEngine::Thread mLoadingThread;
		VQEngine::Thread mSimulationThread;
#else
		std::thread mRenderThread;
		std::thread mSimulationThread;

		std::condition_variable mEvent_StopThreads;
		std::condition_variable mEvent_LoadFinished;
		std::condition_variable mEvent_FrameUpdateFinished;
		std::condition_variable mEvent_FrameRenderFinished;

		std::mutex mEventMutex_LoadFinished;
		std::mutex mEventMutex_FrameUpdateFinished;
		std::mutex mEventMutex_FrameRenderFinished;

		TaskQueue mEngineLoadingTaskQueue;
#endif

		bool mbStopThreads = false;
#else // USE_DX12
		bool mbStopRenderThread = false;
		std::thread mRenderThread;
		std::condition_variable mSignalRender;
#endif // USE_DX12

		// Loading screen + perf numbers rendering on own thread
		//
		void RenderThread(unsigned numWorkers);
		void SimulationThread(unsigned numWorkers);

#if USE_NEW_THREADING
#else
#endif
	//};
public:
	static std::mutex	mLoadRenderingMutex;
};

#define ENGINE Engine::GetEngine()
