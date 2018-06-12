//	VQEngine | DirectX11 Renderer
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

#pragma once

#include <windows.h>

#include "Utilities/PerfTimer.h"
#include "Utilities/Profiler.h"

#include "Application/WorkerPool.h"

#include "Light.h"
#include "Mesh.h"
#include "DataStructures.h"
#include "Skybox.h"
#include "Settings.h"
#include "RenderPasses.h"

#include <memory>

using std::shared_ptr;
using std::unique_ptr;

class Renderer;
class TextRenderer;
class Input;
class Parser;

class CPUProfiler;
class GPUProfiler;
class PerfTimer;

class Scene;
class ObjectsScene;
class SSAOTestScene;
class IBLTestScene;
class StressTestScene;


#ifdef _WIN32
// usage of XMMATRIX in Engine class causes alignment warning: 
// > Engine might not be on 16-byte boundary. 
// To fix this, we declare that we want to align the Engine class to 16-byte boundary.
// We also override new/delete functions to allocate and free aligned memory
#define ALIGNMENT __declspec(align(16))
#else
#define ALIGNMENT
#endif

ALIGNMENT class Engine
{
	friend class BaseSystem;
	friend class SceneManager;	// read/write ZPassObjects

public:	
	//----------------------------------------------------------------------------------------------------------------
	// STATIC INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	static const Settings::Engine&	ReadSettingsFromFile();
	static const Settings::Engine&	GetSettings() { return sEngineSettings; }
	static Engine*					GetEngine();

	~Engine();

#ifdef _WIN32
	void*					operator new(size_t size) { return _mm_malloc(size, 16); }
	void					operator delete(void* p)  { _mm_free(p); }
#endif
	//----------------------------------------------------------------------------------------------------------------
	// CORE INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	bool					Initialize(HWND hwnd);
	void					Exit();
	
	bool					Load();
	bool					LoadSceneFromFile();
	bool					LoadScene(int level);
	bool					ReloadScene();

	void					SendLightData() const;
	bool					UpdateAndRender();
	void					Render();

	void					Pause();
	void					Unpause();
	
	//----------------------------------------------------------------------------------------------------------------
	// GETTERS
	//----------------------------------------------------------------------------------------------------------------
	inline const Input*		INP() const { return mpInput; }
	float					GetTotalTime() const;
	inline ShaderID			GetSelectedShader() const { return mSelectedShader; }
	inline DepthTargetID	GetWorldDepthTarget() const { return mWorldDepthTarget; }
	inline bool				GetSettingShowControls() const { return mbShowControls; }
	bool					IsLightingModelPBR() const { return sEngineSettings.rendering.bUseBRDFLighting; }
	bool inline				IsRenderingStatsOn() const { return mFrameStats.bShow; }
	bool inline				IsProfileRenderingOn() const { return mbShowProfiler; }
	std::pair<BufferID, BufferID> GetGeometryVertexAndIndexBuffers(EGeometry GeomEnum) const { return mBuiltinMeshes[GeomEnum].GetIABuffers(); }

	//----------------------------------------------------------------------------------------------------------------
	// TOGGLES
	//----------------------------------------------------------------------------------------------------------------
	void					ToggleRenderingPath();	// Forward / Deferred
	void					ToggleAmbientOcclusion();
	void inline				ToggleProfilerRendering() { mbShowProfiler = !mbShowProfiler; }
	void inline				ToggleControlsTextRendering() { mbShowControls = !mbShowControls; }
	void inline				ToggleRenderingStats() { mFrameStats.bShow = !mFrameStats.bShow; }

private:
	Engine();
	inline void TogglePause() { mbIsPaused = !mbIsPaused; }

	void CalcFrameStats(float dt);
	bool HandleInput();

	// prepares rendering context: gets data from scene and sets up data structures ready to be sent to GPU
	void PreRender();
	void RenderLoadingScreen() const;
	void RenderLights() const;

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
	Input*							mpInput;
	Renderer*						mpRenderer;
	TextRenderer*					mpTextRenderer;
	PerfTimer*						mpTimer;
	CPUProfiler*					mpCPUProfiler;
	GPUProfiler*					mpGPUProfiler;
	float mCurrentFrameTime;
	
	WorkerPool						mWorkerPool;//(UNSUED)

	//----------------------------------------------------------------------------------------------------------------
	// SCENE
	//----------------------------------------------------------------------------------------------------------------
	Scene*							mpActiveScene;
	SceneView						mSceneView;
	ShadowView						mShadowView;
	SceneLightingData				mSceneLightData;	// more memory than required?

	// #SceneRefactoring
	// current design for adding new scenes is as follows (and is horrible...):
	// - add the .scn scene file to Data/Levels directory
	// - Create a class for your scene and inherit from Scene base class 
	//    - use the .BAT file with scene name as an argument in the Scenes directory
	// - edit settings.ini to start with your scene
	// - remember to add scene name in engine.cpp::LoadScene
	int								mCurrentLevel;
	ObjectsScene*					mpObjectsScene;
	SSAOTestScene*					mpSSAOTestScene;
	IBLTestScene*					mpIBLTestScene;
	StressTestScene*				mpStressTestScene;

	bool							mbUsePaniniProjection;//(UNUSED)
	std::vector<Mesh>				mBuiltinMeshes;

	//----------------------------------------------------------------------------------------------------------------
	// RENDERING 
	//----------------------------------------------------------------------------------------------------------------
	ShaderID						mSelectedShader;
	
	DepthTargetID					mWorldDepthTarget;
	SamplerID						mNormalSampler;

	// keeping passes around this doesn't make sense... todo refactor here.
	ShadowMapPass					mShadowMapPass;
	DeferredRenderingPasses			mDeferredRenderingPasses;
	AmbientOcclusionPass			mSSAOPass;
	PostProcessPass					mPostProcessPass;
	DebugPass						mDebugPass;

	std::vector<const GameObject*>	mZPassObjects;
	std::vector<const GameObject*>	mTBNDrawObjects;

	//----------------------------------------------------------------------------------------------------------------
	// ENGINE STATE
	//----------------------------------------------------------------------------------------------------------------
	FrameStats						mFrameStats;
	bool							mbIsPaused;
	bool							mbShowProfiler;
	bool							mbShowControls;

	bool							mbUseDeferredRendering;	// todo read from sceneview???
	bool							mbIsAmbientOcclusionOn;	// todo read from sceneview???
	bool							mDisplayRenderTargets;	// todo read from sceneview???
	unsigned long long				mFrameCount;
};

#define ENGINE Engine::GetEngine()
