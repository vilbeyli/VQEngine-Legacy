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

#include "Scene.h"
#include "Engine.h"
#include "Application/Input.h"
#include "Application/ThreadPool.h"

#include "Renderer/GeometryGenerator.h"

#include "Utilities/Log.h"

#include <set>

Scene::Scene(Renderer * pRenderer, TextRenderer * pTextRenderer)
	: mpRenderer(pRenderer)
	, mpTextRenderer(pTextRenderer)
	, mSelectedCamera(0)
{
	mModelLoader.Initialize(pRenderer);
}

void Scene::LoadScene(SerializedScene& scene, const Settings::Window& windowSettings, const std::vector<Mesh>& builtinMeshes)
{
#ifdef _DEBUG
	mObjectPool.Initialize(4096);
	mMaterials.Initialize(4096);
#else
	mObjectPool.Initialize(4096 * 8);
	mMaterials.Initialize(4096 * 8);
#endif

	mMeshes.resize(builtinMeshes.size());
	std::copy(builtinMeshes.begin(), builtinMeshes.end(), mMeshes.begin());

	mLights = std::move(scene.lights);
	mMaterials = std::move(scene.materials);

	mSceneRenderSettings = scene.settings;


	for (const Settings::Camera& camSetting : scene.cameras)
	{
		Camera c;
		c.ConfigureCamera(camSetting, windowSettings, mpRenderer);
		mCameras.push_back(c);
	}

	for (size_t i = 0; i < scene.objects.size(); ++i)
	{
		GameObject* pObj = mObjectPool.Create(this);
		*pObj = scene.objects[i];
		pObj->mpScene = this;
		if (!pObj->mModel.mbLoaded && !pObj->mModel.mModelName.empty())
		{
			pObj->mModel = LoadModel(pObj->mModel.mModelName);
		}
		mpObjects.push_back(pObj);
	}

	Load(scene);

	// async model loading
	StartLoadingModels();
	EndLoadingModels();
}

void Scene::UnloadScene()
{
	//---------------------------------------------------------------------------
	// if we clear materials and don't clear the models loaded with them,
	// we'll crash in lookups. this can be improved by only reloading
	// the materials instead of the whole mesh data. #TODO: more granular reload
	mModelLoader.UnloadSceneModels(this);
	mMaterials.Clear();
	//---------------------------------------------------------------------------
	mCameras.clear();
	mObjectPool.Cleanup();
	mLights.clear();
	mMeshes.clear();
	mObjectPool.Cleanup();
	Unload();
}

int Scene::RenderOpaque(const SceneView& sceneView) const
{
	const ShaderID selectedShader = ENGINE->GetSelectedShader();
	const bool bSendMaterialData = (
		selectedShader == EShaders::FORWARD_PHONG
		|| selectedShader == EShaders::UNLIT
		|| selectedShader == EShaders::NORMAL
		|| selectedShader == EShaders::FORWARD_BRDF
		|| selectedShader == EShaders::DEFERRED_GEOMETRY
		|| selectedShader == EShaders::Z_PREPRASS
		);

	int numObj = 0;
	for (const auto* obj : mDrawLists.opaqueList) 
	{
		obj->RenderOpaque(mpRenderer, sceneView, bSendMaterialData, mMaterials);
		++numObj;
	}
	return numObj;
}

int Scene::RenderAlpha(const SceneView & sceneView) const
{
	const ShaderID selectedShader = ENGINE->GetSelectedShader();
	const bool bSendMaterialData = (
		selectedShader == EShaders::FORWARD_PHONG
		|| selectedShader == EShaders::UNLIT
		|| selectedShader == EShaders::NORMAL
		|| selectedShader == EShaders::FORWARD_BRDF
		|| selectedShader == EShaders::DEFERRED_GEOMETRY
		|| selectedShader == EShaders::Z_PREPRASS
		);

	int numObj = 0;
	for (const auto* obj : mDrawLists.alphaList)
	{
		obj->RenderTransparent(mpRenderer, sceneView, bSendMaterialData, mMaterials);
		++numObj;
	}
	return numObj;
}



