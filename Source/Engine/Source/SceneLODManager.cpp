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

#include "SceneLODManager.h"
#include "Camera.h"
#include "GameObject.h"

#include "Application/Input.h"
#include "Engine.h"

#include "Utilities/Log.h"

#define THREADED_UPDATE 0
#define DEBUG_LOD_LEVELS 1

#if DEBUG_LOD_LEVELS
	#define ENABLE_FORCE_LOD_LEVELS 0
#endif

void LODManager::Initialize(const Camera& camera, const std::vector<GameObject*>& pObjects)
{
	Reset();
	mpViewerPosition = &camera.mPosition;

#if ENABLE_FORCE_LOD_LEVELS
	this->mbEnableForceLODLevels = true;
#endif

	// create settings objects
	size_t sz_MeshLODUpdateList = 0;
	for (GameObject* pObj : pObjects)
	{
		if (pObj->GetModelData().mMeshIDs.empty())
		{
			Log::Warning("LODManager::Initialize() Obj with no model/mesh.");
			continue;
		}

		// only a few built-in meshes support LOD levels
		const MeshID objMeshID = pObj->GetModelData().mMeshIDs.back();
		const bool bObjHasBuiltinMesh = objMeshID < EGeometry::MESH_TYPE_COUNT;
		if (!bObjHasBuiltinMesh)
		{
			continue;
		}

		// check if LOD settings exist for the built-in mesh
		const EGeometry meshLODSettingsKey = static_cast<EGeometry>(objMeshID);
		const bool bMeshLODSettingExists = sBuiltinMeshLODSettings.find(meshLODSettingsKey) != sBuiltinMeshLODSettings.end();
		if (!bMeshLODSettingExists)
		{
			continue;
		}

		// LODSettings objects carry the 'current LOD level' information,
		// hence should be attached to all objects that the LODManager will track.
		LODSettings lodSettings = sBuiltinMeshLODSettings.at(meshLODSettingsKey);
		for (int i = 0; i < lodSettings.distanceThresholds.size(); ++i)
		{
			const vec3& scl = pObj->GetTransform()._scale;
			//lodSettings.distanceThresholds[i] *= std::max(std::max(scl.x(), scl.y()), scl.z());
		}

		// register object with LODSettings.
		RegisterMeshLOD(pObj, objMeshID, lodSettings);
		++sz_MeshLODUpdateList;
	}

	mMeshLODUpdateList.resize(sz_MeshLODUpdateList);
}

void LODManager::Reset()
{
	mpViewerPosition = nullptr;
	mbEnableForceLODLevels = false;
	mForcedLODValue = 0;
	mSceneObjectLODSettingsLookup.clear();
	mMeshLODUpdateList.clear();
}

