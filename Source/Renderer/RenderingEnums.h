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

// default states
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

enum EDefaultSamplerState
{
	POINT_SAMPLER,
	LINEAR_FILTER_SAMPLER_WRAP_UVW,
	LINEAR_FILTER_SAMPLER,
	WRAP_SAMPLER,

	DEFAULT_SAMPLER_COUNT
};


enum EDefaultDepthStencilState
{
	DEPTH_STENCIL_WRITE,
	DEPTH_STENCIL_DISABLED,
	DEPTH_WRITE,
	STENCIL_WRITE,
	DEPTH_TEST_ONLY,
	// add more as needed

	DEPTH_STENCIL_STATE_COUNT
};

enum ELayoutFormat
{
	FLOAT32_2 = DXGI_FORMAT_R32G32_FLOAT,
	FLOAT32_3 = DXGI_FORMAT_R32G32B32_FLOAT,

	LAYOUT_FORMAT_COUNT
};

enum ETextureUsage : unsigned
{
	RESOURCE = D3D11_BIND_SHADER_RESOURCE,
	RENDER_TARGET = D3D11_BIND_RENDER_TARGET,
	RENDER_TARGET_RW = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
	DEPTH_TARGET = D3D11_BIND_DEPTH_STENCIL,

	TEXTURE_USAGE_COUNT
};

enum EImageFormat
{
	// RGBA
	RGBA32F = DXGI_FORMAT_R32G32B32A32_FLOAT,
	RGBA16F = DXGI_FORMAT_R16G16B16A16_FLOAT,
	RGBA8UN = DXGI_FORMAT_R8G8B8A8_UNORM,

	// R
	R32F = DXGI_FORMAT_R32_FLOAT,
	R32U = DXGI_FORMAT_R32_UINT,

	// Typeless
	R32		= DXGI_FORMAT_R32_TYPELESS,
	R24G8	= DXGI_FORMAT_R24G8_TYPELESS,

	// 
	R24_UNORM_X8_TYPELESS = DXGI_FORMAT_R24_UNORM_X8_TYPELESS,

	// Depth
	D32F		 = DXGI_FORMAT_D32_FLOAT,
	D24UNORM_S8U = DXGI_FORMAT_D24_UNORM_S8_UINT,

	IMAGE_FORMAT_UNKNOWN,
	IMAGE_FORMAT_COUNT
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

// https://learnopengl.com/#!PBR/IBL/Diffuse-irradiance
// todo: add convolution shaders (compute)
enum EShaders : unsigned	// built-in shaders
{	
	FORWARD_PHONG = 0,
	UNLIT,
	TEXTURE_COORDINATES,
	NORMAL,
	TANGENT,
	BINORMAL,
	LINE,
	TBN,
	DEBUG,
	SKYBOX,
	SKYBOX_EQUIRECTANGULAR,
	FORWARD_BRDF,
	SHADOWMAP_DEPTH,
	BILATERAL_BLUR,
	GAUSSIAN_BLUR_4x4,
	Z_PREPRASS,
	DEFERRED_GEOMETRY = Z_PREPRASS +5,	// zzz... this NEEDS to change. TODO URGENT.

	SHADER_COUNT
};

#include "Application/HandleTypedefs.h"
