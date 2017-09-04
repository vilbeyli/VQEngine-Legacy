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

#include "D3DManager.h"
#include "Log.h"

#include <string>

#define xFORCE_DEBUG

D3DManager::D3DManager()
{
	m_swapChain					= nullptr;
	m_device					= nullptr;
	m_deviceContext				= nullptr;
	m_wndHeight = m_wndWidth	= 0;
}

D3DManager::~D3DManager(){}
float	 D3DManager::AspectRatio() const { return static_cast<float>(m_wndWidth) / static_cast<float>(m_wndHeight); }
unsigned D3DManager::WindowWidth() const { return m_wndWidth; }
unsigned D3DManager::WindowHeight() const { return m_wndHeight; }


bool D3DManager::Initialize(int width, int height, const bool VSYNC, HWND hwnd, const bool FULL_SCREEN, DXGI_FORMAT FrameBufferFormat)
{
	m_hwnd = hwnd;

	HRESULT result;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned numModes;

	// Store the vsync setting.
	m_vsync_enabled = VSYNC;

	// Get System Information
	//----------------------------------------------------------------------------------

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	if (FAILED(result))
	{
		return false;
	}

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	if (FAILED(result))
	{
		return false;
	}

	// Enumerate the primary adapter output (monitor).
	result = adapter->EnumOutputs(0, &adapterOutput);
	if (FAILED(result))
	{
		return false;
	}

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if (FAILED(result))
	{
		return false;
	}

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	DXGI_MODE_DESC* displayModeList;
	displayModeList = new DXGI_MODE_DESC[numModes];
	if (!displayModeList)
	{
		return false;
	}

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	if (FAILED(result))
	{
		return false;
	}

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	unsigned numerator = 0;
	unsigned denominator = 0;
	for (unsigned i = 0; i<numModes; i++)
	{
		if (displayModeList[i].Width == (unsigned int)width)
		{
			if (displayModeList[i].Height == (unsigned int)height)
			{
				numerator = displayModeList[i].RefreshRate.Numerator;
				denominator = displayModeList[i].RefreshRate.Denominator;
			}
		}
	}

	if (numerator == 0 && denominator == 0)
	{
		char info[127];
		numerator	= displayModeList[numModes / 2].RefreshRate.Numerator;
		denominator = displayModeList[numModes / 2].RefreshRate.Denominator;
		sprintf_s(info, "Specified resolution (%ux%u) not found: Using (%ux%u) instead\n",
			width, height,
			displayModeList[numModes / 2].Width, displayModeList[numModes / 2].Height);
		width = displayModeList[numModes / 2].Width;
		height = displayModeList[numModes / 2].Height;
		OutputDebugString(info);

		// also resize window
		SetWindowPos(hwnd, 0, 10, 10, width, height, SWP_NOZORDER);
	}

	// Get the adapter (video card) description.
	DXGI_ADAPTER_DESC adapterDesc;
	result = adapter->GetDesc(&adapterDesc);
	if (FAILED(result))
	{
		return false;
	}

	// Store the dedicated video card memory in megabytes.
	m_VRAM = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
	size_t stringLength;
	int error;
	error = wcstombs_s(&stringLength, m_GPUDescription, 128, adapterDesc.Description, 128);
	if (error != 0)
	{
		return false;
	}

	// Release memory
	delete[] displayModeList;	displayModeList = 0;
	adapterOutput->Release();	adapterOutput = 0;
	adapter->Release();			adapter = 0;
	factory->Release();			factory = 0;

	if (!InitSwapChain(hwnd, FULL_SCREEN, width, height, numerator, denominator, DXGI_FORMAT_B8G8R8A8_UNORM))
	{
		return false;
	}

	m_wndWidth  = width;
	m_wndHeight = height;
	return true;
}

void D3DManager::Shutdown()
{	
	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, NULL);
	}

	if (m_deviceContext)
	{
		m_deviceContext->Release();
		m_deviceContext = nullptr;
	}

	if (m_device)
	{
		m_device->Release();
		m_device = nullptr;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}
