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

#include "Renderer.h"
#include "D3DManager.h"
#include "BufferObject.h"
#include "Shader.h"
#include "Light.h"

#include "SystemDefs.h"
#include "utils.h"
#include "Camera.h"
#include "DirectXTex.h"

#include <mutex>
#include <cassert>

#define SHADER_HOTSWAP 0

const char* Renderer::s_shaderRoot = "Data/Shaders/";
const char* Renderer::s_textureRoot = "Data/Textures/";

void DepthShadowPass::Initialize(Renderer* pRenderer, ID3D11Device* device)
{
	_shadowMapDimension = 512;

	// check feature support & error handle:
	// https://msdn.microsoft.com/en-us/library/windows/apps/dn263150

	// create shadow map texture for the pixel shader stage
	D3D11_TEXTURE2D_DESC shadowMapDesc;
	ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
	shadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	shadowMapDesc.MipLevels = 1;
	shadowMapDesc.ArraySize = 1;
	shadowMapDesc.SampleDesc.Count = 1;
	shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	shadowMapDesc.Height = static_cast<UINT>(_shadowMapDimension);
	shadowMapDesc.Width = static_cast<UINT>(_shadowMapDimension);

	// todo, use add texture or something
	Texture& tex = _shadowMap;
	device->CreateTexture2D(
		&shadowMapDesc,
		nullptr,
		&_shadowMap.tex2D
	);

	// depth stencil view and shader resource view for the shadow map (^ BindFlags)
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	pRenderer->AddDepthStencil(dsvDesc, tex.tex2D);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.Texture2D.MipLevels = 1;

	HRESULT hr = device->CreateShaderResourceView(
		tex.tex2D,
		&srvDesc,
		&tex.srv
	);	// succeed hr? 

	// comparison
	D3D11_SAMPLER_DESC comparisonSamplerDesc;
	ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
	comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.BorderColor[0] = 1.0f;
	comparisonSamplerDesc.BorderColor[1] = 1.0f;
	comparisonSamplerDesc.BorderColor[2] = 1.0f;
	comparisonSamplerDesc.BorderColor[3] = 1.0f;
	comparisonSamplerDesc.MinLOD = 0.f;
	comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	comparisonSamplerDesc.MipLODBias = 0.f;
	comparisonSamplerDesc.MaxAnisotropy = 0;
	comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;

	hr = device->CreateSamplerState(&comparisonSamplerDesc, &tex.samplerState);
	// succeed hr?

	// render states for front face culling 
	_shadowRenderState = pRenderer->AddRSState(RS_CULL_MODE::FRONT, RS_FILL_MODE::SOLID, true);
	_drawRenderState   = pRenderer->AddRSState(RS_CULL_MODE::BACK , RS_FILL_MODE::SOLID, true);

	// shader
	std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 }};
	_shadowShader = pRenderer->GetShader(pRenderer->AddShader("Shadow"     , pRenderer->s_shaderRoot, layout, false));
	_shadowShader = pRenderer->GetShader(pRenderer->AddShader("DepthShader", pRenderer->s_shaderRoot, layout, false));
	
	ZeroMemory(&_shadowViewport, sizeof(D3D11_VIEWPORT));
	_shadowViewport.Height = static_cast<float>(_shadowMapDimension);
	_shadowViewport.Width  = static_cast<float>(_shadowMapDimension);
	_shadowViewport.MinDepth = 0.f;
	_shadowViewport.MaxDepth = 1.f;
}

void DepthShadowPass::RenderDepth(Renderer * pRenderer, const std::vector<const Light*> shadowLights) const
{
	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	return;

	pRenderer->UnbindRenderTarget();						// no render target
	pRenderer->BindDepthStencil(_dsv);					// only depth stencil buffer
	pRenderer->SetRasterizerState(_shadowRenderState);		// shadow render state: cull front faces, fill solid, clip dep
	pRenderer->SetViewport(_shadowViewport);				// lights viewport 512x512
	pRenderer->SetShader(_shadowShader->ID());				// shader for rendering z buffer
	pRenderer->SetConstant4x4f("", shadowLights.front()->GetLightSpaceMatrix());
	pRenderer->Apply();
	pRenderer->Begin(clearColor, 1.0f);
	pRenderer->Draw();	// secne objects
}


