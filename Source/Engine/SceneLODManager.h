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

class LODManager
{
public:
	// Holds distance thresholds in world units for each LOD level.
	struct LODSettings
	{
		int lod;
		std::vector<float> distanceThresholds;
		static int CalculateLODValueFromSquareDistance(float sqDistance, const std::vector<float>& distanceThresholds);
		static int CalculateLODValueFromDistance(float distance, const std::vector<float>& distanceThresholds);
	};

	LODManager(std::vector<Mesh>& meshes) : mMeshContainer(meshes) {}
	void Initialize(const Camera& camera, const std::vector<GameObject*> pObjects);
	void Reset();
	void Update();
	void LateUpdate();

	inline void SetViewer(const vec3* pViewerPos) { mpViewerPosition = pViewerPos; }
	void SetViewer(const Camera& camera);
	
	int GetLODValue(MeshID meshID) const;

private:


private:
	const vec3* mpViewerPosition = nullptr;
	bool mbEnableForceLODLevels = false;
	int  mForcedLODValue = 0;

	// TODO: static vs dynamic mesh lookup (should save a transform read cache miss for static object traversal)
	std::unordered_map<GameObject*, LODSettings>        mGameObjectLODSettingsLookup;
	std::unordered_map<MeshID     , const LODSettings*> mMeshLODSettingsLookup;
	std::vector<Mesh>&                           mMeshContainer; // unneeded

	struct MeshLODUpdateParams
	{
		GameObject* pObj;
		int newLOD;
	};
	std::vector<MeshLODUpdateParams> mMeshLODUpdateList;
};