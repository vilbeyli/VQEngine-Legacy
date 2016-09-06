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
#include <fstream>
#include <sstream>

Shader::Shader(const std::string& shaderFileName)
	:
	m_vertexShader(nullptr),
	m_pixelShader(nullptr),
	m_layout(nullptr),
	m_name(shaderFileName),
	m_id(-1)
{}

Shader::~Shader(void)
{
	// release constants
	for (CBuffer& cbuf : m_cBuffers)
	{
		if (cbuf.data)
		{
			cbuf.data->Release();
			cbuf.data = nullptr;
		}
	}

	for (std::vector<CPUConstant>& consts : m_constants)
	{
		for (CPUConstant& c : consts)
		{
			if (c.data)
			{
				delete[] c.data;
				c.data = nullptr;
			}
		}
	}

	// Release the layout.
	if (m_layout)
	{
		m_layout->Release();
		m_layout = nullptr;
	}

	// release shaders
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

	if (m_vsRefl)
	{
		m_vsRefl->Release();
		m_vsRefl = nullptr;
	}

	if (m_psRefl)
	{
		m_psRefl->Release();
		m_psRefl = nullptr;
	}
}

void Shader::Compile(ID3D11Device* device, const std::string& shaderFileName, const std::vector<InputLayout>& layouts)
{
	HRESULT result;
	ID3D10Blob* errorMessage = nullptr;

	// SHADER PATHS
	//----------------------------------------------------------------------------
	std::string vspath = shaderFileName + "_vs.hlsl";
	std::string gspath = shaderFileName + "_gs.hlsl";
	std::string pspath = shaderFileName + "_ps.hlsl";
	std::wstring vspath_w(vspath.begin(), vspath.end());
	std::wstring gspath_w(gspath.begin(), gspath.end());
	std::wstring pspath_w(pspath.begin(), pspath.end());
	const WCHAR* VSPath = vspath_w.c_str();
	const WCHAR* GSPath = gspath_w.c_str();
	const WCHAR* PSPath = pspath_w.c_str();

	std::string info("\tCompiling  \""); info += m_name; info += "\"";
	OutputDebugString(info.c_str());

	// COMPILE SHADERS
	//----------------------------------------------------------------------------
	//compile vertex shader
	ID3D10Blob* vsBlob = NULL;
	if (FAILED(D3DCompileFromFile(
		VSPath,
		NULL,
		NULL,
		"VSMain",
		"vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&vsBlob,
		&errorMessage)))
	{
		HandleCompileError(errorMessage, vspath);
	}

	//compile pixel shader
	ID3D10Blob* psBlob = NULL;
	if (FAILED(D3DCompileFromFile(
		PSPath,
		NULL,
		NULL,
		"PSMain",
		"ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS,
		0,
		&psBlob,
		&errorMessage)))
	{
		HandleCompileError(errorMessage, pspath);
	}
	
	SetReflections(vsBlob, psBlob);
	CheckSignatures();


	// CREATE SHADER PROGRAMS
	//---------------------------------------------------------------------------
	//create vertex shader buffer
	result = device->CreateVertexShader(vsBlob->GetBufferPointer(), 
										vsBlob->GetBufferSize(), 
										NULL, 
										&m_vertexShader);
	if (FAILED(result))
	{
		OutputDebugString("Error creating vertex shader program");
		assert(false);
	}

	//create pixel shader buffer
	result = device->CreatePixelShader(psBlob->GetBufferPointer(), 
										psBlob->GetBufferSize(), 
										NULL, 
										&m_pixelShader);
	if (FAILED(result))
	{
		OutputDebugString("Error creating pixel shader program");
		assert(false);
	}


	// INPUT LAYOUT
	//---------------------------------------------------------------------------
	//setup the layout of the data that goes into the shader
	std::vector<D3D11_INPUT_ELEMENT_DESC> inputLayout(layouts.size());
	for (unsigned i = 0; i < layouts.size(); ++i)
	{
		inputLayout[i].SemanticName			= layouts[i].semanticName.c_str();
		inputLayout[i].SemanticIndex		= 0;	// TODO: process string for semantic index
		inputLayout[i].Format				= static_cast<DXGI_FORMAT>(layouts[i].format);
		inputLayout[i].InputSlot			= 0;
		inputLayout[i].AlignedByteOffset	= i == 0 ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
		inputLayout[i].InputSlotClass		= D3D11_INPUT_PER_VERTEX_DATA;
		inputLayout[i].InstanceDataStepRate	= 0;
	}
	result = device->CreateInputLayout(	inputLayout.data(), 
										static_cast<UINT>(inputLayout.size()), 
										vsBlob->GetBufferPointer(),
										vsBlob->GetBufferSize(), 
										&m_layout);
	if (FAILED(result))
	{
		OutputDebugString("Error creating input layout");
		assert(false);
	}


	// CBUFFERS & SSHADER RESOURCES
	//---------------------------------------------------------------------------
	SetCBuffers(device);

	ID3D11ShaderResourceView* diffuseMapSRV;
	result = 

	//release shader buffers
	vsBlob->Release();
	vsBlob = 0;

	psBlob->Release();
	psBlob = 0;
	OutputDebugString(" - Done.\n");
}

