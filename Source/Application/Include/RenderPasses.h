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

#include "Renderer.h"
#include <array>

struct DepthShadowPass
{
	void Initialize(Renderer* pRenderer, ID3D11Device* device, const Settings::ShadowMap& shadowMapSettings);
	void RenderDepth(Renderer* pRenderer, const std::vector<const Light*> shadowLights, const std::vector<GameObject*> ZPassObjects) const;
	
	unsigned				_shadowMapDimension;
	TextureID				_shadowMap;
	SamplerID				_shadowSampler;
	const Shader*			_shadowShader;
	RasterizerStateID		_drawRenderState;
	RasterizerStateID		_shadowRenderState;
	D3D11_VIEWPORT			_shadowViewport;
	DepthStencilID			_dsv;
};

struct BloomPass
{
	BloomPass() : _blurSampler(-1), _colorRT(-1), _brightRT(-1), _blurPingPong({ -1, -1 }), _isEnabled(true) {}
	inline void ToggleBloomPass() { _isEnabled = !_isEnabled; }

	SamplerID						_blurSampler;
	RenderTargetID					_colorRT;
	RenderTargetID					_brightRT;
	RenderTargetID					_finalRT;
	std::array<RenderTargetID, 2>	_blurPingPong;
	bool							_isEnabled;
};

struct TonemappingCombinePass
{
	RenderTargetID						_finalRenderTarget;
};

struct PostProcessPass
{
	void Initialize(Renderer* pRenderer, ID3D11Device* device, const Settings::PostProcess& postProcessSettings);
	void Render(Renderer* pRenderer) const;

	RenderTargetID				_worldRenderTarget;
	BloomPass					_bloomPass;
	TonemappingCombinePass		_tonemappingPass;
	Settings::PostProcess		_settings;
};
