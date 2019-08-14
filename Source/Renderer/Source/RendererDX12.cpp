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

#include "Renderer.h"

#if USE_DX12

#include <d3d12.h>
#include <dxgi.h>		// IDXGIAdapter1
#include <dxgi1_6.h>

#include "Utilities/Log.h"

#include "Engine/Mesh.h"

#pragma comment(lib, "d3d12.lib")		// contains all the Direct3D functionality
#pragma comment(lib, "dxgi.lib")		// tools to interface with the hardware

//----------------------------------------------------------------------------------------------------------------
// Adapted from Microsoft's open source D3D12 Sample repo: https://github.com/microsoft/DirectX-Graphics-Samples
// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name){ pObject->SetName(name); }
#else
inline void SetName(ID3D12Object* pObject, LPCWSTR name) {}
#endif
#define NAME_D3D12_OBJECT_COM(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT(x) SetName(x, L#x)
//----------------------------------------------------------------------------------------------------------------


Renderer::Renderer(){}
Renderer::~Renderer() {}


bool Renderer::Initialize(HWND hwnd, const Settings::Window& settings, const Settings::Rendering& rendererSettings)
{
	Mesh::spRenderer = this;

	// SETTINGS
	//
	mWindowSettings = settings;
	const bool& bAntiAliasing = rendererSettings.antiAliasing.IsAAEnabled();
	// assuming SSAA is the only supported AA technique. otherwise, 
	// swapchain initialization should be further customized.
	this->mAntiAliasing = rendererSettings.antiAliasing;

	// RENDERING API
	//
	bool bRenderingAPIInitialized = InitializeRenderingAPI(hwnd);

	return true;
}

void Renderer::Exit()
{
	mCmdQueue_GFX.ptr->Release();
	mCmdQueue_Compute.ptr->Release();
	mCmdQueue_Copy.ptr->Release();

	mSwapChain.ptr->Release();
	mDevice.ptr->Release();
}

//---------------------------------------------------------------------------------------------------------------------
// D3D12 UTILIY FUNCTIONS
//---------------------------------------------------------------------------------------------------------------------
static inline void ThrowIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		Log::Error("Win Call failed, HR=0x%08X", static_cast<UINT>(hr));
		assert(false);
	}
}
// Helper function for acquiring the first available hardware adapter that supports Direct3D 12.
// If no such adapter can be found, *ppAdapter will be set to nullptr.
static void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
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
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	adapter->Release();
	*ppAdapter = nullptr;
}


// Adapted from Microsoft's open source D3D12 Sample repo
// source: https://github.com/microsoft/DirectX-Graphics-Samples
bool Renderer::InitializeRenderingAPI(HWND hwnd)
{
	constexpr unsigned NUM_FRAMES = 3;
	constexpr bool bTearingSupport = true;

	ID3D12Device*& pDevice = mDevice.ptr; // shorthand


	// CREATE FACTORY & DEVICE
	//
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	ID3D12Debug* pDebugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController))))
	{
		pDebugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	pDebugController->Release();
#endif
	IDXGIAdapter1* pHardwareAdapter;
	
	IDXGIFactory4* pFactory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pFactory)));
	
	GetHardwareAdapter(pFactory, &pHardwareAdapter);
	if ( FAILED(D3D12CreateDevice(pHardwareAdapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice.ptr))) )
	{
		; // TODO: Log
		return false;
	}


	// CREATE COMMAND QUEUES
	//
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH; // currently only 1 priority is supported, might iterate on this later.

	// Graphics
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue_GFX.ptr)));
	NAME_D3D12_OBJECT(mCmdQueue_GFX.ptr);

	// Compute
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue_Compute.ptr)));
	NAME_D3D12_OBJECT(mCmdQueue_Compute.ptr);

	// Copy
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCmdQueue_Copy.ptr)));
	NAME_D3D12_OBJECT(mCmdQueue_Copy.ptr);


	// SWAP CHAIN
	//
	// The resolution of the swap chain buffers will match the resolution of the window, enabling the
	// app to enter iFlip when in fullscreen mode. We will also keep a separate buffer that is not part
	// of the swap chain as an intermediate render target, whose resolution will control the rendering
	// resolution of the scene.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = NUM_FRAMES;
	swapChainDesc.Width  = mAntiAliasing.resolutionX;
	swapChainDesc.Height = mAntiAliasing.resolutionY;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // BGRA ?
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1; // MSAA
	swapChainDesc.Flags = bTearingSupport ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	IDXGISwapChain1* pSwapChain;
	ThrowIfFailed(pFactory->CreateSwapChainForHwnd(
		mCmdQueue_GFX.ptr,  // Swap chain needs the queue so that it can force a flush on it.
		hwnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&pSwapChain
	));

	if (bTearingSupport)
	{
		// When tearing support is enabled we will handle ALT+Enter key presses in the
		// window message loop rather than let DXGI handle it by calling SetFullscreenState.
		pFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
	}

	ThrowIfFailed(pSwapChain->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&mSwapChain.ptr));
	pSwapChain->Release();
	pFactory->Release();

	mCurrentFrameIndex = mSwapChain.ptr->GetCurrentBackBufferIndex();


	// RESOURCES: RTV, CBV, SRV, UAV HEAPS
	//
