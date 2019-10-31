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

using VQ_D3D12_UTILS::ThrowIfFailed;

//
// STATICS
//
static const DXGI_FORMAT SWAPCHAIN_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
static const DXGI_FORMAT DEPTH_BUFFER_FORMAT = DXGI_FORMAT_D32_FLOAT;
static const D3D_FEATURE_LEVEL D3D_FEATURE_LEVEL_TO_REQUEST = D3D_FEATURE_LEVEL_12_1;


// from @adam-sawicki-a's D3D12 Memory Allocator: https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator 
// TODO:
// - replace wprintf()
// - declare member CPUAllocCount
constexpr bool ENABLE_DEBUG_LAYER = true;
constexpr bool ENABLE_CPU_ALLOCATION_CALLBACKS = true;
constexpr bool ENABLE_CPU_ALLOCATION_CALLBACKS_PRINT = false;
static void* const CUSTOM_ALLOCATION_USER_DATA = (void*)(uintptr_t)0xDEADC0DE;
constexpr size_t UPLOAD_HEAP_MEMORY = 100 * 1024 * 1024;

constexpr unsigned FRAME_BUFFER_COUNT = 3;
constexpr bool ENABLE_TEARING_SUPPORT = true;
static const UINT PRESENT_SYNC_INTERVAL = 1;

// Custom Allocator keeps track of CPU allocation count through this static global
static std::atomic<size_t> g_CpuAllocationCount;


static void* CustomAllocate(size_t Size, size_t Alignment, void* pUserData)
{
	assert(pUserData == CUSTOM_ALLOCATION_USER_DATA);
	void* memory = _aligned_malloc(Size, Alignment);
	if (ENABLE_CPU_ALLOCATION_CALLBACKS_PRINT)
	{
		wprintf(L"Allocate Size=%llu Alignment=%llu -> %p\n", Size, Alignment, memory);
	}
	++g_CpuAllocationCount;
	return memory;
}

static void CustomFree(void* pMemory, void* pUserData)
{
	assert(pUserData == CUSTOM_ALLOCATION_USER_DATA);
	if (pMemory)
	{
		--g_CpuAllocationCount;
		if (ENABLE_CPU_ALLOCATION_CALLBACKS_PRINT)
		{
			wprintf(L"Free %p\n", pMemory);
		}
		_aligned_free(pMemory);
	}
}

static void InitializeD3DMA(ID3D12Device* pDevice, D3D12MA::Allocator*& mpAllocator)
{
	// Create allocator
	D3D12MA::ALLOCATOR_DESC desc = {};
	desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
	desc.pDevice = pDevice;

	D3D12MA::ALLOCATION_CALLBACKS allocationCallbacks = {};
	if (ENABLE_CPU_ALLOCATION_CALLBACKS)
	{
		allocationCallbacks.pAllocate = &CustomAllocate;
		allocationCallbacks.pFree = &CustomFree;
		allocationCallbacks.pUserData = CUSTOM_ALLOCATION_USER_DATA;
		desc.pAllocationCallbacks = &allocationCallbacks;
	}

	CHECK_HR(D3D12MA::CreateAllocator(&desc, &mpAllocator));

	switch (mpAllocator->GetD3D12Options().ResourceHeapTier)
	{
	case D3D12_RESOURCE_HEAP_TIER_1:
		wprintf(L"ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_1\n");
		break;
	case D3D12_RESOURCE_HEAP_TIER_2:
		wprintf(L"ResourceHeapTier = D3D12_RESOURCE_HEAP_TIER_2\n");
		break;
	default:
		assert(0);
	}
}

static void SetDefaultRasterizerDesc(D3D12_RASTERIZER_DESC & outDesc)
{
	outDesc.FillMode = D3D12_FILL_MODE_SOLID;
	outDesc.CullMode = D3D12_CULL_MODE_BACK;
	outDesc.FrontCounterClockwise = FALSE;
	outDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	outDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	outDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	outDesc.DepthClipEnable = TRUE;
	outDesc.MultisampleEnable = FALSE;
	outDesc.AntialiasedLineEnable = FALSE;
	outDesc.ForcedSampleCount = 0;
	outDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
}

