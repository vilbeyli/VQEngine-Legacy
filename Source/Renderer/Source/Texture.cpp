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

#include "Texture.h"
#include "Renderer.h"
#include "Log.h"

#include <d3d11.h>

Texture::Texture()
	:
	_srv(nullptr),	// assigned and deleted by renderer
	_tex2D(nullptr),
	_width(0),
	_height(0),
	_depth(0),

	_name(""),
	_id(-1)
{}

Texture::~Texture()
{}

bool Texture::InitializeTexture2D(const D3D11_TEXTURE2D_DESC& descriptor, Renderer* pRenderer, bool initializeSRV)
{
	HRESULT hr = pRenderer->m_device->CreateTexture2D(&descriptor, nullptr, &this->_tex2D);
	if (!SUCCEEDED(hr))
	{
		Log::Error("Texture::InitializeTexture2D(): Cannot create texture2D");
		return false;
	}
	this->_width = descriptor.Width;
	this->_height = descriptor.Height;
	
	if (initializeSRV)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = descriptor.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;	// array maybe? check descriptor.
		srvDesc.Texture2D.MipLevels = 1;
		pRenderer->m_device->CreateShaderResourceView(this->_tex2D, &srvDesc, &this->_srv);
	}
	return true;
}

void Texture::Release()
{
	if (_srv)
	{
		_srv->Release();
		_srv = nullptr;
	}

	if (_tex2D)
	{
		_tex2D->Release();
		_tex2D = nullptr;
	}
}
