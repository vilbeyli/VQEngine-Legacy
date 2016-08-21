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

// Direct3D Transformation Pipeline: https://msdn.microsoft.com/en-us/library/windows/desktop/ee418867(v=vs.85).aspx

GeometryGenerator::GeometryGenerator()
	:
	m_device(nullptr)
{}

GeometryGenerator::~GeometryGenerator()
{
}

BufferObject* GeometryGenerator::Triangle()
{
	// create CPU buffer object
	//-----------------------------------------------------------------
	BufferObject* bufferObj		= new BufferObject();	// deleted in renderer->exit()
	bufferObj->m_vertexCount	= 3;
	bufferObj->m_indexCount		= 3;
	bufferObj->m_vertices		= new Vertex[bufferObj->m_vertexCount];		// deleted in dtor
	bufferObj->m_indices		= new unsigned[bufferObj->m_indexCount];	// deleted in dtor

	// set geometry data
	//------------------------------------------------------------------
	const float size = 1.0f;

	// vertices - CW
	bufferObj->m_vertices[0].position	= XMFLOAT3(-size, -size, 0.0f);
	bufferObj->m_vertices[0].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[0].texCoords	= XMFLOAT3(0.0f, 1.0f, 0.0f);

	bufferObj->m_vertices[1].position	= XMFLOAT3(0, size, 0.0f);
	bufferObj->m_vertices[1].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[1].texCoords	= XMFLOAT3(0.5f, 0.0f, 0.0f);

	bufferObj->m_vertices[2].position	= XMFLOAT3(size, -size, 0.0f);
	bufferObj->m_vertices[2].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[2].texCoords	= XMFLOAT3(1.0f, 1.0f, 0.0f);

	// indices
	bufferObj->m_indices[0] = 0;
	bufferObj->m_indices[1] = 1;
	bufferObj->m_indices[2] = 2;

	// initialize GPU buffers
	//------------------------------------------------------------------
	bool writable = false;
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		OutputDebugString("Error Triangle creation failed");
		delete bufferObj;
		bufferObj = nullptr;
	}

	return bufferObj;
}

BufferObject* GeometryGenerator::Quad()
{
	BufferObject* bufferObj		= new BufferObject();
	bufferObj->m_vertexCount	= 4;
	bufferObj->m_indexCount		= 6;
	bufferObj->m_vertices		= new Vertex[bufferObj->m_vertexCount];	// deleted in dtor
	bufferObj->m_indices		= new unsigned[bufferObj->m_indexCount];	// deleted in dtor

	// set geometry data
	//------------------------------------------------------------------
	const float size = 1.0f;

	//	  1	+-----+ 2	0, 1, 2
	//		|	  |		2, 3, 0
	//		|	  |		
	//	  0 +-----+ 3	

	// vertices - CW
	bufferObj->m_vertices[0].position	= XMFLOAT3(-size, -size, 0.0f);
	bufferObj->m_vertices[0].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[0].texCoords	= XMFLOAT3(0.0f, 0.0f, 0.0f);

	bufferObj->m_vertices[1].position	= XMFLOAT3(-size, +size, 0.0f);
	bufferObj->m_vertices[1].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[1].texCoords	= XMFLOAT3(0.0f, 1.0f, 0.0f);

	bufferObj->m_vertices[2].position	= XMFLOAT3(+size, +size, 0.0f);
	bufferObj->m_vertices[2].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[2].texCoords	= XMFLOAT3(1.0f, 1.0f, 0.0f);

	bufferObj->m_vertices[3].position	= XMFLOAT3(+size, -size, 0.0f);
	bufferObj->m_vertices[3].normal		= XMFLOAT3(0.0f, 1.0f, 0.0f);
	bufferObj->m_vertices[3].texCoords	= XMFLOAT3(1.0f, 0.0f, 0.0f);

	// indices
	unsigned indices[] = {
		0, 1, 2,
		2, 3, 0
	};
	memcpy(bufferObj->m_indices, indices, bufferObj->m_indexCount * sizeof(unsigned));

	// initialize GPU buffers
	//------------------------------------------------------------------
	bool writable = false;	
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		OutputDebugString("Error Quad creation failed");
		delete bufferObj;
		bufferObj = nullptr;
	}

	return bufferObj;
}

