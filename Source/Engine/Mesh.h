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

#include "Renderer/RenderingEnums.h"
#include "Renderer/BufferObject.h"

#include "Utilities/utils.h"

#include <vector>


struct LODLevel
{
	BufferID  mVertexBufferID = -1;
	BufferID  mIndexBufferID = -1;

	inline std::pair<BufferID, BufferID> GetIABufferPair() const { return std::make_pair(mVertexBufferID, mIndexBufferID); }
};

template<class VertexBufferType>
struct MeshLODData
{
	MeshLODData< VertexBufferType >() = delete;
	MeshLODData< VertexBufferType >(int numLODs, const char* pMeshName)
		: LODVertices(numLODs)
		, LODIndices(numLODs)
		, meshName(pMeshName)
	{}
	
	std::vector<std::vector<VertexBufferType>> LODVertices;
	std::vector<std::vector<unsigned>> LODIndices ;
	std::string meshName;
};

struct Mesh
{
public:
	friend class Renderer;
	
	// Renderer* is used to create Vertex/Index buffer data from
	// Mesh constructor as the constructor receives raw vertex/index
	// buffer data.
	static Renderer* spRenderer;


	//
	// Constructors / Operators
	//
	template<class VertexBufferType>
	Mesh(
		const std::vector<VertexBufferType>& vertices,
		const std::vector<unsigned>&         indices,
		const std::string&                   name
	);

	template<class VertexBufferType>
	Mesh(const MeshLODData<VertexBufferType>& meshLODData);

	Mesh() = default;
	// Mesh() = delete;
	// Mesh(const Mesh&) = delete; // Model.cpp uses copy
	//Mesh(const Mesh&&) = delete;
	//void operator=(const Mesh&) = delete;



	//
	// Interface
	//
	std::pair<BufferID, BufferID> GetIABuffers(int lod = 0) const;

	
private:
	std::vector<LODLevel> mLODs;

	// Note:
	//
	// if performance of reading Mesh data becomes an issue,
	// std::vector<>s above could be turned into fixed sized
	// arrays to ensure memory alignment. Then, mMeshName 
	// should be tracked by the scene manager through a lookup
	// table to utilize cache lines better.
	//
	// The proposed changes require interface changes to the 
	// constructor, as the systems (GeometryGenerator, Model)
	// that use the constructor provide mesh name without a 
	// scene context.
	//
	std::string mMeshName;
};


//
// Template Definitions
//
template<class VertexBufferType>
Mesh::Mesh(
	const std::vector<VertexBufferType>& vertices,
	const std::vector<unsigned>& indices,
	const std::string& name
)
{
	BufferDesc bufferDesc = {};

	bufferDesc.mType = VERTEX_BUFER;
	bufferDesc.mUsage = GPU_READ_WRITE;
	bufferDesc.mElementCount = static_cast<unsigned>(vertices.size());
	bufferDesc.mStride = sizeof(vertices[0]);
	BufferID vertexBufferID = spRenderer->CreateBuffer(bufferDesc, vertices.data());

	bufferDesc.mType = INDEX_BUFFER;
	bufferDesc.mUsage = GPU_READ_WRITE;
	bufferDesc.mElementCount = static_cast<unsigned>(indices.size());
	bufferDesc.mStride = sizeof(unsigned);
	BufferID indexBufferID = spRenderer->CreateBuffer(bufferDesc, indices.data());

	mLODs.push_back({ vertexBufferID, indexBufferID }); // LOD Level 0
	mMeshName = name;
}

template<class VertexBufferType>
Mesh::Mesh(const MeshLODData<VertexBufferType>& meshLODData)
{
	for (size_t LOD = 0; LOD < meshLODData.LODVertices.size(); ++LOD)
	{
		BufferDesc bufferDesc = {};

		bufferDesc.mType = VERTEX_BUFER;
		bufferDesc.mUsage = GPU_READ_WRITE;
		bufferDesc.mElementCount = static_cast<unsigned>(meshLODData.LODVertices[LOD].size());
		bufferDesc.mStride = sizeof(meshLODData.LODVertices[LOD][0]);
		BufferID vertexBufferID = spRenderer->CreateBuffer(bufferDesc, meshLODData.LODVertices[LOD].data());

		bufferDesc.mType = INDEX_BUFFER;
		bufferDesc.mUsage = GPU_READ_WRITE;
		bufferDesc.mElementCount = static_cast<unsigned>(meshLODData.LODIndices[LOD].size());
		bufferDesc.mStride = sizeof(unsigned);
		BufferID indexBufferID = spRenderer->CreateBuffer(bufferDesc, meshLODData.LODIndices[LOD].data());

		mLODs.push_back({ vertexBufferID, indexBufferID });
	}
	mMeshName = meshLODData.meshName;
}
