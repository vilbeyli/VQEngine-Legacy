//	VQEngine | DirectX11 Renderer
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

#include "RenderingEnums.h"

#include <d3dcompiler.h>

#include <string>
#include <array>
#include <tuple>
#include <vector>
#include <stack>

constexpr size_t MAX_CONSTANT_BUFFERS = 512;

using CPUConstantID = int;
using GPU_ConstantBufferSlotIndex = int;
using ConstantBufferMapping = std::pair<GPU_ConstantBufferSlotIndex, CPUConstantID>;

//----------------------------------------------------------------------------------------------------------------
// PRIMITIVE STRUCTS
//----------------------------------------------------------------------------------------------------------------
struct CPUConstant
{
	using CPUConstantPool = std::array<CPUConstant, MAX_CONSTANT_BUFFERS>;
	using CPUConstantRefIDPair = std::tuple<CPUConstant&, CPUConstantID>;

	friend class Renderer;
	friend class Shader;

	inline static	CPUConstant&			Get(int id) { return s_constants[id]; }
	static			CPUConstantRefIDPair	GetNextAvailable();
	static			void					CleanUp();	// call once

	CPUConstant() : _name(), _size(0), _data(nullptr) {}
	std::string _name;
	size_t		_size;
	void*		_data;

private:
	static CPUConstantPool	s_constants;
	static size_t			s_nextConstIndex;

	inline bool operator==(const CPUConstant& c) const { return (((this->_data == c._data) && this->_size == c._size) && this->_name == c._name); }
	inline bool operator!=(const CPUConstant& c) const { return ((this->_data != c._data) || this->_size != c._size || this->_name != c._name); }
};
struct ConstantBuffer
{	// GPU side constant buffer
	EShaderType shdType;
	unsigned	bufferSlot;
	ID3D11Buffer* data;
	bool dirty;
};
struct ShaderTexture
{
	std::string name;
	unsigned bufferSlot;
	EShaderType shdType;
};
struct ShaderSampler
{
	std::string name;
	unsigned bufferSlot;
	EShaderType shdType;
};
struct InputLayout
{
	std::string		semanticName;
	ELayoutFormat	format;
};

class Shader
{
	friend class Renderer;

	using ShaderArray = std::array<ShaderID, EShaders::SHADER_COUNT>;

public:
	//----------------------------------------------------------------------------------------------------------------
	// STRUCTS/ENUMS
	//----------------------------------------------------------------------------------------------------------------
	// Current limitations for Constant Buffers: 
	//  todo: revise this
	//  - cbuffers with same names in different shaders (PS/VS/GS/...)
	//  - cbuffers with same names in the same shader (not tested)
	union ShaderBlobs
	{
		struct 
		{
			ID3D10Blob* vs;
			ID3D10Blob* gs;
			ID3D10Blob* ds;
			ID3D10Blob* hs;
			ID3D10Blob* cs;
			ID3D10Blob* ps;
		};
		ID3D10Blob* of[EShaderType::COUNT] = { nullptr };
	};
	union ShaderReflections
	{
		struct 
		{
			ID3D11ShaderReflection* vsRefl;
			ID3D11ShaderReflection* gsRefl;
			ID3D11ShaderReflection* dsRefl;
			ID3D11ShaderReflection* hsRefl;
			ID3D11ShaderReflection* csRefl;
			ID3D11ShaderReflection* psRefl;
		};
		ID3D11ShaderReflection* of[EShaderType::COUNT] = { nullptr };
	};
	struct ConstantBufferLayout
	{	// information used to create GPU/CPU constant buffers
		D3D11_SHADER_BUFFER_DESC					desc;
		std::vector<D3D11_SHADER_VARIABLE_DESC>		variables;
		std::vector<D3D11_SHADER_TYPE_DESC>			types;
		unsigned									buffSize;
		EShaderType									shdType;
		unsigned									bufSlot;
	};

public:
	//----------------------------------------------------------------------------------------------------------------
	// STATIC INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	static const ShaderArray&		Get() { return s_shaders; }
	static void						LoadShaders(Renderer* pRenderer);
	static std::stack<std::string>	UnloadShaders(Renderer* pRenderer);
	static bool						IsForwardPassShader(ShaderID shader);

public:
	//----------------------------------------------------------------------------------------------------------------
	// MEMBER INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	Shader(const std::string& shaderFileName);
	~Shader();

	void ClearConstantBuffers();
	void UpdateConstants(ID3D11DeviceContext* context);

	//----------------------------------------------------------------------------------------------------------------
	// GETTERS
	//----------------------------------------------------------------------------------------------------------------
	const std::string&							Name() const;
	ShaderID									ID() const;
	const std::vector<ConstantBufferLayout>&	GetConstantBufferLayouts() const;
	const std::vector<ConstantBuffer>&			GetConstantBuffers() const;

private:
	//----------------------------------------------------------------------------------------------------------------
	// STATIC PRIVATE INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	// Compiles shader from source file
	//
	static bool CompileFromSource(const std::string& pathToFile, const EShaderType& type, ID3D10Blob *& ref_pBob, std::string& errMsg);
	
	// Reads in cached binary from %APPDATA%/VQEngine/ShaderCache folder into ID3D10Blob 
	//
	static ID3D10Blob * CompileFromCachedBinary(const std::string& cachedBinaryFilePath);

	// Writes out compiled ID3D10Blob into %APPDATA%/VQEngine/ShaderCache folder
	//
	static void			CacheShaderBinary(const std::string& shaderCacheFileName, ID3D10Blob * pCompiledBinary);

	// example filePath: "rootPath/filename_vs.hlsl"
	//                                      ^^----- shaderTypeString
	static EShaderType	GetShaderTypeFromSourceFilePath(const std::string& shaderFilePath);

	//----------------------------------------------------------------------------------------------------------------
	// HELPER FUNCTIONS
	//----------------------------------------------------------------------------------------------------------------
	void RegisterConstantBufferLayout(ID3D11ShaderReflection * sRefl, EShaderType type);
	void CompileShaders(ID3D11Device* device, const std::vector<std::string>& filePaths, const std::vector<InputLayout>& layouts);
	void SetReflections(const ShaderBlobs& blobs);
	void CreateShader(ID3D11Device* pDevice, EShaderType type, void* pBuffer, const size_t szShaderBinary);
	void SetConstantBuffers(ID3D11Device* device);
	void CheckSignatures();
	

	void LogConstantBufferLayouts() const;

private:
	//----------------------------------------------------------------------------------------------------------------
	// DATA
	//----------------------------------------------------------------------------------------------------------------
	static ShaderArray s_shaders;
	static std::string s_shaderCacheDirectory;

	const std::string					m_name;
	std::vector<ConstantBuffer>			m_cBuffers;	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb509581(v=vs.85).aspx
	ShaderID							m_id;

	ID3D11VertexShader*					m_vertexShader;
	ID3D11PixelShader*					m_pixelShader;
	ID3D11GeometryShader*				m_geometryShader;
	ID3D11HullShader*					m_hullShader;
	ID3D11DomainShader*					m_domainShader;
	ID3D11ComputeShader*				m_computeShader;
	ShaderReflections					m_shaderReflections;	// shader reflections, temporary?

	ID3D11InputLayout*					m_layout;

	std::vector<ConstantBufferLayout>	m_CBLayouts;
	std::vector<ConstantBufferMapping>	m_constants;
	std::vector<ConstantBufferMapping>	m_constantsUnsorted;	// duplicate data...

	std::vector<ShaderTexture>			m_textures;
	std::vector<ShaderSampler>			m_samplers;
	
};