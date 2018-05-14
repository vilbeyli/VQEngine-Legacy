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

#define LOG_SEARCH 0

#include "Renderer.h"
#include "Mesh.h"
#include "D3DManager.h"
#include "BufferObject.h"
#include "Shader.h"
#include "Light.h"

#include "Engine/Settings.h"

#include "Application/SystemDefs.h"

#include "Utilities/utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "3rdParty/stb/stb_image.h"
#include "3rdParty/DirectXTex/DirectXTex/DirectXTex.h"

#include <mutex>
#include <cassert>
#include <fstream>


// HELPER FUNCTIONS
//=======================================================================================================================================================
std::vector<std::string> GetShaderPaths(const std::string& shaderFileName)
{	// try to open each file
	const std::string path = Renderer::sShaderRoot + shaderFileName;
	const std::string paths[] = {
		path + "_vs.hlsl",
		path + "_gs.hlsl",
		path + "_ds.hlsl",
		path + "_hs.hlsl",
		path + "_cs.hlsl",
		path + "_ps.hlsl",
	};

	std::vector<std::string> existingPaths;
	for (size_t i = 0; i < EShaderType::COUNT; i++)
	{
		std::ifstream file(paths[i]);
		if (file.is_open())
		{
			existingPaths.push_back(paths[i]);
			file.close();
		}
	}

	if (existingPaths.empty())
	{
		Log::Error("No suitable shader paths \"%s_xs\"", shaderFileName.c_str());
	}
	return std::move(existingPaths);
}

