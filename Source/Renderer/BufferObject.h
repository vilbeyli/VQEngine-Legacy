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
#include "Utilities/utils.h"

struct BufferDesc
{
	EBufferType		mType;
	EBufferUsage	mUsage;
	unsigned		mElementCount;
	unsigned		mStride;
};

struct Buffer
{
	BufferDesc		mDesc;
	ID3D11Buffer*	mData;
	bool			mDirty;
	void*			mCPUData;

	int test = 0;

	bool bInitialized;
	void Initialize(ID3D11Device* device = nullptr, const void* pData = nullptr);
	void CleanUp();

	void Update();

	Buffer(const BufferDesc& desc);
};

struct DefaultVertexBufferData
{
	vec3 position;
	vec3 normal;
	vec3 tangent;
	vec2 uv;
};


#if 0
// exploring some ideas here
//
// warning for the feint-hearted: variadic template fun follows.
// https://eli.thegreenplace.net/2014/variadic-templates-in-c/
//
// The idea is simple: flexible vertex buffer struct with struct definitions handled by the compiler.
// Example: VertexBuffer<vec3, vec2> for a vertex buffer for positions(vec3) and UVs(vec2).

// base case
template <class ...Attributes>
struct VertexBuffer {};

// argument peeling
template <class Attribute, class... Attributes> 
struct VertexBuffer<Attribute, Attributes...> : VertexBuffer<Attributes...> 
{
	VertexBuffer(Attribute inAttribute, Attributes... inRestOfTheAttributes) : VertexBuffer<Attributes...>(inRestOfTheAttributes), mAttribute(inAttribute) {}
	Attribute mAttribute;	// tail
};

// variadic example usage: 
//	VertexBuffer<vec3, vec2, vec4> vb(vec3(0,0,0), vec2(1,1), vec4(0,0,1,0));
// 
// argument peeling:
//  -	struct VertexBuffer<vec3, vec2, vec4> : VertexBuffer<vec2, vec4> {
//			vec3 attribute; //position
//		}
//	
//	-	VertexBuffer<vec2, vec4> : VertexBuffer<vec4>{
//			vec2 attribute; // uv
//		}
//
//	-	VertexBuffer<vec2> : VertexBuffer<>{
//			vec4 attribute; // some other packed data
//		}
//
//	-	VertexBuffer<> : VertexBuffer{}	
//							// base case
//
#endif