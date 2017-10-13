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
#include "Settings.h"
#include "Mesh.h"

#include "GeometryGenerator.h"
#include "D3DManager.h"
#include "BufferObject.h"
#include "Shader.h"
#include "Light.h"

#include "SystemDefs.h"
#include "utils.h"
#include "Camera.h"
#include "DirectXTex.h"
#include "D3DManager.h"

#include <mutex>
#include <cassert>

// HELPER FUNCTIONS
//=======================================================================================================================================================
std::vector<std::string> GetShaderPaths(const std::string& shaderFileName)
{	// try to open each file
	const std::string path = Renderer::s_shaderRoot + shaderFileName;
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
	// doesnt work	: modify file
	// source: https://msdn.microsoft.com/en-us/library/aa365261(v=vs.85).aspx
}
//=======================================================================================================================================================


const char*			Renderer::s_shaderRoot		= "Data/Shaders/";
const char*			Renderer::s_textureRoot		= "Data/Textures/";
bool				Renderer::sEnableBlend = true;

Renderer::Renderer()
	:
	m_Direct3D(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	m_bufferObjects     (std::vector<BufferObject*>     (EGeometry::MESH_TYPE_COUNT)      ),
	m_rasterizerStates  (std::vector<RasterizerState*>  ((int)EDefaultRasterizerState::RASTERIZER_STATE_COUNT)),
	m_depthStencilStates(std::vector<DepthStencilState*>(EDefaultDepthStencilState::DEPTH_STENCIL_STATE_COUNT)),
	m_blendStates       (std::vector<BlendState>(EDefaultBlendState::BLEND_STATE_COUNT))
	//,	m_ShaderHotswapPollWatcher("ShaderHotswapWatcher")
{
	for (int i=0; i<(int)EDefaultRasterizerState::RASTERIZER_STATE_COUNT; ++i)
	{
		m_rasterizerStates[i] = (RasterizerState*)malloc(sizeof(*m_rasterizerStates[i]));
		memset(m_rasterizerStates[i], 0, sizeof(*m_rasterizerStates[i]));
	}

	for (int i = 0; i < (int)EDefaultBlendState::BLEND_STATE_COUNT; ++i)
	{
		m_blendStates[i].ptr = (ID3D11BlendState*)malloc(sizeof(*m_blendStates[i].ptr));
		memset(m_blendStates[i].ptr, 0, sizeof(*m_blendStates[i].ptr));
	}
}

Renderer::~Renderer(){}

void Renderer::Exit()
{
	//m_Direct3D->ReportLiveObjects("BEGIN EXIT");
	for (BufferObject*& bo : m_bufferObjects)
	{
		delete bo;
		bo = nullptr;
	}
	m_bufferObjects.clear();

	CPUConstant::CleanUp();
	
	Shader::UnloadShaders(this);

	for (Texture& tex : m_textures)
	{
		tex.Release();
	}
	m_textures.clear();

	for (Sampler& s : m_samplers)
	{
		if (s._samplerState)
		{
			s._samplerState->Release();
			s._samplerState = nullptr;
		}
	}

	for (RenderTarget& rt : m_renderTargets)
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

	for (RasterizerState*& rs : m_rasterizerStates)
	{
		if (rs)
		{
			rs->Release();
			rs = nullptr;
		}
	}

	for (DepthStencilState*& dss : m_depthStencilStates)
	{
		if (dss)
		{
			dss->Release();
			dss = nullptr;
		}
	}

	for (BlendState& bs : m_blendStates)
	{
		if (bs.ptr)
		{
			bs.ptr->Release();
			bs.ptr = nullptr;
		}
	}

	for (DepthTarget& dt : m_depthTargets)
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
	assert(shader_id >= 0 && (int)m_shaders.size() > shader_id);
	return m_shaders[shader_id];
}


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
	m_device		= m_Direct3D->m_device;
	m_deviceContext = m_Direct3D->m_deviceContext;

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

		m_textures.push_back(defaultRT.texture);	// set texture ID by adding it -- TODO: remove duplicate data - don't add texture to vector
		defaultRT.texture._id = static_cast<int>(m_textures.size() - 1);

		m_renderTargets.push_back(defaultRT);
		m_state._mainRenderTarget = static_cast<int>(m_renderTargets.size() - 1);
	}
	m_Direct3D->ReportLiveObjects("Init Default RT\n");


	// DEFAULT DEPTH TARGET
	//--------------------------------------------------------------------
	{
		// Set up the description of the depth buffer.
		const DXGI_FORMAT depthTargetFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		m_state._depthBufferTexture = CreateDepthTexture(settings.width, settings.height, false);
		Texture& depthTexture = static_cast<Texture>(GetTextureObject(m_state._depthBufferTexture));

		// depth stencil view and shader resource view for the shadow map (^ BindFlags)
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = depthTargetFormat;
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
		hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterizerStates[(int)EDefaultRasterizerState::CULL_BACK]);
		if (FAILED(hr))
		{
			Log::Error(err + "Back\n");
		}

		rsDesc.CullMode = D3D11_CULL_FRONT;
		hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterizerStates[(int)EDefaultRasterizerState::CULL_FRONT]);
		if (FAILED(hr))
		{
			Log::Error(err + "Front\n");
		}

		rsDesc.CullMode = D3D11_CULL_NONE;
		hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterizerStates[(int)EDefaultRasterizerState::CULL_NONE]);
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

		m_device->CreateBlendState(&desc, &(m_blendStates[EDefaultBlendState::ADDITIVE_COLOR].ptr));
		m_device->CreateBlendState(&desc, &(m_blendStates[EDefaultBlendState::ALPHA_BLEND].ptr));

		rtBlendDesc.BlendEnable = false;
		desc.RenderTarget[0] = rtBlendDesc;
		m_device->CreateBlendState(&desc, &(m_blendStates[EDefaultBlendState::DISABLED].ptr));
	}
	m_Direct3D->ReportLiveObjects("Init Default BlendStates ");



	// DEFAULT DEPTHSTENCIL SATATES
	//--------------------------------------------------------------------
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
	auto checkFailed = [&](HRESULT hr) 
	{
		if (FAILED(result))
		{
			Log::Error(CANT_CRERATE_RENDER_STATE, "Default Depth Stencil State");
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
	HRESULT hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[EDefaultDepthStencilState::DEPTH_STENCIL_WRITE]);
	if(!checkFailed(hr)) return false;

	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.StencilEnable = false;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED]);
	if (!checkFailed(hr)) return false;

	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.StencilEnable = false;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[EDefaultDepthStencilState::DEPTH_WRITE]);
	if (!checkFailed(hr)) return false;

	depthStencilDesc.DepthEnable = false;
	depthStencilDesc.StencilEnable = true;
	hr = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilStates[EDefaultDepthStencilState::STENCIL_WRITE]);
	if (!checkFailed(hr)) return false;


	// PRIMITIVES
	//--------------------------------------------------------------------
	{
		// cylinder parameters
		const float	 cylHeight = 3.1415f;
		const float	 cylTopRadius = 1.0f;
		const float	 cylBottomRadius = 1.0f;
		const unsigned cylSliceCount = 120;
		const unsigned cylStackCount = 100;

		// grid parameters
		const float gridWidth = 1.0f;
		const float gridDepth = 1.0f;
		const unsigned gridFinenessH = 100;
		const unsigned gridFinenessV = 100;

		// sphere parameters
		const float sphRadius = 2.0f;
		const unsigned sphRingCount = 25;
		const unsigned sphSliceCount = 25;

		GeometryGenerator::SetDevice(m_device);
		m_bufferObjects[TRIANGLE] = GeometryGenerator::Triangle();
		m_bufferObjects[QUAD] = GeometryGenerator::Quad();
		m_bufferObjects[CUBE] = GeometryGenerator::Cube();
		m_bufferObjects[GRID] = GeometryGenerator::Grid(gridWidth, gridDepth, gridFinenessH, gridFinenessV);
		m_bufferObjects[CYLINDER] = GeometryGenerator::Cylinder(cylHeight, cylTopRadius, cylBottomRadius, cylSliceCount, cylStackCount);
		m_bufferObjects[SPHERE] = GeometryGenerator::Sphere(sphRadius, sphRingCount, sphSliceCount);
		m_bufferObjects[BONE] = GeometryGenerator::Sphere(sphRadius / 40, 10, 10);
	}

	// SHADERS
	//--------------------------------------------------------------------
	Shader::LoadShaders(this);
	m_Direct3D->ReportLiveObjects("Shader loaded");

	return true;
}