void PollShaderFiles()
{
	// Concerns:
	// separate thread sharing window resources like context and d3d11device
	// might not perform as expected
	// link: https://www.opengl.org/discussion_boards/showthread.php/185980-recompile-the-shader-on-runtime-like-hot-plug-the-new-compiled-shader
	// source: https://msdn.microsoft.com/en-us/library/aa365261(v=vs.85).aspx
	Log::Info("Thread here : PollStarted.\n");
	Sleep(800);

#if 0
	static HANDLE dwChangeHandle;
	DWORD dwWaitStatus;
	LPTSTR lpDir = "Data/Shaders/";

	dwChangeHandle = FindFirstChangeNotification(
		lpDir,                         // directory to watch 
		TRUE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	if (dwChangeHandle == INVALID_HANDLE_VALUE)
	{
		Log::Error("FindFirstChangeNotification function failed.\n");
		;// ExitProcess(GetLastError());
	}

	while (TRUE)
	{
		//	Wait for notification.
		Log::Info("\nWaiting for notification...\n");

		dwWaitStatus = WaitForSingleObject(dwChangeHandle,
			INFINITE);

		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:

			//A file was created, renamed, or deleted in the directory.
			//Refresh this directory and restart the notification.

			OnShaderChange(lpDir);
			if (FindNextChangeNotification(dwChangeHandle) == FALSE)
			{
				Log::Error("FindNextChangeNotification function failed.\n");
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_OBJECT_0 + 1:

			// A directory was created, renamed, or deleted.
			// Refresh the tree and restart the notification.

			//RefreshTree(lpDrive);
			/*if (FindNextChangeNotification(dwChangeHandles[1]) == FALSE)
			{
			printf("\n ERROR: FindNextChangeNotification function failed.\n");
			ExitProcess(GetLastError());
			}*/
			break;

		case WAIT_TIMEOUT:

			//A timeout occurred, this would happen if some value other 
			//than INFINITE is used in the Wait call and no changes occur.
			//In a single-threaded environment you might not want an
			//INFINITE wait.

			OutputDebugString("\nNo changes in the timeout period.\n");
			break;

		default:
			OutputDebugString("\n ERROR: Unhandled dwWaitStatus.\n");
			ExitProcess(GetLastError());
			break;
		}
	}
	OutputDebugString("Done.\n");
#endif
}

void OnShaderChange(LPTSTR dir)
{
	Log::Info("OnShaderChange(%s)\n\n", dir);
	// we know that a change occurred in the 'dir' directory. Read source again
	// works		: create file, delete file
	// doesn't work	: modify file
	// source: https://msdn.microsoft.com/en-us/library/aa365261(v=vs.85).aspx
}
//=======================================================================================================================================================


const char*			Renderer::sShaderRoot		= "Source/Shaders/";
const char*			Renderer::sTextureRoot		= "Data/Textures/";
const char*			Renderer::sHDRTextureRoot	= "Data/Textures/EnvironmentMaps/";
bool				Renderer::sEnableBlend = true;

Renderer::Renderer()
	:
	m_Direct3D(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	mRasterizerStates  (std::vector<RasterizerState*>  (EDefaultRasterizerState::RASTERIZER_STATE_COUNT)),
	mDepthStencilStates(std::vector<DepthStencilState*>(EDefaultDepthStencilState::DEPTH_STENCIL_STATE_COUNT)),
	mBlendStates       (std::vector<BlendState>(EDefaultBlendState::BLEND_STATE_COUNT)),
	mSamplers			(std::vector<Sampler>(EDefaultSamplerState::DEFAULT_SAMPLER_COUNT))
	//,	m_ShaderHotswapPollWatcher("ShaderHotswapWatcher")
{
	for (int i=0; i<(int)EDefaultRasterizerState::RASTERIZER_STATE_COUNT; ++i)
	{
		mRasterizerStates[i] = (RasterizerState*)malloc(sizeof(*mRasterizerStates[i]));
		memset(mRasterizerStates[i], 0, sizeof(*mRasterizerStates[i]));
	}

	for (int i = 0; i < (int)EDefaultBlendState::BLEND_STATE_COUNT; ++i)
	{
		mBlendStates[i].ptr = (ID3D11BlendState*)malloc(sizeof(*mBlendStates[i].ptr));
		memset(mBlendStates[i].ptr, 0, sizeof(*mBlendStates[i].ptr));
	}
}

Renderer::~Renderer(){}

bool Renderer::Initialize(HWND hwnd, const Settings::Window& settings)
{
	// DIRECT3D 11
	//--------------------------------------------------------------------
	mWindowSettings = settings;
	m_Direct3D = new D3DManager();
	if (!m_Direct3D)
	{
		assert(false);
		return false;
	}

	bool result = m_Direct3D->Initialize(
		settings.width,
		settings.height,
		settings.vsync == 1,
		hwnd,
		settings.fullscreen == 1,
		DXGI_FORMAT_R16G16B16A16_FLOAT
		// swapchain should be bgra unorm 32bit
	);

	if (!result)
	{
		MessageBox(hwnd, "Could not initialize Direct3D", "Error", MB_OK);
		return false;
	}
	m_device = m_Direct3D->m_device;
	m_deviceContext = m_Direct3D->m_deviceContext;
	Mesh::spRenderer = this;

	// DEFAULT RENDER TARGET
	//--------------------------------------------------------------------
	{
		RenderTarget defaultRT;

		ID3D11Texture2D* backBufferPtr;
		HRESULT hr = m_Direct3D->m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
		if (FAILED(hr))
		{
			Log::Error("Cannot get back buffer pointer in DefaultRenderTarget initialization");
			return false;
		}
		defaultRT.texture._tex2D = backBufferPtr;

		D3D11_TEXTURE2D_DESC texDesc;		// get back buffer description
		backBufferPtr->GetDesc(&texDesc);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;	// create shader resource view from back buffer desc
		srvDesc.Format = texDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		m_device->CreateShaderResourceView(backBufferPtr, &srvDesc, &defaultRT.texture._srv);

		hr = m_device->CreateRenderTargetView(backBufferPtr, nullptr, &defaultRT.pRenderTargetView);
		if (FAILED(hr))
		{
			Log::Error("Cannot create default render target view.");
			return false;
		}

		mTextures.push_back(defaultRT.texture);	// set texture ID by adding it -- TODO: remove duplicate data - don't add texture to vector
		defaultRT.texture._id = static_cast<int>(mTextures.size() - 1);

		mRenderTargets.push_back(defaultRT);
		mBackBufferRenderTarget = static_cast<int>(mRenderTargets.size() - 1);
	}
	m_Direct3D->ReportLiveObjects("Init Default RT\n");


	// DEFAULT DEPTH TARGET
	//--------------------------------------------------------------------
	{
		// Set up the description of the depth buffer.
		TextureDesc depthTexDesc;
		depthTexDesc.width = settings.width;
		depthTexDesc.height = settings.height;
		depthTexDesc.arraySize = 1;
		depthTexDesc.mipCount = 1;
		//depthTexDesc.format = R24G8;
		depthTexDesc.format = R32;
		depthTexDesc.usage = ETextureUsage(DEPTH_TARGET | RESOURCE);

		mDefaultDepthBufferTexture = CreateTexture2D(depthTexDesc);
		Texture& depthTexture = static_cast<Texture>(GetTextureObject(mDefaultDepthBufferTexture));

		// depth stencil view and shader resource view for the shadow map (^ BindFlags)
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
		AddDepthTarget(dsvDesc, depthTexture);	// assumes index 0
	}
	m_Direct3D->ReportLiveObjects("Init Depth Buffer\n");

	// DEFAULT RASTERIZER STATES
	//--------------------------------------------------------------------
	{
		HRESULT hr;
		const std::string err("Unable to create Rasterizer State: Cull ");

		// MSDN: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476198(v=vs.85).aspx
		D3D11_RASTERIZER_DESC rsDesc = {};

		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.FrontCounterClockwise = false;
		rsDesc.DepthBias = 0;
		rsDesc.ScissorEnable = false;
		rsDesc.DepthBiasClamp = 0;
		rsDesc.SlopeScaledDepthBias = 0.0f;
		rsDesc.DepthClipEnable = true;
		rsDesc.AntialiasedLineEnable = true;
		rsDesc.MultisampleEnable = true;

		rsDesc.CullMode = D3D11_CULL_BACK;
		hr = m_device->CreateRasterizerState(&rsDesc, &mRasterizerStates[(int)EDefaultRasterizerState::CULL_BACK]);
		if (FAILED(hr))
		{
			Log::Error(err + "Back\n");
		}

		rsDesc.CullMode = D3D11_CULL_FRONT;
		hr = m_device->CreateRasterizerState(&rsDesc, &mRasterizerStates[(int)EDefaultRasterizerState::CULL_FRONT]);
		if (FAILED(hr))
		{
			Log::Error(err + "Front\n");
		}

		rsDesc.CullMode = D3D11_CULL_NONE;
		hr = m_device->CreateRasterizerState(&rsDesc, &mRasterizerStates[(int)EDefaultRasterizerState::CULL_NONE]);
		if (FAILED(hr))
		{
			Log::Error(err + "None\n");
		}
	}
	m_Direct3D->ReportLiveObjects("Init Default RS ");


	// DEFAULT BLEND STATES
	//--------------------------------------------------------------------
	{
		D3D11_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
		rtBlendDesc.BlendEnable = true;
		rtBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
		rtBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_MIN;
		rtBlendDesc.DestBlend = D3D11_BLEND_ONE;
		rtBlendDesc.DestBlendAlpha = D3D11_BLEND_ONE;
		rtBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		rtBlendDesc.SrcBlend = D3D11_BLEND_ONE;
		rtBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;

		D3D11_BLEND_DESC desc = {};
		desc.RenderTarget[0] = rtBlendDesc;

		m_device->CreateBlendState(&desc, &(mBlendStates[EDefaultBlendState::ADDITIVE_COLOR].ptr));
		m_device->CreateBlendState(&desc, &(mBlendStates[EDefaultBlendState::ALPHA_BLEND].ptr));

		rtBlendDesc.BlendEnable = false;
		desc.RenderTarget[0] = rtBlendDesc;
		m_device->CreateBlendState(&desc, &(mBlendStates[EDefaultBlendState::DISABLED].ptr));
	}
	m_Direct3D->ReportLiveObjects("Init Default BlendStates ");


	// DEFAULT SAMPLER STATES
	//--------------------------------------------------------------------
	{
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		m_device->CreateSamplerState(&samplerDesc, &(mSamplers[EDefaultSamplerState::WRAP_SAMPLER]._samplerState));

		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		m_device->CreateSamplerState(&samplerDesc, &(mSamplers[EDefaultSamplerState::POINT_SAMPLER]._samplerState));

		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		m_device->CreateSamplerState(&samplerDesc, &(mSamplers[EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW]._samplerState));

		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		m_device->CreateSamplerState(&samplerDesc, &(mSamplers[EDefaultSamplerState::LINEAR_FILTER_SAMPLER]._samplerState));
	}

	// DEFAULT DEPTHSTENCIL SATATES
	//--------------------------------------------------------------------
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	auto checkFailed = [&](HRESULT hr)
	{
		if (FAILED(result))
		{
			Log::Error("Default Depth Stencil State");
			return false;
		}
		return true;
	};


	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil states.
	HRESULT hr = m_device->CreateDepthStencilState(&depthStencilDesc, &mDepthStencilStates[EDefaultDepthStencilState::DEPTH_STENCIL_WRITE]);
	if (!checkFailed(hr)) return false;

	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.StencilEnable = false;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &mDepthStencilStates[EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED]);
	if (!checkFailed(hr)) return false;

	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.StencilEnable = false;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &mDepthStencilStates[EDefaultDepthStencilState::DEPTH_WRITE]);
	if (!checkFailed(hr)) return false;

	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.StencilEnable = true;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &mDepthStencilStates[EDefaultDepthStencilState::STENCIL_WRITE]);
	if (!checkFailed(hr)) return false;

	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.StencilEnable = true;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &mDepthStencilStates[EDefaultDepthStencilState::DEPTH_TEST_ONLY]);
	if (!checkFailed(hr)) return false;

	// SHADERS
	//--------------------------------------------------------------------
	Shader::LoadShaders(this);
	m_Direct3D->ReportLiveObjects("Shader loaded");

	mPipelineState.bRenderTargetChanged = true;
	return true;
}

