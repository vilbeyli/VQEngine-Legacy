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

#include "GeometryGenerator.h"
#include "BufferObject.h"

GeometryGenerator::GeometryGenerator()
	:
	m_device(nullptr)
{}

GeometryGenerator::~GeometryGenerator()
{
}

BufferObject* GeometryGenerator::Triangle()
{
	HRESULT succeed;

	// create CPU buffer object
	//-----------------------------------------------------------------
	BufferObject* bufferObj = new BufferObject();	// deleted in renderer->exit()
	bufferObj->m_vertexCount = 3;
	bufferObj->m_indexCount = 3;
	bufferObj->m_vertices = new Vertex[bufferObj->m_vertexCount];	// deleted in dtor
	bufferObj->m_indices = new unsigned[bufferObj->m_indexCount];	// deleted in dtor
	

	// set geometry data
	//------------------------------------------------------------------
	const float size = 0.8f;

	// indices
	bufferObj->m_indices[0] = 0;
	bufferObj->m_indices[1] = 1;
	bufferObj->m_indices[2] = 2;

	// vertices
	bufferObj->m_vertices[0].position	= XMFLOAT3(-size, -size, 0.0f);
	bufferObj->m_vertices[0].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[0].texCoords	= XMFLOAT2(0.0f, 1.0f);

	bufferObj->m_vertices[1].position	= XMFLOAT3(0, size, 0.0f);
	bufferObj->m_vertices[1].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[1].texCoords	= XMFLOAT2(0.5f, 0.0f);

	bufferObj->m_vertices[2].position	= XMFLOAT3(size, -size, 0.0f);
	bufferObj->m_vertices[2].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[2].texCoords	= XMFLOAT2(1.0f, 1.0f);

	// buffer descriptions & GPU buffer objects
	//------------------------------------------------------------------
	bool writable = false;

	// vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage					= writable ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth				= sizeof(Vertex) * bufferObj->m_vertexCount;
	vertexBufferDesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags			= writable ? D3D11_CPU_ACCESS_WRITE : 0;
	vertexBufferDesc.MiscFlags				= 0;
	vertexBufferDesc.StructureByteStride	= 0;
	
	D3D11_SUBRESOURCE_DATA vertData;
	vertData.pSysMem			= bufferObj->m_vertices;
	vertData.SysMemPitch		= 0;
	vertData.SysMemSlicePitch	= 0;

	succeed = m_device->CreateBuffer(&vertexBufferDesc, &vertData, &bufferObj->m_vertexBuffer);
	if (FAILED(succeed))
	{
		// TODO: WARN
		return NULL;
	}

	// index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage				= D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth			= sizeof(unsigned) * bufferObj->m_indexCount;
	indexBufferDesc.BindFlags			= D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags		= 0;
	indexBufferDesc.MiscFlags			= 0;
	indexBufferDesc.StructureByteStride = 0;

	//set up index data
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem			= bufferObj->m_indices;
	indexData.SysMemPitch		= 0;
	indexData.SysMemSlicePitch	= 0;

	//create index buffer
	succeed = m_device->CreateBuffer(&indexBufferDesc, &indexData, &bufferObj->m_indexBuffer);
	if (FAILED(succeed))
	{
		// TODO: WARN
		return NULL;
	}

	return bufferObj;
}

BufferObject* GeometryGenerator::Quad()
{
	BufferObject* bufferObj = new BufferObject();


	return bufferObj;
}

BufferObject* GeometryGenerator::Cube()
{
	BufferObject* bufferObj = new BufferObject();


	return bufferObj;
}

BufferObject* GeometryGenerator::Sphere()
{
	BufferObject* bufferObj = new BufferObject();


	return bufferObj;
}