const PipelineState& Renderer::GetState() const
{
	return m_state;
}

ShaderID Renderer::AddShader(const std::string&	shaderFileName,	const std::vector<InputLayout>& layouts)
{
	const std::vector<std::string> paths = GetShaderPaths(shaderFileName);

	Shader* shader = new Shader(shaderFileName);
	shader->CompileShaders(m_device, paths, layouts);
	m_shaders.push_back(shader);
	shader->m_id = (static_cast<int>(m_shaders.size()) - 1);
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
		paths.push_back(std::string(s_shaderRoot + shaderFileName + ".hlsl"));
	}

	Shader* shader = new Shader(shaderName);
	shader->CompileShaders(m_device, paths, layouts);
	m_shaders.push_back(shader);
	shader->m_id = (static_cast<int>(m_shaders.size()) - 1);
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

	m_rasterizerStates.push_back(newRS);
	return static_cast<RasterizerStateID>(m_rasterizerStates.size() - 1);
}

// example params: "bricks_d.png", "Data/Textures/"
TextureID Renderer::CreateTextureFromFile(const std::string& shdFileName, const std::string& fileRoot /*= s_textureRoot*/)
{
	auto found = std::find_if(m_textures.begin(), m_textures.end(), [&shdFileName](auto& tex) { return tex._name == shdFileName; });
	if (found != m_textures.end())
	{
		return (*found)._id;
	}

	{	// push texture right away and hold a reference
		Texture tex;
		m_textures.push_back(tex);
	}
	Texture& tex = m_textures.back();

	tex._name = shdFileName;
	std::string path = fileRoot + shdFileName;
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
		
		tex._id = static_cast<int>(m_textures.size() - 1);
		//m_textures.push_back(tex);

		return m_textures.back()._id;
	}
	else
	{
		Log::Error("Cannot load texture file\n");
		return m_textures[0]._id;
	}

}