void Renderer::Exit()
{
	//m_Direct3D->ReportLiveObjects("BEGIN EXIT");

	std::vector<Buffer>* buffers[2] = { &mVertexBuffers, &mIndexBuffers };
	for (int i = 0; i < 2; ++i)
	{
		auto refBuffer = *buffers[i];
		std::for_each(refBuffer.begin(), refBuffer.end(), [](Buffer& b) {b.CleanUp(); });
		refBuffer.clear();
	}

	CPUConstant::CleanUp();	// todo: move constant buffer logic into Buffer
	
	Shader::UnloadShaders(this);

	for (Texture& tex : mTextures)
	{
		tex.Release();
	}
	mTextures.clear();

	for (Sampler& s : mSamplers)
	{
		if (s._samplerState)
		{
			s._samplerState->Release();
			s._samplerState = nullptr;
		}
	}

	for (RenderTarget& rt : mRenderTargets)
	{
		if (rt.pRenderTargetView)
		{
			rt.pRenderTargetView->Release();
			rt.pRenderTargetView = nullptr;
		}
		if (rt.texture._srv)
		{
			//rs._texture._srv->Release();
			rt.texture._srv = nullptr;
		}
		if (rt.texture._tex2D)
		{
			//rs._texture._tex2D->Release();
			rt.texture._tex2D = nullptr;
		}
	}

	for (RasterizerState*& rs : mRasterizerStates)
	{
		if (rs)
		{
			rs->Release();
			rs = nullptr;
		}
	}

	for (DepthStencilState*& dss : mDepthStencilStates)
	{
		if (dss)
		{
			dss->Release();
			dss = nullptr;
		}
	}

	for (BlendState& bs : mBlendStates)
	{
		if (bs.ptr)
		{
			bs.ptr->Release();
			bs.ptr = nullptr;
		}
	}

	for (DepthTarget& dt : mDepthTargets)
	{
		if (dt.pDepthStencilView)
		{
			dt.pDepthStencilView->Release();
			dt.pDepthStencilView = nullptr;
		}
	}

	m_Direct3D->ReportLiveObjects("END EXIT\n");	// todo: ifdef debug & log_mem
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = nullptr;
	}

	Log::Info("---------------------------");
}

float	 Renderer::AspectRatio()	const { return m_Direct3D->AspectRatio(); };
unsigned Renderer::WindowHeight()	const { return m_Direct3D->WindowHeight(); };
unsigned Renderer::WindowWidth()	const { return m_Direct3D->WindowWidth(); }
vec2	 Renderer::GetWindowDimensionsAsFloat2() const { return vec2(static_cast<float>(this->WindowWidth()), static_cast<float>(this->WindowHeight())); }
HWND	 Renderer::GetWindow()			const { return m_Direct3D->WindowHandle(); };

const Shader* Renderer::GetShader(ShaderID shader_id) const
{
	assert(shader_id >= 0 && (int)mShaders.size() > shader_id);
	return mShaders[shader_id];
}



const PipelineState& Renderer::GetState() const
{
	return mPipelineState;
}

ShaderID Renderer::AddShader(const std::string&	shaderFileName,	const std::vector<InputLayout>& layouts)
{
	const std::vector<std::string> paths = GetShaderPaths(shaderFileName);

	Shader* shader = new Shader(shaderFileName);
	shader->CompileShaders(m_device, paths, layouts);
	mShaders.push_back(shader);
	shader->m_id = (static_cast<int>(mShaders.size()) - 1);
	return shader->ID();
}

ShaderID Renderer::AddShader(
	const std::string&				shaderName,
	const std::vector<std::string>& shaderFileNames, 
	const std::vector<EShaderType>& shaderTypes, 
	const std::vector<InputLayout>& layouts
)
{
	std::vector<std::string> paths;
	for (const auto& shaderFileName : shaderFileNames)
	{
		paths.push_back(std::string(sShaderRoot + shaderFileName + ".hlsl"));
	}

	Shader* shader = new Shader(shaderName);
	shader->CompileShaders(m_device, paths, layouts);
	mShaders.push_back(shader);
	shader->m_id = (static_cast<int>(mShaders.size()) - 1);
	return shader->ID();
	return ShaderID();
}

RasterizerStateID Renderer::AddRasterizerState(ERasterizerCullMode cullMode, ERasterizerFillMode fillMode, bool bEnableDepthClip, bool bEnableScissors)
{
	D3D11_RASTERIZER_DESC RSDesc = {};

	RSDesc.CullMode = static_cast<D3D11_CULL_MODE>(cullMode);
	RSDesc.FillMode = static_cast<D3D11_FILL_MODE>(fillMode);
	RSDesc.DepthClipEnable = bEnableDepthClip;
	RSDesc.ScissorEnable = bEnableScissors;
	// todo: add params, scissors, multisample, antialiased line
	

	ID3D11RasterizerState* newRS;
	int hr = m_device->CreateRasterizerState(&RSDesc, &newRS);
	if (!SUCCEEDED(hr))
	{
		Log::Error("Cannot create Rasterizer State");
		return -1;
	}

	mRasterizerStates.push_back(newRS);
	return static_cast<RasterizerStateID>(mRasterizerStates.size() - 1);
}

// example params: "openart/185.png", "Data/Textures/"
TextureID Renderer::CreateTextureFromFile(const std::string& texFileName, const std::string& fileRoot /*= s_textureRoot*/)
{
	if (texFileName.empty() || texFileName == "\"\"") return -1;
	auto found = std::find_if(mTextures.begin(), mTextures.end(), [&texFileName](auto& tex) { return tex._name == texFileName; });
	if (found != mTextures.end())
	{
		return (*found)._id;
	}
	{	// push texture right away and hold a reference
		Texture tex;
		mTextures.push_back(tex);
	}
	Texture& tex = mTextures.back();

	tex._name = texFileName;
	std::string path = fileRoot + texFileName;
	std::wstring wpath(path.begin(), path.end());
	std::unique_ptr<DirectX::ScratchImage> img = std::make_unique<DirectX::ScratchImage>();
	if (SUCCEEDED(LoadFromWICFile(wpath.c_str(), WIC_FLAGS_NONE, nullptr, *img)))
	{
		CreateShaderResourceView(m_device, img->GetImages(), img->GetImageCount(), img->GetMetadata(), &tex._srv);

		// get srv from img
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		tex._srv->GetDesc(&srvDesc);
		
		// read width & height
		ID3D11Resource* resource = nullptr;
		tex._srv->GetResource(&resource);
		if (SUCCEEDED(resource->QueryInterface(&tex._tex2D)))
		{
			D3D11_TEXTURE2D_DESC desc;
			tex._tex2D->GetDesc(&desc);
			tex._width = desc.Width;
			tex._height = desc.Height;
		}
		resource->Release();
		
		tex._id = static_cast<int>(mTextures.size() - 1);
		//m_textures.push_back(tex);

		return mTextures.back()._id;
	}
	else
	{
		Log::Error("Cannot load texture file: %s\n", texFileName.c_str());
		return mTextures[0]._id;
	}

}

