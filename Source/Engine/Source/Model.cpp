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

#include "Model.h"

#if _DEBUG
#include "Utilities/Log.h"
#endif

void Model::AddMaterialToMesh(MeshID meshID, MaterialID materialID)
{
	MeshToMaterialLookup& mMaterialLookupPerMesh = mData.mMaterialLookupPerMesh;
	auto it = mMaterialLookupPerMesh.find(meshID);
	if (it == mMaterialLookupPerMesh.end())
	{
		mMaterialLookupPerMesh[meshID] = materialID;
	}
	else
	{
#if _DEBUG
		Log::Warning("Overriding Material");
#endif
		mMaterialLookupPerMesh[meshID] = materialID;
	}
}

Model::Model(const std::string& directoryFullPath, const std::string & modelName, ModelData&& modelDataIn)
{
	mModelDirectory = directoryFullPath;
	mModelName = modelName;
	mData.mMeshIDs = std::move(modelDataIn.mMeshIDs);
	mData.mMaterialLookupPerMesh = std::move(modelDataIn.mMaterialLookupPerMesh);
	mbLoaded = true;
}




#include "Utilities/Log.h"
#include "Utilities/PerfTimer.h"

#include "Renderer/Renderer.h"

#include "Scene.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <functional>

const char* ModelLoader::sRootFolderModels = "Data/Models/";

using namespace Assimp;

