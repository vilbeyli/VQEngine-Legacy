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

static std::unordered_map <EShaderStage, std::string> s_ShaderStrTypeLookup =
{
	{EShaderStage::VS, "VS"},
	{EShaderStage::GS, "GS"},
	{EShaderStage::DS, "DS"},
	{EShaderStage::HS, "HS"},
	{EShaderStage::PS, "PS"},
	{EShaderStage::CS, "CS"}
};

static std::unordered_map<EShaderStage, D3D12_SHADER_VISIBILITY> s_ShaderVisibilityTypeLookup =
{
	{EShaderStage::VS, D3D12_SHADER_VISIBILITY_VERTEX   },
	{EShaderStage::GS, D3D12_SHADER_VISIBILITY_GEOMETRY },
	{EShaderStage::DS, D3D12_SHADER_VISIBILITY_DOMAIN   },
	{EShaderStage::HS, D3D12_SHADER_VISIBILITY_HULL     },
	{EShaderStage::PS, D3D12_SHADER_VISIBILITY_PIXEL    },
};


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


Shader::ByteCodeType Shader::GetShaderByteCode(EShaderStage stage) const
{
	Shader::ByteCodeType bc;
	assert(mShaderStageByteCodes.of[stage]);
	bc.pShaderBytecode = mShaderStageByteCodes.of[stage]->GetBufferPointer();
	bc.BytecodeLength = mShaderStageByteCodes.of[stage]->GetBufferSize();
	return bc;
}

Shader::InputLayoutDescType Shader::GetShaderInputLayoutDesc() const
{
	Shader::InputLayoutDescType inputLayout = {};
	inputLayout.NumElements = static_cast<UINT>(mVSInputElements.size());
	inputLayout.pInputElementDescs = mVSInputElements.data();
	return inputLayout;
}


//-------------------------------------------------------------------------------------------------------------
// UTILITY FUNCTIONS
//-------------------------------------------------------------------------------------------------------------


static void SetReflections(const Shader::ShaderBlobs& blobs, Shader::ShaderReflections& outReflections)
{
	for (unsigned stage = EShaderStage::VS; stage < EShaderStage::COUNT; ++stage)
	{
		if (blobs.of[stage])
		{
			void** ppBuffer = reinterpret_cast<void**>(&outReflections.of[stage]);
			if (FAILED(D3DReflect(blobs.of[stage]->GetBufferPointer(), blobs.of[stage]->GetBufferSize(), IID_ID3D12ShaderReflection, ppBuffer)))
			{
				Log::Error("Cannot get %s shader reflection", s_ShaderStrTypeLookup.at(static_cast<EShaderStage>(stage)).c_str());
				assert(false);
			}
		}
	}
}

static void CreateShaderStage(RHIDevice pDevice, EShaderStage stage, void* pBuffer, const size_t szShaderBinary)
{
	//HRESULT result = {};
	//const char* msg = "";

	/////Includer Include;
	//const char *pEntryPoint = "";
	//const char *pTarget     = "";
	//ID3DBlob *pError1, *pCode1;
	//UINT Flags1 = 0;
	//UINT Flags2 = 0;
	//size_t hash = 0;

	//ID3DBlob *pError, *pCode;
	//HRESULT res = D3DCompile(
	//	pCode1->GetBufferPointer()
	//	, pCode1->GetBufferSize()
	//	, NULL, NULL, NULL
	//	, pEntryPoint
	//	, pTarget
	//	, Flags1, Flags2, 
	//	&pCode, &pError
	//);

	//if (res == S_OK)
	//{
	//	pCode1->Release();

	//	*outSpvSize = pCode->GetBufferSize();
	//	*outSpvData = (char *)malloc(*outSpvSize);

	//	memcpy(*outSpvData, pCode->GetBufferPointer(), *outSpvSize);

	//	pCode->Release();

	//	SaveFile(filenameSpv.c_str(), *outSpvData, *outSpvSize, true);
	//	return true;
	//}
	//else
	//{
	//	const char *msg = (const char *)pError->GetBufferPointer();
	//	///std::string err = format("*** Error compiling %p.hlsl ***\n%s\n", hash, msg);
	//	///Trace(err);
	//	Log::Error("Error compiling %p.hlsl ***\n%s", hash, msg);

	//	///MessageBoxA(0, err.c_str(), "Error", 0);
	//	pError->Release();
	//}

	switch (stage)
	{
	case EShaderStage::VS:
		
		break;
	case EShaderStage::PS:
		break;
	case EShaderStage::GS:
		break;
	case EShaderStage::CS:
		break;
	}

	//if (FAILED(result))
	//{
	//	OutputDebugString(msg);
	//	assert(false);
	//}
}