TextureID Renderer::CreateTexture2D(const TextureDesc& texDesc)
{
	Texture tex;
	tex._width  = texDesc.width;
	tex._height = texDesc.height;
	tex._name   = texDesc.texFileName;
	

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
	arrSize = texDesc.bIsCubeMap ? 6 : arrSize;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Format = (DXGI_FORMAT)texDesc.format;
	desc.Height = texDesc.height;
	desc.Width = texDesc.width;
	desc.ArraySize = arrSize;
	desc.MipLevels = texDesc.mipCount;
	desc.SampleDesc = { 1, 0 };
	desc.BindFlags = texDesc.usage;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
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

	// Shader Resource View
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = (DXGI_FORMAT)texDesc.format;
	if (texDesc.bIsCubeMap)
	{
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = texDesc.mipCount;
		srvDesc.TextureCube.MostDetailedMip = 0;
		m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srv);
	}
	else
	{
		if (bIsTextureArray)
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MipLevels = 1;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Format = (DXGI_FORMAT)texDesc.format;

			switch (texDesc.format)
			{
				// caution: if initializing for depth texture, and the depth texture
				//			has stencil defined (d24s8), we have to check for 
				//			DXGI_FORMAT_R24_UNORM_X8_TYPELESS vs R32F
			case EImageFormat::R32:
				srvDesc.Format = (DXGI_FORMAT)EImageFormat::R32F;
				break;
			}

			tex._srvArray.resize(desc.ArraySize, nullptr);
			tex._depth = desc.ArraySize;
			for (unsigned i = 0; i < desc.ArraySize; ++i) 
			{
				srvDesc.Texture2DArray.FirstArraySlice = i;
				srvDesc.Texture2DArray.ArraySize = desc.ArraySize - i;
				m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srvArray[i]);
				if (i == 0)
					tex._srv = tex._srvArray[i];
			}
		}
		else
		{
			srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = -1;
			srvDesc.Texture2D.MostDetailedMip = 0;

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
			m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srv);
		}
	}

	tex._id = static_cast<int>(mTextures.size());
	mTextures.push_back(tex);
	return mTextures.back()._id;
}

TextureID Renderer::CreateTexture2D(D3D11_TEXTURE2D_DESC & textureDesc, bool initializeSRV)
{
	Texture tex;
	tex.InitializeTexture2D(textureDesc, this, initializeSRV);
	mTextures.push_back(tex);
	mTextures.back()._id = static_cast<int>(mTextures.size() - 1);
	return mTextures.back()._id;
}

TextureID Renderer::CreateHDRTexture(const std::string& texFileName, const std::string& fileRoot /*= sHDRTextureRoot*/)
{
	// cache lookup, return early if the texture already exists
	auto found = std::find_if(mTextures.begin(), mTextures.end(), [&texFileName](auto& tex) { return tex._name == texFileName; });
	if (found != mTextures.end())
	{
		return (*found)._id;
	}

	std::string path = fileRoot + texFileName;
	
	int width = 0;
	int height = 0;
	int numComponents = 0;
	float* data = stbi_loadf(path.c_str(), &width, &height, &numComponents, 4);

	if (!data)
	{
		Log::Error("Cannot load HDR Texture: %s", path.c_str());
		return -1;
	}

	TextureDesc texDesc = {};
	texDesc.width = width;
	texDesc.height = height;
	texDesc.format = EImageFormat::RGBA32F;
	texDesc.texFileName = texFileName;
	texDesc.pData = data;
	texDesc.dataPitch = sizeof(vec4) * width;
	texDesc.mipCount = 1;
	texDesc.bGenerateMips = false;

	TextureID newTex = CreateTexture2D(texDesc);
	if (newTex == -1)
	{
		Log::Error("Cannot create HDR Texture from data: %s", path.c_str());
	}
	stbi_image_free(data);
	return newTex;
}

TextureID Renderer::CreateCubemapTexture(const std::vector<std::string>& textureFileNames)
{
	constexpr size_t FACE_COUNT = 6;

	// get subresource data for each texture to initialize the cubemap
	D3D11_SUBRESOURCE_DATA pData[FACE_COUNT];
	std::array<DirectX::ScratchImage, FACE_COUNT> faceImages;
	for (int cubeMapFaceIndex = 0; cubeMapFaceIndex < FACE_COUNT; cubeMapFaceIndex++)
	{
		const std::string path = sTextureRoot + textureFileNames[cubeMapFaceIndex];
		const std::wstring wpath(path.begin(), path.end());

		DirectX::ScratchImage* img = &faceImages[cubeMapFaceIndex];
		if (!SUCCEEDED(LoadFromWICFile(wpath.c_str(), WIC_FLAGS_NONE, nullptr, *img)))
		{
			Log::Error(textureFileNames[cubeMapFaceIndex]);
			continue;
		}

		pData[cubeMapFaceIndex].pSysMem          = img->GetPixels();								// Pointer to the pixel data
		pData[cubeMapFaceIndex].SysMemPitch      = static_cast<UINT>(img->GetImages()->rowPitch);	// Line width in bytes
		pData[cubeMapFaceIndex].SysMemSlicePitch = static_cast<UINT>(img->GetImages()->slicePitch);	// This is only used for 3d textures
	}

	// initialize texture array of 6 textures for cubemap
	TexMetadata meta = faceImages[0].GetMetadata();
	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width     = static_cast<UINT>(meta.width);
	texDesc.Height    = static_cast<UINT>(meta.height);
	texDesc.MipLevels = static_cast<UINT>(meta.mipLevels);
	texDesc.ArraySize = FACE_COUNT;
	texDesc.Format    = meta.format;
	texDesc.CPUAccessFlags = 0;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
	
	// init cubemap texture from 6 textures
	ID3D11Texture2D* cubemapTexture;
	HRESULT hr = m_device->CreateTexture2D(&texDesc, &pData[0], &cubemapTexture);
	if (hr != S_OK)
	{
		Log::Error(std::string("Cannot create cubemap texture: ") + StrUtil::split(textureFileNames.front(), '_').front());
		return -1;
	}

	// create cubemap srv
	ID3D11ShaderResourceView* cubeMapSRV;
	D3D11_SHADER_RESOURCE_VIEW_DESC cubemapDesc;
	cubemapDesc.Format = texDesc.Format;
	cubemapDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
	cubemapDesc.TextureCube.MipLevels = texDesc.MipLevels;
	cubemapDesc.TextureCube.MostDetailedMip = 0;
	hr = m_device->CreateShaderResourceView(cubemapTexture, &cubemapDesc, &cubeMapSRV);
	if (hr != S_OK)
	{
		Log::Error(std::string("Cannot create Shader Resource View for ") + StrUtil::split(textureFileNames.front(), '_').front());
		return -1;
	}

	// return param
	Texture cubemapOut;
	cubemapOut._srv = cubeMapSRV;
	cubemapOut._name = "todo:Skybox file name";
	cubemapOut._tex2D = cubemapTexture;
	cubemapOut._height = static_cast<unsigned>(faceImages[0].GetMetadata().height);
	cubemapOut._width  = static_cast<unsigned>(faceImages[0].GetMetadata().width);
	cubemapOut._id = static_cast<int>(mTextures.size());
	mTextures.push_back(cubemapOut);

	return cubemapOut._id;
}

#ifdef max
#undef max
#endif
BufferID Renderer::CreateBuffer(const BufferDesc & bufferDesc, const void* pData /*=nullptr*/)
{
	Buffer buffer(bufferDesc);
	buffer.Initialize(m_device, pData);
	return static_cast<int>([&]() {
		switch (bufferDesc.mType)
		{
		case VERTEX_BUFER:
			mVertexBuffers.push_back(buffer);
			return mVertexBuffers.size() - 1;
		case INDEX_BUFFER:
			mIndexBuffers.push_back(buffer);
			return mIndexBuffers.size() - 1;
		default:
			Log::Warning("Unknown Buffer Type");
			return std::numeric_limits<size_t>::max();
		}
	}());
}