// can't use std::array<T&, 2>, hence std::array<T*, 2> 
// array of 2: light data for non-shadowing and shadowing lights
constexpr size_t NON_SHADOWING_LIGHT_INDEX = 0;
constexpr size_t SHADOWING_LIGHT_INDEX = 1;
using pPointLightDataArray = std::array<PointLightDataArray*, 2>;
using pSpotLightDataArray = std::array<SpotLightDataArray*, 2>;
using pDirectionalLightDataArray = std::array<DirectionalLightDataArray*, 2>;

// stores the number of lights per light type
using pNumArray = std::array<int*, Light::ELightType::LIGHT_TYPE_COUNT>;
void Scene::GatherLightData(SceneLightingData & outLightingData, ShadowView& outShadowView) const
{
	outLightingData.ResetCounts();
	outShadowView.spots.clear();
	outShadowView.directionals.clear();
	outShadowView.points.clear();
	pNumArray lightCounts
	{
		&outLightingData._cb.pointLightCount,
		&outLightingData._cb.spotLightCount,
		&outLightingData._cb.directionalLightCount
	};
	pNumArray casterCounts
	{
		&outLightingData._cb.pointLightCount_shadow,
		&outLightingData._cb.spotLightCount_shadow,
		&outLightingData._cb.directionalLightCount_shadow
	};

	for (const Light& l : mLights)
	{
		//if (!l._bEnabled) continue;	// #BreaksRelease

		// index in p*LightDataArray to differentiate between shadow casters and non-shadow casters
		const size_t shadowIndex = l._castsShadow ? SHADOWING_LIGHT_INDEX : NON_SHADOWING_LIGHT_INDEX;

		// add to the count of the current light type & whether its shadow casting or not
		pNumArray& refLightCounts = l._castsShadow ? casterCounts : lightCounts;
		const size_t lightIndex = (*refLightCounts[l._type])++;

		switch (l._type)
		{
		case Light::ELightType::POINT:
			outLightingData._cb.pointLights[lightIndex] = l.GetPointLightData();
			//outLightingData._cb.pointLightsShadowing[lightIndex] = l.GetPointLightData();
			break;
		case Light::ELightType::SPOT:
			outLightingData._cb.spotLights[lightIndex] = l.GetSpotLightData();
			outLightingData._cb.spotLightsShadowing[lightIndex] = l.GetSpotLightData();
			break;
		case Light::ELightType::DIRECTIONAL:
			//outLightingData._cb.pointLights[lightIndex] = l.GetPointLightData();
			//outLightingData._cb.pointLightsShadowing[lightIndex] = l.GetPointLightData();
			break;
		default:
			Log::Error("Engine::PreRender(): UNKNOWN LIGHT TYPE");
			continue;
		}
	}

	unsigned numShd = 0;	// only for spot lights for now
	for (const Light& l : mLights)
	{
		//if (!l._bEnabled) continue; // #BreaksRelease

		// shadowing lights
		if (l._castsShadow)
		{
			switch (l._type)
			{
			case Light::ELightType::SPOT:
				outShadowView.spots.push_back(&l);
				outLightingData._cb.shadowViews[numShd++] = l.GetLightSpaceMatrix();
				break;
			case Light::ELightType::POINT:
				outShadowView.points.push_back(&l);
				break;
			case Light::ELightType::DIRECTIONAL:
				outShadowView.directionals.push_back(&l);
				break;
			}
		}

	}
}

void Scene::GatherShadowCasters(std::vector<const GameObject*>& casters) const
{
	for (const GameObject& obj : mObjectPool.mObjects)
	{
		if (obj.mpScene == this && obj.mRenderSettings.bCastShadow)
		{
			casters.push_back(&obj);
		}
	}
}


void Scene::ResetActiveCamera()
{
	mCameras[mSelectedCamera].Reset();
}

