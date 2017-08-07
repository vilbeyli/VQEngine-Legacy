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

#include "GeometryGenerator.h"
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

const char* Renderer::s_shaderRoot = "Data/Shaders/";
const char* Renderer::s_textureRoot = "Data/Textures/";

Renderer::Renderer()
	:
	m_Direct3D(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	m_mainCamera(nullptr),
	m_bufferObjects     (std::vector<BufferObject*>     (GEOMETRY::MESH_TYPE_COUNT)      ),
	m_rasterizerStates  (std::vector<RasterizerState*>  ((int)DEFAULT_RS_STATE::RS_COUNT)),
	m_depthStencilStates(std::vector<DepthStencilState*>()                               )
	//,	m_ShaderHotswapPollWatcher("ShaderHotswapWatcher")
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
	// C-style resource release - not using smart pointers
	// todo: compare performance

	//m_Direct3D->ReportLiveObjects("BEGIN EXIT");
	for (BufferObject*& bo : m_bufferObjects)
	{
		delete bo;
		bo = nullptr;
	}
	m_bufferObjects.clear();

	CPUConstant::CleanUp();
	
	for (Shader*& shd : m_shaders)
	{
		delete shd;
		shd = nullptr;
	}
	m_shaders.clear();

	for (Texture& tex : m_textures)
	{
		tex.Release();
	}
	m_textures.clear();
	m_state._depthBufferTexture.Release();

#if 1
	for (Sampler& s : m_samplers)
	{
		if (s._samplerState)
		{
			s._samplerState->Release();
			s._samplerState = nullptr;
		}
	}

	for (RenderTarget& rs : m_renderTargets)
	{
		if (rs._renderTargetView)
		{
			rs._renderTargetView->Release();
			rs._renderTargetView = nullptr;
		}
		if (rs._texture._srv)
		{
			//rs._texture._srv->Release();
			rs._texture._srv = nullptr;
		}
		if (rs._texture._tex2D)
		{
			//rs._texture._tex2D->Release();
			rs._texture._tex2D = nullptr;
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

	for (DepthStencil*& ds : m_depthStencils)
	{
		if (ds)
		{
			ds->Release();
			ds = nullptr;
		}
	}
#endif

	m_Direct3D->ReportLiveObjects("END EXIT\n");;
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
		m_Direct3D = nullptr;
	}

	Log::Info("---------------------------\n");
}

const Shader* Renderer::GetShader(ShaderID shader_id) const
{
	assert(shader_id >= 0 && (int)m_shaders.size() > shader_id);
	return m_shaders[shader_id];
}


bool Renderer::Initialize(HWND hwnd, const Settings::Window& settings)
{

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
	);
	
	if (!result)
	{
		MessageBox(hwnd, "Could not initialize Direct3D", "Error", MB_OK);
		return false;
	}
	m_device		= m_Direct3D->m_device;
	m_deviceContext = m_Direct3D->m_deviceContext;

	InitializeDefaultRenderTarget();
	m_Direct3D->ReportLiveObjects("Init Default RT\n");

	InitializeDefaultDepthBuffer();
	m_Direct3D->ReportLiveObjects("Init Depth Buffer\n");

	InitializeDefaultRasterizerStates();
	m_Direct3D->ReportLiveObjects("Init Default RS ");


	GeneratePrimitives();
	LoadShaders();
	m_Direct3D->ReportLiveObjects("Shader loaded");

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

	GeometryGenerator::SetDevice(m_device);
	m_bufferObjects[TRIANGLE]	= GeometryGenerator::Triangle();
	m_bufferObjects[QUAD]		= GeometryGenerator::Quad();
	m_bufferObjects[CUBE]		= GeometryGenerator::Cube();
	m_bufferObjects[GRID]		= GeometryGenerator::Grid(gridWidth, gridDepth, gridFinenessH, gridFinenessV);
	m_bufferObjects[CYLINDER]	= GeometryGenerator::Cylinder(cylHeight, cylTopRadius, cylBottomRadius, cylSliceCount, cylStackCount);
	m_bufferObjects[SPHERE]		= GeometryGenerator::Sphere(sphRadius, sphRingCount, sphSliceCount);
	m_bufferObjects[BONE]		= GeometryGenerator::Sphere(sphRadius/40, 10, 10);
}

