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

#define THREADED_FRUSTUM_CULL 0	// uses workers to cull the render lists (not implemented yet)

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
	//-----------------------------------------------------------------------------------------------

	struct ObjectMatrices_WorldSpace
	{
		XMMATRIX wvp;
		XMMATRIX w;
		XMMATRIX n;
	};
	auto Is2DGeometry = [](MeshID mesh)
	{
		return mesh == EGeometry::TRIANGLE || mesh == EGeometry::QUAD || mesh == EGeometry::GRID;
	};
	auto ShouldSendMaterial = [](EShaders shader)
	{
		return (shader == EShaders::FORWARD_PHONG
			 || shader == EShaders::UNLIT
			 || shader == EShaders::NORMAL
			 || shader == EShaders::FORWARD_BRDF
			 || shader == EShaders::DEFERRED_GEOMETRY);
	};
	auto RenderObject = [&](const GameObject* pObj)
	{
		const Transform& tf = pObj->GetTransform();
		const ModelData& model = pObj->GetModelData();

		const EShaders shader = static_cast<EShaders>(mpRenderer->GetActiveShader());
		const XMMATRIX world = tf.WorldTransformationMatrix();
		const XMMATRIX wvp = world * sceneView.viewProj;

		switch (shader)
		{
		case EShaders::TBN:
			mpRenderer->SetConstant4x4f("world", world);
			mpRenderer->SetConstant4x4f("viewProj", sceneView.viewProj);
			mpRenderer->SetConstant4x4f("normalMatrix", tf.NormalMatrix(world));
			break;
		case EShaders::DEFERRED_GEOMETRY:
		{
			const ObjectMatrices mats =
			{
				world * sceneView.view,
				tf.NormalMatrix(world) * sceneView.view,
				wvp,
			};
			mpRenderer->SetConstantStruct("ObjMatrices", &mats);
			break;
		}
		case EShaders::NORMAL:
			mpRenderer->SetConstant4x4f("normalMatrix", tf.NormalMatrix(world));
		case EShaders::UNLIT:
		case EShaders::TEXTURE_COORDINATES:
			mpRenderer->SetConstant4x4f("worldViewProj", wvp);
			break;
		default:	// lighting shaders
		{
			const ObjectMatrices_WorldSpace mats =
			{
				wvp,
				world,
				tf.NormalMatrix(world)
			};
			mpRenderer->SetConstantStruct("ObjMatrices", &mats);
			break;
		}
		}

		// SET GEOMETRY & MATERIAL, THEN DRAW
		mpRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);
		for(MeshID id : model.mMeshIDs)
		{
			const auto IABuffer = mMeshes[id].GetIABuffers();

			// SET MATERIAL CONSTANTS
			if (ShouldSendMaterial(shader))
			{
				const bool bMeshHasMaterial = model.mMaterialLookupPerMesh.find(id) != model.mMaterialLookupPerMesh.end();
				if (bMeshHasMaterial)
				{
					const MaterialID materialID = model.mMaterialLookupPerMesh.at(id);
					const Material* pMat = mMaterials.GetMaterial_const(materialID);
					// #TODO: uncomment below when transparency is implemented.
					//if (pMat->IsTransparent())	// avoidable branching - perhaps keeping opaque and transparent meshes on separate vectors is better.
					//	return;
					pMat->SetMaterialConstants(mpRenderer, shader, sceneView.bIsDeferredRendering);
				}
				else
				{
					mMaterials.GetDefaultMaterial(GGX_BRDF)->SetMaterialConstants(mpRenderer, shader, sceneView.bIsDeferredRendering);
				}
			}

			mpRenderer->SetVertexBuffer(IABuffer.first);
			mpRenderer->SetIndexBuffer(IABuffer.second);
			mpRenderer->Apply();
			mpRenderer->DrawIndexed();
		};
	};
	//-----------------------------------------------------------------------------------------------

	// RENDER NON-INSTANCED SCENE OBJECTS
	//
	int numObj = 0;
	for (const auto* obj : mSceneView.culledOpaqueList)
	{
		RenderObject(obj);
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
	);

	int numObj = 0;
	for (const auto* obj : mSceneView.alphaList)
	{
		obj->RenderTransparent(mpRenderer, sceneView, bSendMaterialData, mMaterials);
		++numObj;
	}
	return numObj;
}