static void SetDefaultBlendDesc(D3D12_BLEND_DESC & outDesc)
{
	outDesc.AlphaToCoverageEnable = FALSE;
	outDesc.IndependentBlendEnable = FALSE;
	const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
		FALSE,FALSE,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP,
		D3D12_COLOR_WRITE_ENABLE_ALL };
	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
		outDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;
}
// from @adam-sawicki-a's D3D12 Memory Allocator -----------------------------------------------------------------


static void CreateCommandQueues(ID3D12Device* pDevice, ID3D12CommandQueue*& pCmdQueueGFX, ID3D12CommandQueue*& pCmdQueueCompute, ID3D12CommandQueue*& pCmdQueueCopy, D3D12_COMMAND_QUEUE_PRIORITY queuePriority)
{
	
#if _DEBUG
	assert(&pCmdQueueGFX != &pCmdQueueCompute);
	assert(&pCmdQueueCompute != &pCmdQueueCopy);
#endif

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Priority = queuePriority;

	// Graphics
	//if (pCmdQueueGFX)
	{
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCmdQueueGFX)));
		NAME_D3D12_OBJECT(pCmdQueueGFX);
	}

	// Compute
	//if (pCmdQueueCompute)
	{
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCmdQueueCompute)));
		NAME_D3D12_OBJECT(pCmdQueueCompute);
	}

	// Copy
	//if (pCmdQueueCopy)
	{
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		ThrowIfFailed(pDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&pCmdQueueCopy)));
		NAME_D3D12_OBJECT(pCmdQueueCopy);
	}
}




// ================================================================================================================
// RENDERER
// ================================================================================================================
Renderer::Renderer()
// TODO: initialize default state
	: mNumPresentedFrames(0)
	, mCurrentFrameIndex(0)
///	, mCPUAllocationCount(0)
	
{}
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
	this->mAntiAliasingSettings = rendererSettings.antiAliasing;

	// RENDERING API
	//
	bool bRenderingAPIInitialized = InitializeRenderingAPI(hwnd);

	return bRenderingAPIInitialized;
}

void Renderer::Exit()
{
	mUploadHeap.Exit();

	mCmdQueue_GFX.ptr->Release();
	mCmdQueue_Compute.ptr->Release();
	mCmdQueue_Copy.ptr->Release();

	mSwapChain.ptr->Release();
	mDevice.ptr->Release();
}


