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

#pragma once

#include "Material.h"
#include "Mesh.h"

#include <vector>
#include <unordered_map>

class GameObject;
struct aiScene;
struct aiNode;
struct aiMesh;
struct aiMaterial;
class Scene;


using MeshToMaterialLookup = std::unordered_map<MeshID, MaterialID>;

struct ModelData
{
	std::vector<MeshID>		mMeshIDs;
	MeshToMaterialLookup	mMaterialLookupPerMesh;
};

struct Model
{
	void AddMaterialToMesh(MeshID meshID, MaterialID materialID);
	Model() = default;
	Model(const std::string& directoryFullPath, const std::string& modelName, ModelData&& modelDataIn);

	ModelData mData;
	
	// cold data (wasting cache line if Model[]s or anything that contains a model in an array are iterated over)
	std::string				mModelName;
	std::string				mModelDirectory;
};

class ModelLoader
{
public:
	inline void Initialize(Renderer* pRenderer) { mpRenderer = pRenderer; }
	Model LoadModel(const std::string& modelPath, Scene* pScene);

private:
	static const char*						sRootFolderModels;
	
	std::unordered_map<std::string, Model>	mLoadedModels;
	Renderer*								mpRenderer;
};