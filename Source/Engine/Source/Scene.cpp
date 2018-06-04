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

#include "Utilities/Log.h"

Scene::Scene()
	: mpRenderer(nullptr)	// pRenderer is initialized at Load()
	, mpTextRenderer(nullptr)	// mpTextRenderer is initialized at Load()
	, mSelectedCamera(0)
{}

void Scene::Initialize(Renderer * pRenderer, TextRenderer * pTextRenderer)
{
	mpRenderer = pRenderer;
	mpTextRenderer = pTextRenderer;
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

	for (size_t i = 0; i < scene.objects.size(); ++i)
	{
		GameObject* pObj = mObjectPool.Create(this);
		*pObj = scene.objects[i];
		pObj->mpScene = this;
		mpObjects.push_back(pObj);
	}
	// mObjects = std::move(scene.objects);
	//std::for_each(mObjects.begin(), mObjects.end(), [&](GameObject& obj) { obj.mpScene = this; });
	
	mLights = std::move(scene.lights);
	mMaterials = std::move(scene.materials);

	mSceneRenderSettings = scene.settings;

	mMeshes.resize(builtinMeshes.size());
	std::copy(builtinMeshes.begin(), builtinMeshes.end(), mMeshes.begin());

	for (const Settings::Camera& camSetting : scene.cameras)
	{
		Camera c;
		c.ConfigureCamera(camSetting, windowSettings, mpRenderer);
		mCameras.push_back(c);
	}

	Load(scene);
}

void Scene::UnloadScene()
{
	mCameras.clear();
	mObjectPool.Cleanup();
	mLights.clear();
	mMaterials.Clear();
	mMeshes.clear();
	mObjectPool.Cleanup();
	Unload();
}

int Scene::Render(const SceneView& sceneView) const
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
	for (const auto& obj : mObjectPool.mObjects) 
	{
		if (obj.mpScene == this && obj.mRenderSettings.bRender)	// #FutureOptimization: sort and iterate for branch prediction gains
		{
			obj.Render(mpRenderer, sceneView, bSendMaterialData, mMaterials);
			++numObj;
		}
	}
	return numObj;
}

// TODO: 
//void Scene::GetSceneObjects(std::vector<const GameObject*>& objs) const
//{
//#if 0
//	for (size_t i = 0; i < objects.size(); i++)
//	{
//		objs[i] = &objects[i];
//	}
//#else
//	for (const GameObject& obj : mObjects)
//		objs.push_back(&obj);
//#endif
//}

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


void Scene::UpdateScene(float dt)
{
	if (ENGINE->INP()->IsKeyTriggered("C"))
	{
		mSelectedCamera = (mSelectedCamera + 1) % mCameras.size();
	}

	mCameras[mSelectedCamera].Update(dt);
	Update(dt);
}

void Scene::SetEnvironmentMap(EEnvironmentMapPresets preset)
{
	mActiveSkyboxPreset = preset;
	mSkybox = Skybox::s_Presets[mActiveSkyboxPreset];
}

GameObject* Scene::CreateNewGameObject(){ mpObjects.push_back(mObjectPool.Create(this)); return mpObjects.back(); }
Material* Scene::CreateNewMaterial(EMaterialType type) { return static_cast<Material*>(mMaterials.CreateAndGetMaterial(type)); }
Material* Scene::CreateRandomMaterialOfType(EMaterialType type) { return static_cast<Material*>(mMaterials.CreateAndGetRandomMaterial(type)); }

GameObject* Scene::LoadModel(const std::string & modelPath)
{
	GameObject* pObj = CreateNewGameObject();
	mModelLoader.LoadModel(pObj, modelPath);
	return pObj;
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

