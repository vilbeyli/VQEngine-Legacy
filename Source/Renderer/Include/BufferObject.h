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

#include <DirectXMath.h>
#include <d3d11_1.h>
#include "utils.h"

using namespace DirectX;

typedef int BufferID;

struct Vertex
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	XMFLOAT2 texCoords;
};

class BufferObject
{
public:
	BufferObject() = default;
	~BufferObject();

	bool FillGPUBuffers(ID3D11Device* device, bool writable);

public:
	// gpu data
	ID3D11Buffer*	m_vertexBuffer;
	ID3D11Buffer*	m_indexBuffer;

	// cpu data
	Vertex*			m_vertices;
	unsigned*		m_indices;

	unsigned		m_vertexCount;
	unsigned		m_indexCount;
};

