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

#pragma once

#include "RenderingEnums.h"

#include <d3dcompiler.h>

#include <string>
#include <array>
#include <tuple>
#include <vector>
#include <stack>
#include <unordered_map>

using CPUConstantID = int;
using GPU_ConstantBufferSlotIndex = int;
using ConstantBufferMapping = std::pair<GPU_ConstantBufferSlotIndex, CPUConstantID>;

//----------------------------------------------------------------------------------------------------------------
// SHADER DATA/RESOURCE INTERFACE STRUCTS
//----------------------------------------------------------------------------------------------------------------
struct CPUConstant
{
	using CPUConstantRefIDPair = std::tuple<CPUConstant&, CPUConstantID>;

	friend class Renderer;
	friend class Shader;

	CPUConstant() : _name(), _size(0), _data(nullptr) {}
	std::string _name;
	size_t		_size;
	void*		_data;

private:
	inline bool operator==(const CPUConstant& c) const { return (((this->_data == c._data) && this->_size == c._size) && this->_name == c._name); }
	inline bool operator!=(const CPUConstant& c) const { return ((this->_data != c._data) || this->_size != c._size || this->_name != c._name); }
};
struct ConstantBuffer	// GPU side constant buffer
{	
	EShaderStage shaderStage;
	unsigned bufferSlot;
	ID3D11Buffer* data;
	bool dirty;
};
struct ShaderTexture
{
	unsigned bufferSlot;
	EShaderStage shaderStage;
};
struct ShaderSampler
{
	std::string name;	// move this out
	unsigned bufferSlot;
	EShaderStage shaderStage;
};
struct InputLayout
{
	std::string		semanticName;
	ELayoutFormat	format;
};


//----------------------------------------------------------------------
// Note:
//
// If a ShaderMacro is added to the shader compilation after the shader 
// is cached and the engine is configured to use the shader cache, the
// engine will use the cached shader which will result in shaders not
// being compiled with the given macro values/definitions.
// 
// To avoid this, do one of the following:
//
// - turn shader caching off from EngineSettings.ini
// - delete the %AppData%\VQEngine\ShaderCache folder
//----------------------------------------------------------------------
struct ShaderMacro
{
	std::string name;
	std::string value;
};

struct ShaderStageDesc
{
	std::string fileName;
	std::vector<ShaderMacro> macros;
};

struct ShaderDesc
{
	using ShaderStageArr = std::array<ShaderStageDesc, EShaderStageFlags::SHADER_STAGE_COUNT>;
	static ShaderStageArr CreateStageDescsFromShaderName(const char* shaderName, unsigned flagStages);

	std::string shaderName;
	std::array<ShaderStageDesc, EShaderStage::COUNT> stages;
};




class Shader
{
	friend class Renderer;

	using ShaderArray = std::array<ShaderID, EShaders::SHADER_COUNT>;
	using ShaderTextureLookup = std::unordered_map<std::string, int>;
	using ShaderSamplerLookup = std::unordered_map<std::string, int>;
	using ShaderDirectoryLookup = std::unordered_map<EShaderStage, std::string>;

public:
	// STRUCTS/ENUMS
	//
	// Current limitations for Constant Buffers: 
	//  - cbuffers with same names in different shaders (PS/VS/GS/...)
	//  - cbuffers with same names in the same shader (not tested)
	//----------------------------------------------------------------------------------------------------------------
	union ShaderBlobs
	{
		struct 
		{
			ID3D10Blob* vs;
			ID3D10Blob* gs;
			ID3D10Blob* ds;
			ID3D10Blob* hs;
			ID3D10Blob* ps;
			ID3D10Blob* cs;
		};
		ID3D10Blob* of[EShaderStage::COUNT] = { nullptr };
	};
	union ShaderReflections
	{
		struct 
		{
			ID3D11ShaderReflection* vsRefl;
			ID3D11ShaderReflection* gsRefl;
			ID3D11ShaderReflection* dsRefl;
			ID3D11ShaderReflection* hsRefl;
			ID3D11ShaderReflection* psRefl;
			ID3D11ShaderReflection* csRefl;
		};
		ID3D11ShaderReflection* of[EShaderStage::COUNT] = { nullptr };
	};
	struct ConstantBufferLayout
	{	// information used to create GPU/CPU constant buffers
		D3D11_SHADER_BUFFER_DESC					desc;
		std::vector<D3D11_SHADER_VARIABLE_DESC>		variables;
		std::vector<D3D11_SHADER_TYPE_DESC>			types;
		unsigned									buffSize;
		EShaderStage								stage;
		unsigned									bufSlot;
	};
	struct ShaderStages
	{
		ID3D11VertexShader*    mVertexShader   = nullptr;
		ID3D11PixelShader*     mPixelShader    = nullptr;
		ID3D11GeometryShader*  mGeometryShader = nullptr;
		ID3D11HullShader*      mHullShader     = nullptr;
		ID3D11DomainShader*    mDomainShader   = nullptr;
		ID3D11ComputeShader*   mComputeShader  = nullptr;

