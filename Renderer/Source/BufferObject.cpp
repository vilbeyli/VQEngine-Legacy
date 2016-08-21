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

#include "BufferObject.h"



BufferObject::BufferObject()
{
}


BufferObject::~BufferObject()
{
	if(m_vertices)
	{ 
		delete[] m_vertices;
		m_vertices = nullptr;
	}

	if (m_indices)
	{
		delete[] m_indices;
		m_indices = nullptr;
	}

	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}

	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}

}

bool BufferObject::FillGPUBuffers(ID3D11Device* device, bool writable)
{
		// vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.Usage					= writable ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth				= sizeof(Vertex) * m_vertexCount;
	vertexBufferDesc.BindFlags				= D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags			= writable ? D3D11_CPU_ACCESS_WRITE : 0;
	vertexBufferDesc.MiscFlags				= 0;
	vertexBufferDesc.StructureByteStride	= 0;
	
	D3D11_SUBRESOURCE_DATA vertData;
	vertData.pSysMem			= m_vertices;
	vertData.SysMemPitch		= 0;
	vertData.SysMemSlicePitch	= 0;

	if (FAILED(device->CreateBuffer(
		&vertexBufferDesc, 
		&vertData, 
		&m_vertexBuffer)))
	{
		OutputDebugString("Error: Failed to create vertex buffer!");
		return false;
	}

	// index buffer
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.Usage				= D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth			= sizeof(unsigned) * m_indexCount;
	indexBufferDesc.BindFlags			= D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags		= 0;
	indexBufferDesc.MiscFlags			= 0;
	indexBufferDesc.StructureByteStride = 0;

	//set up index data
	D3D11_SUBRESOURCE_DATA indexData;
	indexData.pSysMem			= m_indices;
	indexData.SysMemPitch		= 0;
	indexData.SysMemSlicePitch	= 0;

	//create index buffer
	if (FAILED(device->CreateBuffer(
		&indexBufferDesc, 
		&indexData, 
		&m_indexBuffer)))
	{
		OutputDebugString("Error: Failed to create index buffer!");
		return false;
	}

	return true;
}