#if 0
	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = NUM_FRAMES + 1; // + 1 for the intermediate render target.
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(pDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));


		// Describe and create a constant buffer view (CBV) and shader resource view (SRV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
		cbvSrvHeapDesc.NumDescriptors = NUM_FRAMES + 1; // One CBV per frame and one SRV for the intermediate render target.
		cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(pDevice->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));

		m_rtvDescriptorSize    = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_cbvSrvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	// Create command allocators for each frame.
	for (UINT n = 0; n < NUM_FRAMES; n++)
	{
		ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_sceneCommandAllocators[n])));
		ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_postCommandAllocators[n])));
	}
#endif

	return true;
}

// called from a worker.
bool Renderer::LoadDefaultResources()
{
	ID3D12Device*& pDevice = mDevice.ptr; // shorthand


	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version - If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Create a root signature consisting of a descriptor table with a single CBV.
#if 0
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];

		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		// Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_sceneRootSignature)));
		NAME_D3D12_OBJECT(m_sceneRootSignature);
	}
#endif

	// Create a root signature consisting of a descriptor table with a SRV and a sampler.
#if 0
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];

		// We don't modify the SRV in the post-processing command list after
		// SetGraphicsRootDescriptorTable is executed on the GPU so we can use the default
		// range behavior: D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

		// Allow input layout and pixel shader access and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		// Create a sampler.
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error));
		ThrowIfFailed(pDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_postRootSignature)));
		NAME_D3D12_OBJECT(m_postRootSignature);
	}

#endif

	// Create the pipeline state, which includes compiling and loading shaders.
