#pragma once

#include <DirectXMath.h>
#include <d3d11_1.h>

using namespace DirectX;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT2 texCoords;
};

class BufferObject
{
public:
	BufferObject();
	~BufferObject();

public:
	ID3D11Buffer*	m_vertexBuffer;
	ID3D11Buffer*	m_indexBuffer;

	// cpu data
	unsigned		m_vertexCount;
	unsigned		m_indexCount;
	Vertex*			m_vertices;
	unsigned*		m_indices;
};

