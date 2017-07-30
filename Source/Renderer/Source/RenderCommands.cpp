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
//	&ID3D11DeviceContext::CSSetConstantBuffers,
//	&ID3D11DeviceContext::PSSetConstantBuffers
//};

static void(__cdecl ID3D11DeviceContext:: *SetShaderResources[6])
(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView *const *ppShaderResourceViews) =
{
	&ID3D11DeviceContext::VSSetShaderResources,
	&ID3D11DeviceContext::GSSetShaderResources,
	&ID3D11DeviceContext::DSSetShaderResources,
	&ID3D11DeviceContext::HSSetShaderResources,
	&ID3D11DeviceContext::CSSetShaderResources,
	&ID3D11DeviceContext::PSSetShaderResources
};

void SetTextureCommand::SetResource(Renderer * pRenderer)
{
	assert(texID >= 0);
	(pRenderer->m_deviceContext->*SetShaderResources[shdTex.shdType])(shdTex.bufferSlot, 1, &pRenderer->m_textures[texID]._srv);
}

static void(__cdecl ID3D11DeviceContext:: *SetSampler[6])
(UINT StartSlot, UINT NumViews, ID3D11SamplerState* const *ppShaderResourceViews) =
{
	&ID3D11DeviceContext::VSSetSamplers,
	&ID3D11DeviceContext::GSSetSamplers,
	&ID3D11DeviceContext::DSSetSamplers,
	&ID3D11DeviceContext::HSSetSamplers,
	&ID3D11DeviceContext::CSSetSamplers,
	&ID3D11DeviceContext::PSSetSamplers
};

void SetSamplerCommand::SetResource(Renderer * pRenderer)
{
	assert(samplerID >= 0);
	(pRenderer->m_deviceContext->*SetSampler[shdSampler.shdType])(shdSampler.bufferSlot, 1, &pRenderer->m_samplers[samplerID]._samplerState);
}