TextureID Renderer::CreateTexture2D(int width, int height, void* data /* = nullptr */)
{
	const DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;

	Texture tex;
	tex._width = width;
	tex._height = height;
	
	D3D11_TEXTURE2D_DESC desc = {};
	desc.Format = format;
	desc.Height = height;
	desc.Width = width;
	desc.ArraySize = 1;
	desc.MipLevels = 1;
	desc.SampleDesc = { 1, 0 };
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA dataDesc = {};
	dataDesc.pSysMem = data;
	dataDesc.SysMemPitch = sizeof(vec4) * width;
	dataDesc.SysMemSlicePitch = 0;

	m_device->CreateTexture2D(&desc, &dataDesc, &tex._tex2D);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srv);

	tex._id = static_cast<int>(m_textures.size());
	m_textures.push_back(tex);
	return m_textures.back()._id;
}

TextureID Renderer::CreateTexture2D(D3D11_TEXTURE2D_DESC & textureDesc, bool initializeSRV)
{
	Texture tex;
	tex.InitializeTexture2D(textureDesc, this, initializeSRV);
	m_textures.push_back(tex);
	m_textures.back()._id = static_cast<int>(m_textures.size() - 1);
	return m_textures.back()._id;
}

TextureID Renderer::CreateCubemapTexture(const std::vector<std::string>& textureFileNames)
{
	constexpr size_t FACE_COUNT = 6;

	// get subresource data for each texture to initialize the cubemap
	D3D11_SUBRESOURCE_DATA pData[FACE_COUNT];
	std::array<DirectX::ScratchImage, FACE_COUNT> faceImages;
	for (int cubeMapFaceIndex = 0; cubeMapFaceIndex < FACE_COUNT; cubeMapFaceIndex++)
	{
		const std::string path = s_textureRoot + textureFileNames[cubeMapFaceIndex];
		const std::wstring wpath(path.begin(), path.end());

		DirectX::ScratchImage* img = &faceImages[cubeMapFaceIndex];
		if (!SUCCEEDED(LoadFromWICFile(wpath.c_str(), WIC_FLAGS_NONE, nullptr, *img)))
		{
			Log::Error(EErrorLog::CANT_OPEN_FILE, textureFileNames[cubeMapFaceIndex]);
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
		Log::Error(std::string("Cannot create cubemap texture: ") + split(textureFileNames.front(), '_').front());
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
		Log::Error(std::string("Cannot create Shader Resource View for ") + split(textureFileNames.front(), '_').front());
		return -1;
	}

	// return param
	Texture cubemapOut;
	cubemapOut._srv = cubeMapSRV;
	cubemapOut._name = "todo:Skybox file name";
	cubemapOut._tex2D = cubemapTexture;
	cubemapOut._height = static_cast<unsigned>(faceImages[0].GetMetadata().height);
	cubemapOut._width  = static_cast<unsigned>(faceImages[0].GetMetadata().width);
	cubemapOut._id = static_cast<int>(m_textures.size());
	m_textures.push_back(cubemapOut);

	return cubemapOut._id;
}

TextureID Renderer::CreateDepthTexture(unsigned width, unsigned height, bool bDepthOnly)
{
	D3D11_TEXTURE2D_DESC depthTextureDescriptor = {};
	depthTextureDescriptor.Format = bDepthOnly ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_R24G8_TYPELESS;
	depthTextureDescriptor.MipLevels = 1;
	depthTextureDescriptor.ArraySize = 1;
	depthTextureDescriptor.SampleDesc.Count = 1;
	depthTextureDescriptor.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	depthTextureDescriptor.Height = height;
	depthTextureDescriptor.Width  = width;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = bDepthOnly ? DXGI_FORMAT_R32_FLOAT : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;	// array maybe? check descriptor.
	srvDesc.Texture2D.MipLevels = 1;

	Texture tex;	
	tex._width  = width;
	tex._height = height;
	m_device->CreateTexture2D(&depthTextureDescriptor, nullptr, &tex._tex2D);;
	m_device->CreateShaderResourceView(tex._tex2D, &srvDesc, &tex._srv);

	m_textures.push_back(tex);
	m_textures.back()._id = static_cast<int>(m_textures.size() - 1);
	return m_textures.back()._id;
}

SamplerID Renderer::CreateSamplerState(D3D11_SAMPLER_DESC & samplerDesc)
{
	ID3D11SamplerState*	pSamplerState;
	HRESULT hr = m_device->CreateSamplerState(&samplerDesc, &pSamplerState);
	if (FAILED(hr))
	{
		Log::Error(EErrorLog::CANT_CREATE_RESOURCE, "Cannot create sampler state\n");
	}

	Sampler out;
	out._id = static_cast<SamplerID>(m_samplers.size());
	out._samplerState = pSamplerState;
	out._name = "";	// ?
	m_samplers.push_back(out);
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
		Log::Error(CANT_CRERATE_RENDER_STATE, "Depth Stencil");
		return false;
	}

	m_depthStencilStates.push_back(newDSState);
	return static_cast<DepthStencilStateID>(m_depthStencilStates.size() - 1);
}

