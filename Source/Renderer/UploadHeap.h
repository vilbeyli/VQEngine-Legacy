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

//
// Adapted from AMD's Cauldron framework
// src: https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron/blob/master/src/DX12/base/UploadHeap.h
//

#if _WIN32

#include <windows.h>

struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12Fence;

class Renderer;

struct UploadHeap
{
public:
	void Initialize(ID3D12Device* pDevice, ID3D12CommandQueue* pCmdQueue,  SIZE_T uSize);
	void Exit();

	UINT8* Suballocate(SIZE_T uSize, UINT64 uAlign);

	UINT8* BasePtr() const { return mpDataBegin; }
	ID3D12Resource* GetResource() const { return mpUploadHeap; }
	ID3D12GraphicsCommandList* GetCommandList() const { return mpCommandList; }
	ID3D12CommandQueue* GetCommandQueue() const { return mpCommandQueue; }


	void FlushAndFinish(Renderer* pRenderer);

private:
	ID3D12Device*               mpDevice = nullptr;
	ID3D12Resource*             mpUploadHeap = nullptr;
	ID3D12GraphicsCommandList*  mpCommandList = nullptr;
	ID3D12CommandQueue*         mpCommandQueue = nullptr;
	ID3D12CommandAllocator*     mpCommandAllocator = nullptr;

	UINT8*                      mpDataCur = nullptr;      // current position of upload heap
	UINT8*                      mpDataEnd = nullptr;      // ending position of upload heap 
	UINT8*                      mpDataBegin = nullptr;    // starting position of upload heap
	ID3D12Fence*                mpFence = nullptr;
	UINT64                      mFenceValue = 0;
	HANDLE                      mhEvent;
};

#endif //_WIN32