		ShaderDirectoryLookup mDirectories;
	};

public:
	//----------------------------------------------------------------------------------------------------------------
	// MEMBER INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	Shader(const ShaderDesc& desc);
	Shader(const std::string& shaderFileName);
	~Shader();

	void ClearConstantBuffers();
	void UpdateConstants(ID3D11DeviceContext* context);

	//----------------------------------------------------------------------------------------------------------------
	// GETTERS
	//----------------------------------------------------------------------------------------------------------------
	const std::string& Name() const { return mName; }
	inline ShaderID    ID()   const { return mID; }
	const std::vector<ConstantBufferLayout>&	GetConstantBufferLayouts() const;
	const std::vector<ConstantBuffer>&			GetConstantBuffers() const;
	
	const ShaderTexture& GetTextureBinding(const std::string& textureName) const;
	const ShaderSampler& GetSamplerBinding(const std::string& samplerName) const;
	bool HasTextureBinding(const std::string& textureName) const;
	bool HasSamplerBinding(const std::string& samplerName) const;

private:
	//----------------------------------------------------------------------------------------------------------------
	// STATIC PRIVATE INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	// Compiles shader from source file with the given shader macros
	//
	static bool CompileFromSource(
		  const std::string&              pathToFile
		, const EShaderStage&             type
		, ID3D10Blob *&                   ref_pBob
		, std::string&                    outErrMsg
		, const std::vector<ShaderMacro>& macros);
	
	// Reads in cached binary from %APPDATA%/VQEngine/ShaderCache folder into ID3D10Blob 
	//
	static ID3D10Blob * CompileFromCachedBinary(const std::string& cachedBinaryFilePath);

	// Writes out compiled ID3D10Blob into %APPDATA%/VQEngine/ShaderCache folder
	//
	static void			CacheShaderBinary(const std::string& shaderCacheFileName, ID3D10Blob * pCompiledBinary);

	// example filePath: "rootPath/filename_vs.hlsl"
	//                                      ^^----- shaderTypeString
	static EShaderStage	GetShaderTypeFromSourceFilePath(const std::string& shaderFilePath);

	//----------------------------------------------------------------------------------------------------------------
	// UTILITY FUNCTIONS
	//----------------------------------------------------------------------------------------------------------------
	void ReflectConstantBufferLayouts(ID3D11ShaderReflection * sRefl, EShaderStage type);
	void CompileShaders(ID3D11Device* device, const ShaderDesc& desc);
	void SetReflections(const ShaderBlobs& blobs);
	void CreateShaderStage(ID3D11Device* pDevice, EShaderStage stage, void* pBuffer, const size_t szShaderBinary);
	void CreateConstantBuffers(ID3D11Device* device);
	void CheckSignatures();
	void LogConstantBufferLayouts() const;

private:
	//----------------------------------------------------------------------------------------------------------------
	// DATA
	//----------------------------------------------------------------------------------------------------------------
	ShaderID mID;
	ShaderStages mStages;

	ShaderReflections	mReflections;	// shader reflections, temporary?
	ID3D11InputLayout*	mpInputLayout = nullptr;

	std::string	mName;

	std::vector<ConstantBuffer>	mConstantBuffers;	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb509581(v=vs.85).aspx
	std::vector<ConstantBufferLayout>  m_CBLayouts;
	std::vector<ConstantBufferMapping> m_constants;// currently redundant
	std::vector<CPUConstant> mCPUConstantBuffers;

	std::vector<ShaderTexture> mTextureBindings;
	std::vector<ShaderSampler> mSamplerBindings;
	
	ShaderTextureLookup mShaderTextureLookup;
	ShaderSamplerLookup mShaderSamplerLookup;

	ShaderDesc mDescriptor;	// used for shader reloading
};