int Scene::RenderDebug(const XMMATRIX& viewProj) const
{
	const auto IABuffers = mMeshes[EGeometry::CUBE].GetIABuffers();

	// set debug render state
	mpRenderer->SetShader(EShaders::UNLIT);
	mpRenderer->SetConstant3f("diffuse", LinearColor::yellow);
	mpRenderer->SetConstant1f("isDiffuseMap", 0.0f);
	mpRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());
	mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_TEST_ONLY);
	mpRenderer->SetBlendState(EDefaultBlendState::DISABLED);
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::WIREFRAME);
	mpRenderer->SetVertexBuffer(IABuffers.first);
	mpRenderer->SetIndexBuffer(IABuffers.second);

	// scene bounding box
	mBoundingBox.Render(mpRenderer, viewProj);
	
	// game object bounding boxes
	std::vector<const GameObject*> pObjects(
		mSceneView.opaqueList.size() + mSceneView.alphaList.size()
		, nullptr
	);
	std::copy(RANGE(mSceneView.opaqueList), pObjects.begin());
	std::copy(RANGE(mSceneView.alphaList), pObjects.begin() + mSceneView.opaqueList.size());
	mpRenderer->SetConstant3f("diffuse", LinearColor::cyan);
	for(const GameObject* pObj : pObjects)
	{
		pObj->mBoundingBox.Render(mpRenderer, pObj->GetTransform().WorldTransformationMatrix() * viewProj);
	};


	// TODO: camera frustum
	if (mCameras.size() > 1 && mSelectedCamera != 0 && false)
	{
		// render camera[0]'s frustum
		const XMMATRIX viewProj = mCameras[mSelectedCamera].GetViewMatrix() * mCameras[mSelectedCamera].GetProjectionMatrix();
		
		auto a = FrustumPlaneset::ExtractFromMatrix(viewProj); // world space frustum plane equations
		
		// IA: model space camera frustum 
		// world matrix from camera[0] view ray
		// viewProj from camera[selected]
		mpRenderer->SetConstant3f("diffuse", LinearColor::orange);

		
		Transform tf;
		//const vec3 diag = this->hi - this->low;
		//const vec3 pos = (this->hi + this->low) * 0.5f;
		//tf.SetScale(diag * 0.5f);
		//tf.SetPosition(pos);
		XMMATRIX wvp = tf.WorldTransformationMatrix() * viewProj;
		//mpRenderer->SetConstant4x4f("worldViewProj", wvp);
		mpRenderer->Apply();
		mpRenderer->DrawIndexed();
	}

	mpRenderer->UnbindDepthTarget();
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_NONE);
	mpRenderer->Apply();

	return 1 + (int)pObjects.size(); // objects rendered
}



// can't use std::array<T&, 2>, hence std::array<T*, 2> 
// array of 2: light data for non-shadowing and shadowing lights
constexpr size_t NON_SHADOWING_LIGHT_INDEX = 0;
constexpr size_t SHADOWING_LIGHT_INDEX = 1;
using pPointLightDataArray = std::array<PointLightDataArray*, 2>;
using pSpotLightDataArray = std::array<SpotLightDataArray*, 2>;