Renderer::Renderer()
	:
	m_Direct3D(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	m_mainCamera(nullptr),
	   m_bufferObjects(std::vector<BufferObject*>   (MESH_TYPE::MESH_TYPE_COUNT)),
	m_rasterizerStates(std::vector<RasterizerState*>((int)DEFAULT_RS_STATE::RS_COUNT)),
	m_depthStencilStates(std::vector<DepthStencilState*>())
{
	for (int i=0; i<(int)DEFAULT_RS_STATE::RS_COUNT; ++i)
	{
		m_rasterizerStates[i] = (RasterizerState*)malloc(sizeof(*m_rasterizerStates[i]));
		memset(m_rasterizerStates[i], 0, sizeof(*m_rasterizerStates[i]));
	}
}

Renderer::~Renderer(){}

void Renderer::Exit()
{
	// Release the Direct3D object.
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
	}

	// cleanup buffer objects
	for (BufferObject* bo : m_bufferObjects)
	{
		delete bo;
		bo = nullptr;
	}

	for (Shader* shd : m_shaders)
	{
		delete shd;
		shd = nullptr;
	}

	for (Texture& tex : m_textures)
	{
		tex.srv->Release();
		tex.srv = nullptr;
	}

	return;
}

HWND Renderer::GetWindow() const
{
	return m_hWnd;
}

float Renderer::AspectRatio() const
{
	return m_Direct3D->AspectRatio();
}

unsigned Renderer::WindowHeight() const
{
	return m_Direct3D->WindowHeight();
}

unsigned Renderer::WindowWidth() const
{
	return m_Direct3D->WindowWidth();
}

const Shader* Renderer::GetShader(ShaderID shader_id) const
{
	return m_shaders[shader_id];
}


bool Renderer::Initialize(int width, int height, HWND hwnd)
{
	m_hWnd = hwnd;

	m_Direct3D = new D3DManager();
	if (!m_Direct3D)
	{
		assert(false);
		return false;
	}

	bool result = m_Direct3D->Initialize(width, height, Renderer::VSYNC, m_hWnd, FULL_SCREEN);
	if (!result)
	{
		MessageBox(m_hWnd, "Could not initialize Direct3D", "Error", MB_OK);
		return false;
	}
	m_device		= m_Direct3D->GetDevice();
	m_deviceContext = m_Direct3D->GetDeviceContext();

	ID3D11Texture2D* backBufferPtr;
	result = m_Direct3D->m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(result))
	{
		return false;
	}
	AddRenderTarget(backBufferPtr);	// default render target

	// TODO: abstract d3d11 away....

	D3D11_TEXTURE2D_DESC depthBufferDesc;

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = m_Direct3D->m_wndWidth;
	depthBufferDesc.Height = m_Direct3D->m_wndHeight;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;
	
	// Create the texture for the depth buffer using the filled out description.
	result = m_device->CreateTexture2D(&depthBufferDesc, NULL, &m_stateObjects._depthBufferTexture.tex2D);

	// depth stencil view and shader resource view for the shadow map (^ BindFlags)
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	AddDepthStencil(dsvDesc, m_stateObjects._depthBufferTexture.tex2D);
	

	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	depthStencilDesc.StencilEnable = false;
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
	AddDepthStencilState(depthStencilDesc);
	


	InitializeDefaultRasterizerStates();
	GeneratePrimitives();
	LoadShaders();
	return true;
}

void Renderer::GeneratePrimitives()
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
	const unsigned sphRingCount = 40;
	const unsigned sphSliceCount = 40;

	m_geom.SetDevice(m_device);
	m_bufferObjects[TRIANGLE]	= m_geom.Triangle();
	m_bufferObjects[QUAD]		= m_geom.Quad();
	m_bufferObjects[CUBE]		= m_geom.Cube();
	m_bufferObjects[GRID]		= m_geom.Grid(gridWidth, gridDepth, gridFinenessH, gridFinenessV);
	m_bufferObjects[CYLINDER]	= m_geom.Cylinder(cylHeight, cylTopRadius, cylBottomRadius, cylSliceCount, cylStackCount);
	m_bufferObjects[SPHERE]		= m_geom.Sphere(sphRadius, sphRingCount, sphSliceCount);
	m_bufferObjects[BONE]		= m_geom.Sphere(sphRadius/40, 10, 10);
}