DepthStencilStateID Renderer::AddDepthStencilState(const D3D11_DEPTH_STENCIL_DESC & dsDesc)
{
	DepthStencilState* newDSState = (DepthStencilState*)malloc(sizeof(DepthStencilState));
	HRESULT result;

	result = m_device->CreateDepthStencilState(&dsDesc, &newDSState);
	if (FAILED(result))
	{
		Log::Error(CANT_CRERATE_RENDER_STATE, "Depth Stencil");
		return false;
	}

	m_depthStencilStates.push_back(newDSState);
	return static_cast<DepthStencilStateID>(m_depthStencilStates.size() - 1);
}

RenderTargetID Renderer::AddRenderTarget(D3D11_TEXTURE2D_DESC & RTTextureDesc, D3D11_RENDER_TARGET_VIEW_DESC& RTVDesc)
{
	RenderTarget newRenderTarget;
	newRenderTarget.texture = GetTextureObject(CreateTexture2D(RTTextureDesc, true));
	HRESULT hr = m_device->CreateRenderTargetView(newRenderTarget.texture._tex2D, &RTVDesc, &newRenderTarget.pRenderTargetView);
	if (!SUCCEEDED(hr))
	{
		Log::Error(CANT_CREATE_RESOURCE, "Render Target View");
		return -1;
	}

	m_renderTargets.push_back(newRenderTarget);
	return static_cast<int>(m_renderTargets.size() - 1);
}

