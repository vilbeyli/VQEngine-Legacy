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
#include <unordered_map>
#include <functional>
#include <stack>

//-------------------------------------------------------------------------------------------------------------
// CONSTANTS & STATICS
//-------------------------------------------------------------------------------------------------------------
static const std::unordered_map<EShaderStage, const char*> SHADER_COMPILER_VERSION_5_LOOKUP = 
{
	{ EShaderStage::VS, "vs_5_0" },
	{ EShaderStage::GS, "gs_5_0" },
	{ EShaderStage::DS, "ds_5_0" },
	{ EShaderStage::HS, "hs_5_0" },
	{ EShaderStage::PS, "ps_5_0" },
	{ EShaderStage::CS, "cs_5_0" },
};
static const std::unordered_map<EShaderStage, const char*> SHADER_COMPILER_VERSION_6_LOOKUP =
{
	{ EShaderStage::VS, "vs_6_0" },
	{ EShaderStage::GS, "gs_6_0" },
	{ EShaderStage::DS, "ds_6_0" },
	{ EShaderStage::HS, "hs_6_0" },
	{ EShaderStage::PS, "ps_6_0" },
	{ EShaderStage::CS, "cs_6_0" },
};
static const std::unordered_map<EShaderStage, const char*> SHADER_DEFAULT_ENTRY_POINT_LOOKUP =
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



#if USE_DX12
#include <d3d11.h> /// D3D10_SHADER_MACRO
#pragma comment(lib, "d3dcompiler.lib")	// functionality for compiling shaders

#else
static void(CALLING_CONVENTION ID3D11DeviceContext:: *SetShaderConstants[EShaderStage::COUNT])
(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) = 
{
	&ID3D11DeviceContext::VSSetConstantBuffers,
	&ID3D11DeviceContext::GSSetConstantBuffers,
	&ID3D11DeviceContext::DSSetConstantBuffers,
	&ID3D11DeviceContext::HSSetConstantBuffers,
	&ID3D11DeviceContext::PSSetConstantBuffers, 
	&ID3D11DeviceContext::CSSetConstantBuffers,
};
#endif // USE_DX12

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

bool ShaderUtils::AreIncludesDirty(const std::string& srcPath, const std::string& cachePath)
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

bool ShaderUtils::IsCacheDirty(const std::string& sourcePath, const std::string& cachePath)
{
	if (!DirectoryUtil::FileExists(cachePath))
	{
		return true;
	}

	return DirectoryUtil::IsFileNewer(sourcePath, cachePath) || AreIncludesDirty(sourcePath, cachePath);
}


bool ShaderUtils::CompileFromSource(const std::string& pathToFile, const EShaderStage& type, ID3D10Blob *& ref_pBob, std::string& errMsg, const std::vector<ShaderMacro>& macros)
{
	const StrUtil::UnicodeString Path = pathToFile;
	const WCHAR* PathStr = Path.GetUnicodePtr();
	ID3D10Blob* errorMessage = nullptr;

	int i = 0;
	std::vector<D3D10_SHADER_MACRO> d3dMacros(macros.size() + 1);
	std::for_each(RANGE(macros), [&](const ShaderMacro& macro)
	{
		d3dMacros[i++] = D3D10_SHADER_MACRO({ macro.name.c_str(), macro.value.c_str() });
	});
	d3dMacros[i] = { NULL, NULL };

	if (FAILED(D3DCompileFromFile(
		PathStr,
		d3dMacros.data(),
		SHADER_INCLUDE_HANDLER,
		SHADER_DEFAULT_ENTRY_POINT_LOOKUP.at(type),
		SHADER_COMPILER_VERSION_5_LOOKUP.at(type),
		SHADER_COMPILE_FLAGS,
		0,
		&ref_pBob,
		&errorMessage)))
	{

		errMsg = GetCompileError(errorMessage, pathToFile);
		return false;
	}
	return true;
}

ID3D10Blob * ShaderUtils::CompileFromCachedBinary(const std::string & cachedBinaryFilePath)
{
	std::ifstream cache(cachedBinaryFilePath, std::ios::in | std::ios::binary | std::ios::ate);
	const size_t shaderBinarySize = cache.tellg();
	void* pBuffer = calloc(1, shaderBinarySize);
	cache.seekg(0);
	cache.read(reinterpret_cast<char*>(pBuffer), shaderBinarySize);
	cache.close();

	ID3D10Blob* pBlob = { nullptr };
	D3DCreateBlob(shaderBinarySize, &pBlob);
	memcpy(pBlob->GetBufferPointer(), pBuffer, shaderBinarySize);
	free(pBuffer);

	return pBlob;
}

void ShaderUtils::CacheShaderBinary(const std::string& shaderCacheFileName, ID3D10Blob * pCompiledBinary)
{
	const size_t shaderBinarySize = pCompiledBinary->GetBufferSize();

	char* pBuffer = reinterpret_cast<char*>(pCompiledBinary->GetBufferPointer());
	std::ofstream cache(shaderCacheFileName, std::ios::out | std::ios::binary);
	cache.write(pBuffer, shaderBinarySize);
	cache.close();
}

