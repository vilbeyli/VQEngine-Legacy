//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018 - Volkan Ilbeyli
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
	// SOLID
	FRONT = D3D11_CULL_FRONT,
	NONE = D3D11_CULL_NONE,
	BACK = D3D11_CULL_BACK,

	// WIREFRAME
	WIREFRAME = D3D11_FILL_WIREFRAME,
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
	// FILL
	CULL_NONE = 0,
	CULL_FRONT,
	CULL_BACK,

	// WIREFRAME
	WIREFRAME,

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
	
	WRAP_SAMPLER, // ?

	ANISOTROPIC_1_CLAMPED_SAMPLER,
	ANISOTROPIC_2_CLAMPED_SAMPLER,
	ANISOTROPIC_4_CLAMPED_SAMPLER,
	ANISOTROPIC_16_CLAMPED_SAMPLER,

	ANISOTROPIC_1_WRAPPED_SAMPLER,
	ANISOTROPIC_2_WRAPPED_SAMPLER,
	ANISOTROPIC_4_WRAPPED_SAMPLER,
	ANISOTROPIC_16_WRAPPED_SAMPLER,

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
	FLOAT32_4 = DXGI_FORMAT_R32G32B32A32_FLOAT,

	LAYOUT_FORMAT_COUNT
};

enum ETextureUsage : unsigned
{
	RESOURCE = D3D11_BIND_SHADER_RESOURCE,
	RENDER_TARGET = D3D11_BIND_RENDER_TARGET,
	RENDER_TARGET_RW = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET,
	DEPTH_TARGET = D3D11_BIND_DEPTH_STENCIL,
	COMPUTE_RW_TEXTURE = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,

	TEXTURE_USAGE_COUNT
};

enum ECPUAccess : unsigned
{
	NONE = 0,
	CPU_R = D3D11_CPU_ACCESS_READ,
	CPU_W = D3D11_CPU_ACCESS_WRITE,
	CPU_RW = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE
};

enum EImageFormat
{
	// RGBA
	RGBA32F = DXGI_FORMAT_R32G32B32A32_FLOAT,
	RGBA16F = DXGI_FORMAT_R16G16B16A16_FLOAT,
	RGBA8UN = DXGI_FORMAT_R8G8B8A8_UNORM,

	// RGB
	RGB32F = DXGI_FORMAT_R32G32B32_FLOAT,
	R11G11B10F = DXGI_FORMAT_R11G11B10_FLOAT,

	// RG
	RG32F = DXGI_FORMAT_R32G32_FLOAT,
	RG16F = DXGI_FORMAT_R16G16_FLOAT,

	// R
	R32F = DXGI_FORMAT_R32_FLOAT,
	R32U = DXGI_FORMAT_R32_UINT,

	R8U = DXGI_FORMAT_R8_UINT,
	R8UN = DXGI_FORMAT_R8_UNORM,

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



enum EGeometry
{
	TRIANGLE = 0,
	QUAD,
	FULLSCREENQUAD,
	CUBE,
	CYLINDER,
	SPHERE,
	GRID,
	CONE,
	LIGHT_CUE_CONE,
	//BONE,

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
	SHADOWMAP_DEPTH_INSTANCED,
	DEFERRED_GEOMETRY = SHADOWMAP_DEPTH +8,	// zzz... this NEEDS to change. TODO URGENT.

	SHADER_COUNT
};

enum EShaderStageFlags : unsigned
{
	SHADER_STAGE_NONE = 0x00000000,
	SHADER_STAGE_VS = 0x00000001,
	SHADER_STAGE_GS = 0x00000002,
	SHADER_STAGE_DS = 0x00000004,
	SHADER_STAGE_HS = 0x00000008,
	SHADER_STAGE_PS = 0x00000010,
	SHADER_STAGE_ALL_GRAPHICS = 0X0000001F,
	SHADER_STAGE_CS = 0x00000020,

	SHADER_STAGE_COUNT = 6
};

enum EShaderStage : unsigned	// array-index enum mapping
{	// used to map **SetShaderConstant(); function in Renderer::Apply()
	VS = 0,
	GS,
	DS,
	HS,
	PS,
	CS,

	COUNT
};

enum EBufferUsage
{	// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476259(v=vs.85).aspx
	GPU_READ = D3D11_USAGE_IMMUTABLE,        
	GPU_READ_WRITE = D3D11_USAGE_DEFAULT,    
	GPU_READ_CPU_WRITE = D3D11_USAGE_DYNAMIC,
	GPU_READ_CPU_READ = D3D11_USAGE_STAGING, 

	BUFFER_USAGE_COUNT
};

enum EBufferType : unsigned
{
	VERTEX_BUFER = D3D11_BIND_VERTEX_BUFFER,
	INDEX_BUFFER = D3D11_BIND_INDEX_BUFFER,
	// CONSTANT_BUFFER, // this can fit here
	COMPUTE_RW_BUFFER = D3D11_BIND_UNORDERED_ACCESS,
//	COMPUTE_RW_TEXTURE = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,

	BUFFER_TYPE_UNKNOWN,
	BUFFER_TYPE_COUNT
};

enum EMaterialType
{
	GGX_BRDF = 0,
	BLINN_PHONG,

	MATERIAL_TYPE_COUNT
};

#include "Application/HandleTypedefs.h"
