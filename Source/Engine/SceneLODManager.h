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

#include <unordered_map>
#include <vector>

#include "Mesh.h"

#include "Utilities/vectormath.h"

#include "Application/HandleTypedefs.h"

class Camera;
class GameObject;

// TODO: either change std::vector to fixed sized float array
#define USE_NEW_CONTAINERS 0
constexpr size_t NUM_MAX_LOD_LEVELS = 6;
class LODManager
{
public:
	// Holds distance thresholds in world units for each LOD level.
	struct LODSettings
	{
		int   activeLOD;
#if USE_NEW_CONTAINERS
		std::unordered_map<MeshID, float[NUM_MAX_LOD_LEVELS]> meshLODDistanceLookup;
#else
		std::vector<float> distanceThresholds;
#endif
		//------------------------------------------------------------------------------------------------------------
		static int CalculateLODValueFromSquareDistance(float sqDistance, const std::vector<float>& distanceThresholds);
		static int CalculateLODValueFromDistance(float distance, const std::vector<float>& distanceThresholds);
	};

	LODManager(std::vector<Mesh>& meshes) : mMeshContainer(meshes) {}
	void Initialize(const Camera& camera, const std::vector<GameObject*>& pObjects);

	void Reset();
	void Update();
	void LateUpdate();

	inline void SetViewer(const vec3* pViewerPos) { mpViewerPosition = pViewerPos; }
	void SetViewer(const Camera& camera);
	
	int GetLODValue(const GameObject* pObj, MeshID meshID) const;

	void RegisterMeshLOD(const GameObject* pObj, MeshID meshID, const LODSettings& _LODSettings);


private:
	const vec3* mpViewerPosition = nullptr;
	bool mbEnableForceLODLevels = false;
	int  mForcedLODValue = 0;


	// Scene will have a container of GameObjects, but not all will have LOD settings
	// so LOD manager will only track what's necessary, stored in this container.
	// TODO: [PERF] static vs dynamic mesh lookup (should save a transform read cache miss for static object traversal)
	//
	std::vector<const GameObject*> mLODObjects;
	

	// LODSettings should exist per gameobject-meshID combination.
	//
	struct GameObjectLODSettings
	{
		// GameObjects can contain many meshes, and not all of them have the guarantee
		// for having LOD levels. Hence we only map the necessary IDs per game object.
		std::vector<MeshID>                     LODMeshes; // duplicates keys of meshLODSettingsLookup, consider iterating over keys?
		std::unordered_map<MeshID, LODSettings> meshLODSettingsLookup;
		const LODSettings& GetLODSettings(MeshID meshID) const;
	};
	std::unordered_map <const GameObject*, GameObjectLODSettings> mSceneObjectLODSettingsLookup;
	const LODSettings& GetLODSettings(const GameObject* pObj, MeshID meshID) const;


	std::vector<Mesh>&                           mMeshContainer; // unneeded

	struct MeshLODUpdateParams
	{
		const GameObject* pObj;
		MeshID meshID;
		int newLOD;
	};
	std::vector<MeshLODUpdateParams> mMeshLODUpdateList;


//
// STATICS
//
public:
	static void InitializeBuiltinMeshLODSettings();
private:
	static std::unordered_map<EGeometry, LODSettings> sBuiltinMeshLODSettings;
};