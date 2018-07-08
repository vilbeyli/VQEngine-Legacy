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

#include <string>
#include <vector>
#include "RenderingEnums.h"

using TextureID = int;
using SamplerID = int;

class Renderer;
struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;
struct D3D11_TEXTURE2D_DESC;
struct ID3D11Texture3D;
struct ID3D11Texture2D;

struct TextureDesc
{
	int width;
	int height;
	EImageFormat format;
	ETextureUsage usage;
	std::string	 texFileName;
	void* pData;
	int dataPitch;		// used for 2D/3D Textures - how many bytes per row of pixels
	int dataSlicePitch;	// used for 3D Textures
	int mipCount;
	int arraySize;
	bool bIsCubeMap;
	bool bGenerateMips;

	TextureDesc() :
		width(1),
		height(1),
		format(RGBA32F),
		usage(RESOURCE),
		texFileName(""),
		pData(nullptr),
		dataSlicePitch(0),
		mipCount(1),
		arraySize(1),
		bIsCubeMap(false),
		bGenerateMips(false)
	{}

	D3D11_TEXTURE2D_DESC dxDesc;
};


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
	std::vector<ID3D11ShaderResourceView*> _srvArray;
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