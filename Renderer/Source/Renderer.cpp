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

Renderer::Renderer()
	:
	m_Direct3D(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	m_mainCamera(nullptr),
	m_bufferObjects(std::vector<BufferObject*>(MESH_TYPE::MESH_TYPE_COUNT)),
	m_rasterizerStates(std::vector<RasterizerState*>(RASTERIZER_STATE::RS_COUNT))
{}


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
	bool result = m_Direct3D->Init(width, height, Renderer::VSYNC, m_hWnd, FULL_SCREEN, NEAR_PLANE, FAR_PLANE);
	if (!result)
	{
		MessageBox(m_hWnd, "Could not initialize Direct3D", "Error", MB_OK);
		return false;
	}
	m_device		= m_Direct3D->GetDevice();
	m_deviceContext = m_Direct3D->GetDeviceContext();

	InitRasterizerStates();

	GeneratePrimitives();
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
	const unsigned sphRingCount = 90;
	const unsigned sphSliceCount = 120;

	m_geom.SetDevice(m_device);
	m_bufferObjects[TRIANGLE]	= m_geom.Triangle();
	m_bufferObjects[QUAD]		= m_geom.Quad();
	m_bufferObjects[CUBE]		= m_geom.Cube();
	m_bufferObjects[GRID]		= m_geom.Grid(gridWidth, gridDepth, gridFinenessH, gridFinenessV);
	m_bufferObjects[CYLINDER]	= m_geom.Cylinder(cylHeight, cylTopRadius, cylBottomRadius, cylSliceCount, cylStackCount);
	m_bufferObjects[SPHERE]		= m_geom.Sphere(sphRadius, sphRingCount, sphSliceCount);
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
	ID3D11RasterizerState*& cullNone = m_rasterizerStates[CULL_NONE];
	//ID3D11RasterizerState* cullBack;
	//ID3D11RasterizerState* cullFront;
	
	// MSDN: https://msdn.microsoft.com/en-us/library/windows/desktop/ff476198(v=vs.85).aspx
	D3D11_RASTERIZER_DESC rsDesc;
	//ZeroMemory(&rsDesc, sizeof(D3D11_RASTERIZER_DESC));

	rsDesc.FillMode					= D3D11_FILL_SOLID;
	rsDesc.FrontCounterClockwise	= false;
	rsDesc.DepthBias				= 0;
	rsDesc.ScissorEnable			= false;
	rsDesc.DepthBiasClamp			= 0;
	rsDesc.SlopeScaledDepthBias		= 0.0f;

	
	//rsDesc.CullMode					= D3D11_CULL_BACK;
	//if (!m_device->CreateRasterizerState(&rsDesc, &cullBack))
	//{
	//	sprintf_s(info, "Error creating Rasterizer State: Cull Back\n");
	//	OutputDebugString(info);
	//}

	//rsDesc.CullMode = D3D11_CULL_FRONT;
	//if (!m_device->CreateRasterizerState(&rsDesc, &cullFront))
	//{
	//	sprintf_s(info, "Error creating Rasterizer State: Cull Front\n");
	//	OutputDebugString(info);
	//}

	rsDesc.CullMode = D3D11_CULL_NONE;
	hr = m_device->CreateRasterizerState(&rsDesc, &cullNone);
	if (FAILED(hr))
	{
		sprintf_s(info, "Error creating Rasterizer State: Cull None\n");
		OutputDebugString(info);
	}

	//m_rasterizerStates[CULL_NONE] = cullNone;
	//m_rasterizerStates[CULL_FRONT] = cullFront;
	//m_rasterizerStates[CULL_BACK] = cullBack;
}

ShaderID Renderer::AddShader(const std::string& shdFileName, 
							 const std::string& fileRoot,
							 const std::vector<InputLayout>& layouts)
{
	// example params: "TextureCoord", "Data/Shaders/"
	std::string path = fileRoot + shdFileName;

	Shader* shd = new Shader(shdFileName);
	shd->Compile(m_device, path, layouts);
	m_shaders.push_back(shd);
	shd->AssignID(static_cast<int>(m_shaders.size()) - 1);
	return shd->ID();
}