BufferObject* GeometryGenerator::Cube()
{
	BufferObject* bufferObj		= new BufferObject();
	bufferObj->m_vertexCount	= 24;
	bufferObj->m_indexCount		= 36;
	bufferObj->m_vertices		= new Vertex[bufferObj->m_vertexCount];		// deleted in dtor
	bufferObj->m_indices		= new unsigned[bufferObj->m_indexCount];	// deleted in dtor

	// set geometry data
	//------------------------------------------------------------------
//		ASCII Cube art from: http://www.lonniebest.com/ASCII/Art/?ID=2
// 
//			   0 _________________________ 1		0, 1, 2, 0, 2, 3,		// Top
//		        / _____________________  /|			4, 5, 6, 4, 6, 7,		// back
//		       / / ___________________/ / |			8, 9, 10, 8, 10, 11,	// Right
//		      / / /| |               / /  |			12, 13, 14, 12, 14, 15, // Left
//		     / / / | |              / / . |			16, 17, 18, 16, 18, 19, // Back
//		    / / /| | |             / / /| |			20, 22, 21, 20, 23, 22, // Bottom
//		   / / / | | |            / / / | |			
//		  / / /  | | |           / / /| | |		   +Y
//		 / /_/__________________/ / / | | |			|  +Z
//	4,3 /________________________/5/  | | |			|  /
//		| ______________________8|2|  | | |			| /
//		| | |    | | |_________| | |__| | |			|/______+X
//		| | |    | |___________| | |____| |			
//		| | |   / / ___________| | |_  / /		
//		| | |  / / /           | | |/ / /		
//		| | | / / /            | | | / /		
//		| | |/ / /             | | |/ /			
//		| | | / /              | | ' /			
//		| | |/_/_______________| |  /			
//		| |____________________| | /			
//		|________________________|/6			
//		7

	// vertices - CW 
	bufferObj->m_vertices[0].position	= XMFLOAT3(-1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[0].normal		= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[0].texCoords	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[0].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[1].position	= XMFLOAT3(+1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[1].normal		= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[1].texCoords	= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[1].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[2].position	= XMFLOAT3(+1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[2].normal		= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[2].texCoords	= XMFLOAT3(+1.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[2].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[3].position	= XMFLOAT3(-1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[3].normal		= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[3].texCoords	= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[3].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[4].position	= XMFLOAT3(-1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[4].normal		= XMFLOAT3(+0.0f, +0.0f, -1.0f);
	bufferObj->m_vertices[4].texCoords	= XMFLOAT3(+1.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[4].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[5].position	= XMFLOAT3(+1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[5].normal		= XMFLOAT3(+0.0f, +0.0f, -1.0f);
	bufferObj->m_vertices[5].texCoords	= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[5].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[6].position	= XMFLOAT3(+1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[6].normal		= XMFLOAT3(+0.0f, +0.0f, -1.0f);
	bufferObj->m_vertices[6].texCoords	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[6].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[7].position	= XMFLOAT3(-1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[7].normal		= XMFLOAT3(+0.0f, +0.0f, -1.0f);
	bufferObj->m_vertices[7].texCoords	= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[7].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[8].position	= XMFLOAT3(+1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[8].normal		= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[8].texCoords	= XMFLOAT3(+1.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[8].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[9].position	= XMFLOAT3(+1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[9].normal		= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[9].texCoords	= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[9].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[10].position	= XMFLOAT3(+1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[10].normal	= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[10].texCoords	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[10].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[11].position	= XMFLOAT3(+1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[11].normal	= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[11].texCoords	= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[11].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[12].position	= XMFLOAT3(-1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[12].normal	= XMFLOAT3(-1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[12].texCoords	= XMFLOAT3(+1.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[12].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[13].position	= XMFLOAT3(-1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[13].normal	= XMFLOAT3(-1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[13].texCoords	= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[13].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[14].position	= XMFLOAT3(-1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[14].normal	= XMFLOAT3(-1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[14].texCoords	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[14].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[15].position	= XMFLOAT3(-1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[15].normal	= XMFLOAT3(-1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[15].texCoords	= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[15].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[16].position	= XMFLOAT3(+1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[16].normal	= XMFLOAT3(+0.0f, +0.0f, +1.0f);
	bufferObj->m_vertices[16].texCoords	= XMFLOAT3(+1.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[16].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[17].position	= XMFLOAT3(-1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[17].normal	= XMFLOAT3(+0.0f, +0.0f, +1.0f);
	bufferObj->m_vertices[17].texCoords	= XMFLOAT3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[17].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[18].position	= XMFLOAT3(-1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[18].normal	= XMFLOAT3(+0.0f, +0.0f, +1.0f);
	bufferObj->m_vertices[18].texCoords	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[18].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[19].position	= XMFLOAT3(+1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[19].normal	= XMFLOAT3(+0.0f, +0.0f, +1.0f);
	bufferObj->m_vertices[19].texCoords	= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[19].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[20].position	= XMFLOAT3(+1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[20].normal	= XMFLOAT3(+0.0f, -1.0f, +0.0f);
	bufferObj->m_vertices[20].texCoords	= XMFLOAT3(+1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[20].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[21].position	= XMFLOAT3(-1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[21].normal	= XMFLOAT3(+0.0f, -1.0f, +0.0f);
	bufferObj->m_vertices[21].texCoords	= XMFLOAT3(+0.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[21].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[22].position	= XMFLOAT3(-1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[22].normal	= XMFLOAT3(+0.0f, -1.0f, +0.0f);
	bufferObj->m_vertices[22].texCoords	= XMFLOAT3(+0.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[22].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[23].position	= XMFLOAT3(+1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[23].normal	= XMFLOAT3(+0.0f, -1.0f, +0.0f);
	bufferObj->m_vertices[23].texCoords	= XMFLOAT3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[23].tangent	= XMFLOAT3(+0.0f, +0.0f, +0.0f);

	// Create the index buffer
	unsigned indices[] = {
		0, 1, 2, 0, 2, 3,		// Top
		4, 5, 6, 4, 6, 7,		// back
		8, 9, 10, 8, 10, 11,	// Right
		12, 13, 14, 12, 14, 15, // Left
		16, 17, 18, 16, 18, 19, // Back
		20, 22, 21, 20, 23, 22, // Bottom
	};
	memcpy(bufferObj->m_indices, indices, bufferObj->m_indexCount * sizeof(unsigned));
	

	// initialize GPU buffers
	//------------------------------------------------------------------
	bool writable = false;	
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		OutputDebugString("Error Quad creation failed");
		delete bufferObj;
		bufferObj = nullptr;
	}

	return bufferObj;
}

BufferObject* GeometryGenerator::Sphere()
{
	BufferObject* bufferObj = new BufferObject();


	return bufferObj;
}
