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

#include "RenderPasses.h"

#include "Light.h"

void DepthShadowPass::Initialize(Renderer* pRenderer, ID3D11Device* device)
{
	this->_shadowMapDimension = 1024;

	// check feature support & error handle:
	// https://msdn.microsoft.com/en-us/library/windows/apps/dn263150

	// create shadow map texture for the pixel shader stage
	D3D11_TEXTURE2D_DESC shadowMapDesc;
	ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
	shadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	shadowMapDesc.MipLevels = 1;
	shadowMapDesc.ArraySize = 1;
	shadowMapDesc.SampleDesc.Count = 1;
	shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	shadowMapDesc.Height = static_cast<UINT>(_shadowMapDimension);
	shadowMapDesc.Width = static_cast<UINT>(_shadowMapDimension);
	this->_shadowMap = pRenderer->CreateTexture2D(shadowMapDesc, false);

	// careful: removing const qualified from texture. rethink this
	Texture& shadowMap = const_cast<Texture&>(pRenderer->GetTextureObject(_shadowMap));

	// depth stencil view and shader resource view for the shadow map (^ BindFlags)
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;			// check format
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	this->_dsv = pRenderer->AddDepthStencil(dsvDesc, shadowMap._tex2D);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.Texture2D.MipLevels = 1;

	HRESULT hr = device->CreateShaderResourceView(shadowMap._tex2D, &srvDesc, &shadowMap._srv);
	if (FAILED(hr))
	{
		Log::Error(ERROR_LOG::CANT_CREATE_RESOURCE, "Cannot create SRV in InitilizeDepthPass\n");

	}

	// comparison sampler
	D3D11_SAMPLER_DESC comparisonSamplerDesc;
	ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
	comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.BorderColor[0] = 1.0f;
	comparisonSamplerDesc.BorderColor[1] = 1.0f;
	comparisonSamplerDesc.BorderColor[2] = 1.0f;
	comparisonSamplerDesc.BorderColor[3] = 1.0f;
	comparisonSamplerDesc.MinLOD = 0.f;
	comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	comparisonSamplerDesc.MipLODBias = 0.f;
	comparisonSamplerDesc.MaxAnisotropy = 0;
	//comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;	// todo: set default sampler elsewhere
	comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	//comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	comparisonSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	this->_shadowSampler = pRenderer->CreateSamplerState(comparisonSamplerDesc);

	// render states for front face culling 
	this->_shadowRenderState = pRenderer->AddRSState(RS_CULL_MODE::FRONT, RS_FILL_MODE::SOLID, true);
	this->_drawRenderState = pRenderer->AddRSState(RS_CULL_MODE::BACK, RS_FILL_MODE::SOLID, true);

	// shader
	std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 } };
	this->_shadowShader = pRenderer->GetShader(pRenderer->AddShader("DepthShader", pRenderer->s_shaderRoot, layout, false));

	ZeroMemory(&_shadowViewport, sizeof(D3D11_VIEWPORT));
	_shadowViewport.Height = static_cast<float>(_shadowMapDimension);
	_shadowViewport.Width = static_cast<float>(_shadowMapDimension);
	_shadowViewport.MinDepth = 0.f;
	_shadowViewport.MaxDepth = 1.f;
}

void DepthShadowPass::RenderDepth(Renderer* pRenderer, const std::vector<const Light*> shadowLights, const std::vector<GameObject*> ZPassObjects) const
{
	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	pRenderer->UnbindRenderTarget();						// no render target
	pRenderer->BindDepthStencil(_dsv);						// only depth stencil buffer
	pRenderer->SetRasterizerState(_shadowRenderState);		// shadow render state: cull front faces, fill solid, clip dep
	pRenderer->SetViewport(_shadowViewport);				// lights viewport 512x512
	pRenderer->SetShader(_shadowShader->ID());				// shader for rendering z buffer
	pRenderer->SetConstant4x4f("viewProj", shadowLights.front()->GetLightSpaceMatrix());
	pRenderer->SetConstant4x4f("view", shadowLights.front()->GetViewMatrix());
	pRenderer->SetConstant4x4f("proj", shadowLights.front()->GetProjectionMatrix());
	pRenderer->Apply();
	pRenderer->Begin(clearColor, 1.0f);
	size_t idx = 0;
	for (const GameObject* obj : ZPassObjects)
	{
		obj->RenderZ(pRenderer);
	}
}