//static void SetInputLayouts(RHIDevice pDevice, const Shader::ShaderReflections& shaderReflections, Shader::InputLayoutDescType& outInputLayout)
static void SetInputLayouts(RHIDevice pDevice, const Shader::ShaderReflections& shaderReflections, std::vector<D3D12_INPUT_ELEMENT_DESC>& outInputElementDescs)
{
	if (shaderReflections.vsRefl)
	{
		D3D12_SHADER_DESC shaderDesc = {};
		shaderReflections.vsRefl->GetDesc(&shaderDesc);

		outInputElementDescs.resize(shaderDesc.InputParameters);
		for (unsigned i = 0; i < shaderDesc.InputParameters; ++i)
		{
			D3D12_SIGNATURE_PARAMETER_DESC paramDesc = {};
			shaderReflections.vsRefl->GetInputParameterDesc(i, &paramDesc);

			// fill out input element desc
			D3D12_INPUT_ELEMENT_DESC elementDesc = {};
			elementDesc.SemanticName = paramDesc.SemanticName;
			elementDesc.SemanticIndex = paramDesc.SemanticIndex;
			elementDesc.InputSlot = 0;
			elementDesc.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
			elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
			elementDesc.InstanceDataStepRate = 0;

			// determine DXGI format
			if (paramDesc.Mask == 1)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  elementDesc.Format = DXGI_FORMAT_R32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  elementDesc.Format = DXGI_FORMAT_R32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			else if (paramDesc.Mask <= 3)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  elementDesc.Format = DXGI_FORMAT_R32G32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  elementDesc.Format = DXGI_FORMAT_R32G32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			else if (paramDesc.Mask <= 7)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  elementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  elementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			else if (paramDesc.Mask <= 15)
			{
				if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_UINT32)  elementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_SINT32)  elementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
				else if (paramDesc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) elementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}

			outInputElementDescs[i] = elementDesc; //save element desc
		}
	}
}

static void SetRootParameters(RHIDevice pDevice, const Shader::ShaderReflections& shaderReflections)
{
	// TODO: finalize this later on, move on with hardcoded root sign for now
	return;

	// This is the highest version - If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	D3D12_SIGNATURE_PARAMETER_DESC signParamDesc = {};
	
	std::array< std::vector<D3D12_DESCRIPTOR_RANGE1>, EShaderStage::COUNT> descRanges;
	std::vector<D3D12_ROOT_PARAMETER1> rootParameters;

	int reg = 0;
	int numDescs = 0;
	for (unsigned stage = 0; stage < EShaderStage::COUNT; ++stage)
	{
		if (shaderReflections.of[stage])
		{
			const D3D12_SHADER_VISIBILITY shaderVisiblity = s_ShaderVisibilityTypeLookup.at(static_cast<EShaderStage>(stage));

			// get shader desc
			D3D12_SHADER_DESC shaderDesc = {};
			shaderReflections.of[stage]->GetDesc(&shaderDesc);

			//
			// cbuffers
			//

			D3D12_DESCRIPTOR_RANGE1 descRange = {};
			descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			descRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;  // ?
			descRange.NumDescriptors = shaderDesc.ConstantBuffers; 
			for(UINT cBuffer = 0; cBuffer < shaderDesc.ConstantBuffers; ++cBuffer)
			{
				ID3D12ShaderReflectionConstantBuffer* pReflCBuffer = shaderReflections.of[stage]->GetConstantBufferByIndex(cBuffer);
				D3D12_SHADER_BUFFER_DESC bufDesc = {};
				pReflCBuffer->GetDesc(&bufDesc);
				


			}
			
			if (shaderDesc.ConstantBuffers != 0)
				descRanges[stage].push_back(descRange);

			//
			// Textures / Samplers
			//
			for (UINT resource = 0; resource < shaderDesc.BoundResources; ++resource)
			{
				D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
				shaderReflections.of[stage]->GetResourceBindingDesc(resource, &bindDesc);

				D3D12_DESCRIPTOR_RANGE1 descRange = {};
				switch (bindDesc.Type)
				{
				case D3D_SIT_CBUFFER:
					continue; // skip cbuffers as process them early on
					break;
				case D3D_SIT_TBUFFER:

					break;

				case D3D_SIT_TEXTURE:
				{
					descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
					descRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_NONE;  // ?
					descRange.NumDescriptors = 1; // ?
					descRange.OffsetInDescriptorsFromTableStart = numDescs++;
					descRanges[stage].push_back(descRange);
				} break;

				case D3D_SIT_SAMPLER:
					//descRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
					//descRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_NONE;  // ?
					//descRange.NumDescriptors = 1; // ?
					//descRange.OffsetInDescriptorsFromTableStart = numDescs++;
					//descRanges[stage].push_back(descRange);
					break;

				default:
					assert(false);
					break;
				}
				
					
			}


			// create root parameter descriptor
			if (!descRanges[stage].empty())
			{

				D3D12_ROOT_PARAMETER1 rootParameter = {};
				rootParameter.ShaderVisibility = shaderVisiblity;
				rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

				rootParameter.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descRanges[stage].size());
				rootParameter.DescriptorTable.pDescriptorRanges = descRanges[stage].data();
				rootParameters.push_back(rootParameter);
			}
		} // if stage != null
	}

