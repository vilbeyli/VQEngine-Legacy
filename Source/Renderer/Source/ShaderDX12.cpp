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

Shader::ByteCodeType Shader::GetShaderByteCode(EShaderStage stage) const
{
	assert(false);
	return ByteCodeType();
}

Shader::InputLayoutType Shader::GetShaderInputLayoutType() const
{
	Shader::InputLayoutType inputLayout = {};
	// TODO: populate inputLayout
	assert(false);
	return inputLayout;
}


bool Shader::CompileShaderStages(RHIDevice pDevice)
{
	constexpr const char * SHADER_BINARY_EXTENSION = ".bin";
	HRESULT result;
	ShaderBlobs blobs;
	bool bPrinted = false;

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
		const bool bUseCachedShaders =
			DirectoryUtil::FileExists(cacheFilePath)
			&& !ShaderUtils::IsCacheDirty(sourceFilePath, cacheFilePath);
		//---------------------------------------------------------------------------------
		if (!bPrinted)	// quick status print here
		{
			const char* pMsgLoad = bUseCachedShaders ? "Loading cached shader binaries" : "Compiling shader from source";
			Log::Info("\t%s %s...", pMsgLoad, mName.c_str());
			bPrinted = true;
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

		// TODO:
		//CreateShaderStage(pDevice, stage, blobs.of[stage]->GetBufferPointer(), blobs.of[stage]->GetBufferSize());
		//SetReflections(blobs);
		//CheckSignatures();

		mDirectories[stage] = ShaderLoadDesc(sourceFilePath, cacheFilePath);
	}

#if 0
	// INPUT LAYOUT (VS)
	//---------------------------------------------------------------------------
	// src: https://stackoverflow.com/questions/42388979/directx-11-vertex-shader-reflection
	// setup the layout of the data that goes into the shader
	//
	if (mReflections.vsRefl)
	{

		D3D11_SHADER_DESC shaderDesc = {};
		mReflections.vsRefl->GetDesc(&shaderDesc);
		std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout(shaderDesc.InputParameters);

		D3D_PRIMITIVE primitiveDesc = shaderDesc.InputPrimitive;

		for (unsigned i = 0; i < shaderDesc.InputParameters; ++i)
		{
			D3D11_SIGNATURE_PARAMETER_DESC paramDesc;
			mReflections.vsRefl->GetInputParameterDesc(i, &paramDesc);

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

			inputLayout[i] = elementDesc; //save element desc
		}

		// Try to create Input Layout
		const auto* pData = inputLayout.data();
		if (pData)
		{
			result = pDevice->CreateInputLayout(
				pData,
				shaderDesc.InputParameters,
				blobs.vs->GetBufferPointer(),
				blobs.vs->GetBufferSize(),
				&mpInputLayout);

			if (FAILED(result))
			{
				OutputDebugString("Error creating input layout");
				return false;
			}
		}
	}

	// CONSTANT BUFFERS 
	//---------------------------------------------------------------------------
	// Obtain cbuffer layout information
	for (EShaderStage type = EShaderStage::VS; type < EShaderStage::COUNT; type = (EShaderStage)(type + 1))
	{
		if (mReflections.of[type])
		{
			ReflectConstantBufferLayouts(mReflections.of[type], type);
		}
	}

	// Create CPU & GPU constant buffers
	// ...

	// TEXTURES & SAMPLERS
	//---------------------------------------------------------------------------
	for (int shaderStage = 0; shaderStage < EShaderStage::COUNT; ++shaderStage)
	{
		unsigned texSlot = 0;	unsigned smpSlot = 0;
		unsigned uavSlot = 0;
		auto& sRefl = mReflections.of[shaderStage];
		if (sRefl)
		{
			D3D11_SHADER_DESC desc = {};
			sRefl->GetDesc(&desc);

			for (unsigned i = 0; i < desc.BoundResources; ++i)
			{
				D3D11_SHADER_INPUT_BIND_DESC shdInpDesc;
				sRefl->GetResourceBindingDesc(i, &shdInpDesc);

				switch (shdInpDesc.Type)
				{
				case D3D_SIT_SAMPLER:
				{
					SamplerBinding smp;
					smp.shaderStage = static_cast<EShaderStage>(shaderStage);
					smp.samplerSlot = smpSlot++;
					mSamplerBindings.push_back(smp);
					mShaderSamplerLookup[shdInpDesc.Name] = static_cast<int>(mSamplerBindings.size() - 1);
				} break;

				case D3D_SIT_TEXTURE:
				{
					TextureBinding tex;
					tex.shaderStage = static_cast<EShaderStage>(shaderStage);
					tex.textureSlot = texSlot++;
					mTextureBindings.push_back(tex);
					mShaderTextureLookup[shdInpDesc.Name] = static_cast<int>(mTextureBindings.size() - 1);
				} break;

				case D3D_SIT_UAV_RWTYPED:
				{
					TextureBinding tex;
					tex.shaderStage = static_cast<EShaderStage>(shaderStage);
					tex.textureSlot = uavSlot++;
					mTextureBindings.push_back(tex);
					mShaderTextureLookup[shdInpDesc.Name] = static_cast<int>(mTextureBindings.size() - 1);
				} break;

				case D3D_SIT_CBUFFER: break;


				default:
					Log::Warning("Unhandled shader input bind type in shader reflection");
					break;

				} // switch shader input type
			} // bound resource
		} // sRefl
	} // shaderStage
#endif
	// release blobs
	for (unsigned type = EShaderStage::VS; type < EShaderStage::COUNT; ++type)
	{
		if (blobs.of[type])
			blobs.of[type]->Release();
	}

	return true;
}