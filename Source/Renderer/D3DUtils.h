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

#include <windows.h>
#include <d3d12.h>

struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12RootSignature;
struct ID3D12PipelineState;

struct IDXGIFactory2;
struct IDXGIAdapter1;

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define FAIL(msg) do { \
        assert(0 && msg); \
        throw std::runtime_error(msg); \
    } while(false)

#define CHECK_BOOL(expr)  do { if(!(expr)) FAIL(__FILE__ "(" LINE_STRING "): !( " #expr " )"); } while(false)
#define CHECK_HR(expr)  do { if(FAILED(expr)) FAIL(__FILE__ "(" LINE_STRING "): FAILED( " #expr " )"); } while(false)

//----------------------------------------------------------------------------------------------------------------
// Adapted from Microsoft's open source D3D12 Sample repo: https://github.com/microsoft/DirectX-Graphics-Samples
// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name) { pObject->SetName(name); }
#else
struct ID3D12Object;
inline void SetName(ID3D12Object* pObject, LPCWSTR name) {}
#endif
#define NAME_D3D12_OBJECT_COM(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT(x) SetName(x, L#x)
//----------------------------------------------------------------------------------------------------------------

namespace VQ_D3D12_UTILS
{
	void ThrowIfFailed(HRESULT hr);
	
	// align uLocation to the next multiple of uAlign
	inline SIZE_T AlignOffset(SIZE_T uOffset, SIZE_T uAlign) { return ((uOffset + (uAlign - 1)) & ~(uAlign - 1)); }
}
