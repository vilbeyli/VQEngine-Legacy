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

#include "Texture.h"
#include "Renderer.h"

#include "Utilities/Log.h"

#include <d3d11.h>

DirectX::XMMATRIX Texture::CubemapUtility::GetViewMatrix(Texture::CubemapUtility::ECubeMapLookDirections cubeFace, const vec3& position)
{
// cube face order: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
//------------------------------------------------------------------------------------------------------
// 0: RIGHT		1: LEFT
// 2: UP		3: DOWN
// 4: FRONT		5: BACK
//------------------------------------------------------------------------------------------------------
	switch (cubeFace)
	{
	case Texture::CubemapUtility::CUBEMAP_LOOK_RIGHT:	return XMMatrixLookAtLH(position, position + vec3::Right  , vec3::Up);
	case Texture::CubemapUtility::CUBEMAP_LOOK_LEFT:	return XMMatrixLookAtLH(position, position + vec3::Left   , vec3::Up);
	case Texture::CubemapUtility::CUBEMAP_LOOK_UP:		return XMMatrixLookAtLH(position, position + vec3::Up     , vec3::Back);
	case Texture::CubemapUtility::CUBEMAP_LOOK_DOWN:	return XMMatrixLookAtLH(position, position + vec3::Down   , vec3::Forward);
	case Texture::CubemapUtility::CUBEMAP_LOOK_FRONT:	return XMMatrixLookAtLH(position, position + vec3::Forward, vec3::Up);
	case Texture::CubemapUtility::CUBEMAP_LOOK_BACK:	return XMMatrixLookAtLH(position, position + vec3::Back   , vec3::Up);
	default: return XMMatrixIdentity(); 
	}
}


Texture::Texture()
	:
	_srv(nullptr),	// assigned and deleted by renderer
	_uav(nullptr),	// assigned and deleted by renderer
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
#if defined(_DEBUG) || defined(PROFILE)
	if (!this->_name.empty())
	{
		this->_tex2D->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(this->_name.length()), this->_name.c_str());
	}
#endif
	if (!SUCCEEDED(hr))
	{
		Log::Error("Texture::InitializeTexture2D(): Cannot create texture2D");
		return false;
	}
	this->_width = descriptor.Width;
	this->_height = descriptor.Height;
	
	if (initializeSRV)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = descriptor.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;	// array maybe? check descriptor.
		srvDesc.Texture2D.MipLevels = descriptor.MipLevels;
		pRenderer->m_device->CreateShaderResourceView(this->_tex2D, &srvDesc, &this->_srv);
#if defined(_DEBUG) || defined(PROFILE)
		if (!this->_name.empty())
		{
			this->_srv->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(this->_name.length()), this->_name.c_str());
		}
#endif
	}
	return true;
}

template<class T>
void ReleaseResources(std::vector<T*>& resources, T*& pResourceView)
{
	if (resources.size() > 0)
	{
		for (size_t i = 0; i < resources.size(); ++i)
		{
			if (resources[i])
			{
				resources[i]->Release();
				resources[i] = nullptr;
			}
		}
	}
	else
	{
		if (pResourceView)
		{
			pResourceView->Release();
			pResourceView = nullptr;
		}
	}
}

void Texture::Release()
{
	ReleaseResources(_srvArray, _srv);
	ReleaseResources(_uavArray, _uav);
	if (_tex2D)
	{
		_tex2D->Release();
		_tex2D = nullptr;
	}
	_width = _height = _depth = 0;
	_name = "";
	_id = -1;
}

EImageFormat Texture::GetImageFormat() const
{
	assert(false);	// TODO: implement
	return EImageFormat::D24UNORM_S8U;
}
