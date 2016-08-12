#include "Renderer.h"
#include "D3DManager.h"
#include "BufferObject.h"
#include "Shader.h"
#include "Mesh.h"
#include "SystemDefs.h"

#include <atlbase.h>
#include <atlconv.h>

#include <cassert>

Renderer::Renderer()
	:
	m_Direct3D(nullptr),
	m_device(nullptr),
	m_deviceContext(nullptr),
	m_bufferObjects(std::vector<BufferObject*>(MESH_TYPE::COUNT))
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
	bool result = m_Direct3D->Init(width, height, Renderer::VSYNC, m_hWnd, FULL_SCREEN, g_nearPlane, g_farPlane);
	if (!result)
	{
		MessageBox(m_hWnd, "Could not initialize Direct3D", "Error", MB_OK);
		return false;
	}
	m_device		= m_Direct3D->GetDevice();
	m_deviceContext = m_Direct3D->GetDeviceContext();


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

	return;
}

bool Renderer::MakeFrame()
{
	bool success = Render();

	return success;
}

ShaderID Renderer::AddShader(const std::string& shdFileName, const std::string& fileRoot)
{
	// tex, Shader/
	Shader* shd = new Shader(shdFileName);

	// shader file path: convert from std::string to WCHAR*
	std::string shdFilePath(fileRoot + shdFileName);
	CA2W ca2w(shdFilePath.c_str());
	WCHAR* wPath = ca2w;

	shd->Compile(m_device, m_hWnd, wPath);

	m_shaders.push_back(shd);
	

	return 0;	// todo rethink
}

bool Renderer::Render()
{
	const float clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	m_Direct3D->BeginFrame(clearColor);

	XMMATRIX world, view, proj;


	m_deviceContext->IASetInputLayout(m_shaders[0]->m_layout);
	m_deviceContext->VSSetShader(m_shaders[0]->m_vertexShader, nullptr, 0);
	m_deviceContext->PSSetShader(m_shaders[0]->m_pixelShader, nullptr, 0);

	D3D11_VIEWPORT viewPort = { 0 };
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = SCREEN_WIDTH;
	viewPort.Height = SCREEN_HEIGHT;
	m_deviceContext->RSSetViewports(1, &viewPort);

	unsigned stride = sizeof(Vertex);
	unsigned offset = 0;

	m_deviceContext->IASetVertexBuffers(0, 1, &m_bufferObjects[0]->m_vertexBuffer, &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_bufferObjects[0]->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_deviceContext->DrawIndexed(m_bufferObjects[0]->m_indexCount, 0, 0);

	m_Direct3D->EndFrame();
	return true;
}

void Renderer::GeneratePrimitives()
{
	m_geom.SetDevice(m_device);
	m_bufferObjects[TRIANGLE]	= m_geom.Triangle();
	//m_bufferObjects[QUAD]		= m_geom.Quad();
}
