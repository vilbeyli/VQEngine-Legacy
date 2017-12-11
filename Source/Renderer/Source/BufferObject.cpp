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

#include "BufferObject.h"

#include <d3d11_1.h>

#include "Utilities/Log.h"

Buffer::Buffer(const BufferDesc& desc)
	: 
	mDesc(desc),
	mData(nullptr),
	mDirty(true),
	mCPUData(nullptr)
{}

void Buffer::Initialize(ID3D11Device* device, const void* data /*=nullptr*/)
{
	D3D11_BUFFER_DESC bufDesc;
	bufDesc.Usage = static_cast<D3D11_USAGE>(mDesc.mUsage);
	bufDesc.BindFlags = static_cast<D3D11_BIND_FLAG>(mDesc.mType);
	bufDesc.ByteWidth = mDesc.mStride * mDesc.mElementCount;
	bufDesc.CPUAccessFlags = mDesc.mUsage == EBufferUsage::DYNAMIC ? D3D11_CPU_ACCESS_WRITE : 0;	// dynamic r/w?
	bufDesc.MiscFlags = 0;
	bufDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA* pBufData = nullptr;
	D3D11_SUBRESOURCE_DATA bufData = {};
	if (data)
	{
		bufData.pSysMem = data;
		bufData.SysMemPitch = 0;		// irrelevant for non-texture sub-resources
		bufData.SysMemSlicePitch = 0;	// irrelevant for non-texture sub-resources
		pBufData = &bufData;
	}

	int hr = device->CreateBuffer(&bufDesc, pBufData, &this->mData);
	if (FAILED(hr))
	{
		Log::Error("Failed to create vertex buffer!");
	}
}

void Buffer::CleanUp()
{
	if (mData)
	{
		mData->Release();
		mData = nullptr;
	}

	if (mCPUData)
	{
		// todo:
	}
}

void Buffer::Update()
{
	// todo
}