void Renderer::LoadShaders()
{
	Log::Info("\r------------------------ COMPILING SHADERS ------------------------ \n");
	
	// todo: layouts from reflection?
	const std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 },
	};
	Shader::s_shaders[SHADERS::FORWARD_PHONG      ] = AddShader("Forward_Phong"    , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::UNLIT              ] = AddShader("UnlitTextureColor", s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::TEXTURE_COORDINATES] = AddShader("TextureCoord"     , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::NORMAL             ]	= AddShader("Normal"           , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::TANGENT            ] = AddShader("Tangent"          , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::BINORMAL           ]	= AddShader("Binormal"         , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::LINE               ]	= AddShader("Line"             , s_shaderRoot, layout, true);
	Shader::s_shaders[SHADERS::TBN                ]	= AddShader("TNB"              , s_shaderRoot, layout, true);
	Shader::s_shaders[SHADERS::DEBUG              ]	= AddShader("Debug"            , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::SKYBOX             ]	= AddShader("Skybox"           , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::BLOOM              ]	= AddShader("Bloom"            , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::BLUR               ]	= AddShader("Blur"             , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::BLOOM_COMBINE      ]	= AddShader("BloomCombine"     , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::TONEMAPPING        ]	= AddShader("Tonemapping"      , s_shaderRoot, layout);
	Shader::s_shaders[SHADERS::FORWARD_BRDF       ]	= AddShader("Forward_BRDF"     , s_shaderRoot, layout);
	Log::Info("\r---------------------- COMPILING SHADERS DONE ---------------------\n");
}

void Renderer::ReloadShaders()
{
	Log::Info("Reloading Shaders...");



	Log::Info("Done");
}

void Renderer::InitializeDefaultDepthBuffer()
{
	// Initialize the description of the depth buffer.
	D3D11_TEXTURE2D_DESC depthBufferDesc;
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
	HRESULT result = m_device->CreateTexture2D(&depthBufferDesc, NULL, &m_state._depthBufferTexture._tex2D);

	// depth stencil view and shader resource view for the shadow map (^ BindFlags)
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	AddDepthStencil(dsvDesc, m_state._depthBufferTexture._tex2D);


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
}


void Renderer::InitializeDefaultRasterizerStates()
{
	HRESULT hr;
	const std::string err("Unable to create Rasterizer State: Cull ");

	//ID3D11RasterizerState*& cullNone  = m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_NONE];
	//ID3D11RasterizerState*& cullBack  = m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_BACK];
	//ID3D11RasterizerState*& cullFront = m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_FRONT];
	
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
	//hr = m_device->CreateRasterizerState(&rsDesc, &cullBack);
	hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_BACK]);
	if (FAILED(hr))
	{
		Log::Error(err + "Back\n");
	}

	rsDesc.CullMode = D3D11_CULL_FRONT;
	//hr = m_device->CreateRasterizerState(&rsDesc, &cullFront);
	hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_FRONT]);
	if (FAILED(hr))
	{
		Log::Error(err + "Front\n");
	}

	rsDesc.CullMode = D3D11_CULL_NONE;
	//hr = m_device->CreateRasterizerState(&rsDesc, &cullNone);
	hr = m_device->CreateRasterizerState(&rsDesc, &m_rasterizerStates[(int)DEFAULT_RS_STATE::CULL_NONE]);
	if (FAILED(hr))
	{
		Log::Error(err + "None\n");
	}
}

ShaderID Renderer::AddShader(const std::string& shdFileName, 
							 const std::string& fileRoot,
							 const std::vector<InputLayout>& layouts,
							 bool geoShader /*= false*/)
{
	const std::string path = fileRoot + shdFileName;

	Shader* shader = new Shader(shdFileName);
#if 0
	if (shdFileName == "Forward_BRDF")
		shader->Compile(m_device, path, layouts, geoShader);
#else
	shader->Compile(m_device, path, layouts, geoShader);
#endif
	m_shaders.push_back(shader);
	shader->m_id = (static_cast<int>(m_shaders.size()) - 1);
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
		Log::Error("Cannot create Rasterizer State");
		return -1;
	}

	m_rasterizerStates.push_back(newRS);
	return static_cast<RasterizerStateID>(m_rasterizerStates.size() - 1);
}