// Adapted from Microsoft's open source D3D12 Sample repo
// source: https://github.com/microsoft/DirectX-Graphics-Samples
bool Renderer::InitializeRenderingAPI(HWND hwnd)
{
	ID3D12Device*& pDevice = mDevice.ptr; // shorthand
	IDXGIFactory4* pFactory = nullptr;

	// CREATE FACTORY & DEVICE
	//
	{
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

		ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pFactory)));
		VQ_D3D12_UTILS::GetHardwareAdapter(pFactory, &pHardwareAdapter, D3D_FEATURE_LEVEL_TO_REQUEST); // D3D_FEATURE_LEVEL_12_0 in here, would this cause an issue?
		ThrowIfFailed(D3D12CreateDevice(pHardwareAdapter, D3D_FEATURE_LEVEL_TO_REQUEST, IID_PPV_ARGS(&mDevice.ptr)));
	}

	// CREATE COMMAND QUEUES
	//
	// currently only 1 priority is supported, might iterate on this later.
	CreateCommandQueues(pDevice, mCmdQueue_GFX.ptr, mCmdQueue_Compute.ptr, mCmdQueue_Copy.ptr, D3D12_COMMAND_QUEUE_PRIORITY::D3D12_COMMAND_QUEUE_PRIORITY_HIGH);
	
	
	// SWAP CHAIN
	//
	{
		// The resolution of the swap chain buffers will match the resolution of the window, enabling the
		// app to enter iFlip when in fullscreen mode. We will also keep a separate buffer that is not part
		// of the swap chain as an intermediate render target, whose resolution will control the rendering
		// resolution of the scene.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FRAME_BUFFER_COUNT;
		swapChainDesc.Width  = mWindowSettings.width;
		swapChainDesc.Height = mWindowSettings.height;
		swapChainDesc.Format = SWAPCHAIN_FORMAT; // BGRA ?
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1; // MSAA
		swapChainDesc.Flags = ENABLE_TEARING_SUPPORT ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		IDXGISwapChain1* pSwapChain;
		ThrowIfFailed(pFactory->CreateSwapChainForHwnd(
			mCmdQueue_GFX.ptr,  // Swap chain needs the queue so that it can force a flush on it.
			hwnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&pSwapChain
		));

		if (ENABLE_TEARING_SUPPORT)
		{
			// When tearing support is enabled we will handle ALT+Enter key presses in the
			// window message loop rather than let DXGI handle it by calling SetFullscreenState.
			pFactory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
		}

		ThrowIfFailed(pSwapChain->QueryInterface(__uuidof(IDXGISwapChain4), (void**)&mSwapChain.ptr));
		pSwapChain->Release();
		pFactory->Release();
	}

	mCurrentFrameIndex = mSwapChain.ptr->GetCurrentBackBufferIndex();


	// INITIALIZE D3D12MA, UPLOAD HEAP & COMMAND ALLOCATORS PER FRAME
	//
	InitializeD3DMA(pDevice, mpAllocator);
	mUploadHeap.Initialize(pDevice, mCmdQueue_GFX.ptr, UPLOAD_HEAP_MEMORY);
	mFrameCmdAllocators.resize(FRAME_BUFFER_COUNT);
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		CHECK_HR(mDevice.ptr->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mFrameCmdAllocators[i])));
	}


	// RESOURCE HEAPS: RTV, DSV, CBV, SRV, UAV 
	//
	{
		//
		// BACK BUFFER RTVs
		//
		// describe an RTV descriptor heap and create
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FRAME_BUFFER_COUNT; // number of descriptors for this heap.
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // this heap is a render target view heap

		// This heap will not be directly referenced by the shaders (not shader visible), as this will store the output from the pipeline
		// otherwise we would set the heap's flag to D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_HR(mDevice.ptr->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRTVDescHeap.ptr)));

		// get the size of a descriptor in this heap (this is a rtv heap, so only rtv descriptors should be stored in it.
		// descriptor sizes may vary from mpDevice.ptr to mpDevice.ptr, which is why there is no set size and we must ask the 
		// mpDevice.ptr to give us the size. we will use this size to increment a descriptor handle offset
		mRTVDescHeap.mDescSize = mDevice.ptr->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// get a handle to the first descriptor in the descriptor heap. a handle is basically a pointer,
		// but we cannot literally use it like a c++ pointer.
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{ mRTVDescHeap.ptr->GetCPUDescriptorHandleForHeapStart() };

		// Create a RTV for each buffer (double buffering is two buffers, tripple buffering is 3).
		mpRenderTargets.resize(FRAME_BUFFER_COUNT);
		for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
		{
			// first we get the n'th buffer in the swap chain and store it in the n'th
			// position of our ID3D12Resource array
			CHECK_HR(mSwapChain.ptr->GetBuffer(i, IID_PPV_ARGS(&mpRenderTargets[i])));

			// the we "create" a render target view which binds the swap chain buffer (ID3D12Resource[n]) to the rtv handle
			mDevice.ptr->CreateRenderTargetView(mpRenderTargets[i], nullptr, rtvHandle);

			// we increment the rtv handle by the rtv descriptor size we got above
			rtvHandle.ptr += mRTVDescHeap.mDescSize;
		}

		//
		// MAIN DEPTH TARGET DSV
		//
		// create a depth stencil descriptor heap so we can get a pointer to the depth stencil buffer
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		CHECK_HR(pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDSVDescHeap.ptr)));

		D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
		depthOptimizedClearValue.Format = DEPTH_BUFFER_FORMAT;
		depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
		depthOptimizedClearValue.DepthStencil.Stencil = 0;

		D3D12MA::ALLOCATION_DESC depthStencilAllocDesc = {};
		depthStencilAllocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_DESC depthStencilResourceDesc = {};
		depthStencilResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilResourceDesc.Alignment = 0;
		depthStencilResourceDesc.Width  = mWindowSettings.width;
		depthStencilResourceDesc.Height = mWindowSettings.height;
		depthStencilResourceDesc.DepthOrArraySize = 1;
		depthStencilResourceDesc.MipLevels = 1;
		depthStencilResourceDesc.Format = DEPTH_BUFFER_FORMAT;
		depthStencilResourceDesc.SampleDesc.Count = 1;
		depthStencilResourceDesc.SampleDesc.Quality = 0;
		depthStencilResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		depthStencilResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		CHECK_HR(mpAllocator->CreateResource(
			&depthStencilAllocDesc,
			&depthStencilResourceDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthOptimizedClearValue,
			&mpDepthStencilAllocation,
			IID_PPV_ARGS(&mpDepthTarget)
		));
		CHECK_HR(mpDepthTarget->SetName(L"Depth/Stencil Resource Heap"));

		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
		depthStencilDesc.Format = DEPTH_BUFFER_FORMAT;
		depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;
		pDevice->CreateDepthStencilView(mpDepthTarget, &depthStencilDesc, mDSVDescHeap.ptr->GetCPUDescriptorHandleForHeapStart());
	}


	// 
	// CREATE FENCES & FENCE EVENTS
	//
	mFences.resize(FRAME_BUFFER_COUNT);
	mFenceValues.resize(FRAME_BUFFER_COUNT);
	for (int i = 0; i < FRAME_BUFFER_COUNT; i++)
	{
		CHECK_HR(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFences[i])));
		mFenceValues[i] = 0; // set the initial fence values to 0
	}
	mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(mFenceEvent); // create a handle to a mFenceEvent



	return true;
}

