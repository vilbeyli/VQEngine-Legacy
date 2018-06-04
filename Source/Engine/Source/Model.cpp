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

void Model::AddMaterialToMesh(MeshID meshID, MaterialID materialID)
{
	auto it = mMaterialLookupPerMesh.find(meshID);
	if (it == mMaterialLookupPerMesh.end())
	{
		mMaterialLookupPerMesh[meshID].push_back(materialID);
	}
	else
	{
		mMaterialLookupPerMesh.at(meshID).push_back(materialID);
	}
}




#include "Utilities/Log.h"

#include "Renderer/Renderer.h"

#include "Scene.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

const char* ModelLoader::sRootFolderModels = "Data/Models/";

using namespace Assimp;

void ModelLoader::LoadModel(GameObject*& pObj, const std::string & modelPath, Scene* pScene)
{
	const std::string fullPath = sRootFolderModels + modelPath;

	Importer importer;
	const aiScene* scene = importer.ReadFile(fullPath, aiProcess_Triangulate | aiProcess_FlipUVs);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		Log::Error("Assimp error: %s", importer.GetErrorString());
		assert(false);
	}

	// TODO: set models directory
	// directory = path.substr(0, path.find_last_of('/'));

	std::vector<MeshID> mMeshes = processNode(scene->mRootNode, scene, pScene->mMeshes);
	for(MeshID id : mMeshes)
		pObj->AddMesh(id);
}



std::vector<MeshID> ModelLoader::processNode(aiNode* const pNode, const aiScene * pScene, std::vector<Mesh>& SceneMeshes)
{
	std::vector<MeshID> ModelMeshes;

	// process all the node's meshes (if any)
	for (unsigned int i = 0; i < pNode->mNumMeshes; i++)
	{
		aiMesh *mesh = pScene->mMeshes[pNode->mMeshes[i]];
		SceneMeshes.push_back(processMesh(mesh, pScene));
		ModelMeshes.push_back(SceneMeshes.size() - 1);
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < pNode->mNumChildren; i++)
	{
		std::vector<MeshID> ChildMeshes = processNode(pNode->mChildren[i], pScene, SceneMeshes);
		std::copy(ChildMeshes.begin(), ChildMeshes.end(), std::back_inserter(ModelMeshes));
	}
	return ModelMeshes;
}

#if 0
void ModelLoader::processMesh(aiMesh * mesh, const aiScene * scene) {}
#else
Mesh ModelLoader::processMesh(aiMesh * mesh, const aiScene * scene)
{
	std::vector<DefaultVertexBufferData> Vertices;
	std::vector<unsigned> Indices;

	// Walk through each of the mesh's vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		DefaultVertexBufferData Vert;
		// we declare a placeholder vector since assimp uses its own vector class that doesn't directly 
		// convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
		
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

	// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		// retrieve all indices of the face and store them in the indices vector
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			Indices.push_back(face.mIndices[j]);
	}

#if 0
	//vector<Texture> textures;
	// process materials
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
	// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
	// Same applies to other texture as the following list summarizes:
	// diffuse: texture_diffuseN
	// specular: texture_specularN
	// normal: texture_normalN

	// 1. diffuse maps
	vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
	textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
	// 2. specular maps
	//vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
	//textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
	// 3. normal maps
	std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
	textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
	// 4. height maps
	//std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
	//textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
#endif

	// return a mesh object created from the extracted mesh data
	return Mesh(Vertices, Indices, "ImportedModelMesh0");
}
#endif