// assumes unique shader file names (even in different folders)
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

const Texture & Renderer::CreateTexture2D(int widht, int height)
{
	Texture tex;
	assert(false); // todo
	//m_device->CreateTexture2D(
	//	&DESC,
	//	nullptr,
	//	&tex.tex2D
	//);

	m_textures.push_back(tex);
	return m_textures.back();
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
			Log::Error(ERROR_LOG::CANT_OPEN_FILE, textureFileNames[cubeMapFaceIndex]);
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

SamplerID Renderer::CreateSamplerState(D3D11_SAMPLER_DESC & samplerDesc)
{
	ID3D11SamplerState*	pSamplerState;
	HRESULT hr = m_device->CreateSamplerState(&samplerDesc, &pSamplerState);
	if (FAILED(hr))
	{
		Log::Error(ERROR_LOG::CANT_CREATE_RESOURCE, "Cannot create sampler state\n");
	}

	Sampler out;
	out._id = static_cast<SamplerID>(m_samplers.size());
	out._samplerState = pSamplerState;
	out._name = "";	// ?
	m_samplers.push_back(out);
	return out._id;
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

void Renderer::InitializeDefaultRenderTarget()
{
	RenderTarget defaultRT;

	ID3D11Texture2D* backBufferPtr;
	HRESULT hr = m_Direct3D->m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	if (FAILED(hr))
	{
		Log::Error("Cannot get back buffer pointer in DefaultRenderTarget initialization");
		return;
	}
	defaultRT._texture._tex2D = backBufferPtr;
	
	D3D11_TEXTURE2D_DESC texDesc;		// get back buffer description
	backBufferPtr->GetDesc(&texDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;	// create shader resource view from back buffer desc
	srvDesc.Format = texDesc.Format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	m_device->CreateShaderResourceView(backBufferPtr, &srvDesc, &defaultRT._texture._srv);
		
	hr = m_device->CreateRenderTargetView(backBufferPtr, nullptr, &defaultRT._renderTargetView);
	if (FAILED(hr))
	{
		Log::Error("Cannot create default render target view.");
		return;
	}

	m_textures.push_back(defaultRT._texture);	// set texture ID by adding it -- TODO: remove duplicate data - dont add texture to vector
	defaultRT._texture._id = static_cast<int>(m_textures.size() - 1);

	m_renderTargets.push_back(defaultRT);
	m_state._mainRenderTarget = static_cast<int>(m_renderTargets.size() - 1);
}

RenderTargetID Renderer::AddRenderTarget(D3D11_TEXTURE2D_DESC & RTTextureDesc, D3D11_RENDER_TARGET_VIEW_DESC& RTVDesc)
{
	RenderTarget newRenderTarget;
	newRenderTarget._texture = GetTextureObject(CreateTexture2D(RTTextureDesc, true));
	HRESULT hr = m_device->CreateRenderTargetView(newRenderTarget._texture._tex2D, &RTVDesc, &newRenderTarget._renderTargetView);
	if (!SUCCEEDED(hr))
	{
		Log::Error(CANT_CREATE_RESOURCE, "Render Target View");
		return -1;
	}

	m_renderTargets.push_back(newRenderTarget);
	return static_cast<int>(m_renderTargets.size() - 1);
}

DepthStencilID Renderer::AddDepthStencil(const D3D11_DEPTH_STENCIL_VIEW_DESC& dsvDesc, ID3D11Texture2D*& surface)
{
	DepthStencil* newDSV; 
	newDSV =  (DepthStencil*)malloc(sizeof(DepthStencil));
	memset(newDSV, 0, sizeof(*newDSV));

	HRESULT hr = m_device->CreateDepthStencilView(surface, &dsvDesc, &newDSV);
	if (FAILED(hr))
	{
		Log::Error(CANT_CREATE_RESOURCE, "Depth Stencil Target View");
		return -1;
	}

	m_depthStencils.push_back(newDSV);
	return static_cast<int>(m_depthStencils.size() - 1);
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
				case ShaderType::VS:
					m_deviceContext->VSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case ShaderType::GS:
					m_deviceContext->GSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case ShaderType::HS:
					m_deviceContext->HSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case ShaderType::DS:
					m_deviceContext->DSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case ShaderType::PS:
					m_deviceContext->PSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				case ShaderType::CS:
					m_deviceContext->CSSetShaderResources(tex.bufferSlot, NumNullSRV, nullSRV);
					break;
				default:
					break;
				}
			}

			ID3D11RenderTargetView* nullRTV[6] = { nullptr };
			ID3D11DepthStencilView* nullDSV = { nullptr };
			m_deviceContext->OMSetRenderTargets(6, nullRTV, nullDSV);

		} // if not same shader
	}	// if valid shader
	else
	{
		//Log::("Warning: invalid shader is active\n");
	}

	if (id != m_state._activeShader)
	{
		m_state._activeShader = id;
		m_shaders[id]->ClearConstantBuffers();
	}
}