void Renderer::LoadShaders()
{
	std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 },
	};
	m_renderData.unlitShader	= AddShader("UnlitTextureColor", s_shaderRoot, layout);
	m_renderData.texCoordShader = AddShader("TextureCoord", s_shaderRoot, layout);
	m_renderData.normalShader	= AddShader("Normal", s_shaderRoot, layout);
	m_renderData.tangentShader	= AddShader("Tangent", s_shaderRoot, layout);
	m_renderData.binormalShader	= AddShader("Binormal", s_shaderRoot, layout);
	m_renderData.phongShader	= AddShader("Forward_Phong", s_shaderRoot, layout);
	m_renderData.lineShader		= AddShader("Line", s_shaderRoot, layout, true);
	m_renderData.TNBShader		= AddShader("TNB", s_shaderRoot, layout, true);
	
	m_renderData.errorTexture	= AddTexture("errTexture.png", s_textureRoot).id;
	m_renderData.exampleTex		= AddTexture("bricks_d.png", s_textureRoot).id;
	m_renderData.exampleNormMap	= AddTexture("bricks_n.png", s_textureRoot).id;
	m_renderData.depthPass.Initialize(this, m_device);
}

void Renderer::PollThread()
{
	// Concerns:
	// separate thread sharing window resources like context and d3d11device
	// might not perform as expected
	// link: https://www.opengl.org/discussion_boards/showthread.php/185980-recompile-the-shader-on-runtime-like-hot-plug-the-new-compiled-shader
	// source: https://msdn.microsoft.com/en-us/library/aa365261(v=vs.85).aspx

	char info[129];
	sprintf_s(info, "Thread here : PollStarted.\n");
	OutputDebugString(info);
	Sleep(800);


	static HANDLE dwChangeHandle;
	DWORD dwWaitStatus;
	LPTSTR lpDir = "Data/Shaders/";

	dwChangeHandle = FindFirstChangeNotification(
		lpDir,                         // directory to watch 
		TRUE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME); // watch file name changes 

	if (dwChangeHandle == INVALID_HANDLE_VALUE)
	{
		OutputDebugString("\n ERROR: FindFirstChangeNotification function failed.\n");
		;// ExitProcess(GetLastError());
	}

	while (TRUE)
	{
		//	Wait for notification.
		OutputDebugString("\nWaiting for notification...\n");

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
				OutputDebugString("\n ERROR: FindNextChangeNotification function failed.\n");
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
}

void Renderer::OnShaderChange(LPTSTR dir)
{
	char info[129];
	sprintf_s(info, "OnShaderChange(%s)\n\n", dir);
	OutputDebugString(info);

	// we know that a change occurred in the 'dir' directory. Read source again
	// works		: create file, delete file
	// doesnt work	: modify file
	// source: https://msdn.microsoft.com/en-us/library/aa365261(v=vs.85).aspx
}

void Renderer::PollShaderFiles()
{
#if (SHADER_HOTSWAP != 0)
	static bool pollStarted = false;
	if (!pollStarted)
	{
		std::thread t1(&Renderer::PollThread, this);
		t1.detach();	// dont wait for t1 to finish;

		OutputDebugString("poll started main thread\n");
		pollStarted = true;
	}
#endif
}

void Renderer::InitializeDefaultRasterizerStates()
{
	HRESULT hr;
	char info[129];
	ID3D11RasterizerState*& cullNone  = m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_NONE];
	ID3D11RasterizerState*& cullBack  = m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_BACK];
	ID3D11RasterizerState*& cullFront = m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_FRONT];
	
	// MSDN: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476198(v=vs.85).aspx
	D3D11_RASTERIZER_DESC rsDesc;
	ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));

	rsDesc.FillMode					= D3D11_FILL_SOLID;
	rsDesc.FrontCounterClockwise	= false;
	rsDesc.DepthBias				= 0;
	rsDesc.ScissorEnable			= false;
	rsDesc.DepthBiasClamp			= 0;
	rsDesc.SlopeScaledDepthBias		= 0.0f;
	rsDesc.DepthClipEnable			= true;
	rsDesc.AntialiasedLineEnable	= true;
	rsDesc.MultisampleEnable		= true;
	
	rsDesc.CullMode = D3D11_CULL_BACK;
	hr = m_device->CreateRasterizerState(&rsDesc, &cullBack);
	if (FAILED(hr))
	{
		sprintf_s(info, "Error creating Rasterizer State: Cull Back\n");
		OutputDebugString(info);
	}

	rsDesc.CullMode = D3D11_CULL_FRONT;
	hr = m_device->CreateRasterizerState(&rsDesc, &cullFront);
	if (FAILED(hr))
	{
		sprintf_s(info, "Error creating Rasterizer State: Cull Front\n");
		OutputDebugString(info);
	}

	rsDesc.CullMode = D3D11_CULL_NONE;
	hr = m_device->CreateRasterizerState(&rsDesc, &cullNone);
	if (FAILED(hr))
	{
		sprintf_s(info, "Error creating Rasterizer State: Cull None\n");
		OutputDebugString(info);
	}
}

