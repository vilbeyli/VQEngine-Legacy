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
	std::vector<MeshID>		mTransparentMeshIDs;
	MeshToMaterialLookup	mMaterialLookupPerMesh;
};

struct Model
{
	void AddMaterialToMesh(MeshID meshID, MaterialID materialID, bool bTransparent);

	Model() = default;
	Model(const std::string&	directoryFullPath
		, const std::string&	modelName
		, ModelData&&			modelDataIn);


	ModelData		mData;
	
	// cold data (wasting cache line if Model[]s or anything that contains a model in an array are iterated over)
	std::string		mModelName;
	std::string		mModelDirectory;
	bool			mbLoaded = false;
};

class ModelLoader
{
public:
	inline void Initialize(Renderer* pRenderer) { mpRenderer = pRenderer; }

	// Loads the Model in a serial fashion - blocks thread
	//
	Model	LoadModel(const std::string& modelPath, Scene* pScene);
	
#if 0
	// Begins async loading - doesn't block the thread
	//
	void	LoadModel_Begin(const std::string& modelPath, Scene* pScene);
	
	// Ends async loading.
	//
	Model	LoadModel_End(const std::string& modelPath);
#endif

	void UnloadSceneModels(Scene* pScene);


private:
	static const char*						sRootFolderModels;
	
	// Key -> Value := model_path -> ModelData
	using ModelLookUpTable = std::unordered_map<std::string, Model>;

	// Key -> Value := Scene* -> model_path[]
	using PerSceneModelNameLookupTable = std::unordered_map<Scene*, std::vector<std::string>>;

	ModelLookUpTable				mLoadedModels;
	PerSceneModelNameLookupTable	mSceneModels;

	Renderer*						mpRenderer;
};