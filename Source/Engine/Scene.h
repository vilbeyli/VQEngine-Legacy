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

#include "Settings.h"	// todo: is this needed?
#include "Light.h"
#include "Mesh.h"
#include "Skybox.h"
#include "GameObject.h"
#include "GameObjectPool.h"

#include "Camera.h"
#include "SceneView.h"

#include <memory>
#include <mutex>
#include <future>





struct SerializedScene;
struct SceneView;
struct ShadowView;
struct DrawLists;

class SceneManager;
class Renderer;
class TextRenderer;
class MaterialPool;
class CPUProfiler;

namespace VQEngine { class ThreadPool; }

#define DO_NOT_LOAD_SCENES 0

struct ModelLoadQueue
{
	std::mutex mutex;
	std::unordered_map<GameObject*, std::string> objectModelMap;
	std::unordered_map<std::string, std::future<Model>> asyncModelResults;
};


//----------------------------------------------------------------------------------------------------------------
// https://en.wikipedia.org/wiki/Template_method_pattern
// https://stackoverflow.com/questions/9724371/force-calling-base-class-virtual-function
// https://isocpp.org/wiki/faq/strange-inheritance#two-strategies-for-virtuals
// template method seems like a good idea here:
//   base class takes care of the common tasks among all scenes and calls the 
//   customized functions of the derived classes through pure virtual functions
//----------------------------------------------------------------------------------------------------------------
class Scene
{
protected:
	//----------------------------------------------------------------------------------------------------------------
	// INTERFACE FOR SCENE INSTANCES
	//----------------------------------------------------------------------------------------------------------------

	// Update() is called each frame before Engine::Render(). Scene-specific update logic goes here.
	//
	virtual void Update(float dt) = 0;

	// Scene-specific loading logic goes here
	//
	virtual void Load(SerializedScene& scene) = 0;
	
	// Scene-specific unloading logic goes here
	//
	virtual void Unload() = 0;

	// Each scene has to implement scene-specific RenderUI() function. 
	// RenderUI() is called after post processing is finished and it is 
	// the last rendering workload before presenting the frame.
	//
	virtual void RenderUI() const = 0;

	//------------------------------------------------------------------------

	//
	// SCENE RESOURCE MANAGEMENT
	//

	//	Use this function to programmatically create new objects in the scene.
	//
	GameObject* CreateNewGameObject();

	// Adds a light to the scene resources. Scene manager differentiates between
	// static and dynamic (moving and non-moving) lights for optimization reasons.
	//
	void AddLight(const Light& l);

	// Use this function to programmatically create new lights in the scene.
	// TODO: finalize design after light refactor
	//
	//Light* CreateNewLight();
	
	//	Loads an assimp model - blocks the thread until the model loads
	//
	Model LoadModel(const std::string& modelPath);

	// Queues a task for loading an assimp model for the GameObject* pObject
	// - ModelData will be assigned when the models finish loading which is sometime 
	//   after Load() and before Render(), it won't be immediately available.
	//
	void LoadModel_Async(GameObject* pObject, const std::string& modelPath);

public:
	//----------------------------------------------------------------------------------------------------------------
	// ENGINE INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	struct BaseSceneParams
	{
		Renderer*     pRenderer     = nullptr; 
		TextRenderer* pTextRenderer = nullptr;
		CPUProfiler*  pCPUProfiler  = nullptr;
	};
	Scene(const BaseSceneParams& params);
	~Scene() = default;


	// Moves objects from serializedScene into objects vector and sets the scene pointer in objects
	//
	void LoadScene(SerializedScene& scene, const Settings::Window& windowSettings, const std::vector<Mesh>& builtinMeshes);

	// Clears object/light containers and camera settings
	//
	void UnloadScene();

	// Updates selected camera and calls overridden Update from derived scene class
	//
	void UpdateScene(float dt);

	// Prepares the scene and shadow views for culling, sorting, instanced draw lists, lights, etc.
	//
	void PreRender(FrameStats& stats, SceneLightingConstantBuffer & outLightingData);


	// Renders the meshes in the scene which have materials with alpha=1.0f
	//
	int RenderOpaque(const SceneView& sceneView) const;

	// Renders the transparent meshes in the scene, on a separate draw pass
	//
	int RenderAlpha(const SceneView& sceneView) const;

	// Renders debugging information such as bounding boxes, wireframe meshes, etc.
	//
	int RenderDebug(const XMMATRIX& viewProj) const;

	void RenderLights() const;

	//	Use these functions to programmatically create material instances which you can add to game objects in the scene. 
	//
	Material* CreateNewMaterial(EMaterialType type); // <Thread safe>
	Material* CreateRandomMaterialOfType(EMaterialType type); // <Thread safe>

	inline const EnvironmentMap&		GetEnvironmentMap() const { return mSkybox.GetEnvironmentMap(); }
	inline const Camera&				GetActiveCamera() const { return mCameras[mSelectedCamera]; }
	inline const Settings::SceneRender& GetSceneRenderSettings() const { return mSceneRenderSettings; }
	inline const std::vector<Light>&	GetDynamicLights() { return mLightsDynamic; }
	inline bool							HasSkybox() const { return mSkybox.GetSkyboxTexture() != -1; }
	inline void							RenderSkybox(const XMMATRIX& viewProj) const { mSkybox.Render(viewProj); }
	inline EEnvironmentMapPresets		GetActiveEnvironmentMapPreset() const { return mActiveSkyboxPreset; }

	void CalculateSceneBoundingBox();
	void SetEnvironmentMap(EEnvironmentMapPresets preset);
	void ResetActiveCamera();