// called from a worker.
bool Renderer::LoadDefaultResources()
{

	ID3D12Device*& pDevice = mDevice.ptr; // shorthand

#if 1
	// This is the highest version - If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Create a root signature consisting of a descriptor table with a single CBV.
	{
		D3D12_DESCRIPTOR_RANGE1 ranges[1] = {};
		D3D12_ROOT_PARAMETER1 rootParameters[1] = {};

		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
		ranges[0].NumDescriptors = 1;

		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];

		// Allow input layout and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC  rootSignatureDesc = { };
		rootSignatureDesc.Version = featureData.HighestVersion;
		rootSignatureDesc.Desc_1_1.Flags = rootSignatureFlags;
		rootSignatureDesc.Desc_1_1.NumParameters = _countof(rootParameters);
		rootSignatureDesc.Desc_1_1.pParameters = &rootParameters[0];
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
		rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

		ID3DBlob* pSignature = nullptr;
		ID3DBlob* pError     = nullptr;
		ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &pSignature, &pError));
		ThrowIfFailed(pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&mpRootSignature_Default.ptr)));
		NAME_D3D12_OBJECT(mpRootSignature_Default.ptr);
	}

	// Create a root signature consisting of a descriptor table with a SRV and a sampler.
	{
		D3D12_DESCRIPTOR_RANGE1 ranges[1] = {};
		D3D12_ROOT_PARAMETER1 rootParameters[1] = {};

		// We don't modify the SRV in the post-processing command list after
		// SetGraphicsRootDescriptorTable is executed on the GPU so we can use the default
		// range behavior: D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
		
		///ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAGS::D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		ranges[0].NumDescriptors = 1;

		///rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE::D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];


		// Allow input layout and pixel shader access and deny unnecessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		// Create a sampler.
		D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		samplerDesc.MipLODBias = 0;
		samplerDesc.MaxAnisotropy = 0;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ShaderRegister = 0;
		samplerDesc.RegisterSpace = 0;
		samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC  rootSignatureDesc = { };
		rootSignatureDesc.Version = featureData.HighestVersion;
		rootSignatureDesc.Desc_1_1.Flags = rootSignatureFlags;
		rootSignatureDesc.Desc_1_1.NumParameters = _countof(rootParameters);
		rootSignatureDesc.Desc_1_1.pParameters = &rootParameters[0];
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
		rootSignatureDesc.Desc_1_1.pStaticSamplers = &samplerDesc;

		ID3DBlob* pSignature = nullptr;
		ID3DBlob* pError     = nullptr;
		ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &pSignature, &pError));
		ThrowIfFailed(pDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&mpRootSignature_LoadingScreen.ptr)));
		NAME_D3D12_OBJECT(mpRootSignature_LoadingScreen.ptr);
	}
#endif

	// Create the pipeline state, which includes compiling and loading shaders.