DepthTargetID Renderer::AddDepthTarget(const D3D11_DEPTH_STENCIL_VIEW_DESC& dsvDesc, Texture& depthTexture)
{
	DepthTarget newDepthTarget;
	newDepthTarget.pDepthStencilView = (ID3D11DepthStencilView*)malloc(sizeof(*newDepthTarget.pDepthStencilView));
	memset(newDepthTarget.pDepthStencilView, 0, sizeof(*newDepthTarget.pDepthStencilView));

	HRESULT hr = m_device->CreateDepthStencilView(depthTexture._tex2D, &dsvDesc, &newDepthTarget.pDepthStencilView);
	if (FAILED(hr))
	{
		Log::Error(CANT_CREATE_RESOURCE, "Depth Stencil Target View");
		return -1;
	}
	newDepthTarget.texture = depthTexture;
	m_depthTargets.push_back(newDepthTarget);
	return static_cast<int>(m_depthTargets.size() - 1);
}

const Texture& Renderer::GetTextureObject(TextureID id) const
{
	assert(id >= 0 && static_cast<unsigned>(id) < m_textures.size());
	return m_textures[id];
}

const TextureID Renderer::GetTexture(const std::string name) const
{
	auto found = std::find_if(m_textures.begin(), m_textures.end(), [&name](auto& tex) { return tex._name == name; });
	if (found != m_textures.end())
	{
		return found->_id;
	}
	Log::Error("Texture not found: " + name);
	return -1;
}


void Renderer::SetShader(ShaderID id)
{
	assert(id >= 0 && static_cast<unsigned>(id) < m_shaders.size());
	if (m_state._activeShader != -1)		// if valid shader
	{
		if (id != m_state._activeShader)	// if not the same shader
		{
			Shader* shader = m_shaders[m_state._activeShader];

			// nullify texture units 
			for (ShaderTexture& tex : shader->m_textures)
			{
				constexpr UINT NumNullSRV = 6;
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

			ID3D11RenderTargetView* nullRTV[6] = { nullptr };
			ID3D11DepthStencilView* nullDSV = { nullptr };
			m_deviceContext->OMSetRenderTargets(6, nullRTV, nullDSV);

			const float blendFactor[4] = { 1,1,1,1 };
			m_deviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

		} // if not same shader
	}	// if valid shader

	if (id != m_state._activeShader)
	{
		m_state._activeShader = id;
		m_shaders[id]->ClearConstantBuffers();
	}
}

void Renderer::Reset()
{
	m_state._activeShader = -1;
	m_state._activeInputBuffer = -1;
}


void Renderer::SetViewport(const unsigned width, const unsigned height)
{
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Width	= static_cast<float>(width);
	m_viewPort.Height	= static_cast<float>(height);
	m_viewPort.MinDepth = 0;
	m_viewPort.MaxDepth = 1;
}

void Renderer::SetViewport(const D3D11_VIEWPORT & viewport)
{
	m_viewPort = viewport;
}

void Renderer::SetBufferObj(int BufferID)
{
	assert(BufferID >= 0);
	m_state._activeInputBuffer = BufferID;
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
	// Here, we write to the CPU version of the constant buffer -- if the contents are updated 
	// ofc, otherwise we don't write -- and set the dirty flag of the GPU CBuffer counterpart.
	// When all the constants are set on the CPU side, right before the draw call,
	// we will use a mapped resource as a block of memory, transfer our CPU
	// version of constants to there, and then send it to GPU CBuffers at one call as a block memory.
	// Otherwise, we would have to make an API call each time we set the constants, which
	// would be slower.
	// Read more here: https://developer.nvidia.com/sites/default/files/akamai/gamedev/files/gdc12/Efficient_Buffer_Management_McDonald.pdf
	//      and  here: https://developer.nvidia.com/content/constant-buffers-without-constant-pain-0

	Shader* shader = m_shaders[m_state._activeShader];

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
	Shader* shader = m_shaders[m_state._activeShader];
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
			m_setTextureCmds.push(cmd);
		}
	}