Model ModelLoader::LoadModel(const std::string & modelPath, Scene* pScene)
{
	assert(mpRenderer);
	const std::string fullPath = sRootFolderModels + modelPath;
	const std::string modelDirectory = DirectoryUtil::GetFolderPath(fullPath);
	const std::string modelName = DirectoryUtil::GetFileNameWithoutExtension(fullPath);
	

	// ASSIMP_LOAD_LAMBDA_FUNCTIONS (nothing much interesting here)
	//
	#pragma region ASSIMP_LOAD_LAMBDA_FUNCTIONS
	using fnTypeProcessNode = std::function<ModelData(aiNode* const, const aiScene*, std::vector<Mesh>&)>;
	using fnTypeProcessMesh = std::function<Mesh(aiMesh*, const aiScene*)>;
	auto loadMaterialTextures = [&](aiMaterial* pMaterial, aiTextureType type, const std::string& textureName) -> std::vector<TextureID>
	{
		std::vector<TextureID> textures;
		for (unsigned int i = 0; i < pMaterial->GetTextureCount(type); i++)
		{
			aiString str;
			pMaterial->GetTexture(type, i, &str);
			std::string path = str.C_Str();
			textures.push_back(mpRenderer->CreateTextureFromFile(path, modelDirectory));
		}
		return textures;
	};
	fnTypeProcessMesh processMesh = [&](aiMesh * mesh, const aiScene * scene) -> Mesh
	{
		std::vector<DefaultVertexBufferData> Vertices;
		std::vector<unsigned> Indices;

		// Walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			DefaultVertexBufferData Vert;

			// POSITIONS
			Vert.position = vec3(
				mesh->mVertices[i].x,
				mesh->mVertices[i].y,
				mesh->mVertices[i].z
			);

			// NORMALS
			Vert.normal = vec3(
				mesh->mNormals[i].x,
				mesh->mNormals[i].y,
				mesh->mNormals[i].z
			);

			// TEXTURE COORDINATES
			// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't 
			// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
			Vert.uv = mesh->mTextureCoords[0]
				? vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
				: vec2(0, 0);

			// TANGENT
			if (mesh->mTangents)
			{
				Vert.tangent = vec3(
					mesh->mTangents[i].x,
					mesh->mTangents[i].y,
					mesh->mTangents[i].z
				);
			}

			// BITANGENT ( NOT USED )
			// Vert.bitangent = vec3(
			// 	mesh->mBitangents[i].x,
			// 	mesh->mBitangents[i].y,
			// 	mesh->mBitangents[i].z
			// );
			Vertices.push_back(Vert);
		}

		// now walk through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				Indices.push_back(face.mIndices[j]);
		}

		return Mesh(Vertices, Indices, "ImportedModelMesh0");	// return a mesh object created from the extracted mesh data
	};
	fnTypeProcessNode processNode = [&](aiNode* const pNode, const aiScene* pAiScene, std::vector<Mesh>& SceneMeshes) -> ModelData
	{
		ModelData modelData;
		std::vector<MeshID>& ModelMeshIDs = modelData.mMeshIDs;


		for (unsigned int i = 0; i < pNode->mNumMeshes; i++)
		{	// process all the node's meshes (if any)
			aiMesh *mesh = pAiScene->mMeshes[pNode->mMeshes[i]];

			// MATERIALS
			aiMaterial* material = pAiScene->mMaterials[mesh->mMaterialIndex];
			std::vector<TextureID> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
			//vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
			std::vector<TextureID> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
			//std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");

			Material* pBRDF = pScene->CreateNewMaterial(GGX_BRDF);
			assert(diffuseMaps.size() <= 1);
			assert(normalMaps.size() <= 1);
			if (!diffuseMaps.empty()) pBRDF->diffuseMap = diffuseMaps[0];
			if (!normalMaps.empty()) pBRDF->normalMap = normalMaps[0];

			aiColor3D color(0.f, 0.f, 0.f);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color))
			{
				pBRDF->diffuse = vec3(color.r, color.g, color.b);
			}

			aiColor3D specular(0.f, 0.f, 0.f);
			if (aiReturn_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, specular))
			{
				pBRDF->specular = vec3(specular.r, specular.g, specular.b);
			}


			SceneMeshes.push_back(processMesh(mesh, pAiScene));
			ModelMeshIDs.push_back(MeshID(SceneMeshes.size() - 1));
			modelData.mMaterialLookupPerMesh[ModelMeshIDs.back()] = pBRDF->ID;
		}
		for (unsigned int i = 0; i < pNode->mNumChildren; i++)
		{	// then do the same for each of its children
			ModelData childModelData = processNode(pNode->mChildren[i], pAiScene, SceneMeshes);
			std::vector<MeshID>& ChildMeshes = childModelData.mMeshIDs;

			std::copy(ChildMeshes.begin(), ChildMeshes.end(), std::back_inserter(ModelMeshIDs));
			for (auto& kvp : childModelData.mMaterialLookupPerMesh)
			{
#if _DEBUG
				assert(modelData.mMaterialLookupPerMesh.find(kvp.first) == modelData.mMaterialLookupPerMesh.end());
#endif
				modelData.mMaterialLookupPerMesh[kvp.first] = kvp.second;
			}
		}
		return modelData;
	};
	#pragma endregion 
	

	// CHECK CACHE FIRST - don't load the same model more than once
	//
	if (mLoadedModels.find(fullPath) != mLoadedModels.end())
	{	
		return mLoadedModels.at(fullPath);
	}

	PerfTimer t;
	t.Start();

	Log::Info("Loading Model: %s ...", modelName.c_str());

	// IMPORT SCENE
	//
	Importer importer;
	const aiScene* scene = importer.ReadFile(fullPath, aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		Log::Error("Assimp error: %s", importer.GetErrorString());
		assert(false);
	}
	ModelData data = processNode(scene->mRootNode, scene, pScene->mMeshes);

	// cache the model
	Model model = Model(modelDirectory, modelName, std::move(data));
	mLoadedModels[fullPath] = model;

	// register scene and the model loaded
	if (mSceneModels.find(pScene) == mSceneModels.end())	// THIS BEGS FOR TEMPLATE PROGRAMMING
	{	// first
		mSceneModels[pScene] = std::vector<std::string> { fullPath };
	}
	else
	{
		mSceneModels.at(pScene).push_back(fullPath);
	}
	t.Stop();
	Log::Info("Loaded Model '%s' in %.2f seconds.", modelName.c_str(), t.DeltaTime());
	return model;
}

#if 0
void ModelLoader::LoadModel_Begin(const std::string & modelPath, Scene * pScene)
{
}

Model ModelLoader::LoadModel_End(const std::string & modelPath)
{
	return Model();
}
#endif

void ModelLoader::UnloadSceneModels(Scene * pScene)
{
	if (mSceneModels.find(pScene) == mSceneModels.end()) return;
	for (std::string& modelDirectory : mSceneModels.at(pScene))
	{
		mLoadedModels.erase(modelDirectory);
	}
}
