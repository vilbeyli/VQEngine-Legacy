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