#ifdef _DEBUG
	if (!found)
	{
		Log::Error("Texture not found: \"%s\" in Shader(Id=%d) \"%s\"\n", texName, m_state._activeShader, shader->Name().c_str());
	}
#endif
}

void Renderer::SetSamplerState(const char * samplerName, SamplerID samplerID)
{
	Shader* shader = m_shaders[m_state._activeShader];
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
			m_setSamplerCmds.push(cmd);
		}
	}

#ifdef _DEBUG
	if (!found)
	{
		Log::Error("Sampler not found: \"%s\" in Shader(Id=%d) \"%s\"\n", samplerName, m_state._activeShader, shader->Name().c_str());
	}
#endif
}

void Renderer::SetRasterizerState(RasterizerStateID rsStateID)
{
	assert(rsStateID > -1 && static_cast<size_t>(rsStateID) < m_rasterizerStates.size());
	m_state._activeRSState = rsStateID;
}

void Renderer::SetBlendState(BlendStateID blendStateID)
{
	assert(blendStateID > -1 && static_cast<size_t>(blendStateID) < m_blendStates.size());
	m_state._activeBlendState = blendStateID;
}

void Renderer::SetDepthStencilState(DepthStencilStateID depthStencilStateID)
{
	assert(depthStencilStateID > -1 && static_cast<size_t>(depthStencilStateID) < m_depthStencilStates.size());
	m_state._activeDepthStencilState = depthStencilStateID;
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
	assert(rtvID > -1 && static_cast<size_t>(rtvID) < m_renderTargets.size());
	//for(RenderTargetID& hRT : m_state._boundRenderTargets) 
	m_state._boundRenderTargets = { rtvID };
}

void Renderer::BindDepthTarget(DepthTargetID dsvID)
{
	assert(dsvID > -1 && static_cast<size_t>(dsvID) < m_depthTargets.size());
	m_state._boundDepthTarget = dsvID;
}

void Renderer::UnbindRenderTarget()
{
	m_state._boundRenderTargets = { -1, -1, -1, -1, -1, -1 };
}

void Renderer::UnbindDepthTarget()
{
	m_state._boundDepthTarget = -1;
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
	Draw(EPrimitiveTopology::POINT_LIST);
}

void Renderer::DrawLine(const vec3& pos1, const vec3& pos2, const vec3& color)
{
	SetConstant3f("p1", pos1);
	SetConstant3f("p2", pos2);
	SetConstant3f("color", color);
	Apply();
	Draw(EPrimitiveTopology::POINT_LIST);
}

// assumes (0, 0) is Bottom Left corner of the screen.
void Renderer::DrawQuadOnScreen(const DrawQuadOnScreenCommand& cmd)
{																// warning:
	const int screenWidth =  mWindowSettings.width;	// 2 copies of renderer settings, one here on in Engine
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
	SetBufferObj(EGeometry::QUAD);
	Apply();
	DrawIndexed();
}

void Renderer::Begin(const float clearColor[4], const float depthValue)
{
	const DepthTargetID dsv = m_state._boundDepthTarget;	
	for (const auto renderTarget : m_state._boundRenderTargets)
	{
		if(renderTarget>=0)
			m_deviceContext->ClearRenderTargetView(m_renderTargets[renderTarget].pRenderTargetView, clearColor);
	}
	if(dsv >= 0) m_deviceContext->ClearDepthStencilView(m_depthTargets[dsv].pDepthStencilView, D3D11_CLEAR_DEPTH, depthValue, 0);
}

void Renderer::Begin(const ClearCommand & clearCmd)
{
	if (clearCmd.bDoClearColor)
	{
		for (const RenderTargetID rtv : m_state._boundRenderTargets)
		{
			if (rtv >= 0)	m_deviceContext->ClearRenderTargetView(m_renderTargets[rtv].pRenderTargetView, clearCmd.clearColor.data());
			else			Log::Error("Begin called with clear color command without a render target bound to pipeline!");
		}
	}

	const bool bClearDepthStencil = clearCmd.bDoClearDepth || clearCmd.bDoClearStencil;
	if (bClearDepthStencil)
	{
		const DepthTargetID dsv = m_state._boundDepthTarget;
		UINT clearFlag = [&]() -> UINT	{
			if (clearCmd.bDoClearDepth && clearCmd.bDoClearStencil)
				return D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL;

			if (clearCmd.bDoClearDepth)
				return D3D11_CLEAR_DEPTH;

			return D3D11_CLEAR_STENCIL;
		}();

		if (dsv >= 0)	m_deviceContext->ClearDepthStencilView(m_depthTargets[dsv].pDepthStencilView, clearFlag, clearCmd.clearDepth, clearCmd.clearStencil);
		else			Log::Error("Begin called with clear depth_stencil command without a depth target bound to pipeline!");
	}
}

