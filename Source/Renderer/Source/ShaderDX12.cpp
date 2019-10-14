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

#include "Shader.h"


#include "Renderer.h"
#include "Utilities/Log.h"
#include "Utilities/utils.h"
#include "Utilities/PerfTimer.h"
#include "Application/Application.h"

#include <fstream>
#include <sstream>
#include <functional>
#include <stack>

//-------------------------------------------------------------------------------------------------------------
// CONSTANTS & STATICS
//-------------------------------------------------------------------------------------------------------------
static const std::unordered_map<EShaderStage, const char*> SHADER_COMPILER_VERSION_LOOKUP =
{
	{ EShaderStage::VS, "vs_5_0" },
	{ EShaderStage::GS, "gs_5_0" },
	{ EShaderStage::DS, "ds_5_0" },
	{ EShaderStage::HS, "hs_5_0" },
	{ EShaderStage::PS, "ps_5_0" },
	{ EShaderStage::CS, "cs_5_0" },
};
static const std::unordered_map<EShaderStage, const char*> SHADER_ENTRY_POINT_LOOKUP =
{
	{ EShaderStage::VS, "VSMain" },
	{ EShaderStage::GS, "GSMain" },
	{ EShaderStage::DS, "DSMain" },
	{ EShaderStage::HS, "HSMain" },
	{ EShaderStage::PS, "PSMain" },
	{ EShaderStage::CS, "CSMain" },
};

ID3DInclude* const SHADER_INCLUDE_HANDLER = D3D_COMPILE_STANDARD_FILE_INCLUDE;		// use default include handler for using #include in shader files

#if defined( _DEBUG ) || defined ( FORCE_DEBUG )
const UINT SHADER_COMPILE_FLAGS = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
const UINT SHADER_COMPILE_FLAGS = D3DCOMPILE_ENABLE_STRICTNESS;
#endif



#ifdef _WIN64
#define CALLING_CONVENTION __cdecl
#else	// _WIN32
#define CALLING_CONVENTION __stdcall
#endif


///static void(CALLING_CONVENTION ID3D11DeviceContext:: *SetShaderConstants[EShaderStage::COUNT])
///(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) =
///{
///	&ID3D11DeviceContext::VSSetConstantBuffers,
///	&ID3D11DeviceContext::GSSetConstantBuffers,
///	&ID3D11DeviceContext::DSSetConstantBuffers,
///	&ID3D11DeviceContext::HSSetConstantBuffers,
///	&ID3D11DeviceContext::PSSetConstantBuffers,
///	&ID3D11DeviceContext::CSSetConstantBuffers,
///};


static std::unordered_map <std::string, EShaderStage > s_ShaderTypeStrLookup =
{
	{"vs", EShaderStage::VS},
	{"gs", EShaderStage::GS},
	{"ds", EShaderStage::DS},
	{"hs", EShaderStage::HS},
	{"cs", EShaderStage::CS},
	{"ps", EShaderStage::PS}
};


//-------------------------------------------------------------------------------------------------------------
// STATIC FUNCTIONS
//-------------------------------------------------------------------------------------------------------------
static std::string GetCompileError(ID3D10Blob*& errorMessage, const std::string& shdPath)
{
	if (errorMessage)
	{
		char* compileErrors = (char*)errorMessage->GetBufferPointer();
		size_t bufferSize = errorMessage->GetBufferSize();

		std::stringstream ss;
		for (unsigned int i = 0; i < bufferSize; ++i)
		{
			ss << compileErrors[i];
		}
		errorMessage->Release();
		return ss.str();
	}
	else
	{
		Log::Error(shdPath);
		return ("Error: " + shdPath);
	}
}

static std::string GetIncludeFileName(const std::string& line)
{
	const std::string str_search = "#include \"";
	const size_t foundPos = line.find(str_search);
	if (foundPos != std::string::npos)
	{
		std::string quotedFileName = line.substr(foundPos + strlen("#include "), line.size() - foundPos);// +str_search.size() - 1);
		return quotedFileName.substr(1, quotedFileName.size() - 2);
	}
	return std::string();
}