#if 1
	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS 
		//| D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
		;

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC  rootSignatureDesc = { };
	rootSignatureDesc.Version = featureData.HighestVersion;
	rootSignatureDesc.Desc_1_1.Flags = rootSignatureFlags;
	rootSignatureDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParameters.size());
	rootSignatureDesc.Desc_1_1.pParameters = &rootParameters[0];
	///rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
	///rootSignatureDesc.Desc_1_1.pStaticSamplers   = &samplerDesc;

	Renderer::RootSignature pRootSign;
	ID3DBlob* pSignature = nullptr;
	ID3DBlob* pError = nullptr;
	VQ_D3D12_UTILS::ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &pSignature, &pError));
	VQ_D3D12_UTILS::ThrowIfFailed(pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&pRootSign.ptr)));
#endif
}

bool Shader::CompileShaderStages(RHIDevice pDevice)
{
	bool bPrinted = false;
	constexpr const char * SHADER_BINARY_EXTENSION = ".bin";
	HRESULT result;
	ShaderBlobs& blobs = mShaderStageByteCodes;
	ShaderReflections reflections;

	PerfTimer timer;
	timer.Start();

	// COMPILE SHADER STAGES
	//----------------------------------------------------------------------------
	for (const ShaderStageDesc& stageDesc : mDescriptor.stages)
	{
		if (stageDesc.fileName.empty())
			continue;

		// stage.macros
		
		const std::string shaderSourceRootDir = Application::mWorkspaceDirectoryLookup.at(Application::EDirectories::SHADER_SOURCE);
		const std::string sourceFilePath = std::string(shaderSourceRootDir + stageDesc.fileName);

		const EShaderStage stage = ShaderUtils::GetShaderTypeFromSourceFilePath(sourceFilePath);

		// USE SHADER CACHE
		//
		const size_t ShaderHash = ShaderUtils::GeneratePreprocessorDefinitionsHash(stageDesc.macros);
		const std::string cacheFileName = stageDesc.macros.empty()
			? DirectoryUtil::GetFileNameFromPath(sourceFilePath) + SHADER_BINARY_EXTENSION
			: DirectoryUtil::GetFileNameFromPath(sourceFilePath) + "_" + std::to_string(ShaderHash) + SHADER_BINARY_EXTENSION;
		const std::string cacheFilePath = Application::GetDirectory(Application::EDirectories::SHADER_BINARY_CACHE) + "\\" + cacheFileName;
		const bool bUseCachedShaders      =  DirectoryUtil::FileExists(cacheFilePath) && !ShaderUtils::IsCacheDirty(sourceFilePath, cacheFilePath);
		const bool bNoShaderFileAvailable = !DirectoryUtil::FileExists(cacheFilePath) && !DirectoryUtil::FileExists(sourceFilePath);
		//---------------------------------------------------------------------------------
		if (bNoShaderFileAvailable)
		{
			Log::Error("No shader file (%s) found for shader: %s"
				, bUseCachedShaders ? cacheFilePath.c_str()	: sourceFilePath.c_str()
				, mName.c_str());
			return false;
		}
		if (!bPrinted)	// quick status print here
		{
			const char* pMsgLoad = bUseCachedShaders 
				? "Loading cached shader binaries" 
				: "Compiling shader from source";
			Log::Info("\t%s %s...", pMsgLoad, mName.c_str());
			bPrinted = true;
		}
		{
			std::string pShaderStageMsg = s_ShaderStrTypeLookup.at(stage) + "_" + 
				(bUseCachedShaders ? cacheFilePath: sourceFilePath);
			Log::Info("\t%s", pShaderStageMsg.c_str());
		}
		//---------------------------------------------------------------------------------
		if (bUseCachedShaders)
		{
			blobs.of[stage] = ShaderUtils::CompileFromCachedBinary(cacheFilePath);
		}
		else
		{
			std::string errMsg;
			ID3D10Blob* pBlob;
			if (ShaderUtils::CompileFromSource(sourceFilePath, stage, pBlob, errMsg, stageDesc.macros))
			{
				blobs.of[stage] = pBlob;
				ShaderUtils::CacheShaderBinary(cacheFilePath, blobs.of[stage]);
			}
			else
			{
				Log::Error(errMsg);
				return false;
			}
		}

		mDirectories[stage] = ShaderLoadDesc(sourceFilePath, cacheFilePath);
	}

	SetReflections(blobs   , reflections);
	SetInputLayouts(pDevice, reflections, /*mInputLayoutDesc*/ mVSInputElements);

	SetRootParameters(pDevice, reflections);



	// no longer release blobs, we're storing them to retrieve for PSO compilation later on
	constexpr bool bReleaseBlobs = false;
	if (bReleaseBlobs)
	{
		for (unsigned type = EShaderStage::VS; type < EShaderStage::COUNT; ++type)
		{
			if (blobs.of[type])
			{
				blobs.of[type]->Release();
				blobs.of[type] = nullptr;
			}
		}
	}

	return true;
}