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
#include "ObjectCullingSystem.h"

#include "Application/Input.h"
#include "Application/ThreadPool.h"
#include "Renderer/GeometryGenerator.h"
#include "Utilities/Log.h"

#include <numeric>
#include <set>

#define THREADED_FRUSTUM_CULL 0	// uses workers to cull the render lists (not implemented yet)

Scene::Scene(const BaseSceneParams& params)
	: mpRenderer(params.pRenderer)
	, mpTextRenderer(params.pTextRenderer)
	, mSelectedCamera(0)
	, mpCPUProfiler(params.pCPUProfiler)
{
	mModelLoader.Initialize(mpRenderer);
}


//----------------------------------------------------------------------------------------------------------------
// LOAD / UNLOAD FUNCTIONS
//----------------------------------------------------------------------------------------------------------------
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
		Model m = {};
		if (loadedModels.find(modelPath) == loadedModels.end())
		{
			m = mModelLoadQueue.asyncModelResults.at(modelPath).get();
			loadedModels[modelPath] = m;
		}
		else
		{
			m = loadedModels.at(modelPath);
		}

		// override material if any.
		if (!pObj->mModel.mMaterialAssignmentQueue.empty())
		{
			MaterialID matID = pObj->mModel.mMaterialAssignmentQueue.front();
			pObj->mModel.mMaterialAssignmentQueue.pop(); // we only use the first material for now...
			m.OverrideMaterials(matID);
		}

		// assign the model to object
		pObj->SetModel(m);
	});
}

Model Scene::LoadModel(const std::string & modelPath)
{
	return mModelLoader.LoadModel(modelPath, this);
}

void Scene::LoadModel_Async(GameObject* pObject, const std::string& modelPath)
{
	std::unique_lock<std::mutex> lock(mModelLoadQueue.mutex);
	mModelLoadQueue.objectModelMap[pObject] = modelPath;
}


