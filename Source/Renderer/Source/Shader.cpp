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

// CONSTANTS
// ============================================================================

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


std::string GetCompileError(ID3D10Blob*& errorMessage, const std::string& shdPath)
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

#ifdef _WIN64
#define CALLING_CONVENTION __cdecl
#else	// _WIN32
#define CALLING_CONVENTION __stdcall
#endif

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

static std::unordered_map <std::string, EShaderStage > s_ShaderTypeStrLookup = 
{
	{"vs", EShaderStage::VS},
	{"gs", EShaderStage::GS},
	{"ds", EShaderStage::DS},
	{"hs", EShaderStage::HS},
	{"cs", EShaderStage::CS},
	{"ps", EShaderStage::PS}
};
// ============================================================================

std::array<ShaderID, EShaders::SHADER_COUNT> Shader::s_shaders;
std::string Shader::s_shaderCacheDirectory = "";

CPUConstant::CPUConstantPool CPUConstant::s_constants;
size_t CPUConstant::s_nextConstIndex = 0;

bool Shader::IsForwardPassShader(ShaderID shader)
{
	return shader == EShaders::FORWARD_BRDF ||	shader == EShaders::FORWARD_PHONG;
}

void Shader::LoadShaders(Renderer* pRenderer)
{
	// create the ShaderCache folder if it doesn't exist
	s_shaderCacheDirectory = Application::s_WorkspaceDirectory + "\\ShaderCache";
	DirectoryUtil::CreateFolderIfItDoesntExist(s_shaderCacheDirectory);

	PerfTimer timer;
	timer.Start();

	Log::Info("------------------------ COMPILING SHADERS ------------------------");
	const std::vector<EShaders> shaderEnumsInOrder = 
	{
		EShaders::FORWARD_PHONG				,
		EShaders::UNLIT						,
		EShaders::TEXTURE_COORDINATES		,
		EShaders::NORMAL					,
		EShaders::TANGENT					,
		EShaders::BINORMAL					,
		EShaders::LINE						,
		EShaders::TBN						,
		EShaders::DEBUG						,
		EShaders::SKYBOX					,
		EShaders::SKYBOX_EQUIRECTANGULAR	,
		EShaders::FORWARD_BRDF				,
		EShaders::SHADOWMAP_DEPTH			,
		EShaders::BILATERAL_BLUR			,
		EShaders::GAUSSIAN_BLUR_4x4			,
		EShaders::Z_PREPRASS				
	};

	constexpr unsigned VS_PS = SHADER_STAGE_VS | SHADER_STAGE_PS;
	const std::vector<ShaderDesc> shaderDescs =
	{
		ShaderDesc{ "PhongLighting<forward>",
		{
			ShaderStageDesc{ "Forward_Phong_vs.hlsl", {} },
			ShaderStageDesc{ "Forward_Phong_ps.hlsl", {} }
		}},
		ShaderDesc{ "UnlitTextureColor" , ShaderDesc::CreateStageDescsFromShaderName("UnlitTextureColor", VS_PS) },
		ShaderDesc{ "TextureCoordinates", 
		{
			ShaderStageDesc{"MVPTransformationWithUVs_vs.hlsl", {} },
			ShaderStageDesc{"TextureCoordinates_ps.hlsl"      , {} }
		}},
		ShaderDesc{ "Normal"            , ShaderDesc::CreateStageDescsFromShaderName("Normal", VS_PS)},
		ShaderDesc{ "Tangent"           , ShaderDesc::CreateStageDescsFromShaderName("Tangent", VS_PS)},
		ShaderDesc{ "Binormal"          , ShaderDesc::CreateStageDescsFromShaderName("Binormal", VS_PS)},
		ShaderDesc{ "Line"              , ShaderDesc::CreateStageDescsFromShaderName("Line", VS_PS)},
		ShaderDesc{ "TNB"               , ShaderDesc::CreateStageDescsFromShaderName("TNB", VS_PS)},
		ShaderDesc{ "Debug"             , ShaderDesc::CreateStageDescsFromShaderName("Debug", VS_PS)},
		ShaderDesc{ "Skybox"            , ShaderDesc::CreateStageDescsFromShaderName("Skybox", VS_PS)},
		ShaderDesc{ "SkyboxEquirectangular",
		{
			ShaderStageDesc{"Skybox_vs.hlsl"               , {} },
			ShaderStageDesc{"SkyboxEquirectangular_ps.hlsl", {} }
		}},
		ShaderDesc{ "Forward_BRDF"      , ShaderDesc::CreateStageDescsFromShaderName("Forward_BRDF", VS_PS)},
		ShaderDesc{ "DepthShader"       , ShaderDesc::CreateStageDescsFromShaderName("DepthShader", VS_PS)},
		ShaderDesc{ "BilateralBlur",
		{
			ShaderStageDesc{"FullscreenQuad_vs.hlsl", {} },
			ShaderStageDesc{"BilateralBlur_ps.hlsl" , {} }
		}},
		ShaderDesc{ "GaussianBlur4x4",
		{
			ShaderStageDesc{"FullscreenQuad_vs.hlsl" , {} },
			ShaderStageDesc{"GaussianBlur4x4_ps.hlsl", {} }
		}},
		ShaderDesc{ "ZPrePass",
		{
			ShaderStageDesc{"Deferred_Geometry_vs.hlsl"            , {} },
			ShaderStageDesc{"ViewSpaceNormalsAndPositions_ps.hlsl" , {} }
		}},
	};

	assert(shaderEnumsInOrder.size() == shaderDescs.size());
	
	// todo: do not depend on array index, use a lookup, remove s_shaders[]
	for (int i = 0; i < shaderEnumsInOrder.size(); ++i)
	{
		s_shaders[shaderEnumsInOrder[i]] = pRenderer->CreateShader(shaderDescs[i]);
	}

	timer.Stop();
	Log::Info("---------------------- COMPILING SHADERS DONE IN %.2fs ---------------------", timer.DeltaTime());
}