ShaderID Renderer::AddShader(const std::string& shdFileName, 
							 const std::string& fileRoot,
							 const std::vector<InputLayout>& layouts,
							 bool geoShader /*= false*/)
{
	// example params: "TextureCoord", "Data/Shaders/"
	std::string path = fileRoot + shdFileName;

	Shader* shader = new Shader(shdFileName);
	shader->Compile(m_device, path, layouts, geoShader);
	m_shaders.push_back(shader);
	shader->AssignID(static_cast<int>(m_shaders.size()) - 1);
	return shader->ID();
}

RasterizerStateID Renderer::AddRSState(RS_CULL_MODE cullMode, RS_FILL_MODE fillMode, bool enableDepthClip)
{
	D3D11_RASTERIZER_DESC RSDesc;
	ZeroMemory(&RSDesc, sizeof(D3D11_RASTERIZER_DESC));

	RSDesc.CullMode = static_cast<D3D11_CULL_MODE>(cullMode);
	RSDesc.FillMode = static_cast<D3D11_FILL_MODE>(fillMode);
	RSDesc.DepthClipEnable = enableDepthClip;
	// todo: add params, scissors, multisample, antialiased line
	

	ID3D11RasterizerState* newRS;
	int hr = m_device->CreateRasterizerState(&RSDesc, &newRS);
	if (!SUCCEEDED(hr))
	{
		assert(false);
		// todo
	}

	m_rasterizerStates.push_back(newRS);
	return static_cast<RasterizerStateID>(m_rasterizerStates.size() - 1);
}

// assumes unique shader file names (even in different folders)
const Texture& Renderer::AddTexture(const std::string& texFileName, const std::string& fileRoot /*= s_textureRoot*/)
{
	// example params: "bricks_d.png", "Data/Textures/"
	std::string path = fileRoot + texFileName;
	std::wstring wpath(path.begin(), path.end());

	auto found = std::find_if(m_textures.begin(), m_textures.end(), [texFileName](auto& tex) { return tex.name == texFileName; });
	if (found != m_textures.end())
	{
		//OutputDebugString("found\n\n");
		//return found->id;
		return *found;
	}

	Texture tex;
	tex.name = texFileName;

	std::unique_ptr<DirectX::ScratchImage> img = std::make_unique<DirectX::ScratchImage>();
	if (SUCCEEDED(LoadFromWICFile(wpath.c_str(), WIC_FLAGS_NONE, nullptr, *img)))
	{
		CreateShaderResourceView(m_device, img->GetImages(), img->GetImageCount(), img->GetMetadata(), &tex.srv);

		// get srv from img
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		tex.srv->GetDesc(&srvDesc);
		
		// read width & height
		ID3D11Resource* resource = nullptr;
		tex.srv->GetResource(&resource);
		ID3D11Texture2D* tex2D = nullptr;
		if (SUCCEEDED(resource->QueryInterface(&tex2D)))
		{
			D3D11_TEXTURE2D_DESC desc;
			tex2D->GetDesc(&desc);
			tex.width = desc.Width;
			tex.height = desc.Height;
			tex.tex2D = tex2D;
		}

		tex.samplerState = nullptr;		// default? todo

		tex.id = static_cast<int>(m_textures.size());
		m_textures.push_back(tex);
		return m_textures.back();
	}
	else
	{
		OutputDebugString("Error loading texture file\n");
		return m_textures[0];
	}

}

DepthStencilStateID Renderer::AddDepthStencilState()
{
	DepthStencilState* newDSState = (DepthStencilState*)malloc(sizeof(DepthStencilState));

	HRESULT result;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

	depthStencilDesc.StencilEnable = false;
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
		return false;
	}

	m_depthStencilStates.push_back(newDSState);
	return static_cast<DepthStencilStateID>(m_depthStencilStates.size() - 1);
}

RenderTargetID Renderer::AddRenderTarget(ID3D11Texture2D*& surface)
{
	RenderTarget* newRTV;	// is malloc necessary?
	newRTV = (RenderTarget*)malloc(sizeof(RenderTarget));
	memset(newRTV, 0, sizeof(*newRTV));

	HRESULT hr = m_device->CreateRenderTargetView(
		surface,
		nullptr,
		&newRTV
	);	// succeed hr ?

	m_renderTargets.push_back(newRTV);
	return static_cast<int>(m_renderTargets.size() - 1);
}