void LODManager::Update()
{
	if (!mpViewerPosition)
	{
		Log::Warning("LODManager::Update() called with null viewer!");
		return;
	}


#if DEBUG_LOD_LEVELS
	if (ENGINE->INP()->IsKeyTriggered("]"))
	{
		this->mForcedLODValue += 1;
		Log::Info("Force LOD Level = %d", this->mForcedLODValue);
	}
	if (ENGINE->INP()->IsKeyTriggered("["))
	{
		this->mForcedLODValue = std::max(0, this->mForcedLODValue - 1);
		Log::Info("Force LOD Level = %d", this->mForcedLODValue);
	}
#if 0
	if (ENGINE->INP()->IsKeyTriggered("L"))
	{
		for (const GameObject* pObj : mLODObjects)
		{
			for (MeshID meshID : mSceneObjectLODSettingsLookup.at(pObj).LODMeshes)
			{
				const LODSettings& lodSettings = GetLODSettings(pObj, meshID);
				// TODO: print LOD settings for each object that has mesh LODs
			}
		}
	}
#endif
#endif


	if (mbEnableForceLODLevels)
		;// return;

#if THREADED_UPDATE 
	// multi-threaded LOD update

#else // THREADED_UPDATE 
	// single-threaded LOD update

	const vec3 viewPos = *this->mpViewerPosition; // cache the memory indirection


	// calculate new LODs and output a list of <MeshID, lodVal> pair.
	int numLODUpdates = 0;
	for(const GameObject* pObj : mLODObjects)
	{
		for (MeshID meshID : mSceneObjectLODSettingsLookup.at(pObj).LODMeshes)
		{
			const LODSettings& lodSettings = GetLODSettings(pObj, meshID);

			// calculate square distance to the viewer
			vec3 sqDistV = pObj->GetTransform()._position - viewPos;
			sqDistV = XMVector3Dot(sqDistV, sqDistV);
			const float& sqDist = sqDistV.x();

			const int newLODval = LODSettings::CalculateLODValueFromSquareDistance(sqDist, lodSettings.distanceThresholds);
			if (newLODval != lodSettings.activeLOD)
			{
				mMeshLODUpdateList[numLODUpdates].pObj = pObj;
				mMeshLODUpdateList[numLODUpdates].meshID = meshID;
				mMeshLODUpdateList[numLODUpdates].newLOD = newLODval;
				++numLODUpdates;
			}
		}
	}

	// update the LOD values
	for(int i=0; i<numLODUpdates; ++i)
	{
		mSceneObjectLODSettingsLookup.at(mMeshLODUpdateList[i].pObj)
			.meshLODSettingsLookup.at(mMeshLODUpdateList[i].meshID)
			.activeLOD = mMeshLODUpdateList[i].newLOD;
	}
#endif // THREADED_UPDATE 
}

void LODManager::LateUpdate()
{
#if THREADED_UPDATE 

#endif // THREADED_UPDATE 
}

void LODManager::SetViewer(const Camera& camera) { mpViewerPosition = &camera.mPosition; }

const LODManager::LODSettings& LODManager::GetMeshLODSettings(const GameObject* pObj, MeshID meshID) const
{

	const GameObjectLODSettings& objLODSettings = mSceneObjectLODSettingsLookup.at(pObj);

	if (objLODSettings.meshLODSettingsLookup.find(meshID) == objLODSettings.meshLODSettingsLookup.end())
	{
		assert(false);
	}

	return objLODSettings.meshLODSettingsLookup.at(meshID);
}

int LODManager::GetLODValue(const GameObject* pObj, MeshID meshID) const
{
	// TODO: shadow map pass is set with only MeshID's and the
	//       pass doesn't know about 'game objects'. 
	//
	//       Need to:
	//        * find a way to quickly fetch LODSettings with only a meshID,
	//          not sure if searching for meshID->pObj is a good idea...
	//          meshID's are, in theory, only associated with Transforms
	//          rather than game objects, so that might be an option?
	//       OR 
	//        * change the shadow pass to include game object pointers as well
	//          for the parameter to the SceneResourceView::GetVertexAndIndexBufferIDsOfMesh()
	//          function, which calls this function to get an LOD level.
	//
	// This will result in UI updating with correct stats but renderer
	// only seeing LOD=0 for all meshes because they're not providing pObj.
	//
#if 0 // disable assert for now
	assert(pObj);
#endif

	assert(meshID != -1);


#if ENABLE_FORCE_LOD_LEVELS
	if (this->mbEnableForceLODLevels) // this if is unnecessary
	{
		return this->mForcedLODValue;
	}
#endif
	
	if (mSceneObjectLODSettingsLookup.find(pObj) == mSceneObjectLODSettingsLookup.end())
	{
#if _DEBUG
		if(std::find(RANGE(this->mLODObjects), pObj) != this->mLODObjects.end())
			Log::Warning("LODManager::GetLODValue(): MeshID=%d of GameObj*=TODO doesn't exist in SceneMeshLODSettingsLookup table.", meshID); // TODO msg
#endif
		return 0;
	}

	const GameObjectLODSettings& objLODSettings = mSceneObjectLODSettingsLookup.at(pObj);

	if (objLODSettings.meshLODSettingsLookup.find(meshID) == objLODSettings.meshLODSettingsLookup.end())
	{
		Log::Warning("LODManager::GetLODValue(): MeshID=%d of GameObj*=TODO doesn't exist in SceneMeshLODSettingsLookup table.", meshID); // TODO msg
		return 0;
	}

	return objLODSettings.meshLODSettingsLookup.at(meshID).activeLOD;
}


