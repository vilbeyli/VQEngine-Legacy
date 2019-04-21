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

#define MESH_LOD_SYSTEM 0

#if !MESH_LOD_SYSTEM
class Mesh // TODO: struct
{

	friend class Renderer;
	static Renderer* spRenderer;


public:
	template<class VertexBufferType> 
	Mesh(const std::vector<VertexBufferType>& vertices, const std::vector<unsigned>& indices, const std::string& name);
	//	template<class VertexBufferType> Mesh(const std::vector<VertexBufferType>& vertices, const std::vector<unsigned>& indices, const std::vector<std::string> textureFileNames);	// TODO

	inline std::pair<BufferID, BufferID> GetIABuffers() const { return std::make_pair(mVertexBufferID, mIndexBufferID); }

	Mesh() = default;

private:
	BufferID  mVertexBufferID = -1;
	BufferID  mIndexBufferID = -1;

	// TODO: LOD
	//std::vector<> mLOD

	std::string mMeshName; // TODO: move to scene manager
};


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
	mVertexBufferID = spRenderer->CreateBuffer(bufferDesc, vertices.data());

	bufferDesc.mType = INDEX_BUFFER;
	bufferDesc.mUsage = GPU_READ_WRITE;
	bufferDesc.mElementCount = static_cast<unsigned>(indices.size());
	bufferDesc.mStride = sizeof(unsigned);
	mIndexBufferID = spRenderer->CreateBuffer(bufferDesc, indices.data());

	mMeshName = name;
}

#else



struct LODLevel
{
	int LODValue = 0;
	BufferID  mVertexBufferID = -1;
	BufferID  mIndexBufferID = -1;

	inline std::pair<BufferID, BufferID> GetIABufferPair() const { return std::make_pair(mVertexBufferID, mIndexBufferID); }
};

struct Mesh
{
public:
	std::pair<BufferID, BufferID> GetIABuffers(int lod = 0);
	
private:
	std::vector<LODLevel> mLODs;
};



#endif