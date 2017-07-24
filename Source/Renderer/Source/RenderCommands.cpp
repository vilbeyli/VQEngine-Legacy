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

void TextureSetCommand::SetResource(Renderer * pRenderer)
{
	(pRenderer->m_deviceContext->*SetShaderResources[shdTex.shdType])(shdTex.bufferSlot, 1, &pRenderer->m_textures[texID].srv);
}
