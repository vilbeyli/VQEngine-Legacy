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

#include "Renderer/RenderingEnums.h"
#include "Renderer/BufferObject.h"

#include "Utilities/utils.h"

#include <vector>

using MeshID = int;
class Mesh
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

	std::string mMeshName;
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
	bufferDesc.mUsage = STATIC_RW;
	bufferDesc.mElementCount = static_cast<unsigned>(vertices.size());
	bufferDesc.mStride = sizeof(vertices[0]);
	mVertexBufferID = spRenderer->CreateBuffer(bufferDesc, vertices.data());

	bufferDesc.mType = INDEX_BUFFER;
	bufferDesc.mUsage = STATIC_RW;
	bufferDesc.mElementCount = static_cast<unsigned>(indices.size());
	bufferDesc.mStride = sizeof(unsigned);
	mIndexBufferID = spRenderer->CreateBuffer(bufferDesc, indices.data());

	mMeshName = name;
}
