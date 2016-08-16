#include "Shader.h"
#include <d3dcompiler.h>
#include <fstream>
#include <sstream>

Shader::Shader(const std::string& shaderFileName)
	:
	m_vertexShader(nullptr),
	m_pixelShader(nullptr),
	m_layout(nullptr),
	m_matrixBuffer(nullptr),
	m_name(shaderFileName),
	m_id(-1)
{}

Shader::~Shader(void)
{
	// Release the matrix constant buffer.
	if (m_matrixBuffer)
	{
		m_matrixBuffer->Release();
		m_matrixBuffer = 0;
	}

	// Release the layout.
	if (m_layout)
	{
		m_layout->Release();
		m_layout = 0;
	}

	// Release the pixel shader.
	if (m_pixelShader)
	{
		m_pixelShader->Release();
		m_pixelShader = 0;
	}

	// Release the vertex shader.
	if (m_vertexShader)
	{
		m_vertexShader->Release();
		m_vertexShader = 0;
	}
}

void Shader::Compile(ID3D11Device* device, HWND hwnd, const std::string& shaderFileName)
{
	HRESULT result;
	ID3D10Blob* errorMessage = 0;

	// SHADER PATHS
	//----------------------------------------------------------------------------
	std::string vspath = shaderFileName + ".vs";
	std::string gspath = shaderFileName + ".gs";
	std::string pspath = shaderFileName + ".ps";
	std::wstring vspath_w(vspath.begin(), vspath.end());
	std::wstring gspath_w(gspath.begin(), gspath.end());
	std::wstring pspath_w(pspath.begin(), pspath.end());
	const WCHAR* VSPath = vspath_w.c_str();
	const WCHAR* GSPath = gspath_w.c_str();
	const WCHAR* PSPath = pspath_w.c_str();

	std::string info("\nCompiling shader "); info += m_name; info += "\n";
	OutputDebugString(info.c_str());

	// COMPILE SHADERS
	//----------------------------------------------------------------------------
	//compile vertex shader
	ID3D10Blob* vertexShaderBuffer = NULL;
	result = D3DCompileFromFile(VSPath, NULL, NULL, "VSMain", "vs_4_0",
								D3DCOMPILE_ENABLE_STRICTNESS,
								0, &vertexShaderBuffer, &errorMessage);
	if (FAILED(result))			HandleCompileError(errorMessage, vspath);
	

	//compile pixel shader
	ID3D10Blob* pixelShaderBuffer = NULL;
	result = D3DCompileFromFile(PSPath, NULL, NULL, "PSMain", "ps_4_0",
								D3DCOMPILE_ENABLE_STRICTNESS, 
								0, &pixelShaderBuffer, &errorMessage);
	if (FAILED(result))			HandleCompileError(errorMessage, pspath);

	// CREATER SHADER PROGRAMS
	//---------------------------------------------------------------------------
	//create vertex shader buffer
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), 
										vertexShaderBuffer->GetBufferSize(), 
										NULL, &m_vertexShader);
	if (FAILED(result))
	{
		OutputDebugString("Error creating vertex shader program");
		assert(false);
	}

	//create pixel shader buffer
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), 
										pixelShaderBuffer->GetBufferSize(), 
										NULL, &m_pixelShader);
	if (FAILED(result))
	{
		OutputDebugString("Error creating pixel shader program");
		assert(false);
	}

	// INPUT & BUFFER DESCRIPTIONS	
	//---------------------------------------------------------------------------
	//setup the layout of the data that goes into the shader
	D3D11_INPUT_ELEMENT_DESC polygonLayout[3];
	polygonLayout[0].SemanticName			= "POSITION";
	polygonLayout[0].SemanticIndex			= 0;
	polygonLayout[0].Format					= DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].InputSlot				= 0;
	polygonLayout[0].AlignedByteOffset		= 0;
	polygonLayout[0].InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].InstanceDataStepRate	= 0;

	polygonLayout[1].SemanticName			= "NORMAL";
	polygonLayout[1].SemanticIndex			= 0;
	polygonLayout[1].Format					= DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[1].InputSlot				= 0;
	polygonLayout[1].AlignedByteOffset		= 0;
	polygonLayout[1].InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].InstanceDataStepRate	= 0;

	polygonLayout[2].SemanticName			= "TEXCOORD";
	polygonLayout[2].SemanticIndex			= 0;
	polygonLayout[2].Format					= DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[2].InputSlot				= 0;
	polygonLayout[2].AlignedByteOffset		= 0;
	polygonLayout[2].InputSlotClass			= D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].InstanceDataStepRate	= 0;

	unsigned numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	//create the vertex input layout
	result = device->CreateInputLayout(polygonLayout, numElements, 
										vertexShaderBuffer->GetBufferPointer(),
										vertexShaderBuffer->GetBufferSize(), &m_layout);
	if (FAILED(result))
	{
		OutputDebugString("Error creating input layout");
		assert(false);
	}

	//release shader buffers
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = 0;

	//setup the matrix buffer description
	D3D11_BUFFER_DESC matrixBufferDesc;
	matrixBufferDesc.Usage					= D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth				= sizeof(MatrixBuffer);
	matrixBufferDesc.BindFlags				= D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags			= D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags				= 0;
	matrixBufferDesc.StructureByteStride	= 0;

	//create the constant buffer pointer
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
	if (FAILED(result))
	{
		OutputDebugString("Error creating constant buffer");
		assert(false);
	}

}

void Shader::OutputShaderErrorMessage(ID3D10Blob* errorMessage, const CHAR* shaderFileName)
{
	char* compileErrors = (char*)errorMessage->GetBufferPointer();
	unsigned long bufferSize = errorMessage->GetBufferSize();

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