//----------------------------------------------------------------------------------------------------------------
// RENDER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------
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
	const bool bRenderPointLightCues = true;
	const bool bRenderSpotLightCues = true;
	const bool bRenderObjectBoundingBoxes = false;
	const bool bRenderMeshBoundingBoxes = false;
	const bool bRenderSceneBoundingBox = true;

	const auto IABuffersCube = mMeshes[EGeometry::CUBE].GetIABuffers();
	const auto IABuffersSphere = mMeshes[EGeometry::SPHERE].GetIABuffers();
	const auto IABuffersCone = mMeshes[EGeometry::LIGHT_CUE_CONE].GetIABuffers();
	
	XMMATRIX wvp;

	// set debug render states
	mpRenderer->SetShader(EShaders::UNLIT);
	mpRenderer->SetConstant3f("diffuse", LinearColor::yellow);
	mpRenderer->SetConstant1f("isDiffuseMap", 0.0f);
	mpRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());
	mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_TEST_ONLY);
	mpRenderer->SetBlendState(EDefaultBlendState::DISABLED);
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::WIREFRAME);
	mpRenderer->SetVertexBuffer(IABuffersCube.first);
	mpRenderer->SetIndexBuffer(IABuffersCube.second);


	// SCENE BOUNDING BOX
	//
	if (bRenderSceneBoundingBox)
	{
		wvp = mBoundingBox.GetWorldTransformationMatrix() * viewProj;
		mpRenderer->SetConstant4x4f("worldViewProj", wvp);
		mpRenderer->Apply();
		mpRenderer->DrawIndexed();
	}

	// GAME OBJECT OBB & MESH BOUNDING BOXES
	//
	int numRenderedObjects = 0;
	if (bRenderObjectBoundingBoxes)
	{
		std::vector<const GameObject*> pObjects(
			mSceneView.opaqueList.size() + mSceneView.alphaList.size()
			, nullptr
		);
		std::copy(RANGE(mSceneView.opaqueList), pObjects.begin());
		std::copy(RANGE(mSceneView.alphaList), pObjects.begin() + mSceneView.opaqueList.size());
		for (const GameObject* pObj : pObjects)
		{
			const XMMATRIX matWorld = pObj->GetTransform().WorldTransformationMatrix();
			wvp = pObj->mBoundingBox.GetWorldTransformationMatrix() * matWorld * viewProj;
#if 1
			mpRenderer->SetConstant3f("diffuse", LinearColor::cyan);
			mpRenderer->SetConstant4x4f("worldViewProj", wvp);
			mpRenderer->Apply();
			mpRenderer->DrawIndexed();
#endif

			// mesh bounding boxes (currently broken...)
			if (bRenderMeshBoundingBoxes)
			{
				mpRenderer->SetConstant3f("diffuse", LinearColor::orange);
#if 0
				for (const MeshID meshID : pObj->mModel.mData.mMeshIDs)
				{
					wvp = pObj->mMeshBoundingBoxes[meshID].GetWorldTransformationMatrix() * matWorld * viewProj;
					mpRenderer->SetConstant4x4f("worldViewProj", wvp);
					mpRenderer->Apply();
					mpRenderer->DrawIndexed();
				}
#endif
			}
		}
		numRenderedObjects = (int)pObjects.size();
	}

	// LIGHT VOLUMES
	//
	mpRenderer->SetConstant3f("diffuse", LinearColor::yellow);

	// light's transform hold Translation and Rotation data,
	// Scale is used for rendering. Hence, we use another transform
	// here to use mRange as the lights render scale signifying its 
	// radius of influence.
	Transform tf;

	// point lights
	if (bRenderPointLightCues)
	{
		mpRenderer->SetVertexBuffer(IABuffersSphere.first);
		mpRenderer->SetIndexBuffer(IABuffersSphere.second);
		for (const Light& l : mLights)
		{
			if (l.mType == Light::ELightType::POINT)
			{
				mpRenderer->SetConstant3f("diffuse", l.mColor);

				tf.SetPosition(l.mTransform._position);
				tf.SetScale(l.mRange * 0.5f); // Mesh's model space R = 2.0f, hence scale it by 0.5f...
				wvp = tf.WorldTransformationMatrix() * viewProj;
				mpRenderer->SetConstant4x4f("worldViewProj", wvp);
				mpRenderer->Apply();
				mpRenderer->DrawIndexed();
			}
		}
	}

	// spot lights 
	if (bRenderSpotLightCues)
	{
		mpRenderer->SetVertexBuffer(IABuffersCone.first);
		mpRenderer->SetIndexBuffer(IABuffersCone.second);
		for (const Light& l : mLights)
		{
			if (l.mType == Light::ELightType::SPOT)
			{
				mpRenderer->SetConstant3f("diffuse", l.mColor);

				tf = l.mTransform;
				
				// reset scale as it holds the scale value for light's render mesh
				tf.SetScale(1, 1, 1); 
				
				 // align with spot light's local space
				tf.RotateAroundLocalXAxisDegrees(-90.0f); 


				XMMATRIX alignConeToSpotLightTransformation = XMMatrixIdentity();
				alignConeToSpotLightTransformation.r[3].m128_f32[0] = 0.0f;
				alignConeToSpotLightTransformation.r[3].m128_f32[1] = -l.mRange;
				alignConeToSpotLightTransformation.r[3].m128_f32[2] = 0.0f;
				//tf.SetScale(1, 20, 1);

				const float coneBaseRadius = std::tanf(l.mSpotOuterConeAngleDegrees * DEG2RAD) * l.mRange;
				XMMATRIX scaleConeToRange = XMMatrixIdentity();
				scaleConeToRange.r[0].m128_f32[0] = coneBaseRadius;
				scaleConeToRange.r[1].m128_f32[1] = l.mRange;
				scaleConeToRange.r[2].m128_f32[2] = coneBaseRadius;

				//wvp = alignConeToSpotLightTransformation * tf.WorldTransformationMatrix() * viewProj;
				wvp = scaleConeToRange * alignConeToSpotLightTransformation * tf.WorldTransformationMatrix() * viewProj;
				mpRenderer->SetConstant4x4f("worldViewProj", wvp);
				mpRenderer->Apply();
				mpRenderer->DrawIndexed();
			}
		}
	}


	// TODO: CAMERA FRUSTUM
	//
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


	return numRenderedObjects; // objects rendered
}