// stores the number of lights per light type (2 types : point and spot)
using pNumArray = std::array<int*, 2>;
void Scene::GatherLightData(SceneLightingData & outLightingData)
{
	SceneLightingData::cb& cbuffer = outLightingData._cb;

	outLightingData.ResetCounts();
	mShadowView.Clear();

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

	unsigned numShdSpot = 0;
	unsigned numPtSpot = 0;
	for (const Light& l : mLights)
	{
		//if (!l._bEnabled) continue;	// #BreaksRelease

		// index in p*LightDataArray to differentiate between shadow casters and non-shadow casters
		//const size_t shadowIndex = l._mbCastingShadows ? SHADOWING_LIGHT_INDEX : NON_SHADOWING_LIGHT_INDEX;

		// add to the count of the current light type & whether its shadow casting or not
		pNumArray& refLightCounts = l.mbCastingShadows ? casterCounts : lightCounts;

		switch (l.mType)
		{
		case Light::ELightType::POINT:
		{
			const size_t lightIndex = (*refLightCounts[l.mType])++;

			PointLightGPU plData;
			l.GetGPUData(plData);
			cbuffer.pointLights[lightIndex] = plData;
			cbuffer.pointLightsShadowing[lightIndex] = plData;

			if (l.mbCastingShadows)
			{
				cbuffer.pointProjMats[numPtSpot++] = l.GetProjectionMatrix();
				mShadowView.points.push_back(&l);
			}
		}
		break;
		case Light::ELightType::SPOT:
		{
			const size_t lightIndex = (*refLightCounts[l.mType])++;
			//const SpotLight& sl = static_cast<const SpotLight&>(l);
			//cbuffer.spotLights[lightIndex] = sl.GetGPUData();
			//cbuffer.spotLightsShadowing[lightIndex] = sl.GetGPUData();
			SpotLightGPU slData;
			l.GetGPUData(slData);
			cbuffer.spotLights[lightIndex] = slData;
			cbuffer.spotLightsShadowing[lightIndex] = slData;

			if (l.mbCastingShadows)
			{
				cbuffer.shadowViews[numShdSpot++] = l.GetLightSpaceMatrix();
				mShadowView.spots.push_back(&l);
			}
		}
		break;
		default:
			Log::Error("Engine::PreRender(): UNKNOWN LIGHT TYPE");
			continue;
		}
	}

	if (mDirectionalLight.mbEnabled)
	{
		mDirectionalLight.GetGPUData(cbuffer.directionalLight);
		cbuffer.shadowViewDirectional = mDirectionalLight.GetLightSpaceMatrix();
		mShadowView.pDirectional = &mDirectionalLight;
	}
	else
	{
		cbuffer.directionalLight = {};
	}
}

bool IsVisible(const FrustumPlaneset& frustum, const BoundingBox& aabb);

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
	PerfTimer timer;
	timer.Start();
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
			constexpr float DegenerateMeshPositionChannelValueMax = 15000.0f; // make sure no vertex.xyz is > 15,000.0f

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
					const vec3 worldPos = vec3(XMVector4Transform(vec4(pData[i].position, 1.0f), worldMatrix));
					const float x_mesh = std::min(worldPos.x(), DegenerateMeshPositionChannelValueMax);
					const float y_mesh = std::min(worldPos.y(), DegenerateMeshPositionChannelValueMax);
					const float z_mesh = std::min(worldPos.z(), DegenerateMeshPositionChannelValueMax);

					const float x_mesh_local = pData[i].position.x();
					const float y_mesh_local = pData[i].position.y();
					const float z_mesh_local = pData[i].position.z();

					// scene bounding box - world space
					mins = vec3
					(
						std::min(x_mesh, mins.x()),
						std::min(y_mesh, mins.y()),
						std::min(z_mesh, mins.z())
					);
					maxs = vec3
					(
						std::max(x_mesh, maxs.x()),
						std::max(y_mesh, maxs.y()),
						std::max(z_mesh, maxs.z())
					);

					// object bounding box - model space
					mins_obj = vec3
					(
						std::min(x_mesh_local, mins_obj.x()),
						std::min(y_mesh_local, mins_obj.y()),
						std::min(z_mesh_local, mins_obj.z())
					);
					maxs_obj = vec3
					(
						std::max(x_mesh_local, maxs_obj.x()),
						std::max(y_mesh_local, maxs_obj.y()),
						std::max(z_mesh_local, maxs_obj.z())
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

	timer.Stop();
	Log::Info("SceneBoundingBox:lo=(%.2f, %.2f, %.2f)\thi=(%.2f, %.2f, %.2f) in %.2fs"
		, mins.x() , mins.y() , mins.z()
		, maxs.x() , maxs.y() , maxs.z()
		, timer.DeltaTime()
	);
	mBoundingBox.hi = maxs;
	mBoundingBox.low = mins;
}


#if _DEBUG
static bool bReportedList = true;
#endif

void Scene::UpdateScene(float dt)
{
	// CYCLE THROUGH CAMERAS
	//
	if (ENGINE->INP()->IsKeyTriggered("C"))
	{
		mSelectedCamera = (mSelectedCamera + 1) % mCameras.size();
	}


	// OPTIMIZATION SETTINGS TOGGLES
	//
#if _DEBUG
	if (ENGINE->INP()->IsKeyTriggered("F7"))
	{
		bool& toggle = ENGINE->INP()->IsKeyDown("Shift")
			? mSceneRenderSettings.optimization.bViewFrustumCull_LocalLights
			: mSceneRenderSettings.optimization.bViewFrustumCull_MainView;
		
		toggle = !toggle;
	}
	if (ENGINE->INP()->IsKeyTriggered("F8"))
	{
		mSceneRenderSettings.optimization.bViewFrustumCull_LocalLights= !mSceneRenderSettings.optimization.bViewFrustumCull_LocalLights;
	}
	if (ENGINE->INP()->IsKeyTriggered("F9"))
	{

		if (ENGINE->INP()->IsKeyDown("Shift"))
		{
			bReportedList = false;
		}
		else
		{
			mSceneRenderSettings.optimization.bSortRenderLists = !mSceneRenderSettings.optimization.bSortRenderLists;
		}
	}
#endif



	// UPDATE CAMERA & WORLD
	//
	mCameras[mSelectedCamera].Update(dt);
	Update(dt);
}


