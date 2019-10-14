//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
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

#include <string>
#include <vector>
#include <array>
#include <experimental/filesystem>	// cpp17


using FileTimeStamp = std::experimental::filesystem::file_time_type;

// TOOD: add convolution compute shaders from here
// https://learnopengl.com/#!PBR/IBL/Diffuse-irradiance
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
{
	VS = 0,
	GS,
	DS,
	HS,
	PS,
	CS,

	COUNT
};


struct ShaderMacro
{
	std::string name;
	std::string value;
};

struct ShaderStageDesc
{
	std::string fileName;
	std::vector<ShaderMacro> macros;
};

struct ShaderDesc
{
	using ShaderStageArr = std::array<ShaderStageDesc, EShaderStageFlags::SHADER_STAGE_COUNT>;
	static ShaderStageArr CreateStageDescsFromShaderName(const char* shaderName, unsigned flagStages);

	std::string shaderName;
	std::array<ShaderStageDesc, EShaderStage::COUNT> stages;
};

struct ShaderLoadDesc
{
	ShaderLoadDesc() = default;
	ShaderLoadDesc(const std::string& path, const std::string& cachePath_) : fullPath(path), cachePath(cachePath_)
	{
		this->lastWriteTime = std::experimental::filesystem::last_write_time(fullPath);
		this->cacheLastWriteTime = std::experimental::filesystem::last_write_time(cachePath);
	}
	std::string fullPath;
	std::string cachePath;
	FileTimeStamp lastWriteTime;
	FileTimeStamp cacheLastWriteTime;
};


struct ID3D10Blob;

namespace ShaderUtils
{
	// Compiles shader from source file with the given shader macros
	//
	static bool CompileFromSource(
		const std::string&              pathToFile
		, const EShaderStage&             type
		, ID3D10Blob *&                   ref_pBob
		, std::string&                    outErrMsg
		, const std::vector<ShaderMacro>& macros);

	// Reads in cached binary from %APPDATA%/VQEngine/ShaderCache folder into ID3D10Blob 
	//
	static ID3D10Blob * CompileFromCachedBinary(const std::string& cachedBinaryFilePath);

	// Writes out compiled ID3D10Blob into %APPDATA%/VQEngine/ShaderCache folder
	//
	static void			CacheShaderBinary(const std::string& shaderCacheFileName, ID3D10Blob * pCompiledBinary);

	// example filePath: "rootPath/filename_vs.hlsl"
	//                                      ^^----- shaderTypeString
	static EShaderStage	GetShaderTypeFromSourceFilePath(const std::string& shaderFilePath);
}

