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

#include "Shader.h"
#include "Renderer.h"
#include "Utilities/Log.h"
#include "Utilities/utils.h"
#include "Utilities/PerfTimer.h"
#include "Application/BaseSystem.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

constexpr const char* SHADER_COMPILER_VERSIONS[]	= { "vs_5_0", "gs_5_0", "", "", "", "ps_5_0" };
constexpr const char* SHADER_ENTRY_POINTS[]			= { "VSMain", "GSMain", "DSMain", "HSMain", "CSMain", "PSMain" };
ID3DInclude* const SHADER_INCLUDE_HANDLER = D3D_COMPILE_STANDARD_FILE_INCLUDE;		// use default include handler for using #include in shader files

#if defined( _DEBUG ) || defined ( FORCE_DEBUG )
const UINT SHADER_COMPILE_FLAGS = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
const UINT SHADER_COMPILE_FLAGS = D3DCOMPILE_ENABLE_STRICTNESS;
#endif

std::string Shader::s_shaderCacheDirectory = "";

CPUConstant::CPUConstantPool CPUConstant::s_constants;
size_t CPUConstant::s_nextConstIndex = 0;

// HELPER FUNCTIONS & CONSTANTS
// ============================================================================
void OutputShaderErrorMessage(ID3D10Blob* errorMessage, const CHAR* shaderFileName)
{
	char* compileErrors = (char*)errorMessage->GetBufferPointer();
	size_t bufferSize = errorMessage->GetBufferSize();

	std::stringstream ss;
	for (unsigned int i = 0; i < bufferSize; ++i)
	{
		ss << compileErrors[i];
	}
	OutputDebugString(ss.str().c_str());

	errorMessage->Release();
	errorMessage = 0;
	return;
}

void HandleCompileError(ID3D10Blob* errorMessage, const std::string& shdPath)
{
	if (errorMessage)
	{
		OutputShaderErrorMessage(errorMessage, shdPath.c_str());
	}
	else
	{
		Log::Error(shdPath);
	}

	// continue execution, make sure error is known
	//assert(false);
}

#ifdef _WIN64
#define CALLING_CONVENTION __cdecl
#else	// _WIN32
#define CALLING_CONVENTION __stdcall
#endif

static void(CALLING_CONVENTION ID3D11DeviceContext:: *SetShaderConstants[EShaderType::COUNT])
(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) = {
	&ID3D11DeviceContext::VSSetConstantBuffers,
	&ID3D11DeviceContext::GSSetConstantBuffers,
	&ID3D11DeviceContext::DSSetConstantBuffers,
	&ID3D11DeviceContext::HSSetConstantBuffers,
	&ID3D11DeviceContext::CSSetConstantBuffers,
	&ID3D11DeviceContext::PSSetConstantBuffers };

static std::unordered_map <std::string, EShaderType > s_ShaderTypeStrLookup = {
	{"vs", EShaderType::VS},
	{"gs", EShaderType::GS},
	{"ds", EShaderType::DS},
	{"hs", EShaderType::HS},
	{"cs", EShaderType::CS},
	{"ps", EShaderType::PS}
};
// ============================================================================

std::array<ShaderID, EShaders::SHADER_COUNT> Shader::s_shaders;

bool Shader::IsForwardPassShader(ShaderID shader)
{
	return shader == EShaders::FORWARD_BRDF ||	shader == EShaders::FORWARD_PHONG;
}