SamplerID Renderer::CreateSamplerState(D3D11_SAMPLER_DESC & samplerDesc)
{
	ID3D11SamplerState*	pSamplerState;
	HRESULT hr = m_device->CreateSamplerState(&samplerDesc, &pSamplerState);
	if (FAILED(hr))
	{
		Log::Error("Cannot create sampler state\n");
	}

	Sampler out;
	out._id = static_cast<SamplerID>(mSamplers.size());
	out._samplerState = pSamplerState;
	out._name = "";	// ?
	mSamplers.push_back(out);
	return out._id;
}

DepthStencilStateID Renderer::AddDepthStencilState(bool bEnableDepth, bool bEnableStencil)
{
	DepthStencilState* newDSState = (DepthStencilState*)malloc(sizeof(DepthStencilState));

	HRESULT result;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = bEnableDepth;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	depthStencilDesc.StencilEnable = bEnableStencil;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &newDSState);
	if (FAILED(result))
	{
		Log::Error("Depth Stencil");
		return false;
	}

	mDepthStencilStates.push_back(newDSState);
	return static_cast<DepthStencilStateID>(mDepthStencilStates.size() - 1);
}

DepthStencilStateID Renderer::AddDepthStencilState(const D3D11_DEPTH_STENCIL_DESC & dsDesc)
{
	DepthStencilState* newDSState = (DepthStencilState*)malloc(sizeof(DepthStencilState));
	HRESULT result;

	result = m_device->CreateDepthStencilState(&dsDesc, &newDSState);
	if (FAILED(result))
	{
		Log::Error("Depth Stencil");
		return false;
	}

	mDepthStencilStates.push_back(newDSState);
	return static_cast<DepthStencilStateID>(mDepthStencilStates.size() - 1);
}

BlendStateID Renderer::AddBlendState()
{
	D3D11_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
	rtBlendDesc.BlendEnable = true;
	rtBlendDesc.BlendOp = D3D11_BLEND_OP_ADD;
	rtBlendDesc.BlendOpAlpha = D3D11_BLEND_OP_MIN;
	rtBlendDesc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtBlendDesc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtBlendDesc.DestBlendAlpha = D3D11_BLEND_ONE;
	rtBlendDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	
	D3D11_BLEND_DESC desc = {};
	desc.RenderTarget[0] = rtBlendDesc;

	BlendState blend;
	m_device->CreateBlendState(&desc, &blend.ptr);
	mBlendStates.push_back(blend);

	return static_cast<BlendStateID>(mBlendStates.size() - 1);
}

RenderTargetID Renderer::AddRenderTarget(D3D11_TEXTURE2D_DESC & RTTextureDesc, D3D11_RENDER_TARGET_VIEW_DESC& RTVDesc)
{
	RenderTarget newRenderTarget;
	newRenderTarget.texture = GetTextureObject(CreateTexture2D(RTTextureDesc, true));
	HRESULT hr = m_device->CreateRenderTargetView(newRenderTarget.texture._tex2D, &RTVDesc, &newRenderTarget.pRenderTargetView);
	if (!SUCCEEDED(hr))
	{
		Log::Error("Render Target View");
		return -1;
	}

	mRenderTargets.push_back(newRenderTarget);
	return static_cast<int>(mRenderTargets.size() - 1);
}

RenderTargetID Renderer::AddRenderTarget(const Texture& textureObj, D3D11_RENDER_TARGET_VIEW_DESC& RTVDesc)
{
	RenderTarget newRenderTarget;
	newRenderTarget.texture = textureObj;
	HRESULT hr = m_device->CreateRenderTargetView(newRenderTarget.texture._tex2D, &RTVDesc, &newRenderTarget.pRenderTargetView);
	if (!SUCCEEDED(hr))
	{
		Log::Error("Render Target View");
		return -1;
	}

	mRenderTargets.push_back(newRenderTarget);
	return static_cast<int>(mRenderTargets.size() - 1);
}

DepthTargetID Renderer::AddDepthTarget(const D3D11_DEPTH_STENCIL_VIEW_DESC& dsvDesc, Texture& depthTexture)
{
	DepthTarget newDepthTarget;
	newDepthTarget.pDepthStencilView = (ID3D11DepthStencilView*)malloc(sizeof(*newDepthTarget.pDepthStencilView));
	memset(newDepthTarget.pDepthStencilView, 0, sizeof(*newDepthTarget.pDepthStencilView));

	HRESULT hr = m_device->CreateDepthStencilView(depthTexture._tex2D, &dsvDesc, &newDepthTarget.pDepthStencilView);
	if (FAILED(hr))
	{
		Log::Error("Depth Stencil Target View");
		return -1;
	}
	newDepthTarget.texture = depthTexture;
	mDepthTargets.push_back(newDepthTarget);
	return static_cast<int>(mDepthTargets.size() - 1);
}

const Texture& Renderer::GetTextureObject(TextureID id) const
{
	assert(id >= 0 && static_cast<unsigned>(id) < mTextures.size());
	return mTextures[id];
}

const TextureID Renderer::GetTexture(const std::string name) const
{
	auto found = std::find_if(mTextures.begin(), mTextures.end(), [&name](auto& tex) { return tex._name == name; });
	if (found != mTextures.end())
	{
		return found->_id;
	}
	Log::Error("Texture not found: " + name);
	return -1;
}


void Renderer::SetShader(ShaderID id)
{
	assert(id >= 0 && static_cast<unsigned>(id) < mShaders.size());
	if (mPipelineState.shader != -1)		// if valid shader
	{
		if (id != mPipelineState.shader)	// if not the same shader
		{
			Shader* shader = mShaders[mPipelineState.shader];

			// nullify texture units 
			for (ShaderTexture& tex : shader->m_textures)
			{
				constexpr UINT NumNullSRV = 12;
				ID3D11ShaderResourceView* nullSRV[NumNullSRV ] = { nullptr };
				//(m_deviceContext->*SetShaderResources[tex.shdType])(tex.bufferSlot, 1, nullSRV);
				switch (tex.shdType)
				{
				case EShaderType::VS:
					m_deviceContext->VSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case EShaderType::GS:
					m_deviceContext->GSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case EShaderType::HS:
					m_deviceContext->HSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case EShaderType::DS:
					m_deviceContext->DSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case EShaderType::PS:
					m_deviceContext->PSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case EShaderType::CS:
					m_deviceContext->CSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				default:
					break;
				}
			}

#if 0
			const size_t numSamplers = shader->m_samplers.size();
			std::vector<ID3D11SamplerState*> nullSSvec(numSamplers, nullptr);
			ID3D11SamplerState** nullSS = nullSSvec.data();
			for(ShaderSampler& smp : shader->m_samplers)
			{
				switch (smp.shdType)
				{
				case EShaderType::VS:
					m_deviceContext->VSSetSamplers(smp.bufferSlot, numSamplers, nullSS);
					break;
				case EShaderType::GS:
					m_deviceContext->GSSetSamplers(smp.bufferSlot, numSamplers, nullSS);
					break;
				case EShaderType::HS:
					m_deviceContext->HSSetSamplers(smp.bufferSlot, numSamplers, nullSS);
					break;
				case EShaderType::DS:
					m_deviceContext->DSSetSamplers(smp.bufferSlot, numSamplers, nullSS);
					break;
				case EShaderType::PS:
					m_deviceContext->PSSetSamplers(smp.bufferSlot, numSamplers, nullSS);
					break;
				case EShaderType::CS:
					m_deviceContext->CSSetSamplers(smp.bufferSlot, numSamplers, nullSS);
					break;
				default:
					break;
				}
			}
#endif

			ID3D11RenderTargetView* nullRTV[6] = { nullptr };
			ID3D11DepthStencilView* nullDSV = { nullptr };
			m_deviceContext->OMSetRenderTargets(6, nullRTV, nullDSV);

			const float blendFactor[4] = { 1,1,1,1 };
			m_deviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

		} // if not same shader
	}	// if valid shader

	if (id != mPipelineState.shader)
	{
		mPipelineState.shader = id;
		mShaders[id]->ClearConstantBuffers();
	}
}

