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

#include "SystemDefs.h"
#include "utils.h"
#include "Camera.h"
#include "DirectXTex.h"

#include <mutex>
#include <cassert>

#define SHADER_HOTSWAP 0

const char* Renderer::s_shaderRoot = "Data/Shaders/";
const char* Renderer::s_textureRoot = "Data/Textures/";

void DepthPass::Initialize(Renderer* pRenderer)
{
	m_shadowMapDimension = 512;

	D3D11_TEXTURE2D_DESC shadowMapDesc;
	ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
	shadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	shadowMapDesc.MipLevels = 1;
	shadowMapDesc.ArraySize = 1;
	shadowMapDesc.SampleDesc.Count = 1;
	shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	shadowMapDesc.Height = static_cast<UINT>(m_shadowMapDimension);
	shadowMapDesc.Width = static_cast<UINT>(m_shadowMapDimension);

//	HRESULT hr = pD3DDevice->CreateTexture2D(
//		&shadowMapDesc,
//		nullptr,
//		&m_shadowMap.srv
//	);

}

Renderer::Renderer()
	:
	m_Direct3D(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	m_mainCamera(nullptr),
	m_bufferObjects(std::vector<BufferObject*>(MESH_TYPE::MESH_TYPE_COUNT)),
	m_rasterizerStates(std::vector<RasterizerState*>(RASTERIZER_STATE::RS_COUNT))
{
	for (int i=0; i<RASTERIZER_STATE::RS_COUNT; ++i)
	{
		m_rasterizerStates[i] = (RasterizerState*)malloc(sizeof(*m_rasterizerStates[i]));
		memset(&*m_rasterizerStates[i], 0, sizeof(*m_rasterizerStates[i]));
	}
}

Renderer::~Renderer()
{
}

bool Renderer::Init(int width, int height, HWND hwnd)
{
	m_hWnd = hwnd;

	// Create the Direct3D object.
	m_Direct3D = new D3DManager();
	if (!m_Direct3D)
	{
		assert(false);
		return false;
	}

	// Initialize the Direct3D object.
	bool result = m_Direct3D->Init(width, height, Renderer::VSYNC, m_hWnd, FULL_SCREEN);
	if (!result)
	{
		MessageBox(m_hWnd, "Could not initialize Direct3D", "Error", MB_OK);
		return false;
	}
	m_device		= m_Direct3D->GetDevice();
	m_deviceContext = m_Direct3D->GetDeviceContext();

	InitRasterizerStates();
	GeneratePrimitives();
	LoadShaders();
	return true;
}

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
	m_renderData.depthPass.Initialize(this);
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

void Renderer::InitRasterizerStates()
{
	HRESULT hr;
	char info[129];
	ID3D11RasterizerState*& cullNone  = m_rasterizerStates[CULL_NONE];
	ID3D11RasterizerState*& cullBack  = m_rasterizerStates[CULL_BACK];
	ID3D11RasterizerState*& cullFront = m_rasterizerStates[CULL_FRONT];
	
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

	Shader* shd = new Shader(shdFileName);
	shd->Compile(m_device, path, layouts, geoShader);
	m_shaders.push_back(shd);
	shd->AssignID(static_cast<int>(m_shaders.size()) - 1);
	return shd->ID();
}

// assumes unique shader file names (even in different folders)
const Texture& Renderer::AddTexture(const std::string& shdFileName, const std::string& fileRoot /*= s_textureRoot*/)
{
	// example params: "bricks_d.png", "Data/Textures/"
	std::string path = fileRoot + shdFileName;
	std::wstring wpath(path.begin(), path.end());

	auto found = std::find_if(m_textures.begin(), m_textures.end(), [shdFileName](auto& tex) { return tex.name == shdFileName; });
	if (found != m_textures.end())
	{
		//OutputDebugString("found\n\n");
		//return found->id;
		return *found;
	}

	Texture tex;
	tex.name = shdFileName;

	std::unique_ptr<DirectX::ScratchImage> img = std::make_unique<DirectX::ScratchImage>();
	if (SUCCEEDED(LoadFromWICFile(wpath.c_str(), WIC_FLAGS_NONE, nullptr, *img)))
	{
		CreateShaderResourceView(m_device, img->GetImages(), img->GetImageCount(), img->GetMetadata(), &tex.srv);
		
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		tex.srv->GetDesc(&srvDesc);

		ID3D11Resource* resource = nullptr;
		tex.srv->GetResource(&resource);

		ID3D11Texture2D* tex2D = nullptr;
		if (SUCCEEDED(resource->QueryInterface(&tex2D)))
		{
			D3D11_TEXTURE2D_DESC desc;
			tex2D->GetDesc(&desc);

			tex.width = desc.Width;
			tex.height = desc.Height;
		}

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
	if (m_activeShader != -1)		// if valid shader
	{
		if (id != m_activeShader)	// if not the same shader
		{
			Shader* shader = m_shaders[m_activeShader];

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

	if (id != m_activeShader)
	{
		m_activeShader = id;
		m_shaders[id]->VoidBuffers();
	}
}

void Renderer::Reset()
{
	m_activeShader = -1;
	m_activeBuffer = -1;
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

void Renderer::SetBufferObj(int BufferID)
{
	assert(BufferID >= 0);
	m_activeBuffer = BufferID;
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
	Shader* shader = m_shaders[m_activeShader];
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
	Shader* shader = m_shaders[m_activeShader];
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
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

void Renderer::SetConstant1f(const char* cName, const float f)
{
	const float* data = &f;
	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_activeShader];
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
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

void Renderer::SetConstant1i(const char* cName, const int i)
{
	const int* data = &i;
	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_activeShader];
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
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

// TODO: this is the same as 4x4. rethink set constant function
void Renderer::SetConstantStruct(const char * cName, void* data)
{
	// find data in CPUConstantBuffer array of shader
	Shader* shader = m_shaders[m_activeShader];
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
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
}

void Renderer::SetTexture(const char * texName, TextureID tex)
{
	Shader* shader = m_shaders[m_activeShader];
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
		sprintf_s(err, "Error: Texture not found: \"%s\" in Shader(Id=%d) \"%s\"\n", texName, m_activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
#endif
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

void Renderer::Begin(const float clearColor[4])
{
	m_Direct3D->BeginFrame(clearColor);
}

void Renderer::End()
{
	m_Direct3D->EndFrame();
	++m_frameCount;
}


void Renderer::Apply()
{
	// Here, we make all the API calls for GPU data

	Shader* shader = m_shaders[m_activeShader];
	m_deviceContext->RSSetViewports(1, &m_viewPort);

	// TODO: check if state is changed

	// set shaders & data layout
	m_deviceContext->IASetInputLayout(shader->m_layout);
	m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
	m_deviceContext->PSSetShader(shader->m_pixelShader , nullptr, 0);
	/*if (shader->m_geoShader)*/	 m_deviceContext->GSSetShader(shader->m_geoShader    , nullptr, 0);
	if (shader->m_hullShader)	 m_deviceContext->HSSetShader(shader->m_hullShader   , nullptr, 0);
	if (shader->m_domainShader)	 m_deviceContext->DSSetShader(shader->m_domainShader , nullptr, 0);
	if (shader->m_computeShader) m_deviceContext->CSSetShader(shader->m_computeShader, nullptr, 0);

	// set vertex & index buffers & topology
	unsigned stride = sizeof(Vertex);	// layout?
	unsigned offset = 0;

	if (m_activeBuffer != -1) m_deviceContext->IASetVertexBuffers(0, 1, &m_bufferObjects[m_activeBuffer]->m_vertexBuffer, &stride, &offset);
	//else OutputDebugString("Warning: no active buffer object (-1)\n");
	if (m_activeBuffer != -1) m_deviceContext->IASetIndexBuffer(m_bufferObjects[m_activeBuffer]->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	//else OutputDebugString("Warning: no active buffer object (-1)\n");

	// viewport
	m_deviceContext->RSSetState(m_rasterizerStates[CULL_BACK]);	// TODO: m_activeRS?

	// test: TODO remove later
	//D3D11_RASTERIZER_DESC rsDesc;
	//m_rasterizerStates[CULL_NONE]->GetDesc(&rsDesc);

	// set shader constants
	for (unsigned i = 0; i < shader->m_cBuffers.size(); ++i)
	{
		CBuffer& cbuf = shader->m_cBuffers[i];
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

			// TODO: research update subresource (Setting cbuffer can be done once in setting the shader, see it)
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

	// set textures and sampler states
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

	m_deviceContext->OMSetDepthStencilState(m_Direct3D->m_depthStencilState, 0);
}

void Renderer::DrawIndexed(TOPOLOGY topology)
{
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->DrawIndexed(m_bufferObjects[m_activeBuffer]->m_indexCount, 0, 0);
}

void Renderer::Draw(TOPOLOGY topology)
{
	m_deviceContext->IASetPrimitiveTopology(static_cast<D3D_PRIMITIVE_TOPOLOGY>(topology));
	m_deviceContext->Draw(1, 0);
}