MeshID Scene::AddMesh_Async(Mesh mesh)
{
	std::unique_lock<std::mutex> l(mSceneMeshMutex);
	mMeshes.push_back(mesh);
	return MeshID(mMeshes.size() - 1);
}

void Scene::StartLoadingModels()
{
	// can have multiple objects pointing to the same path
	// get all the unique paths 
	//
	std::set<std::string> uniqueModelList;
	
	std::for_each(RANGE(mModelLoadQueue.objectModelMap), [&](auto kvp)
	{	// 'value' (.second) of the 'key value pair' (kvp) contains the model path
		uniqueModelList.emplace(kvp.second);
	});
	
	if (uniqueModelList.empty()) return;

	Log::Info("Async Model Load List: ");

	// so we load the models only once
	//
	std::for_each(RANGE(uniqueModelList), [&](const std::string& modelPath)
	{
		Log::Info("\t%s", modelPath.c_str());
		mModelLoadQueue.asyncModelResults[modelPath] = mpThreadPool->AddTask([=]() 
		{ 
			return mModelLoader.LoadModel_Async(modelPath, this);
		});
	});
}

void Scene::EndLoadingModels()
{
	std::unordered_map<std::string, Model> loadedModels;
	std::for_each(RANGE(mModelLoadQueue.objectModelMap), [&](auto kvp)
	{
		const std::string& modelPath = kvp.second;
		GameObject* pObj = kvp.first;
		
		// this will wait on the longest item.
		if (loadedModels.find(modelPath) == loadedModels.end())
		{
			Model m = mModelLoadQueue.asyncModelResults.at(modelPath).get();
			pObj->SetModel(m);
			loadedModels[modelPath] = m;
		}
		else
		{
			pObj->SetModel(loadedModels.at(modelPath));
		}
	});
}


void Scene::UpdateScene(float dt)
{
	if (ENGINE->INP()->IsKeyTriggered("C"))
	{
		mSelectedCamera = (mSelectedCamera + 1) % mCameras.size();
	}

	mCameras[mSelectedCamera].Update(dt);
	Update(dt);
}

void Scene::PreRender()
{
	mDrawLists.opaqueList.clear();
	mDrawLists.alphaList.clear();
	for (const GameObject& obj : mObjectPool.mObjects)
	{
		if (obj.mpScene == this && obj.mRenderSettings.bRender)
		{
			if (!obj.mModel.mData.mTransparentMeshIDs.empty())
			{
				mDrawLists.alphaList.push_back(&obj);
			}
			
			mDrawLists.opaqueList.push_back(&obj);
		}
	}
}

void Scene::SetEnvironmentMap(EEnvironmentMapPresets preset)
{
	mActiveSkyboxPreset = preset;
	mSkybox = Skybox::s_Presets[mActiveSkyboxPreset];
}

GameObject* Scene::CreateNewGameObject(){ mpObjects.push_back(mObjectPool.Create(this)); return mpObjects.back(); }
Material* Scene::CreateNewMaterial(EMaterialType type){ return static_cast<Material*>(mMaterials.CreateAndGetMaterial(type)); }
Material* Scene::CreateRandomMaterialOfType(EMaterialType type) { return static_cast<Material*>(mMaterials.CreateAndGetRandomMaterial(type)); }

Model Scene::LoadModel(const std::string & modelPath)
{
	return mModelLoader.LoadModel(modelPath, this);
}

void Scene::LoadModel_Async(GameObject* pObject, const std::string& modelPath)
{
	std::unique_lock<std::mutex> lock(mModelLoadQueue.mutex);
	mModelLoadQueue.objectModelMap[pObject] = modelPath;
}

// SceneResourceView ------------------------------------------

#include "SceneResources.h"
std::pair<BufferID, BufferID> SceneResourceView::GetVertexAndIndexBuffersOfMesh(Scene* pScene, MeshID meshID)
{
	return pScene->mMeshes[meshID].GetIABuffers();
}

GameObject* SerializedScene::CreateNewGameObject()
{
	objects.push_back(GameObject(nullptr));
	return &objects.back();
}

