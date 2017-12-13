//	DX11Renderer - VDemo | DirectX11 Renderer
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

#include "RenderingEnums.h"
#include "BufferObject.h"

#include "Utilities/utils.h"

#include <vector>

class Mesh
{
	friend class Renderer;
	static ID3D11Device* spDevice;	// initialized by Renderer
public:
	template<class VertexBufferType> Mesh(const std::vector<VertexBufferType>& vertices, const std::vector<unsigned>& indices);
	//	template<class VertexBufferType> Mesh(const std::vector<VertexBufferType>& vertices, const std::vector<unsigned>& indices, const std::vector<std::string> textureFileNames);	// TODO

	void Draw() const;

	void CleanUp();	// ctor initializes d3d buffer, should be cleaned up;
private:
	TextureID textures;
	Buffer	mVertexBuffer;
	Buffer	mIndexBuffer;
};

// template definitions
// --------------------------------------------------------------------------------------------------------------------------
template<class VertexBufferType> 
Mesh::Mesh(
	const std::vector<VertexBufferType>& vertices, 
	const std::vector<unsigned>& indices
)	:
	textures(-1)
{
	BufferDesc bufferDesc = {};

	bufferDesc.mType = VERTEX_BUFER;
	bufferDesc.mUsage = STATIC_RW;
	bufferDesc.mElementCount = static_cast<unsigned>(vertices.size());
	bufferDesc.mStride = sizeof(vertices[0]);
	mVertexBuffer = Buffer(bufferDesc); 
	mVertexBuffer.Initialize(spDevice, static_cast<const void*>(vertices.data()));
	
	bufferDesc.mType = INDEX_BUFFER;
	bufferDesc.mUsage = STATIC_RW;
	bufferDesc.mElementCount = static_cast<unsigned>(indices.size());
	bufferDesc.mStride = sizeof(unsigned);
	mIndexBuffer = Buffer(bufferDesc);
	mIndexBuffer.Initialize(spDevice, static_cast<const void*>(indices.data()));
}

#if 0	// TODO
template<class VertexBufferType> 
Mesh::Mesh(
	const std::vector<VertexBufferType>& vertices, 
	const std::vector<unsigned>& indices, 
	const std::vector<std::string> textureFileNames
)	:
	textures(-1)
{

	// todo: set texture array
}
#endif