#if 1
	{
		//
		// COMPILE SHADERS
		//

		// Shader compile flags
#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ShaderDesc fullScreenTextureVSPSDesc = {};
		fullScreenTextureVSPSDesc.shaderName = "fullScreenTextureGfx";
		//fullScreenTextureVSPSDesc.stages[EShaderStage::VS].fileName = "FullscreenTriangle_vs.hlsl"; // TODO: use this later
		fullScreenTextureVSPSDesc.stages[EShaderStage::VS].fileName = "FullscreenQuad_vs.hlsl";
		fullScreenTextureVSPSDesc.stages[EShaderStage::VS].macros = { {"", ""} };
		fullScreenTextureVSPSDesc.stages[EShaderStage::PS].fileName = "PassThrough_ps.hlsl";
		fullScreenTextureVSPSDesc.stages[EShaderStage::PS].macros = { {"", ""} };
		ShaderID fullScreenTextureVSPS_ID = this->CreateShader(fullScreenTextureVSPSDesc);
		Shader& fullScreenTextureVSPS = *mShaders[fullScreenTextureVSPS_ID];

		//
		// COMPILE PSOs
		//
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.pRootSignature = mpRootSignature_LoadingScreen.ptr;
		psoDesc.VS = D3D12_SHADER_BYTECODE(fullScreenTextureVSPS.GetShaderByteCode(EShaderStage::VS));
		psoDesc.PS = D3D12_SHADER_BYTECODE(fullScreenTextureVSPS.GetShaderByteCode(EShaderStage::PS));
		psoDesc.InputLayout = fullScreenTextureVSPS.GetShaderInputLayoutDesc();
		SetDefaultRasterizerDesc(psoDesc.RasterizerState);
		SetDefaultBlendDesc(psoDesc.BlendState);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(pDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO_FullscreenTexture.ptr)));
		NAME_D3D12_OBJECT(mPSO_FullscreenTexture.ptr);
	}

	return true;
}

bool Renderer::LoadSizeDependentResources()
{
#if 0
	//UpdatePostViewAndScissor();
	{
		float viewWidthRatio = static_cast<float>(m_resolutionOptions[m_resolutionIndex].Width) / m_width;
		float viewHeightRatio = static_cast<float>(m_resolutionOptions[m_resolutionIndex].Height) / m_height;

		float x = 1.0f;
		float y = 1.0f;

		if (viewWidthRatio < viewHeightRatio)
		{
			// The scaled image's height will fit to the viewport's height and 
			// its width will be smaller than the viewport's width.
			x = viewWidthRatio / viewHeightRatio;
		}
		else
		{
			// The scaled image's width will fit to the viewport's width and 
			// its height may be smaller than the viewport's height.
			y = viewHeightRatio / viewWidthRatio;
		}

		m_postViewport.TopLeftX = m_width * (1.0f - x) / 2.0f;
		m_postViewport.TopLeftY = m_height * (1.0f - y) / 2.0f;
		m_postViewport.Width = x * m_width;
		m_postViewport.Height = y * m_height;

		m_postScissorRect.left = static_cast<LONG>(m_postViewport.TopLeftX);
		m_postScissorRect.right = static_cast<LONG>(m_postViewport.TopLeftX + m_postViewport.Width);
		m_postScissorRect.top = static_cast<LONG>(m_postViewport.TopLeftY);
		m_postScissorRect.bottom = static_cast<LONG>(m_postViewport.TopLeftY + m_postViewport.Height);
	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			NAME_D3D12_OBJECT_INDEXED(m_renderTargets, n);
		}
	}

	// Update resolutions shown in app title.
	UpdateTitle();
#endif

	// This is where you would create/resize intermediate render targets, depth stencils, or other resources
	// dependent on the window size.

	return true;
}

