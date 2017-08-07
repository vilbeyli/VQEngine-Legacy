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

#include <string>
#include <d3d11.h>

using TextureID = int;
using SamplerID = int;

class Renderer;

struct Texture
{
public:
	Texture();
	~Texture();

	bool InitializeTexture2D(const D3D11_TEXTURE2D_DESC& descriptor, Renderer* pRenderer, bool initializeSRV);
	void Release();

	// shader resource view does 2 things
	// - tell d3d how the resource will be used: at what stage if the pipeline it will be bound etc.
	// - tell if the resource format was specified as typeless at creation time, then we must specify
	//   the type right before binding by reinterpret casting.
	// note: typed resources are optimized for access. use typeless if you really need. (src?)
	ID3D11ShaderResourceView*	_srv;

	union
	{
		ID3D11Texture3D*		_tex3D;
		ID3D11Texture2D*		_tex2D;
	};

	unsigned					_width;
	unsigned					_height;
	unsigned					_depth;

	std::string					_name;
	TextureID					_id;
};

struct Sampler
{
	ID3D11SamplerState*		_samplerState;
	std::string				_name;
	SamplerID				_id;
};