static bool IsVisible(const FrustumPlaneset& frustum, const BoundingBox& aabb)
{
	const vec4 points[] =
	{
	{ aabb.low.x(), aabb.low.y(), aabb.low.z(), 1.0f },
	{ aabb.hi.x() , aabb.low.y(), aabb.low.z(), 1.0f },
	{ aabb.hi.x() , aabb.hi.y() , aabb.low.z(), 1.0f },
	{ aabb.low.x(), aabb.hi.y() , aabb.low.z(), 1.0f },

	{ aabb.low.x(), aabb.low.y(), aabb.hi.z() , 1.0f},
	{ aabb.hi.x() , aabb.low.y(), aabb.hi.z() , 1.0f},
	{ aabb.hi.x() , aabb.hi.y() , aabb.hi.z() , 1.0f},
	{ aabb.low.x(), aabb.hi.y() , aabb.hi.z() , 1.0f},
	};

	for (int i = 0; i < 6; ++i)	// for each plane
	{
		bool bInside = false;
		for (int j = 0; j < 8; ++j)	// for each point
		{
			if (XMVector4Dot(points[j], frustum.abcd[i]).m128_f32[0] > 0.000002f)
			{
				bInside = true;
				break;
			}
		}
		if (!bInside)
		{
			return false;
		}
	}
	return true;
}


static size_t CullMeshes(const FrustumPlaneset& frustumPlanes, std::vector<MeshID> todo)
{
	// We currently cull based on game object bounding boxes, meaning that we
	// cull meshes in 'gameobject-sized-batches'. Culling can be refined to mesh
	// level by letting each game object have a culled list of meshes to draw.
	//
	return 0;
}

static size_t CullGameObjects(
	const FrustumPlaneset&                  frustumPlanes
	, const std::vector<const GameObject*>& pObjs
	, std::vector<const GameObject*>&       pCulledObjs
)
{
	size_t currIdx = 0;
	std::for_each(RANGE(pObjs), [&](const GameObject* pObj)
	{
		// aabb is static and in world space during load time.
		// this wouldn't work for dynamic objects in this state.
		const BoundingBox aabb_world = [&]() 
		{
			const XMMATRIX world = pObj->GetTransform().WorldTransformationMatrix();
			const XMMATRIX worldRotation = pObj->GetTransform().RotationMatrix();
			const BoundingBox& aabb_local = pObj->GetAABB();
#if 1
			// transform low and high points of the bounding box: model->world
			return BoundingBox(
			{ 
				XMVector4Transform(vec4(aabb_local.low, 1.0f), world), 
				XMVector4Transform(vec4(aabb_local.hi , 1.0f), world) 
			});
#else
			//----------------------------------------------------------------------------------
			// TODO: there's an error in the code below. 
			// bug repro: turn your back to the nanosuit in sponza scene -> suit won't be culled.
			//----------------------------------------------------------------------------------
			// transform center and extent and construct high and low later
			// we can use XMVector3Transform() for the extent vector to save some instructions
			const vec3 extent = aabb_local.hi - aabb_local.low;
			const vec4 center = vec4((aabb_local.hi + aabb_local.low) * 0.5f, 1.0f);
			const vec3 tfC = XMVector4Transform(center, world);
			const vec3 tfEx = XMVector3Transform(extent, worldRotation) * 0.5f;
			return BoundingBox(
			{
				{tfC - tfEx},	// lo
				{tfC + tfEx}	// hi
			});
#endif
		}();

		//assert(!pObj->GetModelData().mMeshIDs.empty());
		if (pObj->GetModelData().mMeshIDs.empty())
		{
#if _DEBUG
			Log::Warning("CullGameObject(): GameObject with empty mesh list.");
#endif
			return;
		}

		if (IsVisible(frustumPlanes, aabb_world))
		{
			pCulledObjs.push_back(pObj);
			++currIdx;
		}
	});
	return pObjs.size() - currIdx;
}