std::stack<std::string> Shader::UnloadShaders(Renderer* pRenderer)
{
	std::stack<std::string> fileNames;
	for (Shader*& shd : pRenderer->mShaders)
	{
		fileNames.push(shd->m_name);
		delete shd;
		shd = nullptr;
	}
	pRenderer->mShaders.clear();
	CPUConstant::s_nextConstIndex = 0;
	return fileNames;
}

Shader::Shader(const std::string& shaderFileName)
	: m_name(shaderFileName)
	, m_id(-1)
{}


Shader::Shader(const ShaderDesc& desc)
	: m_name(desc.shaderName)
	, m_id(-1)
{}

Shader::~Shader(void)
{
#if _DEBUG 
	//Log::Info("Shader dtor: %s", m_name.c_str());
#endif

	// todo: this really could use smart pointers...

	// release constants
	for (ConstantBuffer& cbuf : m_cBuffers)
	{
		if (cbuf.data)
		{
			cbuf.data->Release();
			cbuf.data = nullptr;
		}
	}

	if (m_layout)
	{
		m_layout->Release();
		m_layout = nullptr;
	}

	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = nullptr;
	}

	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = nullptr;
	}

	if (m_geometryShader)
	{
		m_geometryShader->Release();
		m_geometryShader = nullptr;
	}

	for (unsigned type = EShaderStage::VS; type < EShaderStage::COUNT; ++type)
	{
		if (m_shaderReflections.of[type])
		{
			m_shaderReflections.of[type]->Release();
			m_shaderReflections.of[type] = nullptr;
		}
	}
}

