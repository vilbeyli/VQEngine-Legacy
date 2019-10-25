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


#include <d3dcompiler.h>
#include <d3d12.h>
#include <unordered_map>


//----------------------------------------------------------------------------------------------------------------
// SHADER DATA/RESOURCE INTERFACE STRUCTS
//----------------------------------------------------------------------------------------------------------------

class Shader
{
	friend class Renderer;
	using ShaderArray           = std::array<ShaderID, EShaders::SHADER_COUNT>;
	using ShaderTextureLookup   = std::unordered_map<std::string, int>;
	using ShaderSamplerLookup   = std::unordered_map<std::string, int>;
	using ShaderDirectoryLookup = std::unordered_map<EShaderStage, ShaderLoadDesc>;

public:
	using ByteCodeType          = D3D12_SHADER_BYTECODE;
	using InputLayoutDescType   = D3D12_INPUT_LAYOUT_DESC;

	//
	// STRUCTS/ENUMS
	//
	union ShaderBlobs
	{
		struct
		{
			ID3DBlob* vs;
			ID3DBlob* gs;
			ID3DBlob* ds;
			ID3DBlob* hs;
			ID3DBlob* ps;
			ID3DBlob* cs;
		};
		ID3DBlob* of[EShaderStage::COUNT] = { nullptr };
	};
	union ShaderReflections
	{
		struct
		{
			ID3D12ShaderReflection* vsRefl;
			ID3D12ShaderReflection* gsRefl;
			ID3D12ShaderReflection* dsRefl;
			ID3D12ShaderReflection* hsRefl;
			ID3D12ShaderReflection* psRefl;
			ID3D12ShaderReflection* csRefl;
		};
		ID3D12ShaderReflection* of[EShaderStage::COUNT] = { nullptr };
	};


public:
	//----------------------------------------------------------------------------------------------------------------
	// MEMBER INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	Shader(const ShaderDesc& desc);
	~Shader();

	bool CompileShaderStages(RHIDevice pDevice);
	bool HasSourceFileBeenUpdated() const;

	//----------------------------------------------------------------------------------------------------------------
	// GETTERS
	//----------------------------------------------------------------------------------------------------------------
	const std::string& Name() const { return mName; }
	inline ShaderID    ID()   const { return mID; }

	ByteCodeType		GetShaderByteCode(EShaderStage stage) const;
	InputLayoutDescType	GetShaderInputLayoutDesc() const;


private:
	//----------------------------------------------------------------------------------------------------------------
	// DATA
	//----------------------------------------------------------------------------------------------------------------
	ShaderID mID;

#if 0
	ShaderStages mStages;
	ID3D11InputLayout*	mpInputLayout = nullptr;
	std::vector<ConstantBufferBinding>	mConstantBuffers;	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb509581(v=vs.85).aspx
	std::vector<ConstantBufferLayout>  m_CBLayouts;
	std::vector<ConstantBufferMapping> m_constants;// currently redundant
	std::vector<CPUConstant> mCPUConstantBuffers;
	std::vector<TextureBinding> mTextureBindings;
	std::vector<SamplerBinding> mSamplerBindings;
	ShaderTextureLookup mShaderTextureLookup;
	ShaderSamplerLookup mShaderSamplerLookup;
#endif

	//
	// PSO Creation Data
	//
	///ShaderReflections	mReflections;
	///InputLayoutDescType mInputLayoutDesc; // populated by shader reflection
	std::vector<D3D12_INPUT_ELEMENT_DESC> mVSInputElements;
	ShaderBlobs mShaderStageByteCodes;
	///Renderer::RootSignature mpRootSignature;


	ShaderDesc mDescriptor;	// used for shader reloading
	ShaderDirectoryLookup mDirectories;
	std::string	mName;
};

