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

#define NOMINMAX
#include "Scene.h"
#include "Engine.h"
#include "Application/Input.h"
#include "Application/ThreadPool.h"

#include "Renderer/GeometryGenerator.h"

#include "Utilities/Log.h"

#include <numeric>
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
	mDirectionalLight = std::move(scene.directionalLight);
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
			LoadModel_Async(pObj, pObj->mModel.mModelName);
		}
		mpObjects.push_back(pObj);
	}

	Load(scene);

	// async model loading
	StartLoadingModels();
	EndLoadingModels();

	CalculateSceneBoundingBox();	// needs to happen after models are loaded
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

int Scene::RenderDebug(const XMMATRIX& viewProj) const
{
	const auto IABuffers = mMeshes[EGeometry::CUBE].GetIABuffers();
	mpRenderer->SetShader(EShaders::UNLIT);
	mpRenderer->SetConstant3f("diffuse", LinearColor::yellow);
	mpRenderer->SetConstant1f("isDiffuseMap", 0.0f);
	mpRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());
	mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_TEST_ONLY);
	mpRenderer->SetBlendState(EDefaultBlendState::DISABLED);
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::WIREFRAME);
	mpRenderer->SetVertexBuffer(IABuffers.first);
	mpRenderer->SetIndexBuffer(IABuffers.second);
	mBoundingBox.Render(mpRenderer, viewProj);
	
	// game objects
	std::vector<const GameObject*> pObjects(
		mDrawLists.opaqueList.size() + mDrawLists.alphaList.size()
		, nullptr
	);
	std::copy(RANGE(mDrawLists.opaqueList), pObjects.begin());
	std::copy(RANGE(mDrawLists.alphaList), pObjects.begin() + mDrawLists.opaqueList.size());
	mpRenderer->SetConstant3f("diffuse", LinearColor::cyan);
	std::for_each(RANGE(pObjects), [&](const GameObject* pObj)
	{
		pObj->mBoundingBox.Render(mpRenderer, viewProj);
	});
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_NONE);

	return 1 + pObjects.size(); // objects rendered
}



// can't use std::array<T&, 2>, hence std::array<T*, 2> 
// array of 2: light data for non-shadowing and shadowing lights
constexpr size_t NON_SHADOWING_LIGHT_INDEX = 0;
constexpr size_t SHADOWING_LIGHT_INDEX = 1;
using pPointLightDataArray = std::array<PointLightDataArray*, 2>;
using pSpotLightDataArray = std::array<SpotLightDataArray*, 2>;