void Scene::PreRender(CPUProfiler* pCPUProfiler, FrameStats& stats)
{
	// LAMBDA DEFINITIONS
	//---------------------------------------------------------------------------------------------
	// Meshes are sorted according to BUILT_IN_TYPE < CUSTOM, 
	// and BUILT_IN_TYPEs are sorted in themselves
	auto SortByMeshType = [&](const GameObject* pObj0, const GameObject* pObj1)
	{
		const ModelData& model0 = pObj0->GetModelData();
		const ModelData& model1 = pObj1->GetModelData();

		const MeshID mID0 = model0.mMeshIDs.empty() ? -1 : model0.mMeshIDs.back();
		const MeshID mID1 = model1.mMeshIDs.empty() ? -1 : model1.mMeshIDs.back();

		assert(mID0 != -1 && mID1 != -1);

		// case: one of the objects have a custom mesh
		if (mID0 >= EGeometry::MESH_TYPE_COUNT || mID1 >= EGeometry::MESH_TYPE_COUNT)
		{
			if (mID0 < EGeometry::MESH_TYPE_COUNT)
				return true;

			if (mID1 < EGeometry::MESH_TYPE_COUNT)
				return false;

			return false;
		}

		// case: both objects are built-in types
		else
		{
			return mID0 < mID1;
		}
	};
	auto SortByMaterialID = [](const GameObject* pObj0, const GameObject* pObj1)
	{
		// TODO:
		return true;
	};
	auto SortByViewSpaceDepth = [](const GameObject* pObj0, const GameObject* pObj1)
	{
		// TODO:
		return true;
	};
	//---------------------------------------------------------------------------------------------



	// set scene view
	const Camera& viewCamera = GetActiveCamera();
	const XMMATRIX view = viewCamera.GetViewMatrix();
	const XMMATRIX viewInverse = viewCamera.GetViewInverseMatrix();
	const XMMATRIX proj = viewCamera.GetProjectionMatrix();
	XMVECTOR det = XMMatrixDeterminant(proj);
	const XMMATRIX projInv = XMMatrixInverse(&det, proj);

	// scene view matrices
	mSceneView.viewProj = view * proj;
	mSceneView.view = view;
	mSceneView.viewInverse = viewInverse;
	mSceneView.proj = proj;
	mSceneView.projInverse = projInv;

	// render/scene settings
	mSceneView.sceneRenderSettings = GetSceneRenderSettings();
	mSceneView.environmentMap = GetEnvironmentMap();
	mSceneView.cameraPosition = viewCamera.GetPositionF();
	mSceneView.bIsIBLEnabled = mSceneRenderSettings.bSkylightEnabled && mSceneView.bIsPBRLightingUsed;



	//----------------------------------------------------------------------------
	// PREPARE RENDER LISTS
	//----------------------------------------------------------------------------
	// CLEAN UP RENDER LISTS
	//
	//pCPUProfiler->BeginEntry("CleanUp");
	// scene view
	mSceneView.opaqueList.clear();
	mSceneView.culledOpaqueList.clear();
	mSceneView.culluedOpaqueInstancedRenderListLookup.clear();
	mSceneView.alphaList.clear();
	
	// shadow views
	mShadowView.RenderListsPerMeshType.clear();
	mShadowView.casters.clear();
	mShadowView.shadowMapRenderListLookUp.clear();
	mShadowView.shadowMapInstancedRenderListLookUp.clear();
	//pCPUProfiler->EndEntry();

	// POPULATE RENDER LISTS WITH SCENE OBJECTS
	//
	std::vector<const GameObject*> casterList;
	std::vector<const GameObject*> mainViewRenderList;
	static std::vector<const GameObject*> mainViewRenderListNonTextured;

	std::unordered_map<MeshID, std::vector<const GameObject*>>& instancedCasterLists = mShadowView.RenderListsPerMeshType;
	// gather game objects that are to be rendered in the scene
	pCPUProfiler->BeginEntry("Non-Instanced Lists");
	int numObjects = 0;
	for (GameObject& obj : mObjectPool.mObjects)
	{
		if (obj.mpScene == this && obj.mRenderSettings.bRender)
		{
			const bool bMeshListEmpty = obj.mModel.mData.mMeshIDs.empty();
			const bool bTransparentMeshListEmpty = obj.mModel.mData.mTransparentMeshIDs.empty();
			const bool mbCastingShadows = obj.mRenderSettings.bCastShadow && !bMeshListEmpty;

			if (!bMeshListEmpty)            { mSceneView.opaqueList.push_back(&obj); }
			if (!bTransparentMeshListEmpty) { mSceneView.alphaList.push_back(&obj); }
			if (mbCastingShadows)            { casterList.push_back(&obj); }

#if _DEBUG
			if (bMeshListEmpty && bTransparentMeshListEmpty)
			{
				Log::Warning("GameObject with no Mesh Data, turning bRender off");
				obj.mRenderSettings.bRender = false;
			}
#endif
			++numObjects;
		}
	}

	stats.scene.numObjects = numObjects;
	pCPUProfiler->EndEntry();


	//----------------------------------------------------------------------------
	// CULL RENDER LISTS
	//----------------------------------------------------------------------------
	const bool& bSortRenderLists = mSceneRenderSettings.optimization.bSortRenderLists;
	const bool& bCullMainView = mSceneRenderSettings.optimization.bViewFrustumCull_MainView;
	const bool& bCullLightView = mSceneRenderSettings.optimization.bViewFrustumCull_LocalLights;
	const bool& bShadowViewCull = mSceneRenderSettings.optimization.bShadowViewCull;


	// start counting these
	stats.scene.numSpots = 0;
	stats.scene.numPoints = 0;
	stats.scene.numDirectionalCulledObjects = 0;
	stats.scene.numPointsCulledObjects = 0;
	stats.scene.numSpotsCulledObjects = 0;
	
#if THREADED_FRUSTUM_CULL
	// TODO: utilize thread pool for each render list

#else
	// MAIN VIEW
	//
	pCPUProfiler->BeginEntry("Cull Views");
	//pCPUProfiler->BeginEntry("Main View");
	if (bCullMainView)
	{
		stats.scene.numMainViewCulledObjects = static_cast<int>(CullGameObjects(
			FrustumPlaneset::ExtractFromMatrix(mSceneView.viewProj)
			, mSceneView.opaqueList
			, mainViewRenderList));
	}
	else
	{
		mainViewRenderList.resize(mSceneView.opaqueList.size());
		std::copy(RANGE(mSceneView.opaqueList), mainViewRenderList.begin());
		stats.scene.numMainViewCulledObjects = 0;
	}
	//pCPUProfiler->EndEntry();



	// SHADOW FRUSTA
	//
	//pCPUProfiler->BeginEntry("Spots");
	for (const Light& l : mLights)
	{
		if (!l.mbCastingShadows) continue;

		std::vector<const GameObject*> objList;
		switch (l.mType)
		{
		case Light::ELightType::SPOT:
		{
			++stats.scene.numSpots;
			if (bCullLightView)
			{
				stats.scene.numSpotsCulledObjects += static_cast<int>(CullGameObjects(l.GetViewFrustumPlanes(), casterList, objList));
			}
			else
			{
				objList.resize(casterList.size());
				std::copy(RANGE(casterList), objList.begin());
			}
		}
		break;
		case Light::ELightType::POINT:
			++stats.scene.numPoints;
			
			// TODO: cull against 6 frustums
			if (bCullLightView) 
			{ 
				objList.resize(casterList.size());
				std::copy(RANGE(casterList), objList.begin());
			}
			else 
			{ 
				objList.resize(casterList.size());
				std::copy(RANGE(casterList), objList.begin());
			}
			break;
		}

		mShadowView.shadowMapRenderListLookUp[&l] = objList;
	}
	//pCPUProfiler->EndEntry();
#endif
	

	// CULL DIRECTIONAL SHADOW VIEW 
	//
	if (bShadowViewCull)
	{
		//pCPUProfiler->BeginEntry("Directional");
		// TODO: consider this for directionals: http://stefan-s.net/?p=92 or umbra paper
		//pCPUProfiler->EndEntry();
	}
	pCPUProfiler->EndEntry(); // Cull Views


	//----------------------------------------------------------------------------
	// SORT & BATCH RENDER LISTS
	//----------------------------------------------------------------------------
	// SORT
	//
	pCPUProfiler->BeginEntry("Sort");
	if (bSortRenderLists) 
	{ 
		std::sort(RANGE(mSceneView.culledOpaqueList), SortByMeshType);
		std::sort(RANGE(casterList), SortByMeshType);
		for (const Light& l : mLights)
		{
			if (!l.mbCastingShadows) continue;
			RenderList& lightRenderList = mShadowView.shadowMapRenderListLookUp.at(&l);

			std::sort(RANGE(lightRenderList), SortByMeshType);
		}
	}
	pCPUProfiler->EndEntry();


	// PREPARE INSTANCED DRAW BATCHES
	//
	// Shadow Caster Render Lists
	//pCPUProfiler->BeginEntry("Lists");
	pCPUProfiler->BeginEntry("[Instanced] Directional");
	for(int i=0; i<casterList.size(); ++i)
	{
		const GameObject* pCaster = casterList[i];
		const ModelData& model = pCaster->GetModelData();
		const MeshID meshID = model.mMeshIDs.empty() ? -1 : model.mMeshIDs.front();
		if (meshID >= EGeometry::MESH_TYPE_COUNT)
		{
			mShadowView.casters.push_back(std::move(casterList[i]));
			continue;
		}

		if (instancedCasterLists.find(meshID) == instancedCasterLists.end())
		{
			instancedCasterLists[meshID] = std::vector<const GameObject*>();
		}
		std::vector<const GameObject*>& renderList = instancedCasterLists.at(meshID);
		renderList.push_back(std::move(casterList[i]));
	}
	pCPUProfiler->EndEntry();


	// Main View Render Lists
	pCPUProfiler->BeginEntry("[Instanced] Main View");
	for (int i = 0; i < mainViewRenderList.size(); ++i)
	{
		const GameObject* pObj = mainViewRenderList[i];
		const ModelData& model = pObj->GetModelData();
		const MeshID meshID = model.mMeshIDs.empty() ? -1 : model.mMeshIDs.front();

		// instanced is only for built-in meshes (for now)
		if (meshID >= EGeometry::MESH_TYPE_COUNT)
		{
			mSceneView.culledOpaqueList.push_back(std::move(mainViewRenderList[i]));
			continue;
		}

		const bool bMeshHasMaterial = model.mMaterialLookupPerMesh.find(meshID) != model.mMaterialLookupPerMesh.end();
		if (bMeshHasMaterial)
		{
			const MaterialID materialID = model.mMaterialLookupPerMesh.at(meshID);
			const Material* pMat = mMaterials.GetMaterial_const(materialID);
			if (pMat->HasTexture())
			{
				mSceneView.culledOpaqueList.push_back(std::move(mainViewRenderList[i]));
				continue;
			}
		}

		RenderListLookup& instancedRenderLists = mSceneView.culluedOpaqueInstancedRenderListLookup;
		if (instancedRenderLists.find(meshID) == instancedRenderLists.end())
		{
			instancedRenderLists[meshID] = std::vector<const GameObject*>();
		}

		std::vector<const GameObject*>& renderList = instancedRenderLists.at(meshID);
		renderList.push_back(std::move(mainViewRenderList[i]));
	}
	pCPUProfiler->EndEntry();

#if _DEBUG
	if (!bReportedList)
	{
		Log::Info("Mesh Render List (%s): ", bSortRenderLists ? "Sorted" : "Unsorted");
		int num = 0;
		std::for_each(RANGE(mSceneView.culledOpaqueList), [&](const GameObject* pObj)
		{
			Log::Info("\tObj[%d]: ", num);

			int numMesh = 0;
			std::for_each(RANGE(pObj->GetModelData().mMeshIDs), [&](const MeshID& id)
			{
				Log::Info("\t\tMesh[%d]: %d", numMesh, id);
			});
			++num;
		});
		bReportedList = true;
	}
#endif

	//return numFrustumCulledObjs + numShadowFrustumCullObjs;
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
std::pair<BufferID, BufferID> SceneResourceView::GetVertexAndIndexBuffersOfMesh(const Scene* pScene, MeshID meshID)
{
	return pScene->mMeshes[meshID].GetIABuffers();
}

const Material* SceneResourceView::GetMaterial(const Scene* pScene, MaterialID materialID)
{
	return pScene->mMaterials.GetMaterial_const(materialID);
}

GameObject* SerializedScene::CreateNewGameObject()
{
	objects.push_back(GameObject(nullptr));
	return &objects.back();
}
