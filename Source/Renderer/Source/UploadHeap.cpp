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

//
// Adapted from AMD's Cauldron framework
// src: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/master/src/DX12/base/UploadHeap.cpp
//

#include "UploadHeap.h"
#include "D3DUtils.h"

#include <d3d12.h>

#include "Renderer.h"

using namespace VQ_D3D12_UTILS;

void UploadHeap::Initialize(ID3D12Device* pDevice, ID3D12CommandQueue* pCmdQueue, SIZE_T uSize)
{
	mpDevice = pDevice;
	mpCommandQueue = pCmdQueue;

	// Create command list and allocators 

	pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mpCommandAllocator));
	pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mpCommandAllocator, nullptr, IID_PPV_ARGS(&mpCommandList));
	SetName(mpCommandAllocator, L"UploadHeap::m_pCommandAllocator");
	SetName(mpCommandList, L"UploadHeap::m_pCommandList");

	// Create buffer to suballocate

	D3D12_HEAP_PROPERTIES heapPropDesc = {};
	heapPropDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
	
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = uSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.SampleDesc = { 1, 0 };
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ThrowIfFailed(
		pDevice->CreateCommittedResource(
			&heapPropDesc,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mpUploadHeap)
		)
	);

	ThrowIfFailed(mpUploadHeap->Map(0, NULL, (void**)&mpDataBegin));

	mpDataCur = mpDataBegin;
	mpDataEnd = mpDataBegin + mpUploadHeap->GetDesc().Width;

	mFenceValue = 0;
}

void UploadHeap::Exit()
{
	mpUploadHeap->Release();

	mpCommandList->Release();
	mpCommandAllocator->Release();
}


UINT8* UploadHeap::Suballocate(SIZE_T uSize, UINT64 uAlign)
{
	mpDataCur = reinterpret_cast<UINT8*>(VQ_D3D12_UTILS::AlignOffset(reinterpret_cast<SIZE_T>(mpDataCur), SIZE_T(uAlign)));

	// return NULL if we ran out of space in the heap
	if (mpDataCur >= mpDataEnd || mpDataCur + uSize >= mpDataEnd)
	{
		return NULL;
	}

	UINT8* pRet = mpDataCur;
	mpDataCur += uSize;
	return pRet;
}


void UploadHeap::FlushAndFinish(Renderer* pRenderer)
{
	// Close & submit

	ThrowIfFailed(mpCommandList->Close());
	mpCommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const *>(&mpCommandList));

	// Make sure it's been processed by the GPU
	pRenderer->GPUFlush();

	// Reset so it can be reused
	mpCommandAllocator->Reset();
	mpCommandList->Reset(mpCommandAllocator, nullptr);

	mpDataCur = mpDataBegin;
}