// stores the number of lights per light type (2 types : point and spot)
using pNumArray = std::array<int*, 2>;
void Scene::GatherLightData(SceneLightingData & outLightingData, ShadowView& outShadowView) const
{
	SceneLightingData::cb& cbuffer = outLightingData._cb;

	outLightingData.ResetCounts();
	outShadowView.Clear();

	cbuffer.directionalLight.shadowFactor = 0.0f;
	cbuffer.directionalLight.brightness = 0.0f;

	pNumArray lightCounts
	{
		&outLightingData._cb.pointLightCount,
		&outLightingData._cb.spotLightCount
	};
	pNumArray casterCounts
	{
		&outLightingData._cb.pointLightCount_shadow,
		&outLightingData._cb.spotLightCount_shadow
	};

	for (const Light& l : mLights)
	{
		//if (!l._bEnabled) continue;	// #BreaksRelease

		// index in p*LightDataArray to differentiate between shadow casters and non-shadow casters
		//const size_t shadowIndex = l._castsShadow ? SHADOWING_LIGHT_INDEX : NON_SHADOWING_LIGHT_INDEX;

		// add to the count of the current light type & whether its shadow casting or not
		pNumArray& refLightCounts = l._castsShadow ? casterCounts : lightCounts;

		unsigned numShdSpot = 0;
		switch (l._type)
		{
		case Light::ELightType::POINT:
		{
			const size_t lightIndex = (*refLightCounts[l._type])++;
			cbuffer.pointLights[lightIndex] = l.GetPointLightData();
			//outLightingData._cb.pointLightsShadowing[lightIndex] = l.GetPointLightData();
			if (l._castsShadow)
			{
				outShadowView.points.push_back(&l);
			}
		}
		break;
		case Light::ELightType::SPOT:
		{
			const size_t lightIndex = (*refLightCounts[l._type])++;
			cbuffer.spotLights[lightIndex] = l.GetSpotLightData();
			cbuffer.spotLightsShadowing[lightIndex] = l.GetSpotLightData();
			if (l._castsShadow)
			{
				cbuffer.shadowViews[numShdSpot++] = l.GetLightSpaceMatrix();
				outShadowView.spots.push_back(&l);
			}
		}
		break;
		default:
			Log::Error("Engine::PreRender(): UNKNOWN LIGHT TYPE");
			continue;
		}
	}

	cbuffer.directionalLight = mDirectionalLight.GetGPUData();
	cbuffer.shadowViewDirectional = mDirectionalLight.GetLightSpaceMatrix();
	if (mDirectionalLight.enabled)
	{
		outShadowView.pDirectional = &mDirectionalLight;
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

void Scene::CalculateSceneBoundingBox()
{
	// get the objects for the scene
	std::vector<GameObject*> pObjects;
	for (GameObject& obj : mObjectPool.mObjects)
	{
		if (obj.mpScene == this && obj.mRenderSettings.bRender)
		{
			pObjects.push_back(&obj);
		}
	}

	constexpr float max_f = std::numeric_limits<float>::max();
	vec3 mins(max_f);
	vec3 maxs(-(max_f - 1.0f));
	std::for_each(RANGE(pObjects), [&](GameObject* pObj)
	{
		XMMATRIX worldMatrix = pObj->GetTransform().WorldTransformationMatrix();

		vec3 mins_obj(max_f);
		vec3 maxs_obj(-(max_f - 1.0f));

		const ModelData& modelData = pObj->GetModelData();
		std::for_each(RANGE(modelData.mMeshIDs), [&](const MeshID& meshID)
		{
			const BufferID VertexBufferID = mMeshes[meshID].GetIABuffers().first;
			const Buffer& VertexBuffer = mpRenderer->GetVertexBuffer(VertexBufferID);
			const size_t numVerts = VertexBuffer.mDesc.mElementCount;
			const size_t stride = VertexBuffer.mDesc.mStride;

			constexpr size_t defaultSz = sizeof(DefaultVertexBufferData);
			constexpr float DegenerateMeshPositionChannelValueMax = 15000.0f; // make sure no vertex.xyz is > 30,000.0f

			// #SHADER REFACTOR:
			//
			// currently all the shader input is using default vertex buffer data.
			// we just make sure that we can interpret the position data properly here
			// by ensuring the vertex buffer stride for a given mesh matches
			// the default vertex buffer.
			//
			// TODO:
			// Type information is not preserved once the vertex/index buffer is created.
			// need to figure out a way to interpret the position data in a given buffer
			//
			if (stride == defaultSz)
			{
				const DefaultVertexBufferData* pData = reinterpret_cast<const DefaultVertexBufferData*>(VertexBuffer.mpCPUData);
				if (pData == nullptr)
				{
					Log::Info("Nope: %d", int(stride));
					return;
				}

				for (int i = 0; i < numVerts; ++i)
				{
					const vec3 worldPos = vec3(XMVector3Transform(pData[i].position, worldMatrix));
					const float x_mesh = std::min(worldPos.x(), DegenerateMeshPositionChannelValueMax);
					const float y_mesh = std::min(worldPos.y(), DegenerateMeshPositionChannelValueMax);
					const float z_mesh = std::min(worldPos.z(), DegenerateMeshPositionChannelValueMax);

					// scene bounding box
					mins = vec3(
						std::min(x_mesh, mins.x()),
						std::min(y_mesh, mins.y()),
						std::min(z_mesh, mins.z())
					);
					maxs = vec3(
						std::max(x_mesh, maxs.x()),
						std::max(y_mesh, maxs.y()),
						std::max(z_mesh, maxs.z())
					);

					// object bounding box
					mins_obj = vec3(
						std::min(x_mesh, mins_obj.x()),
						std::min(y_mesh, mins_obj.y()),
						std::min(z_mesh, mins_obj.z())
					);
					maxs_obj = vec3(
						std::max(x_mesh, maxs_obj.x()),
						std::max(y_mesh, maxs_obj.y()),
						std::max(z_mesh, maxs_obj.z())
					);
				}
			}
			else
			{
				Log::Warning("Unsupported vertex stride for mesh.");
			}
		});

		pObj->mBoundingBox.hi = maxs_obj;
		pObj->mBoundingBox.low = mins_obj;
	});

	Log::Info("SceneBoundingBox:lo=(%.2f, %.2f, %.2f)\thi=(%.2f, %.2f, %.2f)"
		, mins.x() , mins.y() , mins.z()
		, maxs.x() , maxs.y() , maxs.z()
	);
	mBoundingBox.hi = maxs;
	mBoundingBox.low = mins;
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
