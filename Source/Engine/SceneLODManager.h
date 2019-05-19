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
class Renderer;

// Scene LOD Manager
//
// Supports Level of Detail functionality for a limited number of meshes, where 
// the  detail mesh to render is selected based on viewer distance.
//
// Limitations:
//
//  - LOD is implemented only for the following meshes: Cylinder, Sphere, Cone & Tessellated Grid.
//    Mesh struct has the LOD support in it so the functionality can be extended to any mesh if needed.
//
//  - LOD distances are calculated based on points, and not using bounds for the LOD object.
//    This makes the LOD distance thresholds (at which distance LOD level changes) not very generic.
//    This can be improved by using bounds for each LOD object during the distance
//    calculation instead of a single point (game objects transform.position).
//
//  - GetLODValue() requires GameObject* to access the LOD level of the mesh as LOD meshes are
//    registered with a game object pointer. GBuffer pass does only have MeshID -> material info
//    and by default uses LOD level 0 for meshes that has support of LOD functionality.
//
//  - Update() is single threaded and the LOD distance calculation loop is not cache friendly
//    due to how transform.position is stored and read. This can be fixed by the following:
//      - Differentiate between dynamic and static objects (moving / non-moving) and store their Transforms
//        in different containers so iterating over them doesn't waste cache like iterating over GameObjects
//        and accessing their transforms individually. 
//      - Static Transforms can be cached but dynamic transforms will still require a read from game objects,
//        unless GameObject class is refactored to have TransformIDs instead of Transforms, and all Transforms
//        are stored in elsewhere in a consecutive manner and accessed as such.
//
constexpr size_t NUM_MAX_LOD_LEVELS = 6;
class LODManager
{
public:
	struct LODSettings;

	LODManager() = delete;
	LODManager(std::vector<Mesh>& meshes, Renderer* pRenderer) : mMeshContainer(meshes), mpRenderer(pRenderer) {}
	void Initialize(const Camera& camera, const std::vector<GameObject*>& pObjects);

	void Reset();
	void Update();
	void LateUpdate();

	inline void SetViewer(const vec3* pViewerPos) { mpViewerPosition = pViewerPos; }
	void SetViewer(const Camera& camera);
	
	const LODSettings& GetMeshLODSettings(const GameObject* pObj, MeshID meshID) const;
	int GetLODValue(const GameObject* pObj, MeshID meshID) const;

	void RegisterMeshLOD(const GameObject* pObj, MeshID meshID, const LODSettings& _LODSettings);


	// Holds distance thresholds in world units for each LOD level.
	// LODSettings should exist per gameobject-meshID combination.
	//
	struct LODSettings	
	{
		int   activeLOD = 0;
		std::vector<float> distanceThresholds;
		//------------------------------------------------------------------------------------------------------------
		static int CalculateLODValueFromSquareDistance(float sqDistance, const std::vector<float>& distanceThresholds);
		static int CalculateLODValueFromDistance(float distance, const std::vector<float>& distanceThresholds);
	};

private:
	// GameObjects can contain many meshes, and not all of them have the guarantee
	// for having LOD levels. Hence we only map the necessary IDs per game object.
	struct GameObjectLODSettings
	{
		std::vector<MeshID>                     LODMeshes; // duplicates keys of meshLODSettingsLookup, consider iterating over keys?
		std::unordered_map<MeshID, LODSettings> meshLODSettingsLookup;
		const LODSettings& GetLODSettings(MeshID meshID) const;
	};

	// Threaded update parameters
	struct MeshLODUpdateParams
	{
		const GameObject* pObj;
		MeshID meshID;
		int newLOD;
	};



	//
	// DATA
	//
	const vec3* mpViewerPosition = nullptr;
	bool mbEnableForceLODLevels = false;
	int  mForcedLODValue = 0;


	// Scene will have a container of GameObjects, but not all will have LOD settings
	// so LOD manager will only track what's necessary, stored in this container.
	// TODO: [PERF] static vs dynamic mesh lookup (should save a transform read cache miss for static object traversal)
	std::vector<const GameObject*>                                mLODObjects;
	std::unordered_map <const GameObject*, GameObjectLODSettings> mSceneObjectLODSettingsLookup;

	std::vector<MeshLODUpdateParams>                              mMeshLODUpdateList;

	// used to access LOD info for reporting.
	std::vector<Mesh>&                                            mMeshContainer;
	Renderer*                                                     mpRenderer;
private:

	const LODSettings& GetLODSettings(const GameObject* pObj, MeshID meshID) const;
	void HandleInput();

	void UpdateLODs_SingleThread();
	void UpdateLODs_MultiThreaded_Begin();
	void UpdateLODs_MultiThreaded_End();

	//
	// STATICS
	//
public:
	static void InitializeBuiltinMeshLODSettings();
private:
	static std::unordered_map<EGeometry, LODSettings> sBuiltinMeshLODSettings;
};