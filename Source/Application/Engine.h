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

#pragma once

#include <windows.h>
#include "Utilities/PerfTimer.h"
#include "WorkerPool.h"
#include "DataStructures.h"

#include "RenderPasses.h"
#include "Renderer/Light.h"
#include "Skybox.h"

#include <memory>
using std::shared_ptr;
using std::unique_ptr;


namespace Settings { struct Rendering; }

class Renderer;
class Input;
class Parser;
class SceneManager;

class PathManager;		// unused
class PhysicsEngine;	// unused


struct ObjectPool
{
	std::vector<GameObject> pool;

};

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
	static const Settings::Engine& ReadSettingsFromFile();
	static const Settings::Engine& GetSettings() { return sEngineSettings; }
	static Engine*			GetEngine();
	~Engine();

#ifdef _WIN32
	void*					operator new(size_t size) { return _mm_malloc(size, 16); }
	void					operator delete(void* p)  { _mm_free(p); }
#endif

	bool					Initialize(HWND hwnd);
	bool					Load();
	void					Exit();


	bool					UpdateAndRender();
	void					Render();

	

	inline const Input*		INP() const { return mpInput; }
	inline float			GetTotalTime() const { return mpTimer->TotalTime(); }
	inline ShaderID			GetSelectedShader() const { return mSelectedShader; }
	void					ToggleLightingModel();	// BRDF / Blinn-Phong
	void					ToggleRenderingPath();	// Forward / Deferred
	void					ToggleAmbientOcclusion();
	bool					IsLightingModelPBR() const { return sEngineSettings.rendering.bUseBRDFLighting; }
	void					Pause();
	void					Unpause();

	void					SendLightData() const;
	
	inline DepthTargetID	GetWorldDepthTarget() const { return mWorldDepthTarget; }

private:
	Engine();

	inline void TogglePause() { mbIsPaused = !mbIsPaused; }
	void CalcFrameStats();
	bool HandleInput();

	// prepares rendering context: gets data from scene and sets up data structures ready to be sent to GPU
	void PreRender();

	void RenderLights() const;

//--------------------------------------------------------------

private:
	static Engine*					sInstance;
	static Settings::Engine			sEngineSettings;
	
	bool							mbIsPaused;

	// systems
	//--------------------------------------------------------
	Input*							mpInput;
	Renderer*						mpRenderer;
	SceneManager*					mpSceneManager;
	PerfTimer*						mpTimer;
	WorkerPool						mWorkerPool;

	// scene
	//--------------------------------------------------------
	SceneView						mSceneView;
	SceneLightData					mSceneLightData;
	std::vector<Light>				mLights;

	bool							mbUsePaniniProjection;

	// rendering 
	//--------------------------------------------------------
	ShaderID						mSelectedShader;
	
	DepthTargetID					mWorldDepthTarget;
	SamplerID						mNormalSampler;

	ShadowMapPass					mShadowMapPass;
	DeferredRenderingPasses			mDeferredRenderingPasses;
	EnvironmentMapLightingPass		mEnvironmentMapLighting;
	AmbientOcclusionPass			mSSAOPass;
	PostProcessPass					mPostProcessPass;
	DebugPass						mDebugPass;

	bool							mbUseDeferredRendering;	// todo read from sceneview
	bool							mbIsAmbientOcclusionOn;	// todo read from sceneview
	bool							mDebugRender;			// todo read from sceneview
	

	std::vector<const GameObject*>	mZPassObjects;
	std::vector<const GameObject*>	mTBNDrawObjects;
};

#define ENGINE Engine::GetEngine()