//----------------------------------------------------------------------------------------------------------------
// UPDATE FUNCTIONS
//----------------------------------------------------------------------------------------------------------------
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
//#if _DEBUG
#if 1
	if (ENGINE->INP()->IsKeyTriggered("F7"))
	{
		bool& toggle = ENGINE->INP()->IsKeyDown("Shift")
			? mSceneRenderSettings.optimization.bViewFrustumCull_LocalLights
			: mSceneRenderSettings.optimization.bViewFrustumCull_MainView;

		toggle = !toggle;
	}
	if (ENGINE->INP()->IsKeyTriggered("F8"))
	{
		mSceneRenderSettings.optimization.bViewFrustumCull_LocalLights = !mSceneRenderSettings.optimization.bViewFrustumCull_LocalLights;
	}
	if (ENGINE->INP()->IsKeyTriggered("F9"))
	{

	}
#endif



	// UPDATE CAMERA & WORLD
	//
	mCameras[mSelectedCamera].Update(dt);
	Update(dt);
}


void Scene::PreRender(FrameStats& stats, SceneLightingData& outLightingData)
{
	using namespace VQEngine;



	// containers we'll work on for preparing draw lists
	std::vector<const GameObject*> mainViewRenderList; // Shadow casters + non-shadow casters
	std::vector<const GameObject*> mainViewShadowCasterRenderList;
	std::vector<const Light*> pShadowingLights;

	// shorthands
	std::unordered_map<MeshID, std::vector<const GameObject*>>& instancedCasterLists = mShadowView.RenderListsPerMeshType;

	//----------------------------------------------------------------------------
	// SET SCENE VIEW / SETTINGS
	//----------------------------------------------------------------------------

	const Camera& viewCamera = GetActiveCamera();
	const XMMATRIX view = viewCamera.GetViewMatrix();
	const XMMATRIX viewInverse = viewCamera.GetViewInverseMatrix();
	const XMMATRIX proj = viewCamera.GetProjectionMatrix();
	XMVECTOR det = XMMatrixDeterminant(proj);
	const XMMATRIX projInv = XMMatrixInverse(&det, proj);
	det = mDirectionalLight.mbEnabled
		? XMMatrixDeterminant(mDirectionalLight.GetProjectionMatrix())
		: XMVECTOR();
	const XMMATRIX directionalLightProjection = mDirectionalLight.mbEnabled
		? mDirectionalLight.GetProjectionMatrix()
		: XMMatrixIdentity();

	// scene view matrices
	mSceneView.viewProj = view * proj;
	mSceneView.view = view;
	mSceneView.viewInverse = viewInverse;
	mSceneView.proj = proj;
	mSceneView.projInverse = projInv;
	mSceneView.directionalLightProjection = directionalLightProjection;

	// render/scene settings
	mSceneView.sceneRenderSettings = GetSceneRenderSettings();
	mSceneView.environmentMap = GetEnvironmentMap();
	mSceneView.cameraPosition = viewCamera.GetPositionF();
	mSceneView.bIsIBLEnabled = mSceneRenderSettings.bSkylightEnabled && mSceneView.bIsPBRLightingUsed && mSceneView.environmentMap.environmentMap != -1;



	//----------------------------------------------------------------------------
	// PREPARE RENDER LISTS
	//----------------------------------------------------------------------------
	PopulateMainViewRenderListsAndCountSceneObjects(mainViewShadowCasterRenderList, stats.scene.numObjects);

	//----------------------------------------------------------------------------
	// CULL LIGHTS
	//----------------------------------------------------------------------------
	pShadowingLights = CullLights(stats.scene.numCulledShadowingPointLights, stats.scene.numCulledShadowingSpotLights);


	//----------------------------------------------------------------------------
	// CULL RENDER LISTS
	//----------------------------------------------------------------------------


	// start counting light stats
	stats.scene.numSpots = 0;
	stats.scene.numPoints = 0;
	//stats.scene.numDirectionalCulledObjects = 0;
	stats.scene.numPointsCulledObjects = 0;
	stats.scene.numSpotsCulledObjects = 0;

#if THREADED_FRUSTUM_CULL
	// TODO: utilize thread pool for each render list

	mpCPUProfiler->BeginEntry("Cull Views");
	// MAIN VIEW
	//


	// CULL DIRECTIONAL SHADOW VIEW 
	//


	// TODO: sync point
	mpCPUProfiler->EndEntry(); // Cull Views
#else

	// MAIN VIEW
	//
	mpCPUProfiler->BeginEntry("Cull Views");
	mainViewRenderList = FrustumCullMainView(stats.scene.numMainViewCulledObjects);
	FrustumCullPointAndSpotLightViews(mainViewShadowCasterRenderList, pShadowingLights, stats);
	if (mSceneRenderSettings.optimization.bShadowViewCull)
	{
		mpCPUProfiler->BeginEntry("Directional_Occl");
		OcclusionCullDirectionalLightView();
		mpCPUProfiler->EndEntry(); // Directional_Occl
	}
	mpCPUProfiler->EndEntry(); // Cull Views

#endif // THREADED_FRUSTUM_CULL




	//----------------------------------------------------------------------------
	// SORT RENDER LISTS
	//----------------------------------------------------------------------------
	mpCPUProfiler->BeginEntry("Sort");
	if (mSceneRenderSettings.optimization.bSortRenderLists)
	{
		SortRenderLists(mainViewShadowCasterRenderList, pShadowingLights);
	}
	mpCPUProfiler->EndEntry();


	//----------------------------------------------------------------------------
	// BATCH RENDER LISTS
	//----------------------------------------------------------------------------
	// PREPARE INSTANCED DRAW BATCHES
	//
	// Shadow Caster Render Lists
	//mpCPUProfiler->BeginEntry("Lists");
	mpCPUProfiler->BeginEntry("[Instanced] Directional");
	for (int i = 0; i < mainViewShadowCasterRenderList.size(); ++i)
	{
		const GameObject* pCaster = mainViewShadowCasterRenderList[i];
		const ModelData& model = pCaster->GetModelData();
		const MeshID meshID = model.mMeshIDs.empty() ? -1 : model.mMeshIDs.front();
		if (meshID >= EGeometry::MESH_TYPE_COUNT)
		{
			mShadowView.casters.push_back(std::move(mainViewShadowCasterRenderList[i]));
			continue;
		}

		if (instancedCasterLists.find(meshID) == instancedCasterLists.end())
		{
			instancedCasterLists[meshID] = std::vector<const GameObject*>();
		}
		std::vector<const GameObject*>& renderList = instancedCasterLists.at(meshID);
		renderList.push_back(std::move(mainViewShadowCasterRenderList[i]));
	}
	mpCPUProfiler->EndEntry();


	// Main View Render Lists
	mpCPUProfiler->BeginEntry("[Instanced] Main View");
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
	mpCPUProfiler->EndEntry();

#if _DEBUG
	if (!bReportedList)
	{
		Log::Info("Mesh Render List (%s): ", mSceneRenderSettings.optimization.bSortRenderLists ? "Sorted" : "Unsorted");
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
#endif // THREADED_FRUSTUM_CULL

	GatherLightData(outLightingData, pShadowingLights);

	//return numFrustumCulledObjs + numShadowFrustumCullObjs;
}


void Scene::ResetActiveCamera()
{
	mCameras[mSelectedCamera].Reset();
}

void Scene::SetEnvironmentMap(EEnvironmentMapPresets preset)
{
	mActiveSkyboxPreset = preset;
	mSkybox = Skybox::s_Presets[mActiveSkyboxPreset];
}



//----------------------------------------------------------------------------------------------------------------
// PRE-RENDER FUNCTIONS
//----------------------------------------------------------------------------------------------------------------
// can't use std::array<T&, 2>, hence std::array<T*, 2> 
// array of 2: light data for non-shadowing and shadowing lights
constexpr size_t NON_SHADOWING_LIGHT_INDEX = 0;
constexpr size_t SHADOWING_LIGHT_INDEX = 1;
using pPointLightDataArray = std::array<PointLightDataArray*, 2>;
using pSpotLightDataArray = std::array<SpotLightDataArray*, 2>;

// stores the number of lights per light type (2 types : point and spot)
using pNumArray = std::array<int*, 2>;
void Scene::GatherLightData(SceneLightingData & outLightingData, const std::vector<const Light*>& pLightList)
{
	SceneLightingData::cb& cbuffer = outLightingData._cb;

	outLightingData.ResetCounts();

	cbuffer.directionalLight.depthBias = 0.0f;
	cbuffer.directionalLight.enabled = 0;
	cbuffer.directionalLight.shadowing = 0;
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

	for (const Light* l : pLightList)
	{
		//if (!l->_bEnabled) continue;	// #BreaksRelease

		pNumArray& refLightCounts = casterCounts;
		const size_t lightIndex = (*refLightCounts[l->mType])++;
		switch (l->mType)
		{
		case Light::ELightType::POINT:
		{
			PointLightGPU plData;
			l->GetGPUData(plData);

			cbuffer.pointLightsShadowing[lightIndex] = plData;
			mShadowView.points.push_back(l);
		} break;
		case Light::ELightType::SPOT:
		{
			SpotLightGPU slData;
			l->GetGPUData(slData);

			cbuffer.spotLightsShadowing[lightIndex] = slData;
			cbuffer.shadowViews[numShdSpot++] = l->GetLightSpaceMatrix();
			mShadowView.spots.push_back(l);
		} break;
		default:
			Log::Error("Engine::PreRender(): UNKNOWN LIGHT TYPE");
			continue;
		}
	}

	// iterate for non-shadowing lights (they won't be in pLightList)
	for (const Light& l : mLights)
	{
		if (l.mbCastingShadows) continue;
		pNumArray& refLightCounts = lightCounts;

		const size_t lightIndex = (*refLightCounts[l.mType])++;
		switch (l.mType)
		{
		case Light::ELightType::POINT:
		{
			PointLightGPU plData;
			l.GetGPUData(plData);
			cbuffer.pointLights[lightIndex] = plData;
		} break;
		case Light::ELightType::SPOT:
		{
			SpotLightGPU slData;
			l.GetGPUData(slData);
			cbuffer.spotLights[lightIndex] = slData;
		} break;
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

void Scene::PopulateMainViewRenderListsAndCountSceneObjects(std::vector <const GameObject*>& mainViewShadowCasterRenderList, int& outNumSceneObjects)
{
	// CLEAN UP RENDER LISTS
	//
	//pCPUProfiler->BeginEntry("CleanUp");
	// scene view
	mSceneView.opaqueList.clear();
	mSceneView.culledOpaqueList.clear();
	mSceneView.culluedOpaqueInstancedRenderListLookup.clear();
	mSceneView.alphaList.clear();

	// shadow views
	mShadowView.Clear();
	mShadowView.RenderListsPerMeshType.clear();
	mShadowView.casters.clear();
	mShadowView.shadowMapRenderListLookUp.clear();
	mShadowView.shadowMapInstancedRenderListLookUp.clear();
	//pCPUProfiler->EndEntry();

	// POPULATE RENDER LISTS WITH SCENE OBJECTS
	//
	mpCPUProfiler->BeginEntry("Non-Instanced Lists");
	outNumSceneObjects = 0;
	for (GameObject& obj : mObjectPool.mObjects) 
	{
		// only gather the game objects that are to be rendered in 'this' scene
		if (obj.mpScene == this && obj.mRenderSettings.bRender)
		{
			const bool bMeshListEmpty = obj.mModel.mData.mMeshIDs.empty();
			const bool bTransparentMeshListEmpty = obj.mModel.mData.mTransparentMeshIDs.empty();
			const bool mbCastingShadows = obj.mRenderSettings.bCastShadow && !bMeshListEmpty;

			// populate render lists
			if (!bMeshListEmpty) 
			{ 
				mSceneView.opaqueList.push_back(&obj); 
			}
			if (!bTransparentMeshListEmpty) 
			{ 
				mSceneView.alphaList.push_back(&obj); 
			}
			if (mbCastingShadows) 
			{ 
				mainViewShadowCasterRenderList.push_back(&obj); 
			}

#if _DEBUG
			if (bMeshListEmpty && bTransparentMeshListEmpty)
			{
				Log::Warning("GameObject with no Mesh Data, turning bRender off");
				obj.mRenderSettings.bRender = false;
			}
#endif
			++outNumSceneObjects;
		}
	}

	mpCPUProfiler->EndEntry();
}


void Scene::SortRenderLists(std::vector <const GameObject*>& mainViewShadowCasterRenderList, std::vector<const Light*>& pShadowingLights)
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

	std::sort(RANGE(mSceneView.culledOpaqueList), SortByMeshType);
	std::sort(RANGE(mainViewShadowCasterRenderList), SortByMeshType);
	for (const Light* pLight : pShadowingLights)
	{
		switch (pLight->mType)
		{

		case Light::ELightType::POINT:
		{
			for (int i = 0; i < 6; ++i)
			{
				;
			}
			break;
		}
		case Light::ELightType::SPOT:
		{
			RenderList& lightRenderList = mShadowView.shadowMapRenderListLookUp.at(pLight);
			std::sort(RANGE(lightRenderList), SortByMeshType);
			break;
		}
		}

	}
}

std::vector<const Light*> Scene::CullLights(int& outNumCulledPoints, int& outNumCulledSpots)
{
	using namespace VQEngine;

	std::vector<const Light*> pLights;

	outNumCulledPoints = 0;
	outNumCulledSpots = 0;

	mpCPUProfiler->BeginEntry("Cull Lights");
	for (const Light& l : mLights)
	{
		if (!l.mbCastingShadows) continue;
		switch (l.mType)
		{
		case Light::ELightType::SPOT:
		{
			// TODO: frustum - cone check
			const bool bCullLight = false;
			//if (IsVisible())
			if (!bCullLight)
				pLights.push_back(&l);
			else
				++outNumCulledSpots;
		}	break;

		case Light::ELightType::POINT:
		{
			vec3 vecCamera = GetActiveCamera().GetPositionF() - l.mTransform._position;
			const float dstSqrCameraToLight = XMVector3Dot(vecCamera, vecCamera).m128_f32[0];
			const float rangeSqr = l.mRange * l.mRange;
			
			const bool bIsCameraInPointLightSphereOfIncluence = dstSqrCameraToLight < rangeSqr;
			const bool bSphereInFrustum = IsSphereInFrustum(GetActiveCamera().GetViewFrustumPlanes(), l.mTransform._position, l.mRange);

			if (bIsCameraInPointLightSphereOfIncluence || bSphereInFrustum)
				pLights.push_back(&l);
			else
				++outNumCulledPoints;

		} break;
		} // light type

	}
	mpCPUProfiler->EndEntry();

	return pLights;
}

std::vector<const GameObject*> Scene::FrustumCullMainView(int& outNumCulledObjects)
{
	using namespace VQEngine;

	const bool& bCullMainView = mSceneRenderSettings.optimization.bViewFrustumCull_MainView;

	std::vector<const GameObject*> mainViewRenderList;

	//mpCPUProfiler->BeginEntry("[Cull Main View]");
	if (bCullMainView)
	{
		outNumCulledObjects = static_cast<int>(CullGameObjects(
			FrustumPlaneset::ExtractFromMatrix(mSceneView.viewProj)
			, mSceneView.opaqueList
			, mainViewRenderList));
	}
	else
	{
		mainViewRenderList.resize(mSceneView.opaqueList.size());
		std::copy(RANGE(mSceneView.opaqueList), mainViewRenderList.begin());
		outNumCulledObjects = 0;
	}
	//mpCPUProfiler->EndEntry();

	return mainViewRenderList;
}


void Scene::FrustumCullPointAndSpotLightViews(const std::vector <const GameObject*>& mainViewShadowCasterRenderList, std::vector<const Light*>& pShadowingLights, FrameStats& stats)
{
	using namespace VQEngine;

	const bool& bCullLightView = mSceneRenderSettings.optimization.bViewFrustumCull_LocalLights;

	RenderList objList;
#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
	std::array< MeshDrawData, 6> meshDrawDataPerFace;
#else
	std::array< MeshDrawList, 6> meshListForPoints;
#endif

	for (const Light* l : pShadowingLights)
	{
		switch (l->mType)
		{
		case Light::ELightType::SPOT:
		{
			//mpCPUProfiler->BeginEntry("Spots");
			++stats.scene.numSpots;
			if (bCullLightView)
			{
				stats.scene.numSpotsCulledObjects += static_cast<int>(CullGameObjects(l->GetViewFrustumPlanes(), mainViewShadowCasterRenderList, objList));
			}
			else
			{
				objList.resize(mainViewShadowCasterRenderList.size());
				std::copy(RANGE(mainViewShadowCasterRenderList), objList.begin());
			}

			mShadowView.shadowMapRenderListLookUp[l] = objList;
			//mpCPUProfiler->EndEntry();
			break;
		}
		case Light::ELightType::POINT:
			//mpCPUProfiler->BeginEntry("[Cull Point View]");
			++stats.scene.numPoints;

			MeshDrawData meshDrawData;
			if (bCullLightView)
			{
				for (int face = 0; face < 6; ++face)
				{
					const Texture::CubemapUtility::ECubeMapLookDirections CubemapFaceDirectionEnum = static_cast<Texture::CubemapUtility::ECubeMapLookDirections>(face);
					for (const GameObject* pObj : mainViewShadowCasterRenderList)
					{
#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
						stats.scene.numPointsCulledObjects += static_cast<int>(CullMeshes
						(
							l->GetViewFrustumPlanes(CubemapFaceDirectionEnum),
							pObj,
							meshDrawDataPerFace[face]
						));
#else
						meshDrawData.meshIDs.clear();
						stats.scene.numPointsCulledObjects += static_cast<int>(CullMeshes
						(
							l->GetViewFrustumPlanes(CubemapFaceDirectionEnum),
							pObj,
							meshDrawData
						));
						meshListForPoints[face].push_back(meshDrawData);
#endif
					}
				}
			}
			else
			{
				for (int face = 0; face < 6; ++face)
				{
					for (const GameObject* pObj : mainViewShadowCasterRenderList)
					{
#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
						const XMMATRIX matWorld = pObj->GetTransform().WorldTransformationMatrix();
						for (MeshID meshID : pObj->GetModelData().mMeshIDs)
							meshDrawDataPerFace[face].AddMeshTransformation(meshID, matWorld);
#else
						meshListForPoints[face].push_back(pObj);
#endif
					}
				}
			}

#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
			mShadowView.shadowCubeMapMeshDrawListLookup[l] = meshDrawDataPerFace;
			for (int i = 0; i < 6; ++i) meshDrawDataPerFace[i].meshTransformListLookup.clear();
#else
			mShadowView.shadowCubeMapMeshDrawListLookup[l] = meshListForPoints;
			for (int i = 0; i < 6; ++i) meshListForPoints[i].clear();
#endif
			//mpCPUProfiler->EndEntry();
			break;
		} // light type


		objList.clear();
	} // for each light
}

void Scene::OcclusionCullDirectionalLightView()
{
	// TODO: consider this for directionals: http://stefan-s.net/?p=92 or umbra paper
}

//static void CalculateSceneBoundingBox(Scene* pScene, )
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
		pObj->mMeshBoundingBoxes.clear();
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

				vec3 mins_mesh(max_f);
				vec3 maxs_mesh(-(max_f - 1.0f));
				for (int i = 0; i < numVerts; ++i)
				{
					const float x_mesh_local = pData[i].position.x();
					const float y_mesh_local = pData[i].position.y();
					const float z_mesh_local = pData[i].position.z();

					mins_mesh = vec3
					(
						std::min(x_mesh_local, mins_mesh.x()),
						std::min(y_mesh_local, mins_mesh.y()),
						std::min(z_mesh_local, mins_mesh.z())
					);
					maxs_mesh = vec3
					(
						std::max(x_mesh_local, maxs_mesh.x()),
						std::max(y_mesh_local, maxs_mesh.y()),
						std::max(z_mesh_local, maxs_mesh.z())
					);


					const vec3 worldPos = vec3(XMVector4Transform(vec4(pData[i].position, 1.0f), worldMatrix));
					const float x_mesh = std::min(worldPos.x(), DegenerateMeshPositionChannelValueMax);
					const float y_mesh = std::min(worldPos.y(), DegenerateMeshPositionChannelValueMax);
					const float z_mesh = std::min(worldPos.z(), DegenerateMeshPositionChannelValueMax);


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

				pObj->mMeshBoundingBoxes.push_back(BoundingBox({ mins_mesh, maxs_mesh }));
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

//----------------------------------------------------------------------------------------------------------------
// RESOURCE MANAGEMENT FUNCTIONS
//----------------------------------------------------------------------------------------------------------------
GameObject* Scene::CreateNewGameObject(){ mpObjects.push_back(mObjectPool.Create(this)); return mpObjects.back(); }

Material* Scene::CreateNewMaterial(EMaterialType type){ return static_cast<Material*>(mMaterials.CreateAndGetMaterial(type)); }
Material* Scene::CreateRandomMaterialOfType(EMaterialType type) { return static_cast<Material*>(mMaterials.CreateAndGetRandomMaterial(type)); }



//----------------------------------------------------------------------------------------------------------------
// SCENE RESOURCE VIEW  FUNCTIONS
//----------------------------------------------------------------------------------------------------------------
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
