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

#include "Utilities/Log.h"

#include <vector>

using std::vector;

// Direct3D Transformation Pipeline: https://msdn.microsoft.com/en-us/library/windows/desktop/ee418867(v=vs.85).aspx

ID3D11Device* GeometryGenerator::m_device = nullptr;

void CalculateTangentsAndBitangents(BufferObject*& obj)
{
	//  Bitangent
	//	
	//	^  (uv1)
	//	|	 V1   ___________________ V2 (uv2)
	//	|		  \                 /
	//	|		   \               /
	//	|		    \             /
	//	|			 \           /
	//	|			  \         /
	//	|  dUV1 | E1   \       /  E2 | dUV2
	//	|			    \     /
	//	|			     \   /
	//	|			      \ /	
	//	|				   V
	//	|				   V0 (uv0)
	//	|				   
	// ----------------------------------------->  Tangent

	Vertex* verts = obj->m_vertices;
	const size_t countVerts = obj->m_vertexCount;
	const size_t countIndices = obj->m_indexCount;
	assert(countIndices % 3 == 0);

	const vec3 N = vec3::Forward;	//  (0, 0, 1)

	for (size_t i = 0; i < countIndices; i += 3)
	{
		Vertex& v0 = verts[obj->m_indices[i]];
		Vertex& v1 = verts[obj->m_indices[i + 1]];
		Vertex& v2 = verts[obj->m_indices[i + 2]];

		const vec3 E1 = v1.position - v0.position;
		const vec3 E2 = v2.position - v0.position;

		const vec2 dUV1 = vec2(v1.texCoords - v0.texCoords);
		const vec2 dUV2 = vec2(v2.texCoords - v0.texCoords);
		
		const float f = 1.0f / ( dUV1.x() * dUV2.y() - dUV1.y() * dUV2.x() );
		assert(!std::isinf(f));

		vec3 T( f * (dUV2.y() * E1.x() - dUV1.y() * E2.x()), 
				f * (dUV2.y() * E1.y() - dUV1.y() * E2.y()), 
				f * (dUV2.y() * E1.z() - dUV1.y() * E2.z()));
		T.normalize();
		
		vec3 B(	f * (-dUV2.x() * E1.x() + dUV1.x() * E2.x()), 
				f * (-dUV2.x() * E1.y() + dUV1.x() * E2.y()), 
				f * (-dUV2.x() * E1.z() + dUV1.x() * E2.z()));
		B.normalize();

		v0.tangent = T;
		v1.tangent = T;
		v2.tangent = T;

		//v0.bitangent = B;
		//v1.bitangent = B;
		//v2.bitangent = B;

		if (v0.normal == vec3::Zero)
		{
			v0.normal = static_cast<const vec3>((XMVector3Cross(T, B))).normalized();
		}
		if (v1.normal == vec3::Zero)
		{
			v1.normal = static_cast<const vec3>((XMVector3Cross(T, B))).normalized();
		}
		if (v2.normal == vec3::Zero)
		{
			v2.normal = static_cast<const vec3>((XMVector3Cross(T, B))).normalized();
		}
	}
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
	bufferObj->m_vertices[0].position	= vec3(-size, -size, 0.0f);
	bufferObj->m_vertices[0].normal		= vec3::Back;
	bufferObj->m_vertices[0].texCoords	= XMFLOAT2(0.0f, 1.0f);

	bufferObj->m_vertices[1].position	= vec3(0, size, 0.0f);
	bufferObj->m_vertices[1].normal		= vec3::Back;
	bufferObj->m_vertices[1].texCoords	= XMFLOAT2(0.5f, 0.0f);

	bufferObj->m_vertices[2].position	= vec3(size, -size, 0.0f);
	bufferObj->m_vertices[2].normal		= vec3::Back;
	bufferObj->m_vertices[2].texCoords	= XMFLOAT2(1.0f, 1.0f);
	
	// indices
	bufferObj->m_indices[0] = 0;
	bufferObj->m_indices[1] = 1;
	bufferObj->m_indices[2] = 2;

	CalculateTangentsAndBitangents(bufferObj);

	// initialize GPU buffers
	//------------------------------------------------------------------
	bool writable = false;
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		Log::Error("Triangle creation failed");
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
	bufferObj->m_vertices		= new Vertex[bufferObj->m_vertexCount];		// deleted in dtor
	bufferObj->m_indices		= new unsigned[bufferObj->m_indexCount];	// deleted in dtor

	// set geometry data
	//------------------------------------------------------------------
	const float size = 1.0f;

	//	  1	+-----+ 2	0, 1, 2
	//		|	  |		2, 3, 0
	//		|	  |		
	//	  0 +-----+ 3	

	// vertices - CW
	bufferObj->m_vertices[0].position	= vec3(-size, -size, 0.0f);
	bufferObj->m_vertices[0].normal		= vec3(0.0f, 0.0f, -1.0f);
	bufferObj->m_vertices[0].texCoords	= XMFLOAT2(0.0f, 1.0f);

	bufferObj->m_vertices[1].position	= vec3(-size, +size, 0.0f);
	bufferObj->m_vertices[1].normal		= vec3(0.0f, 0.0f, -1.0f);
	bufferObj->m_vertices[1].texCoords	= XMFLOAT2(0.0f, 0.0f);

	bufferObj->m_vertices[2].position	= vec3(+size, +size, 0.0f);
	bufferObj->m_vertices[2].normal		= vec3(0.0f, 0.0f, -1.0f);
	bufferObj->m_vertices[2].texCoords	= XMFLOAT2(1.0f, 0.0f);

	bufferObj->m_vertices[3].position	= vec3(+size, -size, 0.0f);
	bufferObj->m_vertices[3].normal		= vec3(0.0f, 0.0f, -1.0f);
	bufferObj->m_vertices[3].texCoords	= XMFLOAT2(1.0f, 1.0f);

	unsigned indices[] = {
		0, 1, 2,
		2, 3, 0
	};
	memcpy(bufferObj->m_indices, indices, bufferObj->m_indexCount * sizeof(unsigned));

	CalculateTangentsAndBitangents(bufferObj);

	// initialize GPU buffers
	//------------------------------------------------------------------
	bool writable = false;	
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		Log::Error("Quad creation failed");
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
//		        / _____________________  /|			4, 5, 6, 4, 6, 7,		// Front
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

	bufferObj->m_vertices[0].texCoords	= XMFLOAT2(+0.0f, +0.0f);
	bufferObj->m_vertices[1].texCoords	= XMFLOAT2(+1.0f, +0.0f);
	bufferObj->m_vertices[2].texCoords	= XMFLOAT2(+1.0f, +1.0f);

	bufferObj->m_vertices[3].texCoords	= XMFLOAT2(+0.0f, +1.0f);
	bufferObj->m_vertices[4].texCoords	= XMFLOAT2(+0.0f, +0.0f);
	bufferObj->m_vertices[5].texCoords	= XMFLOAT2(+1.0f, +0.0f);

	bufferObj->m_vertices[6].texCoords	= XMFLOAT2(+1.0f, +1.0f);
	bufferObj->m_vertices[7].texCoords	= XMFLOAT2(+0.0f, +1.0f);
	bufferObj->m_vertices[8].texCoords	= XMFLOAT2(+0.0f, +0.0f);

	bufferObj->m_vertices[9].texCoords	= XMFLOAT2(+1.0f, +0.0f);
	bufferObj->m_vertices[10].texCoords = XMFLOAT2(+1.0f, +1.0f);
	bufferObj->m_vertices[11].texCoords = XMFLOAT2(+0.0f, +1.0f);

	bufferObj->m_vertices[12].texCoords = XMFLOAT2(+0.0f, +0.0f);
	bufferObj->m_vertices[13].texCoords = XMFLOAT2(+1.0f, +0.0f);
	bufferObj->m_vertices[14].texCoords = XMFLOAT2(+1.0f, +1.0f);
	bufferObj->m_vertices[15].texCoords = XMFLOAT2(+0.0f, +1.0f);
	bufferObj->m_vertices[16].texCoords = XMFLOAT2(+0.0f, +0.0f);
	bufferObj->m_vertices[17].texCoords = XMFLOAT2(+1.0f, +0.0f);
	bufferObj->m_vertices[18].texCoords = XMFLOAT2(+1.0f, +1.0f);
	bufferObj->m_vertices[19].texCoords = XMFLOAT2(+0.0f, +1.0f);

	bufferObj->m_vertices[20].texCoords = XMFLOAT2(+1.0f, +0.0f);
	bufferObj->m_vertices[21].texCoords = XMFLOAT2(+0.0f, +0.0f);
	bufferObj->m_vertices[22].texCoords = XMFLOAT2(+0.0f, +1.0f);
	bufferObj->m_vertices[23].texCoords = XMFLOAT2(+1.0f, +1.0f);
	// vertices - CW 
	// TOP
	bufferObj->m_vertices[0].position	= vec3(-1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[0].normal		= vec3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[0].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[1].position	= vec3(+1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[1].normal		= vec3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[1].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[2].position	= vec3(+1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[2].normal		= vec3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[2].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[3].position	= vec3(-1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[3].normal		= vec3(+0.0f, +1.0f, +0.0f);
	bufferObj->m_vertices[3].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	// FRONT
	bufferObj->m_vertices[4].position	= vec3(-1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[4].normal		= vec3(+0.0f, +0.0f, -1.0f);
	bufferObj->m_vertices[4].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[5].position	= vec3(+1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[5].normal		= vec3(+0.0f, +0.0f, -1.0f);
	bufferObj->m_vertices[5].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[6].position	= vec3(+1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[6].normal		= vec3(+0.0f, +0.0f, -1.0f);
	bufferObj->m_vertices[6].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[7].position	= vec3(-1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[7].normal		= vec3(+0.0f, +0.0f, -1.0f);
	bufferObj->m_vertices[7].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	// RIGHT
	bufferObj->m_vertices[8].position	= vec3(+1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[8].normal		= vec3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[8].tangent	= vec3(+0.0f, +0.0f, +1.0f);

	bufferObj->m_vertices[9].position	= vec3(+1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[9].normal		= vec3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[9].tangent	= vec3(+0.0f, +0.0f, +1.0f);

	bufferObj->m_vertices[10].position	= vec3(+1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[10].normal	= vec3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[10].tangent	= vec3(+0.0f, +0.0f, +1.0f);

	bufferObj->m_vertices[11].position	= vec3(+1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[11].normal	= vec3(+1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[11].tangent	= vec3(+0.0f, +0.0f, +1.0f);
	
	// BACK
	bufferObj->m_vertices[12].position	= vec3(+1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[12].normal	= vec3(+0.0f, +0.0f, +1.0f);
	bufferObj->m_vertices[12].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[13].position	= vec3(-1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[13].normal	= vec3(+0.0f, +0.0f, +1.0f);
	bufferObj->m_vertices[13].tangent	= vec3(+1.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[14].position	= vec3(-1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[14].normal	= vec3(+0.0f, +0.0f, +1.0f);
	bufferObj->m_vertices[14].tangent	= vec3(+1.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[15].position	= vec3(+1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[15].normal	= vec3(+0.0f, +0.0f, +1.0f);
	bufferObj->m_vertices[15].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	// LEFT
	bufferObj->m_vertices[16].position	= vec3(-1.0f, +1.0f, +1.0f);
	bufferObj->m_vertices[16].normal	= vec3(-1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[16].tangent	= vec3(+0.0f, +0.0f, -1.0f);

	bufferObj->m_vertices[17].position	= vec3(-1.0f, +1.0f, -1.0f);
	bufferObj->m_vertices[17].normal	= vec3(-1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[17].tangent	= vec3(+0.0f, +0.0f, -1.0f);
	
	bufferObj->m_vertices[18].position	= vec3(-1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[18].normal	= vec3(-1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[18].tangent	= vec3(+0.0f, +0.0f, -1.0f);

	bufferObj->m_vertices[19].position	= vec3(-1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[19].normal	= vec3(-1.0f, +0.0f, +0.0f);
	bufferObj->m_vertices[19].tangent	= vec3(+0.0f, +0.0f, -1.0f);


	// BOTTOM
	bufferObj->m_vertices[20].position	= vec3(+1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[20].normal	= vec3(+0.0f, -1.0f, +0.0f);
	bufferObj->m_vertices[20].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[21].position	= vec3(-1.0f, -1.0f, -1.0f);
	bufferObj->m_vertices[21].normal	= vec3(+0.0f, -1.0f, +0.0f);
	bufferObj->m_vertices[21].tangent	= vec3(+1.0f, +0.0f, +0.0f);
	
	bufferObj->m_vertices[22].position	= vec3(-1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[22].normal	= vec3(+0.0f, -1.0f, +0.0f);
	bufferObj->m_vertices[22].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	bufferObj->m_vertices[23].position	= vec3(+1.0f, -1.0f, +1.0f);
	bufferObj->m_vertices[23].normal	= vec3(+0.0f, -1.0f, +0.0f);
	bufferObj->m_vertices[23].tangent	= vec3(+1.0f, +0.0f, +0.0f);

	//--------------------------------------------------------------

	// make edges unit length
	//for (int i = 0; i < 24; ++i)
	//{
	//	bufferObj->m_vertices[i].position.x() *= 0.5f;
	//	bufferObj->m_vertices[i].position.y() *= 0.5f;
	//	bufferObj->m_vertices[i].position.z() *= 0.5f;
	//}

	unsigned indices[] = {
		0, 1, 2, 0, 2, 3,		// Top
		4, 5, 6, 4, 6, 7,		// back
		8, 9, 10, 8, 10, 11,	// Right
		12, 13, 14, 12, 14, 15, // Back
		16, 17, 18, 16, 18, 19, // Left
		20, 22, 21, 20, 23, 22, // Bottom
	};
	memcpy(bufferObj->m_indices, indices, bufferObj->m_indexCount * sizeof(unsigned));
	
	CalculateTangentsAndBitangents(bufferObj);

	// initialize GPU buffers
	//------------------------------------------------------------------
	bool writable = false;	
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		Log::Error("creating cube failed");
		delete bufferObj;
		bufferObj = nullptr;
	}

	return bufferObj;
}


BufferObject* GeometryGenerator::Sphere(float radius, unsigned ringCount, unsigned sliceCount)
{
	// CYLINDER BODY
	//-----------------------------------------------------------
	// Compute vertices for each stack ring starting at
	// the bottom and moving up.
	std::vector<Vertex> Vertices;
	float dPhi = XM_PI / (ringCount - 1);
	for (float phi = -XM_PIDIV2; phi <= XM_PIDIV2 + 0.00001f; phi += dPhi)
	{
		float y = radius * sinf(phi);	// horizontal slice center height
		float r = radius * cosf(phi);	// horizontal slice radius

		// vertices of ring
		float dTheta = 2.0f*XM_PI / sliceCount;
		for (unsigned j = 0; j <= sliceCount; ++j)	// for each pice(slice) in horizontal slice
		{
			Vertex vertex;
			float theta = j*dTheta;
			float x = r * cosf(theta);
			float z = r * sinf(theta);
			vertex.position = vec3(x, y, z);
			{
				float u = (float)j / sliceCount;
				float v = (y + radius) / (2 * radius);
				//fmod(2 * v, 1.0f);
				vertex.texCoords = XMFLOAT2(u, v);
			}
			// Cylinder can be parameterized as follows, where we
			// introduce v parameter that goes in the same direction
			// as the v tex-coord so that the bitangent goes in the
			// same direction as the v tex-coord.
			// Let r0 be the bottom radius and let r1 be the
			// top radius.
			// y(v) = h - hv for v in [0,1].
			// r(v) = r1 + (r0-r1)v
			//
			// x(t, v) = r(v)*cos(t)
			// y(t, v) = h - hv
			// z(t, v) = r(v)*sin(t)
			//
			// dx/dt = -r(v)*sin(t)
			// dy/dt = 0
			// dz/dt = +r(v)*cos(t)
			//
			// dx/dv = (r0-r1)*cos(t)
			// dy/dv = -h
			// dz/dv = (r0-r1)*sin(t)

			// TangentU us unit length.
			vertex.tangent = vec3(-z, 0.0f, x);
			//float dr = bottomRadius - topRadius;
			//vec3 bitangent(dr*x, -, dr*z);
			//XMVECTOR T = XMLoadFloat3(&vertex.tangent);
			//XMVECTOR B = XMLoadFloat3(&bitangent);
			//XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			//XMStoreFloat3(&vertex.normal, N);
			XMVECTOR N = XMVectorSet(0, 1, 0, 1);
			XMVECTOR ROT = XMQuaternionRotationRollPitchYaw(0.0f, XM_PI-theta, XM_PIDIV2-phi);
			N = XMVector3Rotate(N, ROT);

			vertex.normal = N;
			Vertices.push_back(vertex);
		}
	}

	std::vector<unsigned> Indices;
	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	unsigned ringVertexCount = sliceCount + 1;
	// Compute indices for each stack.
	for (unsigned i = 0; i < ringCount; ++i)
	{
		for (unsigned j = 0; j < sliceCount; ++j)
		{
			Indices.push_back(i*ringVertexCount + j);
			Indices.push_back((i + 1)*ringVertexCount + j);
			Indices.push_back((i + 1)*ringVertexCount + j + 1);
			Indices.push_back(i*ringVertexCount + j);
			Indices.push_back((i + 1)*ringVertexCount + j + 1);
			Indices.push_back(i*ringVertexCount + j + 1);
		}
	}

	//------------------------------------------------
	BufferObject* bufferObj = new BufferObject();
	bufferObj->m_vertexCount = static_cast<unsigned>(Vertices.size());
	bufferObj->m_indexCount  = static_cast<unsigned>(Indices.size());
	bufferObj->m_vertices = new Vertex[bufferObj->m_vertexCount];		// deleted in dtor
	bufferObj->m_indices = new unsigned[bufferObj->m_indexCount];	// deleted in dtor
	memcpy(bufferObj->m_vertices, Vertices.data(), Vertices.size() * sizeof(Vertex));
	memcpy(bufferObj->m_indices, Indices.data(), Indices.size() * sizeof(unsigned));

	// fill gpu buffers
	bool writable = true;
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		Log::Error("Grid creation failed");
		delete bufferObj;
		bufferObj = nullptr;
	}

	return bufferObj;
}

BufferObject* GeometryGenerator::Grid(float width, float depth, unsigned m, unsigned n)
{
	//		Grid of m x n vertices
	//		-----------------------------------------------------------
	//		+	: Vertex
	//		d	: depth
	//		w	: width
	//		dx	: horizontal cell spacing = width / (m-1)
	//		dz	: z-axis	 cell spacing = depth / (n-1)
	// 
	//		  V(0,0)		  V(m-1,0)	^ Z
	//		^	+-------+-------+ ^		|
	//		|	|		|		| |		|
	//		|	|		|		| dz	|
	//		|	|		|		| |		|
	//		d	+-------+-------+ v		+--------> X
	//		|	|		|		|		
	//		|	|		|		|
	//		|	|		|		|
	//		v	+-------+-------+		
	//			<--dx--->		  V(m-1, n-1)
	//			<------ w ------>

	unsigned numQuads  = (m - 1) * (n - 1);
	unsigned faceCount = numQuads * 2; // 2 faces per quad = triangle count
	unsigned vertCount = m * n;
	float dx = width / (n - 1);
	float dz = depth / (m - 1);	// m & n mixed up??

	// offsets for centering the grid : V(0,0) = (-halfWidth, halfDepth)
	float halfDepth = depth / 2;
	float halfWidth = width / 2;

	// texture coord increments
	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	BufferObject* bufferObj		= new BufferObject();
	bufferObj->m_vertexCount	= vertCount;
	bufferObj->m_indexCount		= faceCount * 3;
	bufferObj->m_vertices		= new Vertex[bufferObj->m_vertexCount];		// deleted in dtor
	bufferObj->m_indices		= new unsigned[bufferObj->m_indexCount];	// deleted in dtor

	// position the vertices
	for (unsigned i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (unsigned j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;
			float u = j * du;
			float v = i * dv;
			bufferObj->m_vertices[i*n + j].position		= vec3( x  , 0.0f,  z  );
			bufferObj->m_vertices[i*n + j].normal		= vec3(0.0f, 0.0f, 0.0f);
			bufferObj->m_vertices[i*n + j].texCoords	= XMFLOAT2( u  ,  v  );
			bufferObj->m_vertices[i*n + j].tangent		= vec3(1.0f, 0.0f, 0.0f);
		}
	}

	//	generate indices
	//
	//	  A	+------+ B
	//		|	 / |
	//		|	/  |
	//		|  /   |
	//		| /	   |
	//		|/	   |
	//	  C	+------+ D
	//
	//	A	: V(i  , j  )
	//	B	: V(i  , j+1)
	//	C	: V(i+1, j  )
	//	D	: V(i+1, j+1)
	//
	//	ABC	: (i*n +j    , i*n + j+1, (i+1)*n + j  )
	//	CBD : ((i+1)*n +j, i*n + j+1, (i+1)*n + j+1)

	unsigned k = 0;
	for (unsigned i = 0; i < m-1; ++i)
	{
		for (unsigned j = 0; j < n-1; ++j)
		{
			bufferObj->m_indices[k  ] = i*n + j;
			bufferObj->m_indices[k+1] = i*n + j + 1;
			bufferObj->m_indices[k+2] = (i + 1)*n + j;
			bufferObj->m_indices[k+3] = (i + 1)*n + j;
			bufferObj->m_indices[k+4] = i*n + j + 1;
			bufferObj->m_indices[k+5] = (i + 1)*n + j + 1;
			k += 6;
		}
	}

	// apply height function
	for (unsigned i = 0; i < bufferObj->m_vertexCount; ++i)
	{
		vec3& pos = bufferObj->m_vertices[i].position;
		pos.y() = 0.2f * (pos.z() * sinf(20.0f * pos.x()) + pos.x() * cosf(10.0f * pos.z()));
	}

	CalculateTangentsAndBitangents(bufferObj);
	


	bool writable = true;
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		Log::Error("Grid creation failed");
		delete bufferObj;
		bufferObj = nullptr;
	}

	return bufferObj;
}

BufferObject* GeometryGenerator::Cylinder(float height, float topRadius, float bottomRadius, unsigned sliceCount, unsigned stackCount)
{
	// slice count	: horizontal resolution
	// stack count	: height resolution
	float stackHeight = height / stackCount;
	float radiusStep  = (topRadius - bottomRadius) / stackCount;
	unsigned ringCount = stackCount + 1;
	
	// CYLINDER BODY
	//-----------------------------------------------------------
	// Compute vertices for each stack ring starting at
	// the bottom and moving up.
	std::vector<Vertex> Vertices;
	for (unsigned i = 0; i < ringCount; ++i)
	{
		float y = -0.5f*height + i*stackHeight;
		float r = bottomRadius + i*radiusStep;

		// vertices of ring
		float dTheta = 2.0f*XM_PI / sliceCount;
		for (unsigned j = 0; j <= sliceCount; ++j)
		{
			Vertex vertex;
			float c = cosf(j*dTheta);
			float s = sinf(j*dTheta);
			vertex.position = vec3(r*c, y, r*s);
			{
				float u = (float)j / sliceCount;
				float v = 1.0f - (float)i / stackCount;
				vertex.texCoords = XMFLOAT2(u, v);
			}
			// Cylinder can be parameterized as follows, where we
			// introduce v parameter that goes in the same direction
			// as the v tex-coord so that the bitangent goes in the
			// same direction as the v tex-coord.
			// Let r0 be the bottom radius and let r1 be the
			// top radius.
			// y(v) = h - hv for v in [0,1].
			// r(v) = r1 + (r0-r1)v
			//
			// x(t, v) = r(v)*cos(t)
			// y(t, v) = h - hv
			// z(t, v) = r(v)*sin(t)
			//
			// dx/dt = -r(v)*sin(t)
			// dy/dt = 0
			// dz/dt = +r(v)*cos(t)
			//
			// dx/dv = (r0-r1)*cos(t)
			// dy/dv = -h
			// dz/dv = (r0-r1)*sin(t)

			// TangentU us unit length.
			vertex.tangent = vec3(-s, 0.0f, c);
			float dr = bottomRadius - topRadius;
			vec3 bitangent(dr*c, -height, dr*s);
			XMVECTOR T = vertex.tangent;
			XMVECTOR B = bitangent;
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			vertex.normal = N;
			Vertices.push_back(vertex);
		}
	}

	std::vector<unsigned> Indices;
	// Add one because we duplicate the first and last vertex per ring
	// since the texture coordinates are different.
	unsigned ringVertexCount = sliceCount + 1;
	// Compute indices for each stack.
	for (unsigned i = 0; i < stackCount; ++i)
	{
		for (unsigned j = 0; j < sliceCount; ++j)
		{
			Indices.push_back(i*ringVertexCount + j);
			Indices.push_back((i + 1)*ringVertexCount + j);
			Indices.push_back((i + 1)*ringVertexCount + j + 1);
			Indices.push_back(i*ringVertexCount + j);
			Indices.push_back((i + 1)*ringVertexCount + j + 1);
			Indices.push_back(i*ringVertexCount + j + 1);
		}
	}

	// CYLINDER TOP
	//-----------------------------------------------------------
	{
		unsigned baseIndex = (unsigned)Vertices.size();
		float y = 0.5f*height;
		float dTheta = 2.0f*XM_PI / sliceCount;
		// Duplicate cap ring vertices because the texture coordinates
		// and normals differ.
		for (unsigned i = 0; i <= sliceCount; ++i)
		{
			float x = topRadius*cosf(i*dTheta);
			float z = topRadius*sinf(i*dTheta);
			// Scale down by the height to try and make top cap
			// texture coord area proportional to base.
			float u = x / height + 0.5f;
			float v = z / height + 0.5f;

			Vertex Vert;
			Vert.position	= vec3(x, y, z);
			Vert.normal		= vec3(0.0f, 1.0f, 0.0f);
			Vert.tangent	= vec3(1.0f, 0.0f, 0.0f);	// ?
			Vert.texCoords  = XMFLOAT2(u, v);
			Vertices.push_back(Vert);
		}

		// Cap center vertex.
		Vertex capCenter;
		capCenter.position	= vec3(0.0f, y, 0.0f);
		capCenter.normal	= vec3(0.0f, 1.0f, 0.0f);
		capCenter.tangent	= vec3(1.0f, 0.0f, 0.0f);
		capCenter.texCoords = XMFLOAT2(0.5f, 0.5f);
		Vertices.push_back(capCenter);

		// Index of center vertex.
		unsigned centerIndex = (unsigned)Vertices.size() - 1;
		for (unsigned i = 0; i < sliceCount; ++i)
		{
			Indices.push_back(centerIndex);
			Indices.push_back(baseIndex + i + 1);
			Indices.push_back(baseIndex + i);
		}
	}


	// CYLINDER BOTTOM
	//-----------------------------------------------------------
	{
		unsigned baseIndex = (unsigned)Vertices.size();
		float y = -0.5f*height;
		float dTheta = 2.0f*XM_PI / sliceCount;
		// Duplicate cap ring vertices because the texture coordinates
		// and normals differ.
		for (unsigned i = 0; i <= sliceCount; ++i)
		{
			float x = bottomRadius*cosf(i*dTheta);
			float z = bottomRadius*sinf(i*dTheta);
			// Scale down by the height to try and make top cap
			// texture coord area proportional to base.
			float u = x / height + 0.5f;
			float v = z / height + 0.5f;

			Vertex Vert;
			Vert.position	= vec3(x, y, z);
			Vert.normal		= vec3(0.0f, -1.0f, 0.0f);
			Vert.tangent	= vec3(-1.0f, 0.0f, 0.0f);	// ?
			Vert.texCoords	= XMFLOAT2(u, v);
			Vertices.push_back(Vert);
		}
		// Cap center vertex.
		Vertex capCenter;
		capCenter.position	= vec3(0.0f, y, 0.0f);
		capCenter.normal	= vec3(0.0f, -1.0f, 0.0f);
		capCenter.tangent	= vec3(-1.0f, 0.0f, 0.0f);
		capCenter.texCoords = XMFLOAT2(0.5f, 0.5f);
		Vertices.push_back(capCenter);

		// Index of center vertex.
		unsigned centerIndex = (unsigned)Vertices.size() - 1;
		for (unsigned i = 0; i < sliceCount; ++i)
		{
			Indices.push_back(centerIndex);
			Indices.push_back(baseIndex + i);
			Indices.push_back(baseIndex + i + 1);
		}
	}

	//------------------------------------------------
	BufferObject* bufferObj		= new BufferObject();
	bufferObj->m_vertexCount	= static_cast<unsigned>(Vertices.size());
	bufferObj->m_indexCount		= static_cast<unsigned>(Indices.size());
	bufferObj->m_vertices		= new Vertex[bufferObj->m_vertexCount];		// deleted in dtor
	bufferObj->m_indices		= new unsigned[bufferObj->m_indexCount];	// deleted in dtor
	memcpy(bufferObj->m_vertices, Vertices.data(), Vertices.size() * sizeof(Vertex));
	memcpy(bufferObj->m_indices, Indices.data(), Indices.size() * sizeof(unsigned) );

	// fill gpu buffers
	bool writable = true;
	if (!bufferObj->FillGPUBuffers(m_device, writable))
	{
		Log::Error("Grid creation failed");
		delete bufferObj;
		bufferObj = nullptr;
	}

	return bufferObj;
}