DepthStencilID Renderer::AddDepthStencil(const D3D11_DEPTH_STENCIL_VIEW_DESC& dsvDesc, ID3D11Texture2D*& surface)
{
	DepthStencil* newDSV; 
	newDSV =  (DepthStencil*)malloc(sizeof(DepthStencil));
	memset(newDSV, 0, sizeof(*newDSV));

	HRESULT hr = m_device->CreateDepthStencilView(
		surface,
		&dsvDesc,
		&newDSV
	);	// succeed hr ?

	m_depthStencils.push_back(newDSV);
	return static_cast<int>(m_depthStencils.size() - 1);
}

const Texture& Renderer::GetTexture(TextureID id) const
{
	assert(id >= 0 && static_cast<unsigned>(id) < m_textures.size());
	return m_textures[id];
}


const ShaderID Renderer::GetLineShader() const
{
	return m_renderData.lineShader;
}

void Renderer::SetShader(ShaderID id)
{
	// boundary check
	assert(id >= 0 && static_cast<unsigned>(id) < m_shaders.size());
	if (m_stateObjects._activeShader != -1)		// if valid shader
	{
		if (id != m_stateObjects._activeShader)	// if not the same shader
		{
			Shader* shader = m_shaders[m_stateObjects._activeShader];

			// nullify texture units
			for (ShaderTexture& tex : shader->m_textures)
			{
				ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
				switch (tex.shdType)
				{
				case ShaderType::VS:
					m_deviceContext->VSSetShaderResources(tex.bufferSlot, 1, nullSRV);
					break;
				case ShaderType::GS:
					m_deviceContext->GSSetShaderResources(tex.bufferSlot, 1, nullSRV);
					break;
				case ShaderType::HS:
					m_deviceContext->HSSetShaderResources(tex.bufferSlot, 1, nullSRV);
					break;
				case ShaderType::DS:
					m_deviceContext->DSSetShaderResources(tex.bufferSlot, 1, nullSRV);
					break;
				case ShaderType::PS:
					m_deviceContext->PSSetShaderResources(tex.bufferSlot, 1, nullSRV);
					break;
				case ShaderType::CS:
					m_deviceContext->CSSetShaderResources(tex.bufferSlot, 1, nullSRV);
					break;
				default:
					break;
				}
			}
		} // if not same shader
	}	// if valid shader
	else
	{
		;// OutputDebugString("Warning: invalid shader is active\n");
	}

	if (id != m_stateObjects._activeShader)
	{
		m_stateObjects._activeShader = id;
		m_shaders[id]->ClearConstantBuffers();
	}
}

void Renderer::Reset()
{
	m_stateObjects._activeShader = -1;
	m_stateObjects._activeBuffer = -1;
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
	m_stateObjects._activeBuffer = BufferID;
}



void Renderer::SetCamera(Camera* cam)
{
	m_mainCamera = cam;
}

void Renderer::SetConstant4x4f(const char* cName, const XMMATRIX& matrix)
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

	// prepare data to be copied
	XMFLOAT4X4 m;
	XMStoreFloat4x4(&m, matrix);
	float* data = &m.m[0][0];

	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_stateObjects._activeShader];
	bool found = false;
	for (size_t i = 0; i < shader->m_constants.size() && !found; i++)
	{
		std::vector<CPUConstant>& cVector = shader->m_constants[i];
		for (CPUConstant& c : cVector)
		{
			if (strcmp(cName, c.name.c_str()) == 0)
			{
				found = true;
				if (memcmp(c.data, data, c.size) != 0)	// copy data if its not the same
				{
					memcpy(c.data, data, c.size);
					shader->m_cBuffers[i].dirty = true;
					break;
				}
			}
		}
	}

#ifdef DEBUG_LOG
	if (!found)
	{
		std::string err("Error: Constant not found: "); err += cName; err += "\n";
		OutputDebugString(err.c_str());
	}
#endif
}

// TODO: this is the same as 4x4. rethink set constant function
void Renderer::SetConstant3f(const char * cName, const vec3 & float3)
{
	const float* data = &float3.x();
	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_stateObjects._activeShader];
	bool found = false;
	for (size_t i = 0; i < shader->m_constants.size() && !found; i++)	// for each cbuffer
	{
		std::vector<CPUConstant>& cVector = shader->m_constants[i];
		for (CPUConstant& c : cVector)					// for each constant in a cbuffer
		{
			if (strcmp(cName, c.name.c_str()) == 0)		// if name matches
			{
				found = true;
				if (memcmp(c.data, data, c.size) != 0)	// copy data if its not the same
				{
					memcpy(c.data, data, c.size);
					shader->m_cBuffers[i].dirty = true;
					//break;	// ensures write on first occurance
				}
			}
		}
	}