void Renderer::SetVertexBuffer(BufferID bufferID)
{
	mPipelineState.vertexBuffer = bufferID;
	UINT offset = 0;

	// temporary
	Buffer& b = mVertexBuffers[bufferID];
	m_deviceContext->IASetVertexBuffers(0, 1, &(b.mData), &b.mDesc.mStride, &offset);
}

void Renderer::SetIndexBuffer(BufferID bufferID)
{
	mPipelineState.indexBuffer = bufferID;

	
	m_deviceContext->IASetIndexBuffer(mIndexBuffers[mPipelineState.indexBuffer].mData, DXGI_FORMAT_R32_UINT, 0);
}

#include "Engine/Engine.h"
void Renderer::SetGeometry(EGeometry GeomEnum)
{
	const auto VertedAndIndexBuffer = ENGINE->GetGeometryVertexAndIndexBuffers(GeomEnum);
	mPipelineState.vertexBuffer = VertedAndIndexBuffer.first;
	mPipelineState.indexBuffer = VertedAndIndexBuffer.second;
}

void Renderer::ResetPipelineState()
{
	mPipelineState.shader = -1;
}


void Renderer::SetViewport(const unsigned width, const unsigned height)
{
	mPipelineState.viewPort.TopLeftX = 0;
	mPipelineState.viewPort.TopLeftY = 0;
	mPipelineState.viewPort.Width	= static_cast<float>(width);
	mPipelineState.viewPort.Height	= static_cast<float>(height);
	mPipelineState.viewPort.MinDepth = 0;
	mPipelineState.viewPort.MaxDepth = 1;
}

void Renderer::SetViewport(const D3D11_VIEWPORT & viewport)
{
	mPipelineState.viewPort = viewport;
}

void Renderer::SetConstant4x4f(const char* cName, const XMMATRIX& matrix)
{
	// maybe read from SIMD registers?
	XMFLOAT4X4 m;	XMStoreFloat4x4(&m, matrix);
	float* data = &m.m[0][0];
	SetConstant(cName, data);
}

void Renderer::SetConstant(const char * cName, const void * data)
{
	// Here, we write to the CPU address of the constant buffer if the contents are updated.
	// otherwise we don't write and flag the buffer that contains the GPU address dirty.
	// When all the constants are set on the CPU side, right before the draw call,
	// we will use a mapped resource as a block of memory, transfer our CPU
	// version of constants to there, and then send it to GPU CBuffers at one call as a block memory.
	// Otherwise, we would have to make an API call each time we set the constants, which would be slower.
	// Read more here: https://developer.nvidia.com/sites/default/files/akamai/gamedev/files/gdc12/Efficient_Buffer_Management_McDonald.pdf
	//      and  here: https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0

	Shader* shader = mShaders[mPipelineState.shader];

#if 1
	// LINEAR LOOKUP
	bool found = false;
	for (const ConstantBufferMapping& bufferSlotIDPair : shader->m_constantsUnsorted)
	{
		const size_t GPUcBufferSlot = bufferSlotIDPair.first;
		const CPUConstantID constID = bufferSlotIDPair.second;
		CPUConstant& c = CPUConstant::Get(constID);
		if (strcmp(cName, c._name.c_str()) == 0)		// if name matches
		{
			found = true;
			if (memcmp(c._data, data, c._size) != 0)	// copy data if its not the same
			{
				memcpy(c._data, data, c._size);
				shader->m_cBuffers[GPUcBufferSlot].dirty = true;
				break;	// ensures write on first occurrence
			}
		}
	}
	if (!found)
	{
		Log::Error("CONSTANT NOT FOUND: %s", cName);
	}

#else
	// TODO: Fix binary search algorithm...
	// BINARY SEARCH 
	const auto& BinarySearch = [cName, &shader]()
	{
		bool bKeepSearching = true;
		size_t lowIndex  = 0;
		size_t highIndex = shader->m_constants.size() - 1;
		size_t currIndex = highIndex / 2;

#if LOG_SEARCH
		{
			Log::Info("BinarySearch: %s", cName);
			for (const auto& slotIndexPair : shader->m_constants)
			{	// dump sorted buffer slot
				const char* constantName = CPUConstant::Get(slotIndexPair.second)._name.c_str();
				Log::Info(" GPU:%d | CPU:%d - %s", slotIndexPair.first, slotIndexPair.second, constantName);
			}
			Log::Info("--------------------------");
		}
#endif

		while (bKeepSearching)
		{
#if LOG_SEARCH
			Log::Info("begin: low:%d\tcur:%d\thi:%d", lowIndex, currIndex, highIndex);
#endif
			const ConstantBufferMapping& bufferSlotIDPair = shader->m_constants[currIndex];
			const CPUConstantID constID = bufferSlotIDPair.second;
			const CPUConstant& c = CPUConstant::Get(constID);
			int res = strcmp(cName, c._name.c_str());
#if LOG_SEARCH
			Log::Info(" \"%s\" strcmp \"%s\" -> %d", cName, c._name.c_str(), res);
#endif
			if (res == 0)		
			{	
#if LOG_SEARCH
				Log::Info("found: %s", c._name.c_str());
#endif
				return currIndex;
			}

			else if (res > 0)
			{
				lowIndex = currIndex;
				currIndex = lowIndex + (highIndex - lowIndex + 1) / 2;
#if LOG_SEARCH
				{
					const ConstantBufferMapping& bufferSlotIDPair = shader->m_constants[currIndex];
					const CPUConstantID constID = bufferSlotIDPair.second;
					const CPUConstant& c = CPUConstant::Get(constID);
					Log::Info("looking next(%s)", c._name.c_str());
				}
#endif
			}

			else
			{
				highIndex = currIndex - 1;
				currIndex = lowIndex + (highIndex - lowIndex) / 2;
#if LOG_SEARCH
				{
					const ConstantBufferMapping& bufferSlotIDPair = shader->m_constants[currIndex];
					const CPUConstantID constID = bufferSlotIDPair.second;
					const CPUConstant& c = CPUConstant::Get(constID);
					Log::Info("looking previous(%s)", c._name.c_str());
				}
#endif
			}

#if LOG_SEARCH
			Log::Info("end: low:%d\tcur:%d\thi:%d\n", lowIndex, currIndex, highIndex);
#endif
			bKeepSearching = lowIndex < highIndex;// || ((currIndex == lowIndex) && (lowIndex == highIndex));
		}

		Log::Error("CONSTANT NOT FOUND: %s", cName);
		return currIndex;
	};

	size_t bufferMappingIndex = BinarySearch();
	const ConstantBufferMapping& bufferSlotIDPair = shader->m_constants[bufferMappingIndex];
	const size_t GPUcBufferSlot = bufferSlotIDPair.first;
	const CPUConstantID constID = bufferSlotIDPair.second;
	CPUConstant& c = CPUConstant::Get(constID);
	if (memcmp(c._data, data, c._size) != 0)	// copy data if its not the same
	{
		memcpy(c._data, data, c._size);
		shader->m_cBuffers[GPUcBufferSlot].dirty = true;
	}
#endif

}