void LODManager::RegisterMeshLOD(const GameObject* pObj, MeshID meshID, const LODSettings& _LODSettings)
{
	// register the pObj if it doesn't exist in the lookup
	if (mSceneObjectLODSettingsLookup.find(pObj) == mSceneObjectLODSettingsLookup.end())
	{
		GameObjectLODSettings& objLODSettings = mSceneObjectLODSettingsLookup[pObj];
		objLODSettings.LODMeshes.push_back(meshID);
		objLODSettings.meshLODSettingsLookup[meshID] = _LODSettings;
	}

	mLODObjects.push_back(pObj);
}

std::unordered_map<EGeometry, LODManager::LODSettings> LODManager::sBuiltinMeshLODSettings;

int LODManager::LODSettings::CalculateLODValueFromSquareDistance(float sqDistance, const std::vector<float>& distanceThresholds)
{
	for (int lod = 0; lod < distanceThresholds.size(); ++lod)
	{
		if (sqDistance < (distanceThresholds[lod] * distanceThresholds[lod]))
		{
			return lod;
		}
	}
	return static_cast<int>(distanceThresholds.size()-1);
}

int LODManager::LODSettings::CalculateLODValueFromDistance(float distance, const std::vector<float>& distanceThresholds)
{
	for (int lod = 0; lod < distanceThresholds.size(); ++lod)
	{
		if (distance < distanceThresholds[lod])
		{
			return lod;
		}
	}
	return 0;
}


const LODManager::LODSettings& LODManager::GetLODSettings(const GameObject* pObj, MeshID meshID) const
{
	// TODO: error check + report
	const GameObjectLODSettings& objLODSettings = mSceneObjectLODSettingsLookup.at(pObj);
	return objLODSettings.meshLODSettingsLookup.at(meshID);
}

void LODManager::InitializeBuiltinMeshLODSettings()
{
	LODSettings coneSettings;
	LODSettings gridSettings;
	LODSettings sphereSettings;
	LODSettings cylinderSettings;

	coneSettings.distanceThresholds.push_back(100.0f);
	coneSettings.distanceThresholds.push_back(300.0f);
	coneSettings.distanceThresholds.push_back(400.0f);
	coneSettings.distanceThresholds.push_back(600.0f);
	coneSettings.distanceThresholds.push_back(800.0f);


	gridSettings.distanceThresholds.push_back(70.0f);
	gridSettings.distanceThresholds.push_back(150.0f);
	gridSettings.distanceThresholds.push_back(400.0f);
	gridSettings.distanceThresholds.push_back(700.0f);
	gridSettings.distanceThresholds.push_back(1000.0f);


	sphereSettings.distanceThresholds.push_back(80.0f);
	sphereSettings.distanceThresholds.push_back(220.0f);
	sphereSettings.distanceThresholds.push_back(400.0f);
	sphereSettings.distanceThresholds.push_back(800.0f);
	sphereSettings.distanceThresholds.push_back(1000.0f);


	cylinderSettings.distanceThresholds.push_back(70.0f);
	cylinderSettings.distanceThresholds.push_back(150.0f);
	cylinderSettings.distanceThresholds.push_back(300.0f);
	cylinderSettings.distanceThresholds.push_back(700.0f);
	cylinderSettings.distanceThresholds.push_back(1000.0f);

	sBuiltinMeshLODSettings[EGeometry::CONE]     = coneSettings;
	sBuiltinMeshLODSettings[EGeometry::GRID]     = gridSettings;
	sBuiltinMeshLODSettings[EGeometry::SPHERE]   = sphereSettings;
	sBuiltinMeshLODSettings[EGeometry::CYLINDER] = cylinderSettings;
}

const LODManager::LODSettings& LODManager::GameObjectLODSettings::GetLODSettings(MeshID meshID) const
{
	// TODO: error check + report
	return meshLODSettingsLookup.at(meshID);
}