void Shader::LoadShaders(Renderer* pRenderer)
{
	// create the ShaderCache folder if it doesn't exist
	s_shaderCacheDirectory = BaseSystem::s_WorkspaceDirectory + "\\ShaderCache";
	if (CreateDirectory(s_shaderCacheDirectory.c_str(), NULL) || ERROR_ALREADY_EXISTS == GetLastError())
	{
		;// directory either successfully created or already exists: NOP
	}
	else
	{
		std::string errMsg = "Failed to create directory " + s_shaderCacheDirectory;
		MessageBox(NULL, errMsg.c_str(), "VQEngine: Error Initializing ShaderCache", MB_OK);
	}


	PerfTimer timer;
	timer.Start();

	Log::Info("------------------------ COMPILING SHADERS ------------------------");
	
	// todo: layouts from reflection?
	const std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 },
	};

	const std::vector<EShaderType> VS_PS  = { EShaderType::VS, EShaderType::PS };

	const std::vector<std::string> TextureCoordinates = { "MVPTransformationWithUVs_vs", "TextureCoordinates_ps" };

	const std::vector<std::string> BilateralBlurShaders			= { "FullscreenQuad_vs", "BilateralBlur_ps" };	// compute?
	const std::vector<std::string> GaussianBlur4x4Shaders		= { "FullscreenQuad_vs", "GaussianBlur4x4_ps" };	// compute?

	const std::vector<std::string> ZPrePassShaders				= { "Deferred_Geometry_vs", "ViewSpaceNormalsAndPositions_ps" };	

	const std::vector<std::string> EQSkybox = {"Skybox_vs", "SkyboxEquirectangular_ps"};

	// todo: limit enumerations? probably better to store just some ids
	s_shaders[EShaders::FORWARD_PHONG			]	= pRenderer->AddShader("Forward_Phong"			, layout);
	s_shaders[EShaders::UNLIT					]	= pRenderer->AddShader("UnlitTextureColor"		, layout);
	s_shaders[EShaders::TEXTURE_COORDINATES		]	= pRenderer->AddShader("TextureCoordinates"		, TextureCoordinates, VS_PS, layout);
	s_shaders[EShaders::NORMAL					]	= pRenderer->AddShader("Normal"					, layout);
	s_shaders[EShaders::TANGENT					]	= pRenderer->AddShader("Tangent"				, layout);
	s_shaders[EShaders::BINORMAL				]	= pRenderer->AddShader("Binormal"				, layout);
	s_shaders[EShaders::LINE					]	= pRenderer->AddShader("Line"					, layout);
	s_shaders[EShaders::TBN						]	= pRenderer->AddShader("TNB"					, layout);
	s_shaders[EShaders::DEBUG					]	= pRenderer->AddShader("Debug"					, layout);
	s_shaders[EShaders::SKYBOX					]	= pRenderer->AddShader("Skybox"					, layout);
	s_shaders[EShaders::SKYBOX_EQUIRECTANGULAR	]	= pRenderer->AddShader("SkyboxEquirectangular"	, EQSkybox, VS_PS, layout);
	s_shaders[EShaders::FORWARD_BRDF			]	= pRenderer->AddShader("Forward_BRDF"			, layout);
	s_shaders[EShaders::SHADOWMAP_DEPTH			]	= pRenderer->AddShader("DepthShader"			, layout);
	s_shaders[EShaders::BILATERAL_BLUR			]	= pRenderer->AddShader("BilateralBlur"			, BilateralBlurShaders		, VS_PS, layout);
	s_shaders[EShaders::GAUSSIAN_BLUR_4x4		]	= pRenderer->AddShader("GaussianBlur4x4"		, GaussianBlur4x4Shaders	, VS_PS, layout);
	s_shaders[EShaders::Z_PREPRASS				]	= pRenderer->AddShader("ZPrePass"				, ZPrePassShaders			, VS_PS, layout);

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

void Shader::ReloadShaders(Renderer* pRenderer)
{
	Log::Info("Reloading Shaders...");
	UnloadShaders(pRenderer);
	LoadShaders(pRenderer);
	Log::Info("Done");
}

Shader::Shader(const std::string& shaderFileName)
	:
	m_vertexShader(nullptr),
	m_pixelShader(nullptr),
	m_geometryShader(nullptr),
	m_layout(nullptr),
	m_name(shaderFileName),
	m_id(-1)
{}


Shader::~Shader(void)
{
#if _DEBUG 
	//Log::Info("Shader dtor: %s", m_name.c_str());
#endif

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

	for (unsigned type = EShaderType::VS; type < EShaderType::COUNT; ++type)
	{
		if (m_shaderReflections.of[type])
		{
			m_shaderReflections.of[type]->Release();
			m_shaderReflections.of[type] = nullptr;
		}
	}
}