#ifdef DEBUG_LOG
	if (!found)
	{
		char err[256];
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_stateObjects._activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

void Renderer::SetConstant1f(const char* cName, const float f)
{
	const float* data = &f;
	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_stateObjects._activeShader];
	bool found = false;
	for (size_t i = 0; i < shader->m_constants.size() && !found; i++)	// for each cbuffer
	{
		std::vector<CPUConstant>& cVector = shader->m_constants[i];
		for (CPUConstant& c : cVector)					// for each constant in a cbuffer
		{
			if (strcmp(cName, c.name.c_str()) == 0)		// if name matches
			{
				found = true;
				if (memcmp(c.data, data, c.size) != 0)	// copy data if its not the same
				{
					memcpy(c.data, data, c.size);
					shader->m_cBuffers[i].dirty = true;
					//break;	// ensures write on first occurance
				}
			}
		}
	}

#ifdef DEBUG_LOG
	if (!found)
	{
		char err[256];
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_stateObjects._activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

void Renderer::SetConstant1i(const char* cName, const int i)
{
	const int* data = &i;
	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_stateObjects._activeShader];
	bool found = false;
	for (size_t i = 0; i < shader->m_constants.size() && !found; i++)	// for each cbuffer
	{
		std::vector<CPUConstant>& cVector = shader->m_constants[i];
		for (CPUConstant& c : cVector)					// for each constant in a cbuffer
		{
			if (strcmp(cName, c.name.c_str()) == 0)		// if name matches
			{
				found = true;
				if (memcmp(c.data, data, c.size) != 0)	// copy data if its not the same
				{
					memcpy(c.data, data, c.size);
					shader->m_cBuffers[i].dirty = true;
					//break;	// ensures write on first occurance
				}
			}
		}
	}

#ifdef DEBUG_LOG
	if (!found)
	{
		char err[256];
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_stateObjects._activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

// TODO: this is the same as 4x4. rethink set constant function
void Renderer::SetConstantStruct(const char * cName, void* data)
{
	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_stateObjects._activeShader];
	bool found = false;
	for (size_t i = 0; i < shader->m_constants.size() && !found; i++)	// for each cbuffer
	{
		std::vector<CPUConstant>& cVector = shader->m_constants[i];
		for (CPUConstant& c : cVector)					// for each constant in a cbuffer
		{
			if (strcmp(cName, c.name.c_str()) == 0)		// if name matches
			{
				found = true;
				if (memcmp(c.data, data, c.size) != 0)	// copy data if its not the same
				{
					memcpy(c.data, data, c.size);
					shader->m_cBuffers[i].dirty = true;
					//break;	// ensures write on first occurance
				}
			}
		}
	}

#ifdef DEBUG_LOG
	if (!found)
	{
		char err[256];
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_stateObjects._activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

void Renderer::SetTexture(const char * texName, TextureID tex)
{
	Shader* shader = m_shaders[m_stateObjects._activeShader];
	bool found = false;

	for (size_t i = 0; i < shader->m_textures.size(); ++i)
	{
		// texture found
		if (strcmp(texName, shader->m_textures[i].name.c_str()) == 0)
		{
			found = true;
			TextureSetCommand cmd;
			cmd.texID = tex;
			cmd.shdTex = shader->m_textures[i];
			m_texSetCommands.push(cmd);
		}
	}

#ifdef DEBUG_LOG
	if (!found)
	{
		char err[256];
		sprintf_s(err, "Error: Texture not found: \"%s\" in Shader(Id=%d) \"%s\"\n", texName, m_stateObjects._activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

void Renderer::SetRasterizerState(RasterizerStateID rsStateID)
{
	assert(rsStateID > -1 && static_cast<size_t>(rsStateID) < m_rasterizerStates.size());
	m_stateObjects._activeRSState = rsStateID;
}

void Renderer::SetDepthStencilState(DepthStencilStateID depthStencilStateID)
{
	assert(depthStencilStateID > -1 && static_cast<size_t>(depthStencilStateID) < m_depthStencilStates.size());
	m_stateObjects._activeDepthStencilState = depthStencilStateID;
}

void Renderer::BindRenderTarget(RenderTargetID rtvID)
{
	assert(rtvID > -1 && static_cast<size_t>(rtvID) < m_renderTargets.size());
	m_stateObjects._boundRenderTarget = rtvID;
}

void Renderer::BindDepthStencil(DepthStencilID dsvID)
{
	assert(dsvID > -1 && static_cast<size_t>(dsvID) < m_depthStencils.size());
	m_stateObjects._boundDepthStencil = dsvID;
}

void Renderer::UnbindRenderTarget()
{
	m_stateObjects._boundRenderTarget = -1;
}

void Renderer::UnbindDepthStencil()
{
	m_stateObjects._boundDepthStencil = -1;
}

// temp
void Renderer::DrawLine()
{
	// draw line between 2 coords
	vec3 pos1 = vec3(0, 0, 0);
	vec3 pos2 = pos1;	pos2.x() += 5.0f;

	SetConstant3f("p1", pos1);
	SetConstant3f("p2", pos2);
	SetConstant3f("color", Color::green.Value());
	Apply();
	Draw(TOPOLOGY::POINT_LIST);
}

void Renderer::DrawLine(const vec3& pos1, const vec3& pos2, const vec3& color)
{
	SetConstant3f("p1", pos1);
	SetConstant3f("p2", pos2);
	SetConstant3f("color", color);
	Apply();
	Draw(TOPOLOGY::POINT_LIST);
}

// todo: add stencil view params
void Renderer::Begin(const float clearColor[4], const float depthValue)
{
	const RenderTargetID rtv = m_stateObjects._boundRenderTarget;
	const DepthStencilID dsv = m_stateObjects._boundDepthStencil;
	if(rtv >= 0) m_deviceContext->ClearRenderTargetView(m_renderTargets[rtv], clearColor);
	if(dsv >= 0) m_deviceContext->ClearDepthStencilView(m_depthStencils[dsv], D3D11_CLEAR_DEPTH, depthValue, 0);
}

void Renderer::End()
{
	m_Direct3D->EndFrame();
	++m_frameCount;
}


void Renderer::Apply()
{	// Here, we make all the API calls
	Shader* shader = m_stateObjects._activeShader >= 0 ? m_shaders[m_stateObjects._activeShader] : nullptr;
	
	// TODO: check if state is changed

	// INPUT ASSEMBLY
	// ----------------------------------------
	unsigned stride = sizeof(Vertex);	// layout?
	unsigned offset = 0;
	if (m_stateObjects._activeBuffer != -1) m_deviceContext->IASetVertexBuffers(0, 1, &m_bufferObjects[m_stateObjects._activeBuffer]->m_vertexBuffer, &stride, &offset);
	//else OutputDebugString("Warning: no active buffer object (-1)\n");
	if (m_stateObjects._activeBuffer != -1) m_deviceContext->IASetIndexBuffer(m_bufferObjects[m_stateObjects._activeBuffer]->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	//else OutputDebugString("Warning: no active buffer object (-1)\n");
	if(shader) m_deviceContext->IASetInputLayout(shader->m_layout);


	// RASTERIZER
	// ----------------------------------------
	m_deviceContext->RSSetViewports(1, &m_viewPort);
	m_deviceContext->RSSetState(m_rasterizerStates[m_stateObjects._activeRSState]);


	// OUTPUT MERGER
	// ----------------------------------------
	const auto indexDSState = m_stateObjects._activeDepthStencilState;
	const auto indexRTV = m_stateObjects._boundRenderTarget;
	const auto indexDSV = m_stateObjects._boundDepthStencil;
	auto** RTV = indexRTV == -1 ? nullptr : &m_renderTargets[indexRTV];
	auto*  DSV = indexDSV == -1 ? nullptr :  m_depthStencils[indexDSV];
	auto*  DSSTATE = m_depthStencilStates[indexDSV];
	m_deviceContext->OMSetDepthStencilState(DSSTATE, 0);
	m_deviceContext->OMSetRenderTargets(1, RTV, DSV);


	// SHADER STAGES
	// ----------------------------------------
	if (shader)
	{
		m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
		m_deviceContext->PSSetShader(shader->m_pixelShader , nullptr, 0);
		/*if (shader->m_geoShader)*/	 m_deviceContext->GSSetShader(shader->m_geoShader    , nullptr, 0);
		if (shader->m_hullShader)	 m_deviceContext->HSSetShader(shader->m_hullShader   , nullptr, 0);
		if (shader->m_domainShader)	 m_deviceContext->DSSetShader(shader->m_domainShader , nullptr, 0);
		if (shader->m_computeShader) m_deviceContext->CSSetShader(shader->m_computeShader, nullptr, 0);


		// CONSTANT BUFFERRS & SHADER RESOURCES
		// ----------------------------------------
		for (unsigned i = 0; i < shader->m_cBuffers.size(); ++i)
		{
			D3DCBuffer& cbuf = shader->m_cBuffers[i];
			if (cbuf.dirty)	// if the CPU-side buffer is updated
			{
				ID3D11Buffer* bufferData = cbuf.data;
				D3D11_MAPPED_SUBRESOURCE mappedResource;

				// Map sub-resource to GPU - update contends - discard the sub-resource
				m_deviceContext->Map(bufferData, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
				char* bufferPos = static_cast<char*>(mappedResource.pData);	// char* so we can advance the pointer
				std::vector<CPUConstant>& cpuConsts = shader->m_constants[i];
				for (CPUConstant& c : cpuConsts)
				{
					memcpy(bufferPos, c.data, c.size);
					bufferPos += c.size;
				}

				// rethink packing;
				//size_t prevSize = 0;
				//for (int i = 0; i < cpuConsts.size(); ++i)
				//{
				//	CPUConstant& c = cpuConsts[i];
				//	memcpy(bufferPos, c.data, c.size);
				//	bufferPos += c.size;

				//	if (i > 0)
				//	{
				//		if ((prevSize + c.size) % 16);
				//	}

				//	prevSize = c.size;
				//}

				m_deviceContext->Unmap(bufferData, 0);

				// TODO: research update sub-resource (Setting constant buffer can be done once in setting the shader, see it)
				switch (cbuf.shdType)
				{
				case ShaderType::VS:
					m_deviceContext->VSSetConstantBuffers(cbuf.bufferSlot, 1, &bufferData);
					break;
				case ShaderType::PS:
					m_deviceContext->PSSetConstantBuffers(cbuf.bufferSlot, 1, &bufferData);
					break;
				case ShaderType::GS:
					m_deviceContext->GSSetConstantBuffers(cbuf.bufferSlot, 1, &bufferData);
					break;
				case ShaderType::DS:
					m_deviceContext->DSSetConstantBuffers(cbuf.bufferSlot, 1, &bufferData);
					break;
				case ShaderType::HS:
					m_deviceContext->HSSetConstantBuffers(cbuf.bufferSlot, 1, &bufferData);
					break;
				case ShaderType::CS:
					m_deviceContext->CSSetConstantBuffers(cbuf.bufferSlot, 1, &bufferData);
					break;
				default:
					OutputDebugString("ERROR: Renderer::Apply() - UNKOWN Shader Type\n");
					break;
				}

				cbuf.dirty = false;
			}
		}


		while (m_texSetCommands.size() > 0)
		{
			TextureSetCommand& cmd = m_texSetCommands.front();
			switch (cmd.shdTex.shdType)
			{
			case ShaderType::VS:
				m_deviceContext->VSSetShaderResources(cmd.shdTex.bufferSlot, 1, &m_textures[cmd.texID].srv);
				break;
			case ShaderType::GS:
				m_deviceContext->GSSetShaderResources(cmd.shdTex.bufferSlot, 1, &m_textures[cmd.texID].srv);
				break;
			case ShaderType::DS:
				m_deviceContext->DSSetShaderResources(cmd.shdTex.bufferSlot, 1, &m_textures[cmd.texID].srv);
				break;
			case ShaderType::HS:
				m_deviceContext->HSSetShaderResources(cmd.shdTex.bufferSlot, 1, &m_textures[cmd.texID].srv);
				break;
			case ShaderType::CS:
				m_deviceContext->CSSetShaderResources(cmd.shdTex.bufferSlot, 1, &m_textures[cmd.texID].srv);
				break;
			case ShaderType::PS:
				m_deviceContext->PSSetShaderResources(cmd.shdTex.bufferSlot, 1, &m_textures[cmd.texID].srv);
				break;
			default:
				break;
			}

			m_texSetCommands.pop();
		}
	}
}

void Renderer::DrawIndexed(TOPOLOGY topology)
{
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->DrawIndexed(m_bufferObjects[m_stateObjects._activeBuffer]->m_indexCount, 0, 0);
}

void Renderer::Draw(TOPOLOGY topology)
{
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->Draw(1, 0);
}