bool Renderer::LoadSceneResolutionDependentResources()
{
#if 0
	// Update resolutions shown in app title.
	UpdateTitle();

	// Set up the scene viewport and scissor rect to match the current scene rendering resolution.
	{
		m_sceneViewport.Width = static_cast<float>(m_resolutionOptions[m_resolutionIndex].Width);
		m_sceneViewport.Height = static_cast<float>(m_resolutionOptions[m_resolutionIndex].Height);

		m_sceneScissorRect.right = static_cast<LONG>(m_resolutionOptions[m_resolutionIndex].Width);
		m_sceneScissorRect.bottom = static_cast<LONG>(m_resolutionOptions[m_resolutionIndex].Height);
	}

	// Update post-process viewport and scissor rectangle.
	UpdatePostViewAndScissor();

	// Create RTV for the intermediate render target.
	{
		D3D12_RESOURCE_DESC swapChainDesc = m_renderTargets[m_frameIndex]->GetDesc();
		const CD3DX12_CLEAR_VALUE clearValue(swapChainDesc.Format, ClearColor);
		const CD3DX12_RESOURCE_DESC renderTargetDesc = CD3DX12_RESOURCE_DESC::Tex2D(
			swapChainDesc.Format,
			m_resolutionOptions[m_resolutionIndex].Width,
			m_resolutionOptions[m_resolutionIndex].Height,
			1u, 1u,
			swapChainDesc.SampleDesc.Count,
			swapChainDesc.SampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
			D3D12_TEXTURE_LAYOUT_UNKNOWN, 0u);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), FrameCount, m_rtvDescriptorSize);
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&renderTargetDesc,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			&clearValue,
			IID_PPV_ARGS(&m_intermediateRenderTarget)));
		m_device->CreateRenderTargetView(m_intermediateRenderTarget.Get(), nullptr, rtvHandle);
		NAME_D3D12_OBJECT(m_intermediateRenderTarget);
	}

	// Create SRV for the intermediate render target.
	m_device->CreateShaderResourceView(m_intermediateRenderTarget.Get(), nullptr, m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
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
unsigned Renderer::FrameRenderTargetHeight() const { return mAntiAliasingSettings.resolutionX; }
unsigned Renderer::FrameRenderTargetWidth()	 const { return mAntiAliasingSettings.resolutionY; }
vec2	 Renderer::FrameRenderTargetDimensionsAsFloat2() const { return vec2(this->FrameRenderTargetWidth(), this->FrameRenderTargetHeight()); }
vec2	 Renderer::GetWindowDimensionsAsFloat2() const { return vec2(this->WindowWidth(), this->WindowHeight()); }
///HWND	 Renderer::GetWindow()			const { return m_Direct3D->WindowHandle(); };


void Renderer::BeginFrame()
{
	// swap the current rtv buffer index so we draw on the correct buffer
	mCurrentFrameIndex = static_cast<int>(this->GetCurrentBackBufferIndex());
	// We have to wait for the GPU to finish with the command allocator before we reset it
	WaitForFrame(mCurrentFrameIndex);
	// increment mFenceValues for next frame
	mFenceValues[mCurrentFrameIndex]++;

	// we can only reset an allocator once the GPU is done with it
	// resetting an allocator frees the memory that the command list was stored in
	CHECK_HR(mFrameCmdAllocators[mCurrentFrameIndex]->Reset());
	
	mCmdListGFX_CurrentFrame = static_cast<ID3D12GraphicsCommandList*>(
		this->CreateCommandList(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)
	);
	ID3D12GraphicsCommandList*& pCmdList = mCmdListGFX_CurrentFrame;
	pCmdList->Close();

	// reset the command list. by resetting the command list we are putting it into
	// a recording state so we can start recording commands into the command allocator.
	// the command allocator that we reference here may have multiple command lists
	// associated with it, but only one can be recording at any time. Make sure
	// that any other command lists associated to this command allocator are in
	// the closed state (not recording).
	// Here you will pass an initial pipeline state object as the second parameter,
	// but in this tutorial we are only clearing the RTV, and do not actually need
	// anything but an initial default pipeline, which is what we get by setting
	// the second parameter to NULL
	CHECK_HR(pCmdList->Reset(mFrameCmdAllocators[mCurrentFrameIndex], NULL));

	// here we start recording commands into the pCmdList (which all the commands will be stored in the mFrameCmdAllocators)

	// transition the "mCurrentFrameIndex" render target from the present state to the render target state so the command list draws to it starting from here
	D3D12_RESOURCE_BARRIER presentToRenderTargetBarrier = {};
	presentToRenderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentToRenderTargetBarrier.Transition.pResource = mpRenderTargets[mCurrentFrameIndex];
	presentToRenderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	presentToRenderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	presentToRenderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCmdList->ResourceBarrier(1, &presentToRenderTargetBarrier);

	mRenderStats = { 0, 0, 0 };
}

void Renderer::EndFrame()
{
	ID3D12GraphicsCommandList*& pCmdList = mCmdListGFX_CurrentFrame;

	// transition the "mCurrentFrameIndex" render target from the render target state to the present state. If the debug layer is enabled, you will receive a
	// warning if present is called on the render target when it's not in the present state
	D3D12_RESOURCE_BARRIER renderTargetToPresentBarrier = {};
	renderTargetToPresentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	renderTargetToPresentBarrier.Transition.pResource = mpRenderTargets[mCurrentFrameIndex];
	renderTargetToPresentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	renderTargetToPresentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	renderTargetToPresentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCmdList->ResourceBarrier(1, &renderTargetToPresentBarrier);

	CHECK_HR(pCmdList->Close());


	// create an array of command lists (only one command list here)
	ID3D12CommandList* ppCommandLists[] = { pCmdList };

	// execute the array of command lists
	mCmdQueue_GFX.ptr->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// this command goes in at the end of our command queue. we will know when our command queue 
	// has finished because the g_Fences value will be set to "mFenceValues" from the GPU since the command
	// queue is being executed on the GPU
	CHECK_HR(mCmdQueue_GFX.ptr->Signal(mFences[mCurrentFrameIndex], mFenceValues[mCurrentFrameIndex]));

	// present the current backbuffer
	CHECK_HR(mSwapChain.ptr->Present(PRESENT_SYNC_INTERVAL, 0));

	++mNumPresentedFrames;
}


void Renderer::SetVertexBuffer(ID3D12GraphicsCommandList *& pCmd, BufferID bufferID)
{
	pCmd->IASetVertexBuffers(0, 1, &mVertexBuffers[bufferID].GetVertexBufferView());
}

void Renderer::SetIndexBuffer(ID3D12GraphicsCommandList *& pCmd, BufferID bufferID)
{
	pCmd->IASetIndexBuffer(&mIndexBuffers[bufferID].GetIndexBufferView());
}


const BufferDesc& Renderer::GetBufferDesc(EBufferType bufferType, BufferID bufferID) const
{
	BufferDesc desc = {};
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

	assert(bufferContainer.size() > bufferID);
	return bufferContainer[bufferID].GetBufferDesc();
}

#ifdef max
#undef max
#endif

static void InitializeBuffer(EBufferType eType, UploadHeap& uploadHeap, const void* pData, Buffer& buffer)
{
	// store buffer in upload heap
	D3D12_SUBRESOURCE_DATA rscData = {};


	const UINT bufferSize = buffer.GetBufferDesc().mStride * buffer.GetBufferDesc().mElementCount;
	rscData.pData = reinterpret_cast<const BYTE*>(pData);
	rscData.RowPitch = bufferSize;
	rscData.SlicePitch = bufferSize;

	ID3D12GraphicsCommandList* cmd = uploadHeap.GetCommandList();

	///CHECK_HR(cmd->Reset(mFrameCmdAllocators[mCurrentFrameIndex], NULL));

	// we are now creating a command with the command list to copy the data from the upload heap to the default heap
	UINT64 r = UpdateSubresources(cmd, buffer.GetResource(), uploadHeap.GetResource(), 0, 0, 1, &rscData);
	assert(r);

	// transition the vertex buffer data from copy destination state to vertex buffer state
	D3D12_RESOURCE_BARRIER vbBarrier = {};
	vbBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	vbBarrier.Transition.pResource = buffer.GetResource();
	vbBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	vbBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	switch (eType)
	{
	case VERTEX_BUFFER:
		vbBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		break;
	case INDEX_BUFFER:
		vbBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
		break;
	case COMPUTE_RW_BUFFER:
		assert(false);
		break;
	//case BUFFER_TYPE_UNKNOWN:
	//	break;
	default:
		Log::Warning("TODO: Warn resource transition for resource uploading for buffers");
		break;
	}
	cmd->ResourceBarrier(1, &vbBarrier);
}


ID3D12CommandList* Renderer::CreateCommandList(D3D12_COMMAND_LIST_TYPE cmdListType)
{
	const size_t frameIndex = this->GetCurrentBackBufferIndex();
	assert(mFrameCmdAllocators.size() > frameIndex);

	ID3D12CommandList * pCmdList = nullptr;
	ID3D12CommandAllocator*& pCmdAllocator = mFrameCmdAllocators[frameIndex];

	switch (cmdListType)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
	case D3D12_COMMAND_LIST_TYPE_COPY:
		break;	// ignore these tpyes
	default:
		Log::Warning("Command List Type not supported");
		return pCmdList; // nullptr
	}
	ThrowIfFailed(mDevice.ptr->CreateCommandList(0, cmdListType, pCmdAllocator, nullptr, IID_PPV_ARGS(&pCmdList)));
	return pCmdList;
}