void Renderer::Reset()
{
	m_state._activeShader = -1;
	m_state._activeBuffer = -1;
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
	m_state._activeBuffer = BufferID;
}



void Renderer::SetCamera(Camera* cam)
{
	m_mainCamera = cam;
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
	bool found = false;

	// todo: compare with unordered_map lookup
	for (size_t i = 0; i < shader->m_constants.size() && !found; i++)	// for each cbuffer
	{
		std::vector<CPUConstantID>& cVector = shader->m_constants[i];
		for (const CPUConstantID& c_id : cVector)	// linear search in constant buffers -> optimization: sort & binary search || unordered_map
		{
			CPUConstant& c = CPUConstant::Get(c_id);
			if (strcmp(cName, c._name.c_str()) == 0)		// if name matches
			{
				found = true;
				if (memcmp(c._data, data, c._size) != 0)	// copy data if its not the same
				{
					memcpy(c._data, data, c._size);
					shader->m_cBuffers[i].dirty = true;
					//break;	// ensures write on first occurance
				}
			}
		}
	}

#ifdef _DEBUG
	if (!found)
	{
		Log::Error("Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_state._activeShader, shader->Name().c_str());	
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
			cmd.texID = tex;
			cmd.shdTex = shader->m_textures[i];
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
			cmd.shdSampler = sampler;
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

void Renderer::SetDepthStencilState(DepthStencilStateID depthStencilStateID)
{
	assert(depthStencilStateID > -1 && static_cast<size_t>(depthStencilStateID) < m_depthStencilStates.size());
	m_state._activeDepthStencilState = depthStencilStateID;
}

void Renderer::BindRenderTarget(RenderTargetID rtvID)
{
	assert(rtvID > -1 && static_cast<size_t>(rtvID) < m_renderTargets.size());
	//for(RenderTargetID& hRT : m_state._boundRenderTargets) 
	m_state._boundRenderTargets = { rtvID };
}

void Renderer::BindDepthStencil(DepthStencilID dsvID)
{
	assert(dsvID > -1 && static_cast<size_t>(dsvID) < m_depthStencils.size());
	m_state._boundDepthStencil = dsvID;
}

void Renderer::UnbindRenderTarget()
{
	m_state._boundRenderTargets = { -1, -1, -1, -1, -1, -1 };
}

void Renderer::UnbindDepthStencil()
{
	m_state._boundDepthStencil = -1;
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
	const RenderTargetID rtv = m_state._boundRenderTargets[0];
	const DepthStencilID dsv = m_state._boundDepthStencil;
	if(rtv >= 0) m_deviceContext->ClearRenderTargetView(m_renderTargets[rtv]._renderTargetView, clearColor);
	if(dsv >= 0) m_deviceContext->ClearDepthStencilView(m_depthStencils[dsv], D3D11_CLEAR_DEPTH, depthValue, 0);
}

void Renderer::End()
{
	auto* backBuffer = m_renderTargets[m_state._mainRenderTarget].GetTextureResource();

	auto* dst = backBuffer;
	auto* src = m_renderTargets[m_state._boundRenderTargets[0]].GetTextureResource();
	if (dst != src)
	{
		// cannot copy entire final texture if sizes are not equal -> use copy subresource region
		// m_deviceContext->CopyResource(backBuffer, m_renderTargets[m_state._boundRenderTarget].GetTextureResource());
		
		m_deviceContext->CopySubresourceRegion(
			dst, 0,	 // dst: backbuffer[0]
			0, 0, 0, // upper left (x,y,z)
			src, 0,  // src: boundRT[0]
			nullptr);
	}

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
	if (m_state._activeBuffer != -1) m_deviceContext->IASetVertexBuffers(0, 1, &m_bufferObjects[m_state._activeBuffer]->m_vertexBuffer, &stride, &offset);
	if (m_state._activeBuffer != -1) m_deviceContext->IASetIndexBuffer(m_bufferObjects[m_state._activeBuffer]->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	if (shader)						 m_deviceContext->IASetInputLayout(shader->m_layout);

	if (shader)
	{	// SHADER STAGES
		// ----------------------------------------
		m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
		m_deviceContext->PSSetShader(shader->m_pixelShader , nullptr, 0);
		if (shader->m_geometryShader)	 m_deviceContext->GSSetShader(shader->m_geometryShader    , nullptr, 0);
		if (shader->m_hullShader)	 m_deviceContext->HSSetShader(shader->m_hullShader   , nullptr, 0);
		if (shader->m_domainShader)	 m_deviceContext->DSSetShader(shader->m_domainShader , nullptr, 0);
		if (shader->m_computeShader) m_deviceContext->CSSetShader(shader->m_computeShader, nullptr, 0);

		// CONSTANT BUFFERS 
		// ----------------------------------------
		shader->UpdateConstants(m_deviceContext);

		// SHADER RESOURCES
		// ----------------------------------------
		while (m_setTextureCmds.size() > 0)
		{
			SetTextureCommand& cmd = m_setTextureCmds.front();
			cmd.SetResource(this);
			m_setTextureCmds.pop();
		}

		while (m_setSamplerCmds.size() > 0)
		{
			SetSamplerCommand& cmd = m_setSamplerCmds.front();
			cmd.SetResource(this);
			m_setSamplerCmds.pop();
		}

		// RASTERIZER
		// ----------------------------------------
		m_deviceContext->RSSetViewports(1, &m_viewPort);
		m_deviceContext->RSSetState(m_rasterizerStates[m_state._activeRSState]);

		// OUTPUT MERGER
		// ----------------------------------------
		const auto indexDSState = m_state._activeDepthStencilState;
		const auto indexRTV = m_state._boundRenderTargets[0];
		
		// get the bound render target addresses
		std::vector<ID3D11RenderTargetView*> RTVs = [&]() {				
			std::vector<ID3D11RenderTargetView*> v(m_state._boundRenderTargets.size(), nullptr);
			size_t i = 0;
			for (RenderTargetID hRT : m_state._boundRenderTargets) 
				if(hRT >= 0) 
					v[i++] = m_renderTargets[hRT]._renderTargetView;
			return v;
		}();

		const auto indexDSV     = m_state._boundDepthStencil;
		//ID3D11RenderTargetView** RTV = indexRTV == -1 ? nullptr : &RTVs[0];
		ID3D11RenderTargetView** RTV = &RTVs[0];
		DepthStencil*  DSV           = indexDSV == -1 ? nullptr : m_depthStencils[indexDSV];

		m_deviceContext->OMSetRenderTargets(RTV ? (unsigned)RTVs.size() : 0, RTV, DSV);
		

		auto*  DSSTATE = m_depthStencilStates[indexDSState];
		m_deviceContext->OMSetDepthStencilState(DSSTATE, 0);
	}
	else
	{
		Log::Error("Renderer::Apply() : Shader null...\n");
	}
}

void Renderer::DrawIndexed(TOPOLOGY topology)
{
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->DrawIndexed(m_bufferObjects[m_state._activeBuffer]->m_indexCount, 0, 0);
}

void Renderer::Draw(TOPOLOGY topology)
{
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->Draw(1, 0);
}



void Renderer::PollShaderFiles()
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

void Renderer::OnShaderChange(LPTSTR dir)
{
	Log::Info("OnShaderChange(%s)\n\n", dir);
	// we know that a change occurred in the 'dir' directory. Read source again
	// works		: create file, delete file
	// doesnt work	: modify file
	// source: https://msdn.microsoft.com/en-us/library/aa365261(v=vs.85).aspx
}