TextureID Renderer::AddTexture(const std::string& textureFileName, const std::string& fileRoot)
{
	// example params: "bricks_d.png", "Data/Textures/"
	std::string path = fileRoot + textureFileName;
	std::wstring wpath(path.begin(), path.end());

	std::unique_ptr<DirectX::ScratchImage> img = std::make_unique<DirectX::ScratchImage>();
	if (FAILED(LoadFromWICFile(wpath.c_str(), WIC_FLAGS_NONE, nullptr, *img)))
	{
		OutputDebugString("Error loading texture file");
		return -1;
	}
	else
	{
		//m_textures.push_back(img);
		return static_cast<int>(m_textures.size()-1);
	}

}

void Renderer::SetShader(ShaderID id)
{
	assert(id >= 0 && static_cast<unsigned>(id) < m_shaders.size());
	m_activeShader = id;
	m_shaders[id]->VoidBuffers();
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
	m_viewPort.MinDepth = NEAR_PLANE;
	m_viewPort.MaxDepth = FAR_PLANE;
}

void Renderer::SetBufferObj(int BufferID)
{
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
	if (!found)
	{
		std::string err("Error: Constant not found: "); err += cName; err += "\n";
		OutputDebugString(err.c_str());
	}

}

// TODO: this is the same as 4x4. rethink set constant function
void Renderer::SetConstant3f(const char * cName, const XMFLOAT3 & float3)
{
	const float* data = &float3.x;
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
	if (!found)
	{
		char err[256];
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
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
	if (!found)
	{
		char err[256];
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
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
				//if (memcmp(c.data, data, c.size) != 0)	// copy data if its not the same
				{
					memcpy(c.data, data, c.size);
					shader->m_cBuffers[i].dirty = true;
					//break;	// ensures write on first occurance
				}
			}
		}
	}
	if (!found)
	{
		char err[256];
		sprintf_s(err, "Error: Constant not found: \"%s\" in Shader(Id=%d) \"%s\"\n", cName, m_activeShader, shader->Name().c_str());
		OutputDebugString(err);
	}
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

	// TODO: check if state is changed

	// set shaders & data layout
	m_deviceContext->IASetInputLayout(shader->m_layout);
	m_deviceContext->VSSetShader(shader->m_vertexShader, nullptr, 0);
	m_deviceContext->PSSetShader(shader->m_pixelShader, nullptr, 0);

	// set vertex & index buffers & topology
	unsigned stride = sizeof(Vertex);	// layout?
	unsigned offset = 0;
	m_deviceContext->IASetVertexBuffers(0, 1, &m_bufferObjects[m_activeBuffer]->m_vertexBuffer, &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_bufferObjects[m_activeBuffer]->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// viewport
	m_deviceContext->RSSetViewports(1, &m_viewPort);
	m_deviceContext->RSSetState(m_rasterizerStates[CULL_NONE]);

	// test
	D3D11_RASTERIZER_DESC rsDesc; 
	m_rasterizerStates[CULL_NONE]->GetDesc(&rsDesc);

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

			switch (cbuf.shdType)
			{
			case ShaderType::VS:
				m_deviceContext->VSSetConstantBuffers(cbuf.bufferSlot, 1, &bufferData);
				break;
			case ShaderType::PS:
				m_deviceContext->PSSetConstantBuffers(cbuf.bufferSlot, 1, &bufferData);
				break;
			default:
				OutputDebugString("ERROR: Renderer::Apply() - UNKOWN Shader Type\n");
				break;
			}

			cbuf.dirty = false;
		}
	}

	m_deviceContext->OMSetDepthStencilState(m_Direct3D->m_depthStencilState, 0);
}

void Renderer::DrawIndexed()
{
	m_deviceContext->DrawIndexed(m_bufferObjects[m_activeBuffer]->m_indexCount, 0, 0);
}