#if _DEBUG
	if (m_annotation)
	{
		m_annotation->Release();
		m_annotation = nullptr;
	}
	
	if (m_debug)
	{
		m_debug->Release();
		m_debug = nullptr;
	}
#endif
	return;
}

void D3DManager::EndFrame()
{
	m_deviceContext->IASetInputLayout(NULL);
	m_deviceContext->VSSetShader(NULL, NULL, 0);
	m_deviceContext->GSSetShader(NULL, NULL, 0);
	m_deviceContext->HSSetShader(NULL, NULL, 0);
	m_deviceContext->DSSetShader(NULL, NULL, 0);
	m_deviceContext->PSSetShader(NULL, NULL, 0);
	m_deviceContext->CSSetShader(NULL, NULL, 0);
	if (m_vsync_enabled)		m_swapChain->Present(1, 0);
	else						m_swapChain->Present(0, 0);
}

void D3DManager::GetVideoCardInfo(char* cardName, int& memory)
{
	strcpy_s(cardName, 128, m_GPUDescription);
	memory = m_VRAM;
	return;
}

void D3DManager::ReportLiveObjects(const std::string& LogHeader /*= ""*/) const
{
#ifdef _DEBUG
	return;	// todo:
	if (!LogHeader.empty())
		Log::Info(LogHeader);

	HRESULT hr = m_device->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_debug);
	hr = m_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
#endif
}

//----------------------------------------------------------------------------------------------------------------------------------------
// private functions

bool D3DManager::InitSwapChain(HWND hwnd, bool fullscreen, int scrWidth, int scrHeight, unsigned numerator, unsigned denominator, DXGI_FORMAT FrameBufferFormat)
{
	HRESULT result;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1;
	swapChainDesc.BufferDesc.Width = scrWidth;
	swapChainDesc.BufferDesc.Height = scrHeight;

	// Set regular 32-bit surface for the back buffer.
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb173064(v=vs.85).aspx
	swapChainDesc.BufferDesc.Format = FrameBufferFormat;

	// Set the refresh rate of the back buffer.
	if (m_vsync_enabled)
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else
	{
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Windowed = !fullscreen;

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

#if defined( _DEBUG )
	UINT flags = D3D11_CREATE_DEVICE_DEBUG;

#else
	UINT flags = 0;
#endif

	// Create the swap chain, Direct3D device, and Direct3D device context.
	result = D3D11CreateDeviceAndSwapChain(NULL, 
											D3D_DRIVER_TYPE_HARDWARE, 
											NULL, 
											flags, 
											&featureLevel, 
											1,
											D3D11_SDK_VERSION, 
											&swapChainDesc, 
											&m_swapChain, 
											&m_device, NULL, &m_deviceContext);
	if (FAILED(result))
	{
		Log::Error("D3DManager: Cannot create swap chain");
		return false;
	}

#ifdef _DEBUG
	// Direct3D SDK Debug Layer Tricks
	//------------------------------------------------------------------------------------------
	// src1: https://blogs.msdn.microsoft.com/chuckw/2012/11/30/direct3d-sdk-debug-layer-tricks/
	// src2: http://seanmiddleditch.com/direct3d-11-debug-api-tricks/
	//------------------------------------------------------------------------------------------
	if ( SUCCEEDED(m_device->QueryInterface(__uuidof(ID3D11Debug), (void**)&m_debug)) )
	{
		ID3D11InfoQueue* d3dInfoQueue = nullptr;
		if (SUCCEEDED(m_debug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&d3dInfoQueue)))
		{
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
			d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
			D3D11_MESSAGE_ID hide[] =
			{
				D3D11_MESSAGE_ID_DEVICE_DRAW_RENDERTARGETVIEW_NOT_SET,
				// Add more message IDs here as needed
			};

			D3D11_INFO_QUEUE_FILTER filter;
			memset(&filter, 0, sizeof(filter));
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			d3dInfoQueue->AddStorageFilterEntries(&filter);
			d3dInfoQueue->Release();
		}
	}

	if ( FAILED(m_deviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_annotation)) )
	{
		Log::Error("Can't Query(ID3DUserDefinedAnnotation)");
		return false;
	}
#endif

	return true;
}
