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

#include "RenderCommands.h"
#include "Renderer.h"



// store XSSetShaderResources function pointers in array and index through ShaderType enum
//static void(__cdecl ID3D11DeviceContext:: *SetShaderConstants[6])
//(UINT StartSlot, UINT NumBuffers, ID3D11Buffer *const *ppConstantBuffers) =
//{
//	&ID3D11DeviceContext::VSSetConstantBuffers,
//	&ID3D11DeviceContext::GSSetConstantBuffers,
//	&ID3D11DeviceContext::DSSetConstantBuffers,
//	&ID3D11DeviceContext::HSSetConstantBuffers,
//	&ID3D11DeviceContext::PSSetConstantBuffers,
//	&ID3D11DeviceContext::CSSetConstantBuffers,
//};

#ifdef _WIN64
#define CALLING_CONVENTION __cdecl
#else	// _WIN32
#define CALLING_CONVENTION __stdcall
#endif
static void(CALLING_CONVENTION ID3D11DeviceContext:: *SetShaderResources[EShaderStage::COUNT])
(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) =
{
	&ID3D11DeviceContext::VSSetShaderResources,
	&ID3D11DeviceContext::GSSetShaderResources,
	&ID3D11DeviceContext::DSSetShaderResources,
	&ID3D11DeviceContext::HSSetShaderResources,
	&ID3D11DeviceContext::PSSetShaderResources,
	&ID3D11DeviceContext::CSSetShaderResources,
};

#if 0 // experimental (there's code duplication that can use some template programming)
template<class T>
void SetResource(SetTextureCommand& cmd, Renderer* pRenderer, ID3D11DeviceContext* pContext)
{
	constexpr UINT NUM_RESOURCE_VIEWS = 1;

	if (cmd.numTextures == 1)
	{
		const TextureID textureID = cmd.textureIDs[0];
		assert(textureID >= 0);
		const bool bUseArray = pRenderer->mTextures[textureID]._uavArray.size() != 0;
		ID3D11UnorderedAccessView** ppUAVs = !bUseArray
			? &pRenderer->mTextures[textureID]._uav
			: &pRenderer->mTextures[textureID]._uavArray[slice];
		pContext->CSSetUnorderedAccessViews(binding.textureSlot, NUM_UAVs, ppUAVs, nullptr);
	}
	else
	{
		std::vector< ID3D11UnorderedAccessView* > pUAVs = [&]()
		{
			std::vector< ID3D11UnorderedAccessView* > pUAVs(numTextures);
			for (unsigned i = 0; i < numTextures; ++i)
			{
				const TextureID textureID = textureIDs[i];
				assert(textureID >= 0);
				pUAVs[i] = pRenderer->mTextures[textureID]._uav;
			}
			return pUAVs;
		}();
		pContext->CSSetUnorderedAccessViews(binding.textureSlot, NUM_UAVs, pUAVs.data(), nullptr);
	}
}
#endif

void SetTextureCommand::SetResource(Renderer * pRenderer)
{
	auto* pContext = pRenderer->m_deviceContext;
	auto* pDevice = pRenderer->m_device;
	if (bUnorderedAccess)
	{
		constexpr UINT NUM_UAVs = 1;
		
		// UAV
		//
		if (numTextures == 1)	
		{	
			const TextureID textureID = textureIDs[0];
			assert(textureID >= 0);
			const bool bUseArray = pRenderer->mTextures[textureID]._uavArray.size() != 0;
			ID3D11UnorderedAccessView** ppUAVs = !bUseArray
				? &pRenderer->mTextures[textureID]._uav
				: &pRenderer->mTextures[textureID]._uavArray[slice];
			pContext->CSSetUnorderedAccessViews(binding.textureSlot, NUM_UAVs, ppUAVs, nullptr);
		}

		// UAV ARRAY
		//
		else
		{
			assert(false);
			std::vector< ID3D11UnorderedAccessView* > pUAVs = [&]()
			{
				std::vector< ID3D11UnorderedAccessView* > pUAVs(numTextures);
				for (unsigned i = 0; i < numTextures; ++i)
				{
					const TextureID textureID = textureIDs[i];
					assert(textureID >= 0);
					pUAVs[i] = pRenderer->mTextures[textureID]._uav;
				}
				return pUAVs;
			}();
			pContext->CSSetUnorderedAccessViews(binding.textureSlot, NUM_UAVs, pUAVs.data(), nullptr);
		}
	}
	else
	{
		constexpr UINT NUM_SRVs = 1;

		// SRV
		//
		if (numTextures == 1)
		{
			const TextureID textureID = textureIDs[0];
			assert(textureID >= 0);
			const bool bUseArray = pRenderer->mTextures[textureID]._srvArray.size() != 0;
			ID3D11ShaderResourceView** ppSRVs = !bUseArray
				? &pRenderer->mTextures[textureID]._srv
				: &pRenderer->mTextures[textureID]._srvArray[slice];
			(pContext->*SetShaderResources[binding.shaderStage])(binding.textureSlot, NUM_SRVs, ppSRVs);
		}

		// SRV ARRAY
		//
		else
		{
			assert(false);
			std::vector< ID3D11ShaderResourceView* > pSRVs = [&]() 
			{
				std::vector< ID3D11ShaderResourceView* > pSRVs(numTextures);
				for (unsigned i = 0; i < numTextures; ++i)
				{
					const TextureID textureID = textureIDs[i];
					assert(textureID >= 0);
					pSRVs[i] = pRenderer->mTextures[textureID]._srv;
				}
				return pSRVs;
			}();
			(pContext->*SetShaderResources[binding.shaderStage])(binding.textureSlot, numTextures, pSRVs.data());
		}
	}
}

static void(CALLING_CONVENTION ID3D11DeviceContext:: *SetSampler[EShaderStage::COUNT])
(UINT StartSlot, UINT NumViews, ID3D11SamplerState* const *ppShaderResourceViews) =
{
	&ID3D11DeviceContext::VSSetSamplers,
	&ID3D11DeviceContext::GSSetSamplers,
	&ID3D11DeviceContext::DSSetSamplers,
	&ID3D11DeviceContext::HSSetSamplers,
	&ID3D11DeviceContext::PSSetSamplers,
	&ID3D11DeviceContext::CSSetSamplers,
};

void SetSamplerCommand::SetResource(Renderer * pRenderer)
{
	assert(samplerID >= 0);
	(pRenderer->m_deviceContext->*SetSampler[binding.shaderStage])(binding.samplerSlot, 1, &pRenderer->mSamplers[samplerID]._samplerState);
}

ClearCommand ClearCommand::Depth(float depthClearValue)
{
	const bool bDoClearColor = false;
	const bool bDoClearDepth = true;
	const bool bDoClearStencil = false;
	return ClearCommand (
		bDoClearColor, bDoClearDepth, bDoClearStencil,
		{ 0, 0, 0, 0 }, depthClearValue, 0
	);
}

ClearCommand ClearCommand::Color(const std::array<float, 4>& clearColorValues)
{
	const bool bDoClearColor = true;
	const bool bDoClearDepth = false;
	const bool bDoClearStencil = false;
	return ClearCommand(
		bDoClearColor, bDoClearDepth, bDoClearStencil,
		clearColorValues, 0, 0
	);
}

ClearCommand ClearCommand::Color(float r, float g, float b, float a)
{
	return ClearCommand::Color({ r,g,b,a });
}

