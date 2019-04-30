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
	#define ENABLE_FORCE_LOD_LEVELS 1
#endif

void LODManager::Initialize(const Camera& camera, const std::vector<GameObject*> pObjects)
{
	Reset();
	mpViewerPosition = &camera.mPosition;

#if ENABLE_FORCE_LOD_LEVELS
	this->mbEnableForceLODLevels = true;
#endif

	// create settings objects
	for (GameObject* pObj : pObjects)
	{
		LODSettings settings;

		mGameObjectLODSettingsLookup[pObj] = settings;
	}

	// set meshID references for the LOD settings
	for (GameObject* pObj : pObjects)
	{
		for (MeshID meshID : pObj->GetModelData().mMeshIDs)
		{
			mMeshLODSettingsLookup[meshID] = &mGameObjectLODSettingsLookup.at(pObj);
		}
	}

	mMeshLODUpdateList.resize(mGameObjectLODSettingsLookup.size());
}

void LODManager::Reset()
{
	mpViewerPosition = nullptr;
	mbEnableForceLODLevels = false;
	mForcedLODValue = 0;
	mGameObjectLODSettingsLookup.clear();
	mMeshLODSettingsLookup.clear();
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
#endif


	if (mbEnableForceLODLevels)
		;// return;

#if THREADED_UPDATE 
	// multi-threaded LOD update

#else // THREADED_UPDATE 
	// single-threaded LOD update

	const vec3 viewPos = *this->mpViewerPosition; // cache the memory indirection


	// calculate new lods and output a list of <MeshID, lodVal> pair.
	int numLODUpdates = 0;
	for (const std::pair<GameObject*, LODSettings>& kvp : mGameObjectLODSettingsLookup)
	{
		GameObject* const& pObj = kvp.first;
		const LODSettings& lodSettings = kvp.second;

		// calculate square distance to the viewer
		vec3 sqDistV = pObj->GetTransform()._position - viewPos;
		sqDistV = XMVector3Dot(sqDistV, sqDistV);
		const float& sqDist = sqDistV.x();

		const int newLODval = LODSettings::CalculateLODValueFromSquareDistance(sqDist, lodSettings.distanceThresholds);

		if (newLODval != lodSettings.lod)
		{
			mMeshLODUpdateList[numLODUpdates++] = MeshLODUpdateParams{ pObj, newLODval };
		}
	}

	// update the LOD values
	for(int i=0; i<numLODUpdates; ++i)
	{
		mGameObjectLODSettingsLookup.at(mMeshLODUpdateList[i].pObj).lod = mMeshLODUpdateList[i].newLOD;
	}
#endif // THREADED_UPDATE 
}

void LODManager::LateUpdate()
{
#if THREADED_UPDATE 

#endif // THREADED_UPDATE 
}

void LODManager::SetViewer(const Camera& camera) { mpViewerPosition = &camera.mPosition; }

int LODManager::GetLODValue(MeshID meshID) const
{
	return this->mbEnableForceLODLevels
		? this->mForcedLODValue
		: mMeshLODSettingsLookup.at(meshID)->lod;
}

int LODManager::LODSettings::CalculateLODValueFromSquareDistance(float sqDistance, const std::vector<float>& distanceThresholds)
{
	for (int lod = 0; lod < distanceThresholds.size(); ++lod)
	{
		if (sqDistance < (distanceThresholds[lod] * distanceThresholds[lod]))
		{
			return lod;
		}
	}
	return 0;
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