void Renderer::SetTexture(const char * texName, TextureID tex)
{
	Shader* shader = mShaders[mPipelineState.shader];
	bool found = false;

	// linear name lookup
	for (size_t i = 0; i < shader->m_textures.size(); ++i)
	{
		if (strcmp(texName, shader->m_textures[i].name.c_str()) == 0)
		{
			found = true;
			SetTextureCommand cmd;
			cmd.textureID = tex;
			cmd.shaderTexture = shader->m_textures[i];
			mSetTextureCmds.push(cmd);
		}
	}

#ifdef _DEBUG
	if (!found)
	{
		Log::Error("Texture not found: \"%s\" in Shader(Id=%d) \"%s\"", texName, mPipelineState.shader, shader->Name().c_str());
	}
#endif
}

void Renderer::SetTextureArray(const char * texName, TextureID texArray)
{
	Shader* shader = mShaders[mPipelineState.shader];
	bool found = false;

	// linear name lookup
	for (size_t i = 0; i < shader->m_textures.size(); ++i)
	{
		if (strcmp(texName, shader->m_textures[i].name.c_str()) == 0)
		{
			found = true;
			SetTextureCommand cmd;
			cmd.textureID = texArray;
			cmd.shaderTexture = shader->m_textures[i];
			mSetTextureCmds.push(cmd);
		}
	}

#ifdef _DEBUG
	if (!found)
	{
		Log::Error("Texture not found: \"%s\" in Shader(Id=%d) \"%s\"", texName, mPipelineState.shader, shader->Name().c_str());
	}
#endif
}

void Renderer::SetSamplerState(const char * samplerName, SamplerID samplerID)
{
	Shader* shader = mShaders[mPipelineState.shader];
	bool found = false;

	// linear name lookup
	for (size_t i = 0; i < shader->m_samplers.size(); ++i)
	{
		const ShaderSampler& sampler = shader->m_samplers[i];
		if (strcmp(samplerName, sampler.name.c_str()) == 0)
		{
			found = true;
			SetSamplerCommand cmd;
			cmd.samplerID = samplerID;
			cmd.shaderSampler = sampler;
			mSetSamplerCmds.push(cmd);
		}
	}

#ifdef _DEBUG
	if (!found)
	{
		Log::Error("Sampler not found: \"%s\" in Shader(Id=%d) \"%s\"\n", samplerName, mPipelineState.shader, shader->Name().c_str());
	}
#endif
}

void Renderer::SetRasterizerState(RasterizerStateID rsStateID)
{
	assert(rsStateID > -1 && static_cast<size_t>(rsStateID) < mRasterizerStates.size());
	mPipelineState.rasterizerState = rsStateID;
}

void Renderer::SetBlendState(BlendStateID blendStateID)
{
	assert(blendStateID > -1 && static_cast<size_t>(blendStateID) < mBlendStates.size());
	mPipelineState.blendState = blendStateID;
}

void Renderer::SetDepthStencilState(DepthStencilStateID depthStencilStateID)
{
	assert(depthStencilStateID > -1 && static_cast<size_t>(depthStencilStateID) < mDepthStencilStates.size());
	mPipelineState.depthStencilState = depthStencilStateID;
}

void Renderer::SetScissorsRect(int left, int right, int top, int bottom)
{
	D3D11_RECT rects[1];
	rects[0].left = left;
	rects[0].right = right;
	rects[0].top = top;
	rects[0].bottom = bottom;
	
	// only called from debug for now, so immediate api call. rethink: make this command?
	m_deviceContext->RSSetScissorRects(1, rects);
}

void Renderer::BindRenderTarget(RenderTargetID rtvID)
{
	assert(rtvID > -1 && static_cast<size_t>(rtvID) < mRenderTargets.size());
	//for(RenderTargetID& hRT : m_state._boundRenderTargets) 
	mPipelineState.renderTargets = { rtvID };
	mPipelineState.bRenderTargetChanged = true;
}

void Renderer::BindDepthTarget(DepthTargetID dsvID)
{
	assert(dsvID > -1 && static_cast<size_t>(dsvID) < mDepthTargets.size());
	mPipelineState.depthTargets = dsvID;
}

void Renderer::UnbindRenderTargets()
{
	mPipelineState.renderTargets = { -1, -1, -1, -1, -1, -1 };
	mPipelineState.bRenderTargetChanged = true;
}

void Renderer::UnbindDepthTarget()
{
	mPipelineState.depthTargets = -1;
}

// temp
void Renderer::DrawLine()
{
	// draw line between 2 coords
	vec3 pos1 = vec3(0, 0, 0);
	vec3 pos2 = pos1;	pos2.x() += 5.0f;

	SetConstant3f("p1", pos1);
	SetConstant3f("p2", pos2);
	SetConstant3f("color", LinearColor::green.Value());
	Apply();
	Draw(1, EPrimitiveTopology::POINT_LIST);
}

void Renderer::DrawLine(const vec3& pos1, const vec3& pos2, const vec3& color)
{
	SetConstant3f("p1", pos1);
	SetConstant3f("p2", pos2);
	SetConstant3f("color", color);
	Apply();
	Draw(1, EPrimitiveTopology::POINT_LIST);
}

// assumes (0, 0) is Bottom Left corner of the screen.
void Renderer::DrawQuadOnScreen(const DrawQuadOnScreenCommand& cmd)
{														// warning:
	const int screenWidth = mWindowSettings.width;		// 2 copies of renderer settings, one here on in Engine
	const int screenHeight = mWindowSettings.height;	// dynamic window size change might break things...
	const float& dimx = cmd.dimensionsInPixels.x();
	const float& dimy = cmd.dimensionsInPixels.y();
	const float posx = cmd.bottomLeftCornerScreenCoordinates.x() * 2.0f - screenWidth;	// NDC is [-1, 1] ; if (0,0) is given
	const float posy = cmd.bottomLeftCornerScreenCoordinates.y() * 2.0f - screenHeight;	// texture is drawn in bottom left corner of the screen
	const vec2 posCenter( (posx + dimx)/screenWidth, (posy + dimy) / screenHeight);

	const XMVECTOR scale = vec3(dimx / screenWidth, dimy / screenHeight, 0.0f);
	const XMVECTOR translation = vec3(posCenter.x(), posCenter.y(), 0);
	const XMMATRIX transformation = XMMatrixAffineTransformation(scale, vec3::Zero, XMQuaternionIdentity(), translation);

	SetConstant4x4f("screenSpaceTransformation", transformation);
	SetConstant1f("isDepthTexture", cmd.bIsDepthTexture ? 1.0f : 0.0f);
	SetTexture("inputTexture", cmd.texture);
	SetGeometry(EGeometry::QUAD);
	Apply();
	DrawIndexed();
}


