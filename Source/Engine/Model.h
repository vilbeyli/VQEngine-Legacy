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

#include "Material.h"
#include "Mesh.h"

#include <vector>
#include <unordered_map>
#include <queue>

#include <mutex>

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
	std::vector<MeshID>		mTransparentMeshIDs;
	MeshToMaterialLookup	mMaterialLookupPerMesh;
	bool AddMaterial(MeshID meshID, MaterialID matID, bool bTransparent = false);
	inline bool HasMaterial() const { return !mMaterialLookupPerMesh.empty(); }
};


// Model struct encapsulates ... 
//
struct Model
{
	void AddMaterialToMesh(MeshID meshID, MaterialID materialID, bool bTransparent);
	void OverrideMaterials(MaterialID materialID);

	Model() = default;
	Model(const std::string&	directoryFullPath
		, const std::string&	modelName
		, ModelData&&			modelDataIn);


	ModelData		mData;
	
	//----------------------------------------------------------
	// NOTE:
	//
	// cold data (wasting cache line if Model[]s or anything 
	// that contains  a model in an array are iterated over)
	//----------------------------------------------------------
	std::string		mModelName;
	std::string		mModelDirectory;
	bool			mbLoaded = false;

	// queue of materials to be assigned in case the model has not been loaded yet.
	std::queue<MaterialID> mMaterialAssignmentQueue;
};

class ModelLoader
{
public:
	inline void Initialize(Renderer* pRenderer) { mpRenderer = pRenderer; }

	// Loads the Model in a serial fashion - blocks thread
	//
	Model	LoadModel(const std::string& modelPath, Scene* pScene);
	
	// Ends async loading.
	//
	Model	LoadModel_Async(const std::string& modelPath, Scene* pScene);


	void UnloadSceneModels(Scene* pScene);


private:
	static const char* sRootFolderModels;
	

	// Key -> Value := model_path -> ModelData
	using ModelLookUpTable = std::unordered_map<std::string, Model>;

	// Key -> Value := Scene* -> model_path[]
	using PerSceneModelNameLookupTable = std::unordered_map<Scene*, std::vector<std::string>>;


private:
	ModelLookUpTable				mLoadedModels;
	PerSceneModelNameLookupTable	mSceneModels;

	Renderer*						mpRenderer;

	std::mutex						mLoadedModelMutex;
	std::mutex						mSceneModelsMutex;
};