void Renderer::End()
{
	m_Direct3D->EndFrame();
	++m_frameCount;
}


void Renderer::Apply()
{	// Here, we make all the API calls
	Shader* shader = m_state._activeShader >= 0 ? m_shaders[m_state._activeShader] : nullptr;

	// INPUT ASSEMBLY
	// ----------------------------------------
	unsigned stride = sizeof(Vertex);	// layout?
	unsigned offset = 0;
	if (m_state._activeInputBuffer != -1) m_deviceContext->IASetVertexBuffers(0, 1, &m_bufferObjects[m_state._activeInputBuffer]->m_vertexBuffer, &stride, &offset);
	if (m_state._activeInputBuffer != -1) m_deviceContext->IASetIndexBuffer(m_bufferObjects[m_state._activeInputBuffer]->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	if (shader)							  m_deviceContext->IASetInputLayout(shader->m_layout);

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

		while (m_setSamplerCmds.size() > 0)
		{
			SetSamplerCommand& cmd = m_setSamplerCmds.front();
			cmd.SetResource(this);
			m_setSamplerCmds.pop();
		}

		// todo: set texture array?
		while (m_setTextureCmds.size() > 0)
		{
			SetTextureCommand& cmd = m_setTextureCmds.front();
			cmd.SetResource(this);
			m_setTextureCmds.pop();
		}

		// RASTERIZER
		// ----------------------------------------
		m_deviceContext->RSSetViewports(1, &m_viewPort);
		m_deviceContext->RSSetState(m_rasterizerStates[m_state._activeRSState]);

		// OUTPUT MERGER
		// ----------------------------------------
		if (sEnableBlend)
		{
			const float blendFactor[4] = { 1,1,1,1 };
			m_deviceContext->OMSetBlendState(m_blendStates[m_state._activeBlendState].ptr, nullptr, 0xffffffff);
		}

		const auto indexDSState = m_state._activeDepthStencilState;
		const auto indexRTV = m_state._boundRenderTargets[0];
		
		// get the bound render target addresses
#if 1
		// todo: perf: this takes as much time as set constants in debug mode
		std::vector<ID3D11RenderTargetView*> RTVs = [&]() {				
			std::vector<ID3D11RenderTargetView*> v(m_state._boundRenderTargets.size(), nullptr);
			size_t i = 0;
			for (RenderTargetID hRT : m_state._boundRenderTargets) 
				if(hRT >= 0) 
					v[i++] = m_renderTargets[hRT].pRenderTargetView;
			return std::move(v);
		}();
#else
		// this is slower ~2ms in debug
		std::vector<ID3D11RenderTargetView*> RTVs;
		for (RenderTargetID hRT : m_state._boundRenderTargets)
			if (hRT >= 0)
				RTVs.push_back(m_renderTargets[hRT].pRenderTargetView);
#endif
		const auto indexDSV     = m_state._boundDepthTarget;
		//ID3D11RenderTargetView** RTV = indexRTV == -1 ? nullptr : &RTVs[0];
		ID3D11RenderTargetView** RTV = RTVs.empty()   ? nullptr : &RTVs[0];
		ID3D11DepthStencilView*  DSV = indexDSV == -1 ? nullptr : m_depthTargets[indexDSV].pDepthStencilView;

		m_deviceContext->OMSetRenderTargets(RTV ? (unsigned)RTVs.size() : 0, RTV, DSV);
		

		auto*  DSSTATE = m_depthStencilStates[indexDSState];
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
	UnicodeString umarker(marker);
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
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->DrawIndexed(m_bufferObjects[m_state._activeInputBuffer]->m_indexCount, 0, 0);
}

void Renderer::Draw(EPrimitiveTopology topology)
{
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->Draw(1, 0);
}