void Shader::CreateShader(ID3D11Device* pDevice, EShaderType type, void* pBuffer, const size_t szShaderBinary)
{
	HRESULT result = {};
	const char* msg = "";
	switch (type)
	{
	case EShaderType::VS:
		if (FAILED(pDevice->CreateVertexShader(pBuffer, szShaderBinary, NULL, &m_vertexShader)))
		{
			msg = "Error creating vertex shader program";
		}
		break;
	case EShaderType::PS:
		if (FAILED(pDevice->CreatePixelShader(pBuffer, szShaderBinary, NULL, &m_pixelShader)))
		{
			msg = "Error creating pixel shader program";
		}
		break;
	case EShaderType::GS:
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
void Shader::CompileShaders(ID3D11Device* pDevice, const std::vector<std::string>& filePaths, const std::vector<InputLayout>& layouts)
{
	HRESULT result;
	ShaderBlobs blobs;
	bool bPrinted = false;

	PerfTimer timer;
	timer.Start();
	
	// COMPILE SHADERS
	//----------------------------------------------------------------------------
	for (const auto& sourceFilePath : filePaths)
	{	
		const EShaderType type = GetShaderTypeFromSourceFilePath(sourceFilePath);
		
		// USE SHADER CACHE
		//
		const std::string cacheFileName = DirectoryUtil::GetFileNameWithoutExtension(sourceFilePath) + ".hlsl.bin";
		const std::string cacheFilePath = s_shaderCacheDirectory + "\\" + cacheFileName;
		const bool bCacheIsDirty = DirectoryUtil::IsFileNewer(sourceFilePath, cacheFilePath);
		const bool bUseCachedShaders = DirectoryUtil::FileExists(cacheFilePath) && !bCacheIsDirty;


		if (bUseCachedShaders)
		{
			blobs.of[type] = CompileFromCachedBinary(cacheFilePath);
		}
		else
		{
			blobs.of[type] = CompileFromSource(sourceFilePath, type);
			CacheShaderBinary(cacheFilePath, blobs.of[type]);
		}

		CreateShader(pDevice, type, blobs.of[type]->GetBufferPointer(), blobs.of[type]->GetBufferSize());
		if (!bPrinted)
		{
			Log::Info("\tCompiling from %s %s...", bUseCachedShaders ? "cached shader binaries" : "source", m_name.c_str());
			bPrinted = true;
		}

		SetReflections(blobs);
		//CheckSignatures();

	}


	// INPUT LAYOUT
	//---------------------------------------------------------------------------
#if 1

	//setup the layout of the data that goes into the shader
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout(layouts.size());
	D3D11_SHADER_DESC shaderDesc = {};
	m_shaderReflections.vsRefl->GetDesc(&shaderDesc);
	shaderDesc.InputParameters;
	D3D_PRIMITIVE primitiveDesc = shaderDesc.InputPrimitive;

	UINT sz = static_cast<UINT>(layouts.size());
	for (unsigned i = 0; i < layouts.size(); ++i)
	{
		inputLayout[i].SemanticName			= layouts[i].semanticName.c_str();
		inputLayout[i].SemanticIndex		= 0;	// TODO: process string for semantic index
		inputLayout[i].Format				= static_cast<DXGI_FORMAT>(layouts[i].format);
		inputLayout[i].InputSlot			= 0;
		//inputLayout[i].AlignedByteOffset	= i == layouts.size() - 1 ? 8 : 12;
		inputLayout[i].AlignedByteOffset	= i == 0 ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
		inputLayout[i].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
		inputLayout[i].InstanceDataStepRate	= 0;
	}
	result = pDevice->CreateInputLayout(	inputLayout.data(), 
										sz, 
										blobs.vs->GetBufferPointer(),
										blobs.vs->GetBufferSize(), 
										&m_layout);
	if (FAILED(result))
	{
		OutputDebugString("Error creating input layout");
		assert(false);
	}
#else
	// todo: get layout from reflection and handle cpu input layouts
	// https://takinginitiative.wordpress.com/2011/12/11/directx-1011-basic-shader-reflection-automatic-input-layout-creation/
#endif

	// CBUFFERS & SHADER RESOURCES
	//---------------------------------------------------------------------------
	SetConstantBuffers(pDevice);
	
	// SET TEXTURES & SAMPLERS
	auto sRefl = m_shaderReflections.psRefl;		// vsRefl? gsRefl?
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
			smp.name = shdInpDesc.Name;
			smp.shdType = EShaderType::PS;
			smp.bufferSlot = smpSlot++;
			m_samplers.push_back(smp);
		}
		else if (shdInpDesc.Type == D3D_SIT_TEXTURE)
		{
			ShaderTexture tex;
			tex.name = shdInpDesc.Name;
			tex.shdType = EShaderType::PS;
			tex.bufferSlot = texSlot++;
			m_textures.push_back(tex);
		}
	}

	// release blobs
	for (unsigned type = EShaderType::VS; type < EShaderType::COUNT; ++type)
	{
		if (blobs.of[type]) 
			blobs.of[type]->Release();
	}
}

void Shader::SetReflections(const ShaderBlobs& blobs)
{
	for(unsigned type = EShaderType::VS; type < EShaderType::COUNT; ++type)
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
	for (EShaderType type = EShaderType::VS; type < EShaderType::COUNT; type = (EShaderType)(type + 1))
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
		cBuffer.shdType = cbLayout.shdType;
		cBuffer.bufferSlot = cbLayout.bufSlot;
		m_cBuffers.push_back(cBuffer);
	}
}

ID3D10Blob* Shader::CompileFromSource(const std::string & filePath, const EShaderType & type)
{
	const StrUtil::UnicodeString Path = filePath;
	const WCHAR* PathStr = Path.GetUnicodePtr();
	ID3D10Blob* errorMessage = nullptr;
	ID3D10Blob* blob = { nullptr };
	if (FAILED(D3DCompileFromFile(
		PathStr,
		NULL,
		SHADER_INCLUDE_HANDLER,
		SHADER_ENTRY_POINTS[type],
		SHADER_COMPILER_VERSIONS[type],
		SHADER_COMPILE_FLAGS,
		0,
		&blob,
		&errorMessage)))
	{
		HandleCompileError(errorMessage, filePath);
		return nullptr;
	}
	return blob;
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

EShaderType Shader::GetShaderTypeFromSourceFilePath(const std::string & shaderFilePath)
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

void Shader::RegisterConstantBufferLayout(ID3D11ShaderReflection* sRefl, EShaderType type)
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
		bufferLayout.shdType = type;
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
				if (indexIDPair.first != i) continue;
				const int slotIndex = indexIDPair.first;
				const CPUConstantID c_id = indexIDPair.second;
				CPUConstant& c = CPUConstant::Get(c_id);
				memcpy(bufferPos, c._data, c._size);
				bufferPos += c._size;
			}
			context->Unmap(data, 0);

			// TODO: research update sub-resource (Setting constant buffer can be done once in setting the shader, see it)

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