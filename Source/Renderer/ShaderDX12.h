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

	using ByteCodeType          = D3D12_SHADER_BYTECODE;
	using InputLayoutType       = D3D12_INPUT_LAYOUT_DESC;

	//
	// STRUCTS/ENUMS
	//
public:
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

	ByteCodeType GetShaderByteCode(EShaderStage stage) const;
	InputLayoutType GetShaderInputLayoutType() const;


private:
	//----------------------------------------------------------------------------------------------------------------
	// UTILITY FUNCTIONS
	//----------------------------------------------------------------------------------------------------------------


private:
	//----------------------------------------------------------------------------------------------------------------
	// DATA
	//----------------------------------------------------------------------------------------------------------------
	ShaderID mID;

#if 0
	ShaderStages mStages;

	ShaderReflections	mReflections;	// shader reflections, temporary?
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


	ShaderDesc mDescriptor;	// used for shader reloading
	ShaderDirectoryLookup mDirectories;
	std::string	mName;
};