#if 0
	{
		ComPtr<ID3DBlob> sceneVertexShader;
		ComPtr<ID3DBlob> scenePixelShader;
		ComPtr<ID3DBlob> postVertexShader;
		ComPtr<ID3DBlob> postPixelShader;
		ComPtr<ID3DBlob> error;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"sceneShaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &sceneVertexShader, &error));
		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"sceneShaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &scenePixelShader, &error));

		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"postShaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &postVertexShader, &error));
		ThrowIfFailed(D3DCompileFromFile(GetAssetFullPath(L"postShaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &postPixelShader, &error));

		// Define the vertex input layouts.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		D3D12_INPUT_ELEMENT_DESC scaleInputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Describe and create the graphics pipeline state objects (PSOs).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_sceneRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(sceneVertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(scenePixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_scenePipelineState)));
		NAME_D3D12_OBJECT(m_scenePipelineState);

		psoDesc.InputLayout = { scaleInputElementDescs, _countof(scaleInputElementDescs) };
		psoDesc.pRootSignature = m_postRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(postVertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(postPixelShader.Get());

		ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_postPipelineState)));
		NAME_D3D12_OBJECT(m_postPipelineState);
	}

	// Single-use command allocator and command list for creating resources.
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList> commandList;

	ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
	ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	// Create the command lists.
	{
		ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_sceneCommandAllocators[m_frameIndex].Get(), m_scenePipelineState.Get(), IID_PPV_ARGS(&m_sceneCommandList)));
		NAME_D3D12_OBJECT(m_sceneCommandList);

		ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_postCommandAllocators[m_frameIndex].Get(), m_postPipelineState.Get(), IID_PPV_ARGS(&m_postCommandList)));
		NAME_D3D12_OBJECT(m_postCommandList);

		// Close the command lists.
		ThrowIfFailed(m_sceneCommandList->Close());
		ThrowIfFailed(m_postCommandList->Close());
	}

	LoadSizeDependentResources();
	LoadSceneResolutionDependentResources();

	// Create/update the vertex buffer.
	ComPtr<ID3D12Resource> sceneVertexBufferUpload;
	{
		// Define the geometry for a thin quad that will animate across the screen.
		const float x = QuadWidth / 2.0f;
		const float y = QuadHeight / 2.0f;
		SceneVertex quadVertices[] =
		{
			{ { -x, -y, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
			{ { -x, y, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
			{ { x, -y, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
			{ { x, y, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(quadVertices);

		ThrowIfFailed(pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_sceneVertexBuffer)));

		ThrowIfFailed(pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&sceneVertexBufferUpload)));

		NAME_D3D12_OBJECT(m_sceneVertexBuffer);

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(sceneVertexBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, quadVertices, sizeof(quadVertices));
		sceneVertexBufferUpload->Unmap(0, nullptr);

		commandList->CopyBufferRegion(m_sceneVertexBuffer.Get(), 0, sceneVertexBufferUpload.Get(), 0, vertexBufferSize);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_sceneVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer views.
		m_sceneVertexBufferView.BufferLocation = m_sceneVertexBuffer->GetGPUVirtualAddress();
		m_sceneVertexBufferView.StrideInBytes = sizeof(SceneVertex);
		m_sceneVertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create/update the fullscreen quad vertex buffer.
	ComPtr<ID3D12Resource> postVertexBufferUpload;
	{
		// Define the geometry for a fullscreen quad.
		PostVertex quadVertices[] =
		{
			{ { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },    // Bottom left.
			{ { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },    // Top left.
			{ { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },    // Bottom right.
			{ { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }        // Top right.
		};

		const UINT vertexBufferSize = sizeof(quadVertices);

		ThrowIfFailed(pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_postVertexBuffer)));

		ThrowIfFailed(pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&postVertexBufferUpload)));

		NAME_D3D12_OBJECT(m_postVertexBuffer);

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(postVertexBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, quadVertices, sizeof(quadVertices));
		postVertexBufferUpload->Unmap(0, nullptr);

		commandList->CopyBufferRegion(m_postVertexBuffer.Get(), 0, postVertexBufferUpload.Get(), 0, vertexBufferSize);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_postVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

		// Initialize the vertex buffer views.
		m_postVertexBufferView.BufferLocation = m_postVertexBuffer->GetGPUVirtualAddress();
		m_postVertexBufferView.StrideInBytes = sizeof(PostVertex);
		m_postVertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create the constant buffer.
	{
		ThrowIfFailed(pDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneConstantBuffer) * FrameCount),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_sceneConstantBuffer)));

		NAME_D3D12_OBJECT(m_sceneConstantBuffer);

		// Describe and create constant buffer views.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_sceneConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = sizeof(SceneConstantBuffer);

		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);

		for (UINT n = 0; n < FrameCount; n++)
		{
			pDevice->CreateConstantBufferView(&cbvDesc, cpuHandle);

			cbvDesc.BufferLocation += sizeof(SceneConstantBuffer);
			cpuHandle.Offset(m_cbvSrvDescriptorSize);
		}

		// Map and initialize the constant buffer. We don't unmap this until the
		// app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_sceneConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
		memcpy(m_pCbvDataBegin, &m_sceneConstantBufferData, sizeof(m_sceneConstantBufferData));
	}

	// Close the resource creation command list and execute it to begin the vertex buffer copy into
	// the default heap.
	ThrowIfFailed(commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Create synchronization objects and wait until assets have been uploaded to the GPU.
	{
		ThrowIfFailed(pDevice->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValues[m_frameIndex]++;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute before continuing.
		WaitForGpu();
	}
#endif

	return true;
}

void Renderer::ReloadShaders()
{
	int reloadedShaderCount = 0;
	std::vector<std::string> reloadedShaderNames;
	for (Shader* pShader : mShaders)
	{
		if (pShader->HasSourceFileBeenUpdated())
		{
			const bool bLoadSuccess = false;/// pShader->Reload(m_device);
			if (!bLoadSuccess)
			{
				//Log::Error("");
				assert(false); // todo;remove
				continue;
			}

			++reloadedShaderCount;
			reloadedShaderNames.push_back(pShader->Name());
		}
	}

	if (reloadedShaderCount == 0)
	{
		Log::Info("No updates have been made to shader source files: no shaders have been loaded");
	}
	else
	{
		Log::Info("Reloaded %d Shaders:", reloadedShaderCount);
		for (const std::string& name : reloadedShaderNames)
			Log::Info("\t%s", name.c_str());
	}
}

float	 Renderer::AspectRatio()	const { return 0; }//m_Direct3D->AspectRatio(); };
unsigned Renderer::WindowHeight()	const { return 0; }//m_Direct3D->WindowHeight(); };
unsigned Renderer::WindowWidth()	const { return 0; }//m_Direct3D->WindowWidth(); }
unsigned Renderer::FrameRenderTargetHeight() const { return mAntiAliasing.resolutionX; }
unsigned Renderer::FrameRenderTargetWidth()	 const { return mAntiAliasing.resolutionY; }
vec2	 Renderer::FrameRenderTargetDimensionsAsFloat2() const { return vec2(this->FrameRenderTargetWidth(), this->FrameRenderTargetHeight()); }
vec2	 Renderer::GetWindowDimensionsAsFloat2() const { return vec2(this->WindowWidth(), this->WindowHeight()); }
///HWND	 Renderer::GetWindow()			const { return m_Direct3D->WindowHandle(); };


void Renderer::BeginFrame()
{
	mRenderStats = { 0, 0, 0 };
}

void Renderer::EndFrame()
{
	;// m_Direct3D->EndFrame();
}


const BufferDesc Renderer::GetBufferDesc(EBufferType bufferType, BufferID bufferID) const
{
	BufferDesc desc = {};

#if 0
	const std::vector<Buffer>& bufferContainer = [&]()
	{
		switch (bufferType)
		{
		case VERTEX_BUFFER:     return mVertexBuffers; break;
		case INDEX_BUFFER:      return mIndexBuffers; break;
		case COMPUTE_RW_BUFFER: return mUABuffers; break;
		default: assert(false); // specify a valid buffer type
		}
		return mVertexBuffers; // eliminate warning: not all control paths return
	}();
#endif

	// TODO:
	return desc;
	///assert(bufferContainer.size() > bufferID);
	///return bufferContainer[bufferID].mDesc;
}

#ifdef max
#undef max
#endif
BufferID Renderer::CreateBuffer(const BufferDesc & bufferDesc, const void* pData /*=nullptr*/, const char* pBufferName /*= nullptr*/)
{
	Buffer buffer(bufferDesc);
	// TODO-DX12:
	///buffer.Initialize(m_device, pData);
#if _DEBUG
	if (pBufferName)
	{
		// TODO-DX12:
		///m_Direct3D->SetDebugName(buffer.mpGPUData, pBufferName);
	}
#endif
	return static_cast<int>([&]() {
		switch (bufferDesc.mType)
		{
		case VERTEX_BUFFER:
			mVertexBuffers.push_back(buffer);
			return mVertexBuffers.size() - 1;
		case INDEX_BUFFER:
			mIndexBuffers.push_back(buffer);
			return mIndexBuffers.size() - 1;
		case COMPUTE_RW_BUFFER:
			mUABuffers.push_back(buffer);
			return mUABuffers.size() - 1;
		default:
			Log::Warning("Unknown Buffer Type");
			return std::numeric_limits<size_t>::max();
		}
	}());
}


TextureID Renderer::CreateTextureFromFile(const std::string& texFileName, const std::string& fileRoot /*= s_textureRoot*/, bool bGenerateMips /*= false*/)
{
	return -1;
}


TextureID Renderer::CreateTexture2D(const TextureDesc& texDesc)
{
	return -1;
#if 0
	Texture tex;
	tex._width = texDesc.width;
	tex._height = texDesc.height;
	tex._name = texDesc.texFileName;


	// check multi sampling quality level
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb173072(v=vs.85).aspx
	//UINT maxMultiSamplingQualityLevel = 0;
	//m_device->CheckMultisampleQualityLevels(, , &maxMultiSamplingQualityLevel);
	//---


	// Texture2D Resource
	UINT miscFlags = 0;
	miscFlags |= texDesc.bIsCubeMap ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
	miscFlags |= texDesc.bGenerateMips ? D3D11_RESOURCE_MISC_GENERATE_MIPS : 0;

	UINT arrSize = texDesc.arraySize;
	const bool bIsTextureArray = texDesc.arraySize > 1;
	arrSize = texDesc.bIsCubeMap ? 6 * arrSize : arrSize;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Format = (DXGI_FORMAT)texDesc.format;
	desc.Height = max(texDesc.height, 1);
	desc.Width = max(texDesc.width, 1);
	desc.ArraySize = arrSize;
	desc.MipLevels = texDesc.mipCount;
	desc.SampleDesc = { 1, 0 };
	desc.BindFlags = texDesc.usage;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = static_cast<D3D11_CPU_ACCESS_FLAG>(texDesc.cpuAccessMode);
	desc.MiscFlags = miscFlags;

	D3D11_SUBRESOURCE_DATA dataDesc = {};
	D3D11_SUBRESOURCE_DATA* pDataDesc = nullptr;
	if (texDesc.pData)
	{
		dataDesc.pSysMem = texDesc.pData;
		dataDesc.SysMemPitch = texDesc.dataPitch;
		dataDesc.SysMemSlicePitch = texDesc.dataSlicePitch;
		pDataDesc = &dataDesc;
	}
	m_device->CreateTexture2D(&desc, pDataDesc, &tex._tex2D);

#if defined(_DEBUG) || defined(PROFILE)
	if (!texDesc.texFileName.empty())
	{
		m_Direct3D->SetDebugName(tex._tex2D, texDesc.texFileName + "_Tex2D");
	}
#endif

	// Shader Resource View
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = (DXGI_FORMAT)texDesc.format;
	switch (texDesc.format)
	{
		// caution: if initializing for depth texture, and the depth texture
		//			has stencil defined (d24s8), we have to check for 
		//			DXGI_FORMAT_R24_UNORM_X8_TYPELESS vs R32F
	case EImageFormat::R24G8:
		srvDesc.Format = (DXGI_FORMAT)EImageFormat::R24_UNORM_X8_TYPELESS;
		break;
	case EImageFormat::R32:
		srvDesc.Format = (DXGI_FORMAT)EImageFormat::R32F;
		break;
	}

	if (texDesc.bIsCubeMap)
	{
		if (bIsTextureArray)
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
			srvDesc.TextureCubeArray.NumCubes = arrSize / 6;
			srvDesc.TextureCubeArray.MipLevels = texDesc.mipCount;
			srvDesc.TextureCubeArray.MostDetailedMip = 0;
			srvDesc.TextureCubeArray.First2DArrayFace = 0;
			m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srv);
		}
		else
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
			srvDesc.TextureCube.MipLevels = texDesc.mipCount;
			srvDesc.TextureCube.MostDetailedMip = 0;
			m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srv);
		}
#if _DEBUG
		if (!texDesc.texFileName.empty())
		{
			m_Direct3D->SetDebugName(tex._srv, texDesc.texFileName + "_SRV");
		}
#endif
	}
	else
	{
		if (bIsTextureArray)
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MipLevels = texDesc.mipCount;
			srvDesc.Texture2DArray.MostDetailedMip = 0;

			tex._srvArray.resize(desc.ArraySize, nullptr);
			tex._depth = desc.ArraySize;
			for (unsigned i = 0; i < desc.ArraySize; ++i)
			{
				srvDesc.Texture2DArray.FirstArraySlice = i;
				srvDesc.Texture2DArray.ArraySize = desc.ArraySize - i;
				m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srvArray[i]);
				if (i == 0)
					tex._srv = tex._srvArray[i];
#if _DEBUG
				if (!texDesc.texFileName.empty())
				{
					m_Direct3D->SetDebugName(tex._srvArray[i], texDesc.texFileName + "_SRV[" + std::to_string(i) + "]");
				}
#endif
			}

			if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = (DXGI_FORMAT)texDesc.format;
				uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2D.MipSlice = 0;

				tex._uavArray.resize(desc.ArraySize, nullptr);
				tex._depth = desc.ArraySize;
				for (unsigned i = 0; i < desc.ArraySize; ++i)
				{
					uavDesc.Texture2DArray.FirstArraySlice = i;
					uavDesc.Texture2DArray.ArraySize = desc.ArraySize - i;
					m_device->CreateUnorderedAccessView(tex._tex2D, &uavDesc, &tex._uavArray[i]);
					if (i == 0)
						tex._uav = tex._uavArray[i];
				}
			}
		}
		else
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = texDesc.mipCount;
			srvDesc.Texture2D.MostDetailedMip = 0;
			m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srv);

			if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
			{
				D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = (DXGI_FORMAT)texDesc.format;
				uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = 0;

				m_device->CreateUnorderedAccessView(tex._tex2D, &uavDesc, &tex._uav);
			}
		}
	}

	TextureID retID = -1;
	auto itTex = std::find_if(mTextures.begin(), mTextures.end(), [](const Texture& tex1) {return tex1._id == -1; });
	if (itTex != mTextures.end())
	{
		*itTex = tex;
		itTex->_id = static_cast<TextureID>((int)std::distance(mTextures.begin(), itTex));
		retID = itTex->_id;
	}
	else
	{
		tex._id = static_cast<int>(mTextures.size());
		mTextures.push_back(tex);
		retID = mTextures.back()._id;
	}
	return retID;
#endif
}

ShaderID Renderer::CreateShader(const ShaderDesc& shaderDesc)
{
#if 0
	Shader* shader = new Shader(shaderDesc.shaderName);
	shader->CompileShaders(m_device, shaderDesc);

	mShaders.push_back(shader);
	shader->mID = (static_cast<int>(mShaders.size()) - 1);
	return shader->ID();
#else
	return -1;
#endif
}

ShaderID Renderer::ReloadShader(const ShaderDesc& shaderDesc, const ShaderID shaderID)
{
	if (shaderID == -1)
	{
		Log::Warning("Reload shader called on uninitialized shader.");
		return CreateShader(shaderDesc);
	}

#if 0
	assert(shaderID >= 0 && shaderID < mShaders.size());
	Shader* pShader = mShaders[shaderID];
	delete pShader;
	pShader = new Shader(shaderDesc.shaderName);

	pShader->CompileShaders(m_device, shaderDesc);
	pShader->mID = shaderID;
	mShaders[shaderID] = pShader;
	return pShader->ID();
#else
	return -1;
#endif
}

#endif