void Shader::CreateShader(ID3D11Device* pDevice, EShaderStage type, void* pBuffer, const size_t szShaderBinary)
{
	HRESULT result = {};
	const char* msg = "";
	switch (type)
	{
	case EShaderStage::VS:
		if (FAILED(pDevice->CreateVertexShader(pBuffer, szShaderBinary, NULL, &m_vertexShader)))
		{
			msg = "Error creating vertex shader program";
		}
		break;
	case EShaderStage::PS:
		if (FAILED(pDevice->CreatePixelShader(pBuffer, szShaderBinary, NULL, &m_pixelShader)))
		{
			msg = "Error creating pixel shader program";
		}
		break;
	case EShaderStage::GS:
		if (FAILED(pDevice->CreateGeometryShader(pBuffer, szShaderBinary, NULL, &m_geometryShader)))
		{
			msg = "Error creating pixel geometry program";
		}
		break;

	}

	if (FAILED(result))
	{
		OutputDebugString(msg);
		assert(false);
	}
}

bool IsCacheDirty(const std::string& sourcePath, const std::string& cachePath)
{
	if (!DirectoryUtil::FileExists(cachePath)) return true;

	auto GetIncludeFileName = [](const std::string& line)
	{
		const std::string str_search = "#include \"";
		if (line.find(str_search) != std::string::npos)
		{
			std::string quotedFileName = line.substr(str_search.size() - 1);
			return quotedFileName.substr(1, quotedFileName.size() - 2);
		}
		return std::string();
	};

	const std::string ShaderSourceDir = DirectoryUtil::GetFolderPath(sourcePath);
	const std::string ShaderCacheDir  = DirectoryUtil::GetFolderPath(cachePath);

	auto AreIncludesDirty = [&](const std::string srcPath)
	{
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
		}
		return false;
	};

	return DirectoryUtil::IsFileNewer(sourcePath, cachePath) || AreIncludesDirty(sourcePath);
}