void Shader::SetReflections(ID3D10Blob* vsBlob, ID3D10Blob* psBlob)
{
	// Vertex Shader
	if (FAILED(D3DReflect(
		vsBlob->GetBufferPointer(),
		vsBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		(void**)&m_vsRefl)))
	{
		OutputDebugString("Error getting vertex shader reflection\n");
		assert(false);
	}

	// Pixel Shader
	if (FAILED(D3DReflect(
		psBlob->GetBufferPointer(),
		psBlob->GetBufferSize(),
		IID_ID3D11ShaderReflection,
		(void**)&m_psRefl)))
	{
		OutputDebugString("Error getting pixel shader reflection\n");
		assert(false);
	}

}

void Shader::CheckSignatures()
{
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
			// TODO: order matters, semantic slot doesnt. check order
			;
		}
	}
}

void Shader::SetCBuffers(ID3D11Device* device)
{
	// example: http://gamedev.stackexchange.com/a/62395/39920

	// OBTAIN CBUFFER LAYOUT INFORMATION
	//---------------------------------------------------------------------------------------
	RegisterCBufferLayout(m_vsRefl, ShaderType::VS);
	RegisterCBufferLayout(m_psRefl, ShaderType::PS);
	

	// CREATE CPU & GPU CONSTANT BUFFERS
	//---------------------------------------------------------------------------------------
	// CPU CBuffers
	for (const cBufferLayout& cbLayout : m_cBufferLayouts)
	{
		std::vector<CPUConstant> cpuBuffers;
		for (D3D11_SHADER_VARIABLE_DESC varDesc : cbLayout.variables)
		{
			CPUConstant c;
			c.name = varDesc.Name;
			c.size = varDesc.Size;
			c.data = new char[c.size];
			memset(c.data, 0, c.size);
			cpuBuffers.push_back(c);
		}
		m_constants.push_back(cpuBuffers);
	}

	// GPU CBuffer Description
	D3D11_BUFFER_DESC cBufferDesc;
	cBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cBufferDesc.MiscFlags = 0;
	cBufferDesc.StructureByteStride = 0;

	// GPU CBuffers
	for (const cBufferLayout& cbLayout : m_cBufferLayouts)
	{
		CBuffer cBuffer;
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

void Shader::RegisterCBufferLayout(ID3D11ShaderReflection* sRefl, ShaderType type)
{
	D3D11_SHADER_DESC desc;
	sRefl->GetDesc(&desc);

	unsigned bufSlot = 0;
	for (unsigned i = 0; i < desc.ConstantBuffers; ++i)
	{
		cBufferLayout bufferLayout;
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
		m_cBufferLayouts.push_back(bufferLayout);
	}
}

void Shader::VoidBuffers()
{
	for (CBuffer& cBuf : m_cBuffers)
	{
		cBuf.dirty = true;
	}
}

void Shader::OutputShaderErrorMessage(ID3D10Blob* errorMessage, const CHAR* shaderFileName)
{
	char* compileErrors = (char*)errorMessage->GetBufferPointer();
	size_t bufferSize = errorMessage->GetBufferSize();

	std::stringstream ss;
	for (unsigned int i = 0; i < bufferSize; ++i)
	{
		ss << compileErrors[i];
	}
	OutputDebugString(ss.str().c_str());

	//release
	errorMessage->Release();
	errorMessage = 0;
	return;
}

void Shader::HandleCompileError(ID3D10Blob* errorMessage, const std::string& shdPath)
{
	if (errorMessage)
	{
		OutputShaderErrorMessage(errorMessage, shdPath.c_str());
	}
	else
	{
		std::string err("Error: Can't find shader file: "); err += shdPath;
		OutputDebugString(err.c_str());
	}

	assert(false);
}

void Shader::AssignID(ShaderID id)
{
	m_id = id;
}

//bool Shader::SetShaderParameters(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* texture)
//{
//	deviceContext->PSSetShaderResources(0, 1, &texture);
//
//	return true;
//}

//bool Shader::SetShaderParameters(ID3D11DeviceContext* deviceContext, 
//								XMFLOAT4X4 worldMatrix,
//								XMFLOAT4X4 viewMatrix, 
//								XMFLOAT4X4 projectionMatrix)
//{
//	HRESULT result;
//	D3D11_MAPPED_SUBRESOURCE mappedResource;
//	unsigned int bufferNumber;
//
//	// Lock the constant buffer so it can be written to.
//	result = deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
//	if (FAILED(result))
//	{
//		return false;
//	}
//
//	// Get a pointer to the data in the constant buffer.
//	
//	MatrixBuffer* dataPtr;
//	dataPtr = (MatrixBuffer*)mappedResource.pData;
//
//	// Copy the matrices into the constant buffer.
//	dataPtr->mWorld = XMMatrixTranspose(XMLoadFloat4x4(&worldMatrix));
//	dataPtr->mView	= XMMatrixTranspose(XMLoadFloat4x4(&viewMatrix));
//	dataPtr->mProj	= XMMatrixTranspose(XMLoadFloat4x4(&projectionMatrix));
//
//	// Unlock the constant buffer.
//	deviceContext->Unmap(m_matrixBuffer, 0);
//
//	// Set the position of the constant buffer in the vertex shader.
//	bufferNumber = 0;
//
//	// Finally set the constant buffer in the vertex shader with the updated values.
//	deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);
//
//	return true;
//}

std::string Shader::Name() const
{
	return m_name;
}

ShaderID Shader::ID() const
{
	return m_id;
}