BufferID Renderer::CreateBuffer(const BufferDesc& bufferDesc, const void* pData /*=nullptr*/, const char* pBufferName /*= nullptr*/)
{
	// Create resource
	Buffer buffer(mpAllocator, bufferDesc);


	// upload resource to GPU
	if (pData)
	{
		InitializeBuffer(bufferDesc.mType, mUploadHeap, pData, buffer);
	}


	// create views



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
	Log::Warning("TODO: Implement CreateTextureFromFile() : texFileName=%s ", texFileName.c_str());
	return -1;
}


TextureID Renderer::CreateTexture2D(const TextureDesc& texDesc)
{
	Log::Warning("TODO: Implement CreateTexture2D() : texFileName=%s ", texDesc.texFileName.c_str());
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
	ShaderID retShaderID = -1;
	Shader* shader = new Shader(shaderDesc);
	
	const bool bShaderCompileSuccess = shader->CompileShaderStages(mDevice.ptr);
	if (bShaderCompileSuccess)
	{
		mShaders.push_back(shader);
		shader->mID = (static_cast<int>(mShaders.size()) - 1);
		retShaderID = shader->ID();
	}

	return retShaderID;
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

	pShader->CompileShaderStages(m_device, shaderDesc);
	pShader->mID = shaderID;
	mShaders[shaderID] = pShader;
	return pShader->ID();
#else
	return -1;
#endif
}