void Shader::CompileShaders(ID3D11Device* device, const ShaderDesc& desc)
{
	HRESULT result;
	ShaderBlobs blobs;
	bool bPrinted = false;

	PerfTimer timer;
	timer.Start();

	

	// COMPILE SHADERS
	//----------------------------------------------------------------------------
	for (const ShaderStageDesc& stage : desc.stages)
	{
		if (stage.fileName.empty())
			continue;

		// stage.macros
		const std::string sourceFilePath = std::string(Renderer::sShaderRoot + stage.fileName);
		
		const EShaderStage type = GetShaderTypeFromSourceFilePath(sourceFilePath);

		// USE SHADER CACHE
		//
		const std::string cacheFileName = DirectoryUtil::GetFileNameFromPath(sourceFilePath) + ".bin";
		const std::string cacheFilePath = s_shaderCacheDirectory + "\\" + cacheFileName;
		const bool bUseCachedShaders = 
			DirectoryUtil::FileExists(cacheFilePath) 
			&& !IsCacheDirty(sourceFilePath, cacheFilePath)
			&& stage.macros.empty();
		//  ^^^^^^^^^^^^^^^^^^^^^^^
		// Currently there's no way to tell if macro value has changed since the caching.
		// Hence, we exclude the cached shader usage for shaders with preprocessor defines.
		//
		// TODO: maybe hash the shaders based on file name + hashes preprocessor defines 
		//---------------------------------------------------------------------------------

		if (bUseCachedShaders)
		{
			blobs.of[type] = CompileFromCachedBinary(cacheFilePath);
		}
		else
		{
			std::string errMsg;
			ID3D10Blob* pBlob;
			if (CompileFromSource(sourceFilePath, type, pBlob, errMsg, stage.macros))
			{
				blobs.of[type] = pBlob;

				// We also create cached binaries only for shaders with no preprocessor defines.
				if (stage.macros.empty())
				{
					CacheShaderBinary(cacheFilePath, blobs.of[type]);
				}
			}
			else
			{
				Log::Error(errMsg);
				assert(false);
				continue;
			}
		}

		CreateShader(device, type, blobs.of[type]->GetBufferPointer(), blobs.of[type]->GetBufferSize());
		if (!bPrinted)
		{
			const char* pMsgLoad = bUseCachedShaders ? "Loading cached shader binaries" : "Compiling shader from source";
			Log::Info("\t%s %s...", pMsgLoad, m_name.c_str());
			bPrinted = true;
		}

		SetReflections(blobs);
		//CheckSignatures();

	}


	// INPUT LAYOUT
	//---------------------------------------------------------------------------
	// https://stackoverflow.com/questions/42388979/directx-11-vertex-shader-reflection
	// setup the layout of the data that goes into the shader
	//
	D3D11_SHADER_DESC shaderDesc = {};
	m_shaderReflections.vsRefl->GetDesc(&shaderDesc);
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout(shaderDesc.InputParameters);

	D3D_PRIMITIVE primitiveDesc = shaderDesc.InputPrimitive;

	for (unsigned i = 0; i < shaderDesc.InputParameters; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
		m_shaderReflections.vsRefl->GetInputParameterDesc(i, &paramDesc);

		// fill out input element desc
		D3D11_INPUT_ELEMENT_DESC elementDesc;
		elementDesc.SemanticName = paramDesc.SemanticName;
		elementDesc.SemanticIndex = paramDesc.SemanticIndex;
		elementDesc.InputSlot = 0;
		elementDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		elementDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		elementDesc.InstanceDataStepRate = 0;

		// determine DXGI format
		if (paramDesc.Mask == 1)
		{
			if      (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  elementDesc.Format = DXGI_FORMAT_R32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  elementDesc.Format = DXGI_FORMAT_R32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
		}
		else if (paramDesc.Mask <= 3)
		{
			if      (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
		}
		else if (paramDesc.Mask <= 7)
		{
			if      (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		}
		else if (paramDesc.Mask <= 15)
		{
			if      (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		}


		//save element desc
		inputLayout[i] = elementDesc;
	}

	// Try to create Input Layout
	const auto* pData = inputLayout.data();
	if (pData)
	{
		result = device->CreateInputLayout(
			pData,
			shaderDesc.InputParameters,
			blobs.vs->GetBufferPointer(),
			blobs.vs->GetBufferSize(),
			&m_layout);

		if (FAILED(result))
		{
			OutputDebugString("Error creating input layout");
			assert(false);
		}
	}


	// CONSTANT BUFFERS 
	//---------------------------------------------------------------------------
	SetConstantBuffers(device);


	// TEXTURES & SAMPLERS
	//---------------------------------------------------------------------------
	auto sRefl = m_shaderReflections.psRefl;		// vsRefl? gsRefl?
	if (sRefl)
	{
		D3D11_SHADER_DESC desc = {};
		sRefl->GetDesc(&desc);

		unsigned texSlot = 0;	unsigned smpSlot = 0;
		for (unsigned i = 0; i < desc.BoundResources; ++i)
		{
			D3D11_SHADER_INPUT_BIND_DESC shdInpDesc;
			sRefl->GetResourceBindingDesc(i, &shdInpDesc);
			if (shdInpDesc.Type == D3D_SIT_SAMPLER)
			{
				ShaderSampler smp;
				smp.shdType = EShaderStage::PS;
				smp.bufferSlot = smpSlot++;
				m_samplers.push_back(smp);
				mShaderSamplerLookup[shdInpDesc.Name] = static_cast<int>(m_samplers.size() - 1);
			}
			else if (shdInpDesc.Type == D3D_SIT_TEXTURE)
			{
				ShaderTexture tex;
				tex.shdType = EShaderStage::PS;
				tex.bufferSlot = texSlot++;
				m_textures.push_back(tex);
				mShaderTextureLookup[shdInpDesc.Name] = static_cast<int>(m_textures.size() - 1);
			}
		}
	}

	// release blobs
	for (unsigned type = EShaderStage::VS; type < EShaderStage::COUNT; ++type)
	{
		if (blobs.of[type])
			blobs.of[type]->Release();
	}
}

void Shader::SetReflections(const ShaderBlobs& blobs)
{
	for(unsigned type = EShaderStage::VS; type < EShaderStage::COUNT; ++type)
	{
		if (blobs.of[type])
		{
			void** ppBuffer = reinterpret_cast<void**>(&this->m_shaderReflections.of[type]);
			if (FAILED(D3DReflect(blobs.of[type]->GetBufferPointer(), blobs.of[type]->GetBufferSize(), IID_ID3D11ShaderReflection, ppBuffer)))
			{
				Log::Error("Cannot get vertex shader reflection");
				assert(false);
			}
		}
	}
}

void Shader::CheckSignatures()
{
#if 0
	// get shader description --> input/output parameters
	std::vector<D3D11_SIGNATURE_PARAMETER_DESC> VSISignDescs, VSOSignDescs, PSISignDescs, PSOSignDescs;
	D3D11_SHADER_DESC VSDesc;
	m_vsRefl->GetDesc(&VSDesc);
	for (unsigned i = 0; i < VSDesc.InputParameters; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC input_desc;
		m_vsRefl->GetInputParameterDesc(i, &input_desc);
		VSISignDescs.push_back(input_desc);
	}

	for (unsigned i = 0; i < VSDesc.OutputParameters; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC output_desc;
		m_vsRefl->GetInputParameterDesc(i, &output_desc);
		VSOSignDescs.push_back(output_desc);
	}


	D3D11_SHADER_DESC PSDesc;
	m_psRefl->GetDesc(&PSDesc);
	for (unsigned i = 0; i < PSDesc.InputParameters; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC input_desc;
		m_psRefl->GetInputParameterDesc(i, &input_desc);
		PSISignDescs.push_back(input_desc);
	}

	for (unsigned i = 0; i < PSDesc.OutputParameters; ++i)
	{
		D3D11_SIGNATURE_PARAMETER_DESC output_desc;
		m_psRefl->GetInputParameterDesc(i, &output_desc);
		PSOSignDescs.push_back(output_desc);
	}

	// check VS-PS signature compatibility | wont be necessary when its 1 file.
	// THIS IS TEMPORARY
	if (VSOSignDescs.size() != PSISignDescs.size())
	{
		OutputDebugString("Error: Incompatible shader input/output signatures (sizes don't match)\n");
		assert(false);
	}
	else
	{
		for (size_t i = 0; i < VSOSignDescs.size(); ++i)
		{
			// TODO: order matters, semantic slot doesn't. check order
			;
		}
	}
#endif
	assert(false); // todo: refactor this
}

void Shader::SetConstantBuffers(ID3D11Device* device)
{
	// example: http://gamedev.stackexchange.com/a/62395/39920

	// OBTAIN CBUFFER LAYOUT INFORMATION
	//---------------------------------------------------------------------------------------
	for (EShaderStage type = EShaderStage::VS; type < EShaderStage::COUNT; type = (EShaderStage)(type + 1))
	{
		if (m_shaderReflections.of[type])
		{
			RegisterConstantBufferLayout(m_shaderReflections.of[type], type);
		}
	}

	// CREATE CPU & GPU CONSTANT BUFFERS
	//---------------------------------------------------------------------------------------
	// CPU CBuffers
	int constantBufferSlot = 0;
	for (const ConstantBufferLayout& cbLayout : m_CBLayouts)
	{
		std::vector<CPUConstantID> cpuBuffers;
		for (D3D11_SHADER_VARIABLE_DESC varDesc : cbLayout.variables)
		{
			auto& next = CPUConstant::GetNextAvailable();
			CPUConstantID c_id = std::get<1>(next);
			CPUConstant& c = std::get<0>(next);

			c._name = varDesc.Name;
			c._size = varDesc.Size;
			c._data = new char[c._size];
			memset(c._data, 0, c._size);
			m_constants.push_back(std::make_pair(constantBufferSlot, c_id));
			
		}
		++constantBufferSlot;
	}

	//LogConstantBufferLayouts();
	m_constantsUnsorted = m_constants;
	std::sort(m_constants.begin(), m_constants.end(), [](const ConstantBufferMapping& lhs, const ConstantBufferMapping& rhs) {
		const std::string& lstr = CPUConstant::Get(lhs.second)._name;	const std::string& rstr = CPUConstant::Get(rhs.second)._name;
		return lstr <= rstr;
	});
	//LogConstantBufferLayouts();

	// GPU CBuffer Description
	D3D11_BUFFER_DESC cBufferDesc;
	cBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cBufferDesc.MiscFlags = 0;
	cBufferDesc.StructureByteStride = 0;

	// GPU CBuffers
	for (const ConstantBufferLayout& cbLayout : m_CBLayouts)
	{
		ConstantBuffer cBuffer;
		cBufferDesc.ByteWidth = cbLayout.desc.Size;
		if (FAILED(device->CreateBuffer(&cBufferDesc, NULL, &cBuffer.data)))
		{
			OutputDebugString("Error creating constant buffer");
			assert(false);
		}
		cBuffer.dirty = true;
		cBuffer.shdType = cbLayout.stage;
		cBuffer.bufferSlot = cbLayout.bufSlot;
		m_cBuffers.push_back(cBuffer);
	}
}

bool Shader::CompileFromSource(const std::string& pathToFile, const EShaderStage& type, ID3D10Blob *& ref_pBob, std::string& errMsg, const std::vector<ShaderMacro>& macros)
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
		SHADER_ENTRY_POINT_LOOKUP.at(type),
		SHADER_COMPILER_VERSION_LOOKUP.at(type),
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

ID3D10Blob * Shader::CompileFromCachedBinary(const std::string & cachedBinaryFilePath)
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

void Shader::CacheShaderBinary(const std::string& shaderCacheFileName, ID3D10Blob * pCompiledBinary)
{
	const size_t shaderBinarySize = pCompiledBinary->GetBufferSize();

	char* pBuffer = reinterpret_cast<char*>(pCompiledBinary->GetBufferPointer());
	std::ofstream cache(shaderCacheFileName, std::ios::out | std::ios::binary);
	cache.write(pBuffer, shaderBinarySize);
	cache.close();
}

EShaderStage Shader::GetShaderTypeFromSourceFilePath(const std::string & shaderFilePath)
{
	const std::string sourceFileName = DirectoryUtil::GetFileNameWithoutExtension(shaderFilePath);
	const std::string shaderTypeStr = { *(sourceFileName.rbegin() + 1), *sourceFileName.rbegin() };
	return s_ShaderTypeStrLookup.at(shaderTypeStr);
}



void Shader::LogConstantBufferLayouts() const
{
	char inputTable[2048];
	sprintf_s(inputTable, "\n%s ConstantBuffers: -----\n", this->m_name.c_str());
	std::for_each(m_constants.begin(), m_constants.end(), [&inputTable](const ConstantBufferMapping& cMapping) {
		char entry[32];
		sprintf_s(entry, "(%d, %d)\t- %s\n", cMapping.first, cMapping.second, CPUConstant::Get(cMapping.second)._name.c_str());
		strcat_s(inputTable, entry);
	});
	strcat_s(inputTable, "-----\n");
	Log::Info(std::string(inputTable));
}

void Shader::RegisterConstantBufferLayout(ID3D11ShaderReflection* sRefl, EShaderStage type)
{
	D3D11_SHADER_DESC desc;
	sRefl->GetDesc(&desc);

	unsigned bufSlot = 0;
	for (unsigned i = 0; i < desc.ConstantBuffers; ++i)
	{
		ConstantBufferLayout bufferLayout;
		bufferLayout.buffSize = 0;
		ID3D11ShaderReflectionConstantBuffer* pCBuffer = sRefl->GetConstantBufferByIndex(i);
		pCBuffer->GetDesc(&bufferLayout.desc);

		// load desc of each variable for binding on buffer later on
		for (unsigned j = 0; j < bufferLayout.desc.Variables; ++j)
		{
			// get variable and type descriptions
			ID3D11ShaderReflectionVariable* pVariable = pCBuffer->GetVariableByIndex(j);
			D3D11_SHADER_VARIABLE_DESC varDesc;
			pVariable->GetDesc(&varDesc);
			bufferLayout.variables.push_back(varDesc);

			ID3D11ShaderReflectionType* pType = pVariable->GetType();
			D3D11_SHADER_TYPE_DESC typeDesc;
			pType->GetDesc(&typeDesc);
			bufferLayout.types.push_back(typeDesc);

			// accumulate buffer size
			bufferLayout.buffSize += varDesc.Size;
		}
		bufferLayout.stage = type;
		bufferLayout.bufSlot = bufSlot;
		++bufSlot;
		m_CBLayouts.push_back(bufferLayout);
	}
}

void Shader::ClearConstantBuffers()
{
	for (ConstantBuffer& cBuffer : m_cBuffers)
	{
		cBuffer.dirty = true;
	}
}

void Shader::UpdateConstants(ID3D11DeviceContext* context)
{
	for (unsigned i = 0; i < m_cBuffers.size(); ++i)
	{
		ConstantBuffer& CB = m_cBuffers[i];
		if (CB.dirty)	// if the CPU-side buffer is updated
		{
			ID3D11Buffer* data = CB.data;
			D3D11_MAPPED_SUBRESOURCE mappedResource;

			// Map sub-resource to GPU - update contents - discard the sub-resource
			context->Map(data, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			char* bufferPos = static_cast<char*>(mappedResource.pData);	// char* so we can advance the pointer
			for (const ConstantBufferMapping& indexIDPair : m_constantsUnsorted)
			{
				if (indexIDPair.first != i)
				{
					continue;
				}

				const int slotIndex = indexIDPair.first;
				const CPUConstantID c_id = indexIDPair.second;

				CPUConstant& c = CPUConstant::Get(c_id);
				memcpy(bufferPos, c._data, c._size);
				bufferPos += c._size;
			}
			context->Unmap(data, 0);

			// TODO: research update sub-resource (Setting constant buffer can be done once in setting the shader)

			// call XSSetConstantBuffers() from array using ShaderType enum
			(context->*SetShaderConstants[CB.shdType])(CB.bufferSlot, 1, &data);
			CB.dirty = false;
		}
	}
}

const std::string& Shader::Name() const
{
	return m_name;
}

ShaderID Shader::ID() const
{
	return m_id;
}

const std::vector<Shader::ConstantBufferLayout>& Shader::GetConstantBufferLayouts() const
{
	return m_CBLayouts;
}

const std::vector<ConstantBuffer>& Shader::GetConstantBuffers() const
{
	return m_cBuffers;
}

const ShaderTexture& Shader::GetTextureBinding(const std::string& textureName) const
{
	return m_textures[mShaderTextureLookup.at(textureName)];
}

const ShaderSampler& Shader::GetSamplerBinding(const std::string& samplerName) const
{
	return m_samplers[mShaderSamplerLookup.at(samplerName)];
}

bool Shader::HasTextureBinding(const std::string& textureName) const
{
	return mShaderTextureLookup.find(textureName) != mShaderTextureLookup.end();
}

bool Shader::HasSamplerBinding(const std::string& samplerName) const
{
	return mShaderSamplerLookup.find(samplerName) != mShaderSamplerLookup.end();
}

std::tuple<CPUConstant&, CPUConstantID> CPUConstant::GetNextAvailable()
{
	const CPUConstantID id = static_cast<CPUConstantID>(s_nextConstIndex++);
	return std::make_tuple(std::ref(s_constants[id]), id);
}

void CPUConstant::CleanUp()
{
	for (size_t i = 0; i < MAX_CONSTANT_BUFFERS; i++)
	{
		if (s_constants[i]._data)
		{
			delete s_constants[i]._data;
			s_constants[i]._data = nullptr;
		}
	}
}

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