static bool AreIncludesDirty(const std::string& srcPath, const std::string& cachePath)
{
	const std::string ShaderSourceDir = DirectoryUtil::GetFolderPath(srcPath);
	const std::string ShaderCacheDir = DirectoryUtil::GetFolderPath(cachePath);

	std::stack<std::string> includeStack;
	includeStack.push(srcPath);
	while (!includeStack.empty())
	{
		const std::string includeFilePath = includeStack.top();
		includeStack.pop();
		std::ifstream src = std::ifstream(includeFilePath.c_str());
		if (!src.good())
		{
			Log::Error("[ShaderCompile] Cannot open source file: %s", includeFilePath.c_str());
			continue;
		}

		std::string line;
		while (getline(src, line))
		{
			const std::string includeFileName = GetIncludeFileName(line);
			if (includeFileName.empty()) continue;

			const std::string includeSourcePath = ShaderSourceDir + includeFileName;
			const std::string includeCachePath = ShaderCacheDir + includeFileName;

			if (DirectoryUtil::IsFileNewer(includeSourcePath, cachePath))
				return true;
			includeStack.push(includeSourcePath);
		}
		src.close();
	}
	return false;
}

static bool IsCacheDirty(const std::string& sourcePath, const std::string& cachePath)
{
	if (!DirectoryUtil::FileExists(cachePath)) return true;

	return DirectoryUtil::IsFileNewer(sourcePath, cachePath) || AreIncludesDirty(sourcePath, cachePath);
}


//-------------------------------------------------------------------------------------------------------------
// PUBLIC INTERFACE
//-------------------------------------------------------------------------------------------------------------
Shader::Shader(const ShaderDesc& desc)
	: mName(desc.shaderName)
	, mDescriptor(desc)
	, mID(-1)
{}

Shader::~Shader(void)
{

}




//-------------------------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
//-------------------------------------------------------------------------------------------------------------
std::array<ShaderStageDesc, EShaderStageFlags::SHADER_STAGE_COUNT> ShaderDesc::CreateStageDescsFromShaderName(const char* pShaderName, unsigned flagStages)
{
	const std::string shaderName = pShaderName;
	std::array<ShaderStageDesc, EShaderStageFlags::SHADER_STAGE_COUNT> descs;
	int idx = 0;
	if (flagStages & SHADER_STAGE_VS)
	{
		descs[idx++] = ShaderStageDesc{ shaderName + "_vs.hlsl", {} };
	}
	if (flagStages & SHADER_STAGE_GS)
	{
		descs[idx++] = ShaderStageDesc{ shaderName + "_gs.hlsl", {} };
	}
	if (flagStages & SHADER_STAGE_DS)
	{
		descs[idx++] = ShaderStageDesc{ shaderName + "_ds.hlsl", {} };
	}
	if (flagStages & SHADER_STAGE_HS)
	{
		descs[idx++] = ShaderStageDesc{ shaderName + "_hs.hlsl", {} };
	}
	if (flagStages & SHADER_STAGE_PS)
	{
		descs[idx++] = ShaderStageDesc{ shaderName + "_ps.hlsl", {} };
	}
	if (flagStages & SHADER_STAGE_CS)
	{
		descs[idx++] = ShaderStageDesc{ shaderName + "_cs.hlsl", {} };
	}
	return descs;
}


Shader::ByteCodeType Shader::GetShaderByteCode(EShaderStage stage) const
{

	return ByteCodeType();
}

Shader::InputLayoutType Shader::GetShaderInputLayoutType() const
{
	Shader::InputLayoutType inputLayout = {};
	// TODO: populate inputLayout
	assert(false);
	return inputLayout;
}

Shader ShaderUtil::CompileShader(const ShaderDesc & desc)
{
	Shader shader(desc);
	//shader.CompileShader();
	assert(false);
	return shader;
}
