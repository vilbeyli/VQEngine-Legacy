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

#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>

using namespace DirectX;


enum LayoutFormat
{
	FLOAT32_2 = DXGI_FORMAT_R32G32_FLOAT,
	FLOAT32_3 = DXGI_FORMAT_R32G32B32_FLOAT,

	LAYOUT_FORMAT_COUNT
};

struct InputLayout
{
	std::string		semanticName;
	LayoutFormat	format;
};

// CONSTANT BUFFER MANAGEMENT
//------------------------------------------------------------------
// Current limitations: 
//  - cbuffers with same names in different shaders (PS/VS/GS/...)
//  - cbuffers with same names in the same shader (not tested)

// used to map **SetShaderConstant(); function in Renderer::Apply()
enum ShaderType
{
	VS,
	PS,

	COUNT
};

// information used to create GPU/CPU cbuffers
struct cBufferLayout
{
	D3D11_SHADER_BUFFER_DESC					desc;
	std::vector<D3D11_SHADER_VARIABLE_DESC>		variables;
	std::vector<D3D11_SHADER_TYPE_DESC>			types;
	unsigned									buffSize;
	ShaderType									shdType;
	unsigned									bufSlot;
};


// CPU side constant buffer
struct CPUConstant
{
	std::string name;
	size_t		size;
	void*		data;
};

// GPU side constant buffer
struct CBuffer
{
	ShaderType shdType;
	unsigned	bufferSlot;
	ID3D11Buffer* data;
	bool dirty;
};
//-------------------------------------------------------------

// typedefs
typedef int ShaderID;

class Shader
{
	friend class Renderer;

public:
	Shader(const std::string& shaderFileName);
	~Shader();

	std::string	Name() const;
	ShaderID	ID() const;

private:
	void Compile(ID3D11Device* device, const std::string& shaderFileName, const std::vector<InputLayout>& layouts);
	void SetReflections(ID3D10Blob* vsBlob, ID3D10Blob* psBlob);
	void CheckSignatures();
	void SetCBuffers(ID3D11Device* device);
	void RegisterCBufferLayout(ID3D11ShaderReflection* sRefl, ShaderType type);
	
	void OutputShaderErrorMessage(ID3D10Blob* errorMessage, const CHAR* shaderFileName);
	void HandleCompileError(ID3D10Blob* errorMessage, const std::string& shdPath);
	void AssignID(ShaderID id);

private:
	const std::string			m_name;
	ShaderID					m_id;

	ID3D11VertexShader*			m_vertexShader;
	ID3D11PixelShader*			m_pixelShader;

	ID3D11ShaderReflection*		m_vsRefl;	// shader reflections, temporary?
	ID3D11ShaderReflection*		m_psRefl;	// shader reflections, temporary?

	ID3D11InputLayout*			m_layout;

	std::vector<CBuffer>					m_cBuffers;
	std::vector<std::vector<CPUConstant>>	m_constants;


	std::vector<cBufferLayout>	m_cBufferLayouts;

};

