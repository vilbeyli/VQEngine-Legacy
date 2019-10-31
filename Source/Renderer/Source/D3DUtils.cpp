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

#include "D3DUtils.h"

#include "Utilities/Log.h"

#include <cassert>

#include <dxgi.h>
#include <dxgi1_6.h>

//---------------------------------------------------------------------------------------------------------------------
// D3D12 UTILIY FUNCTIONS
//---------------------------------------------------------------------------------------------------------------------
namespace VQ_D3D12_UTILS
{
	void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			Log::Error("Win Call failed, HR=0x%08X", static_cast<UINT>(hr));
			assert(false);
		}
	}

	void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL D3D_FEATURE_LEVEL_TO_REQUEST)
	{
		IDXGIAdapter1* adapter;
		*ppAdapter = nullptr;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			ThrowIfFailed(adapter->GetDesc1(&desc));

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_TO_REQUEST, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}

		adapter->Release();
		*ppAdapter = nullptr;
	}

}

