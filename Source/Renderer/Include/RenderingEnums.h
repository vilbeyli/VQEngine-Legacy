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

// DX11 library linking
#pragma comment(lib, "d3d11.lib")		// contains all the Direct3D functionality
#pragma comment(lib, "dxgi.lib")		// tools to interface with the hardware
#pragma comment(lib, "d3dcompiler.lib")	// functionality for compiling shaders

#include <d3d11.h>

enum class ERasterizerCullMode
{
	FRONT = D3D11_CULL_FRONT,
	NONE = D3D11_CULL_NONE,
	BACK = D3D11_CULL_BACK
};

enum class ERasterizerFillMode
{
	SOLID = D3D11_FILL_SOLID,
	WIREFRAME = D3D11_FILL_WIREFRAME,
};

enum class EPrimitiveTopology
{
	POINT_LIST = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
	TRIANGLE_LIST = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
	LINE_LIST = D3D11_PRIMITIVE_TOPOLOGY_LINELIST,

	TOPOLOGY_COUNT
};

enum EDefaultRasterizerState
{
	CULL_NONE = 0,
	CULL_FRONT,
	CULL_BACK,

	RASTERIZER_STATE_COUNT
};

enum EDefaultBlendState
{
	DISABLED,
	ADDITIVE_COLOR,
	ALPHA_BLEND,

	BLEND_STATE_COUNT
};



enum ELayoutFormat
{
	FLOAT32_2 = DXGI_FORMAT_R32G32_FLOAT,
	FLOAT32_3 = DXGI_FORMAT_R32G32B32_FLOAT,

	LAYOUT_FORMAT_COUNT
};



enum EShaderType
{	// used to map **SetShaderConstant(); function in Renderer::Apply()
	VS = 0,
	GS,
	DS,
	HS,
	CS,
	PS,

	COUNT
};


enum EGeometry
{
	TRIANGLE = 0,
	QUAD,
	CUBE,
	CYLINDER,
	SPHERE,
	GRID,
	BONE,

	MESH_TYPE_COUNT
};

enum EShaders
{	
	FORWARD_PHONG,
	UNLIT,
	TEXTURE_COORDINATES,
	NORMAL,
	TANGENT,
	BINORMAL,
	LINE,
	TBN,
	DEBUG,
	SKYBOX,
	BLOOM,
	BLUR,
	BLOOM_COMBINE,
	TONEMAPPING,
	FORWARD_BRDF,
	SHADOWMAP_DEPTH,
	DEFERRED_GEOMETRY,
	DEFERRED_BRDF_AMBIENT,
	DEFERRED_BRDF_LIGHTING,
	DEFERRED_BRDF_POINT,
	DEFERRED_BRDF_SPOT,
	SSAO,
	BILATERAL_BLUR,
	GAUSSIAN_BLUR_4x4,

	SHADER_COUNT
};

#include "HandleTypedefs.h"