void Renderer::BeginRender(const ClearCommand & clearCmd)
{
	if (clearCmd.bDoClearColor)
	{
		for (const RenderTargetID rtv : mPipelineState.renderTargets)
		{
			if (rtv >= 0)	m_deviceContext->ClearRenderTargetView(mRenderTargets[rtv].pRenderTargetView, clearCmd.clearColor.data());
			else			Log::Error("Begin called with clear color command without a render target bound to pipeline!");
		}
	}

	const bool bClearDepthStencil = clearCmd.bDoClearDepth || clearCmd.bDoClearStencil;
	if (bClearDepthStencil)
	{
		const DepthTargetID dsv = mPipelineState.depthTargets;
		UINT clearFlag = [&]() -> UINT	{
			if (clearCmd.bDoClearDepth && clearCmd.bDoClearStencil)
				return D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;

			if (clearCmd.bDoClearDepth)
				return D3D11_CLEAR_DEPTH;

			return D3D11_CLEAR_STENCIL;
		}();

		if (dsv >= 0)	m_deviceContext->ClearDepthStencilView(mDepthTargets[dsv].pDepthStencilView, clearFlag, clearCmd.clearDepth, clearCmd.clearStencil);
		else			Log::Error("Begin called with clear depth_stencil command without a depth target bound to pipeline!");
	}
}

void Renderer::BeginFrame()
{
	mRenderStats = { 0, 0, 0 };
}

void Renderer::EndFrame()
{
	m_Direct3D->EndFrame();
	mPrevPipelineState = mPipelineState;
}


void Renderer::UpdateBuffer(BufferID buffer, const void * pData)
{
	assert(buffer >= 0 && buffer < mVertexBuffers.size());
	mVertexBuffers[buffer].Update(this, pData);
}

void Renderer::Apply()
{	// Here, we make all the API calls
	Shader* shader = mPipelineState.shader >= 0 ? mShaders[mPipelineState.shader] : nullptr;

	// INPUT ASSEMBLY
	// ----------------------------------------
	const Buffer& VertexBuffer = mVertexBuffers[mPipelineState.vertexBuffer];
	const Buffer& IndexBuffer = mIndexBuffers[mPipelineState.indexBuffer];

	unsigned stride = VertexBuffer.mDesc.mStride;
	unsigned offset = 0;

	if (mPipelineState.vertexBuffer != -1)
	{
		m_deviceContext->IASetVertexBuffers(0, 1, &(VertexBuffer.mData), &stride, &offset);
	}
	if (mPipelineState.indexBuffer != -1)
	{
		m_deviceContext->IASetIndexBuffer(IndexBuffer.mData, DXGI_FORMAT_R32_UINT, 0);
	}
	if (shader)
	{
		m_deviceContext->IASetInputLayout(shader->m_layout);
	}

	if (shader)
	{	// SHADER STAGES
		// ----------------------------------------
		m_deviceContext->VSSetShader(shader->m_vertexShader  , nullptr, 0);
		m_deviceContext->PSSetShader(shader->m_pixelShader   , nullptr, 0);
		m_deviceContext->GSSetShader(shader->m_geometryShader, nullptr, 0);
		m_deviceContext->HSSetShader(shader->m_hullShader    , nullptr, 0);
		m_deviceContext->DSSetShader(shader->m_domainShader  , nullptr, 0);
		m_deviceContext->CSSetShader(shader->m_computeShader , nullptr, 0);

		// CONSTANT BUFFERS 
		// ----------------------------------------
		shader->UpdateConstants(m_deviceContext);

		// SHADER RESOURCES
		// ----------------------------------------
		while (mSetSamplerCmds.size() > 0)
		{
			SetSamplerCommand& cmd = mSetSamplerCmds.front();
			cmd.SetResource(this);
			mSetSamplerCmds.pop();
		}

		while (mSetTextureCmds.size() > 0)
		{
			SetTextureCommand& cmd = mSetTextureCmds.front();
			cmd.SetResource(this);
			mSetTextureCmds.pop();
		}

		// RASTERIZER
		// ----------------------------------------
		m_deviceContext->RSSetViewports(1, &mPipelineState.viewPort);
		m_deviceContext->RSSetState(mRasterizerStates[mPipelineState.rasterizerState]);

		// OUTPUT MERGER
		// ----------------------------------------
		if (sEnableBlend)
		{
			const float blendFactor[4] = { 1,1,1,1 };
			m_deviceContext->OMSetBlendState(mBlendStates[mPipelineState.blendState].ptr, nullptr, 0xffffffff);
		}

		const auto indexDSState = mPipelineState.depthStencilState;
		const auto indexRTV = mPipelineState.renderTargets[0];
		
		// get the bound render target addresses
#if 1
		// todo: perf: this takes as much time as set constants in debug mode
		std::vector<ID3D11RenderTargetView*> RTVs = [&]() {				
			std::vector<ID3D11RenderTargetView*> v(mPipelineState.renderTargets.size(), nullptr);
			size_t i = 0;
			for (RenderTargetID hRT : mPipelineState.renderTargets) 
				if(hRT >= 0) 
					v[i++] = mRenderTargets[hRT].pRenderTargetView;
			return std::move(v);
		}();
#else
		// this is slower ~2ms in debug
		std::vector<ID3D11RenderTargetView*> RTVs;
		for (RenderTargetID hRT : mPipelineState.renderTargets)
			if (hRT >= 0)
				RTVs.push_back(mRenderTargets[hRT].pRenderTargetView);
#endif
		const auto indexDSV     = mPipelineState.depthTargets;
		//ID3D11RenderTargetView** RTV = indexRTV == -1 ? nullptr : &RTVs[0];
		ID3D11RenderTargetView** RTV = RTVs.empty()   ? nullptr : &RTVs[0];
		ID3D11DepthStencilView*  DSV = indexDSV == -1 ? nullptr : mDepthTargets[indexDSV].pDepthStencilView;

		if (mPipelineState.bRenderTargetChanged || true)
		{
			m_deviceContext->OMSetRenderTargets(RTV ? (unsigned)RTVs.size() : 0, RTV, DSV);
			mPipelineState.bRenderTargetChanged = false;
		}

		auto*  DSSTATE = mDepthStencilStates[indexDSState];
		m_deviceContext->OMSetDepthStencilState(DSSTATE, 0);
	}
	else
	{
		Log::Error("Renderer::Apply() : Shader null...\n");
	}
}

void Renderer::BeginEvent(const std::string & marker)
{
#if _DEBUG
	StrUtil::UnicodeString umarker(marker);
	m_Direct3D->m_annotation->BeginEvent(umarker.GetUnicodePtr());
#endif
}

void Renderer::EndEvent()
{
#if _DEBUG
	m_Direct3D->m_annotation->EndEvent();
#endif
}

void Renderer::DrawIndexed(EPrimitiveTopology topology)
{
	const Buffer& VertexBuffer = mVertexBuffers[mPipelineState.vertexBuffer];
	const Buffer& IndexBuffer = mIndexBuffers[mPipelineState.indexBuffer];

	const unsigned numIndices = IndexBuffer.mDesc.mElementCount;
	const unsigned numVertices = VertexBuffer.mDesc.mElementCount;

	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->DrawIndexed(numIndices, 0, 0);
	
	++mRenderStats.numDrawCalls;
	mRenderStats.numIndices += numIndices;
	mRenderStats.numVertices += numVertices;
}

void Renderer::Draw(int vertCount, EPrimitiveTopology topology /*= EPrimitiveTopology::POINT_LIST*/)
{
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->Draw(vertCount, 0);
	
	++mRenderStats.numDrawCalls;
	mRenderStats.numVertices += vertCount;
}