EShaderStage ShaderUtils::GetShaderTypeFromSourceFilePath(const std::string & shaderFilePath)
{
	const std::string sourceFileName = DirectoryUtil::GetFileNameWithoutExtension(shaderFilePath);
	const std::string shaderTypeStr = { *(sourceFileName.rbegin() + 1), *sourceFileName.rbegin() };
	return s_ShaderTypeStrLookup.at(shaderTypeStr);
}

size_t ShaderUtils::GeneratePreprocessorDefinitionsHash(const std::vector<ShaderMacro>& macros)
{
	if (macros.empty()) return 0;
	std::string concatenatedMacros;
	for (const ShaderMacro& macro : macros)
		concatenatedMacros += macro.name + macro.value;
	return std::hash<std::string>()(concatenatedMacros);
}




//-------------------------------------------------------------------------------------------------------------
// PUBLIC INTERFACE
//-------------------------------------------------------------------------------------------------------------

#if !USE_DX12
const std::vector<Shader::ConstantBufferLayout>& Shader::GetConstantBufferLayouts() const { return m_CBLayouts; }
const std::vector<ConstantBufferBinding>& Shader::GetConstantBuffers() const { return mConstantBuffers; }
const TextureBinding& Shader::GetTextureBinding(const std::string& textureName) const { return mTextureBindings[mShaderTextureLookup.at(textureName)]; }
const SamplerBinding& Shader::GetSamplerBinding(const std::string& samplerName) const { return mSamplerBindings[mShaderSamplerLookup.at(samplerName)]; }
bool Shader::HasTextureBinding(const std::string& textureName) const { return mShaderTextureLookup.find(textureName) != mShaderTextureLookup.end(); }
bool Shader::HasSamplerBinding(const std::string& samplerName) const { return mShaderSamplerLookup.find(samplerName) != mShaderSamplerLookup.end(); }

Shader::Shader(const std::string& shaderFileName)
	: mName(shaderFileName)
	, mID(-1)
{}

Shader::Shader(const ShaderDesc& desc)
	: mName(desc.shaderName)
	, mDescriptor(desc)
	, mID(-1)
{}

Shader::~Shader(void)
{
#if _DEBUG 
	//Log::Info("Shader dtor: %s", m_name.c_str());
#endif

	// todo: this really could use smart pointers...

	// release constants
	ReleaseResources();
}
#endif


bool Shader::HasSourceFileBeenUpdated() const
{
	bool bUpdated = false;
	for (EShaderStage stage = EShaderStage::VS; stage < EShaderStage::COUNT; stage = (EShaderStage)(stage + 1))
	{
		if (mDirectories.find(stage) != mDirectories.end())
		{
			const std::string& path = mDirectories.at(stage).fullPath;
			const std::string& cachePath = mDirectories.at(stage).cachePath;
			bUpdated |= mDirectories.at(stage).lastWriteTime < std::experimental::filesystem::last_write_time(path);

			if (!bUpdated) // check include files only when source is not updated
			{
				bUpdated |= ShaderUtils::AreIncludesDirty(path, cachePath);
			}
		}
	}
	return bUpdated;
}


#if USE_DX12

#else
bool Shader::Reload(ID3D11Device* device)
{
	Shader copy(this->mDescriptor);
	copy.mID = this->mID;
	ReleaseResources();
	this->mID = copy.mID;
	return CompileShaders(device, copy.mDescriptor);
}

void Shader::ClearConstantBuffers()
{
	for (ConstantBufferBinding& cBuffer : mConstantBuffers)
	{
		cBuffer.dirty = true;
	}
}

void Shader::UpdateConstants(ID3D11DeviceContext* context)
{
	for (unsigned i = 0; i < mConstantBuffers.size(); ++i)
	{
		ConstantBufferBinding& CB = mConstantBuffers[i];
		if (CB.dirty)	// if the CPU-side buffer is updated
		{
			ID3D11Buffer* data = CB.data;
			D3D11_MAPPED_SUBRESOURCE mappedResource;

			// Map sub-resource to GPU - update contents - discard the sub-resource
			context->Map(data, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			char* bufferPos = static_cast<char*>(mappedResource.pData);	// char* so we can advance the pointer
			for (const ConstantBufferMapping& indexIDPair : m_constants)
			{
				if (indexIDPair.first != i)
				{
					continue;
				}

				const int slotIndex = indexIDPair.first;
				const CPUConstantID c_id = indexIDPair.second;
				assert(c_id < mCPUConstantBuffers.size());
				CPUConstant& c = mCPUConstantBuffers[c_id];
				memcpy(bufferPos, c._data, c._size);
				bufferPos += c._size;
			}
			context->Unmap(data, 0);

			// TODO: research update sub-resource (Setting constant buffer can be done once in setting the shader)

			// call XSSetConstantBuffers() from array using ShaderType enum
			(context->*SetShaderConstants[CB.shaderStage])(CB.bufferSlot, 1, &data);
			CB.dirty = false;
		}
	}
}
#endif



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