void Renderer::GPUFlush()
{
	ID3D12Fence *pFence;
	ThrowIfFailed(mDevice.ptr->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence)));

	ThrowIfFailed(mCmdQueue_GFX.ptr->Signal(pFence, 1));

	HANDLE mHandleFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	pFence->SetEventOnCompletion(1, mHandleFenceEvent);
	WaitForSingleObject(mHandleFenceEvent, INFINITE);
	CloseHandle(mHandleFenceEvent);

	pFence->Release();
}


size_t Renderer::GetCurrentBackBufferIndex() const
{
	return mSwapChain.ptr->GetCurrentBackBufferIndex();
}

void Renderer::WaitForFrame(size_t frameIndex)
{
	// if the current mFences value is still less than "mFenceValues", then we know the GPU has not finished executing
	// the command queue since it has not reached the "g_CommandQueue->Signal(mFences, mFenceValues)" command
	if (mFences[frameIndex]->GetCompletedValue() < mFenceValues[frameIndex])
	{
		// we have the mFences create an event which is signaled once the mFences's current value is "mFenceValues"
		CHECK_HR(mFences[frameIndex]->SetEventOnCompletion(mFenceValues[frameIndex], mFenceEvent));

		// We will wait until the mFences has triggered the event that it's current value has reached "mFenceValues". once it's value
		// has reached "mFenceValues", we know the command queue has finished executing
		WaitForSingleObject(mFenceEvent, INFINITE);
	}
}

void Renderer::WaitGPUIdle(size_t frameIndex)
{
	mFenceValues[frameIndex]++;
	CHECK_HR(mCmdQueue_GFX.ptr->Signal(mFences[frameIndex], mFenceValues[frameIndex]));
#if 0
	CHECK_HR(mCmdQueue_Compute.ptr->Signal(mFences[frameIndex], mFenceValues[frameIndex]));
	CHECK_HR(mCmdQueue_Copy.ptr->Signal(mFences[frameIndex], mFenceValues[frameIndex]));
#endif
	WaitForFrame(frameIndex);
}

#endif
#endif