	// ???
	//
	MeshID AddMesh_Async(Mesh m);

protected:
	//----------------------------------------------------------------------------------------------------------------
	// SCENE DATA
	//----------------------------------------------------------------------------------------------------------------
	friend class SceneResourceView; // using attorney method, alternatively can use friend function
	friend class ModelLoader;
	
	//
	// SCENE RESOURCE CONTAINERS
	//
	std::vector<Mesh>			mMeshes;
	std::vector<Camera>			mCameras;
	std::vector<GameObject*>	mpObjects;


	//
	// SCENE STATE
	//
	Light						mDirectionalLight;
	Skybox						mSkybox;
	EEnvironmentMapPresets		mActiveSkyboxPreset;
	int							mSelectedCamera;
	Settings::SceneRender		mSceneRenderSettings;


	//
	// SYSTEMS
	//
	Renderer*					mpRenderer;
	TextRenderer*				mpTextRenderer;
	VQEngine::ThreadPool*		mpThreadPool;	// initialized by the Engine

private:
	//
	// LIGHTS
	//
	struct ShadowingLightIndexCollection
	{
		inline void Clear() { spotLightIndices.clear(); pointLightIndices.clear(); }
		std::vector<int> spotLightIndices;
		std::vector<int> pointLightIndices;
	};
	struct SceneShadowingLightIndexCollection
	{
		inline void Clear() { mStaticLights.Clear(); mDynamicLights.Clear(); }
		inline size_t GetLightCount(Light::ELightType type) const
		{
			switch (type)
			{
			case Light::POINT: return mStaticLights.pointLightIndices.size() + mDynamicLights.pointLightIndices.size(); break;
			case Light::SPOT: return mStaticLights.spotLightIndices.size() + mDynamicLights.spotLightIndices.size(); break;
			default: return 0; break;
			}
		}
		inline std::vector<const Light*> GetFlattenedListOfLights(const std::vector<Light>& staticLights, const std::vector<Light>& dynamicLights) const
		{
			std::vector<const Light*> pLights;
			for (int i : mStaticLights.spotLightIndices)   pLights.push_back(&staticLights[i]);
			for (int i : mStaticLights.pointLightIndices)  pLights.push_back(&staticLights[i]);
			for (int i : mDynamicLights.spotLightIndices)  pLights.push_back(&dynamicLights[i]);
			for (int i : mDynamicLights.pointLightIndices) pLights.push_back(&dynamicLights[i]);
			return pLights;
		}
		ShadowingLightIndexCollection mStaticLights;
		ShadowingLightIndexCollection mDynamicLights;
	};

	// Static lights will not change position or orientation. Here, we cache
	// some light data based on this assumption, such as frustum planes.
	//
	struct StaticLightCache
	{
		std::unordered_map<const Light*, std::array<FrustumPlaneset, 6>> mStaticPointLightFrustumPlanes;
		std::unordered_map<const Light*, FrustumPlaneset               > mStaticSpotLightFrustumPlanes;
		void Clear() { mStaticPointLightFrustumPlanes.clear(); mStaticSpotLightFrustumPlanes.clear(); }
	};

	std::vector<Light>			mLightsStatic;  // non-moving lights
	std::vector<Light>			mLightsDynamic; // moving lights
	StaticLightCache			mStaticLightCache;



private:
	//----------------------------------------------------------------------------------------------------------------
	// INTERNAL DATA
	//----------------------------------------------------------------------------------------------------------------
	friend class Engine;

	GameObjectPool	mObjectPool;
	MaterialPool	mMaterials;
	ModelLoader		mModelLoader;
	ModelLoadQueue	mModelLoadQueue;

	std::mutex		mSceneMeshMutex;


	BoundingBox	mBoundingBox;


	SceneView	mSceneView;
	ShadowView	mShadowView;

	CPUProfiler* mpCPUProfiler;

	int mForceLODLevel = 0;


private:
	void StartLoadingModels();
	void EndLoadingModels();

	void AddStaticLight(const Light& l);
	void AddDynamicLight(const Light& l);

	//-------------------------------
	// PreRender() ROUTINES
	//-------------------------------
	void SetSceneViewData();

	void GatherSceneObjects(std::vector <const GameObject*>& mainViewShadowCasterRenderList, int& outNumSceneObjects);
	void GatherLightData(SceneLightingConstantBuffer& outLightingData, const std::vector<const Light*>& pLightList);

	void SortRenderLists(std::vector <const GameObject*>& mainViewShadowCasterRenderList, std::vector<const Light*>& pShadowingLights);

	SceneShadowingLightIndexCollection CullShadowingLights(int& outNumCulledPoints, int& outNumCulledSpots); // culls lights against main view
	std::vector<const GameObject*> FrustumCullMainView(int& outNumCulledObjects);
	void FrustumCullPointAndSpotShadowViews(const std::vector <const GameObject*>& mainViewShadowCasterRenderList, const SceneShadowingLightIndexCollection& shadowingLightIndices, FrameStats& stats);
	void OcclusionCullDirectionalLightView();

	void BatchMainViewRenderList(const std::vector<const GameObject*> mainViewRenderList);
	void BatchShadowViewRenderLists(const std::vector <const GameObject*>& mainViewShadowCasterRenderList);
	//-------------------------------

	void SetLightCache();
	void ClearLights();
};







struct SerializedScene
{
	GameObject*		CreateNewGameObject();

	std::vector<Settings::Camera>	cameras;
	std::vector<Light>				lights;
	Light							directionalLight;
	MaterialPool					materials;
	std::vector<GameObject>			objects;
	Settings::SceneRender			settings;
	char loadSuccess = '0';
};