void PostProcessPass::Initialize(Renderer * pRenderer, ID3D11Device * device)
{
	DXGI_SAMPLE_DESC smpDesc;
	smpDesc.Count = 1;
	smpDesc.Quality = 0;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D11_TEXTURE2D_DESC rtDesc;
	ZeroMemory(&rtDesc, sizeof(rtDesc));
	rtDesc.Width = pRenderer->WindowWidth();
	rtDesc.Height = pRenderer->WindowHeight();
	rtDesc.MipLevels = 1;
	rtDesc.ArraySize = 1;
	rtDesc.Format = format;
	rtDesc.Usage = D3D11_USAGE_DEFAULT;
	rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtDesc.CPUAccessFlags = 0;
	rtDesc.SampleDesc = smpDesc;
	rtDesc.MiscFlags = 0;

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	ZeroMemory(&RTVDesc, sizeof(RTVDesc));
	RTVDesc.Format = format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;

	this->_finalRenderTarget = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._colorRT = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._brightRT = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._blurPingPong[0] = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._blurPingPong[1] = pRenderer->AddRenderTarget(rtDesc, RTVDesc);

	D3D11_SAMPLER_DESC blurSamplerDesc;
	ZeroMemory(&blurSamplerDesc, sizeof(blurSamplerDesc));
	blurSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	blurSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	blurSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	blurSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	this->_bloomPass._blurSampler = pRenderer->CreateSamplerState(blurSamplerDesc);
}

void PostProcessPass::Render(Renderer * pRenderer) const
{
	const TextureID worldTexture = pRenderer->GetDefaultRenderTargetTexture();

	// ======================================================================================
	// BLOOM  PASS
	// ======================================================================================

	// bright filter
	pRenderer->SetShader(SHADERS::BLOOM);
	pRenderer->BindRenderTargets(_bloomPass._colorRT, _bloomPass._brightRT);
	pRenderer->UnbindDepthStencil();
	pRenderer->SetBufferObj(GEOMETRY::QUAD);
	pRenderer->Apply();
	pRenderer->SetTexture("worldRenderTarget", worldTexture);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	constexpr size_t BLUR_PASS_COUNT = 10;
	const TextureID brightTexture = pRenderer->GetRenderTargetTexture(_bloomPass._brightRT);

	// blur
	pRenderer->SetShader(SHADERS::BLUR);
	for (size_t i = 0; i < BLUR_PASS_COUNT; i++)
	{
		const int isHorizontal = i % 2;
		const TextureID pingPong = pRenderer->GetRenderTargetTexture(_bloomPass._blurPingPong[1 - isHorizontal]);
		const int texWidth = pRenderer->GetTextureObject(pingPong)._width;
		const int texHeight = pRenderer->GetTextureObject(pingPong)._height;

		pRenderer->UnbindRenderTarget();
		pRenderer->Apply();
		pRenderer->BindRenderTarget(_bloomPass._blurPingPong[isHorizontal]);
		pRenderer->SetConstant1i("isHorizontal", 1 - isHorizontal);
		pRenderer->SetConstant1i("textureWidth", texWidth);
		pRenderer->SetConstant1i("textureHeight", texHeight);
		pRenderer->SetTexture("InputTexture", i == 0 ? brightTexture : pingPong);
		pRenderer->SetSamplerState("BlurSampler", _bloomPass._blurSampler);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
	}

	// additive blend combine
	const TextureID colorTex = pRenderer->GetRenderTargetTexture(_bloomPass._colorRT);
	const TextureID bloomTex = pRenderer->GetRenderTargetTexture(_bloomPass._blurPingPong[0]);
	pRenderer->SetShader(SHADERS::BLOOM_COMBINE);
	pRenderer->BindRenderTarget(_finalRenderTarget);
	pRenderer->Apply();
	pRenderer->SetConstant1f("exposure", 1.0f);
	pRenderer->SetTexture("ColorTexture", colorTex);
	pRenderer->SetTexture("BloomTexture", bloomTex);
	pRenderer->SetSamplerState("BlurSampler", _bloomPass._blurSampler);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	// ======================================================================================
	// BLOOM  PASS END
	// ======================================================================================
}
