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
#include "Mesh.h"
#include "../../Application/Source/SystemDefs.h"
#include "../../Application/Source/utils.h"
#include "../../Application/Source/Camera.h"

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

ShaderID Renderer::AddShader(const std::string& shdFileName, 
							 const std::string& fileRoot,
							 const std::vector<InputLayout>& layouts)
{
	// example params: "tex", "Shader/"
	std::string path = fileRoot + shdFileName;

	Shader* shd = new Shader(shdFileName);
	shd->Compile(m_device, path, layouts);
	m_shaders.push_back(shd);
	shd->AssignID(m_shaders.size() - 1);
	return shd->ID();
}

void Renderer::SetShader(ShaderID id)
{
	assert(id >= 0 && static_cast<unsigned>(id) < m_shaders.size());
	m_activeShader = id;
}

void Renderer::Reset()
{
	m_activeShader = -1;
}

void Renderer::SetCamera(Camera* cam)
{
	m_mainCamera = cam;
}

bool Renderer::Render()
{
	const float clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	m_Direct3D->BeginFrame(clearColor);

	XMMATRIX world = XMMatrixIdentity();
	XMMATRIX view = m_mainCamera->GetViewMatrix();
	XMMATRIX proj = m_mainCamera->GetProjectionMatrix();

	// set shaders & data layout
	m_deviceContext->IASetInputLayout(m_shaders[0]->m_layout);
	m_deviceContext->VSSetShader(m_shaders[0]->m_vertexShader, nullptr, 0);
	m_deviceContext->PSSetShader(m_shaders[0]->m_pixelShader, nullptr, 0);

	// set shader constants
	ID3D11Buffer* shaderConsts = m_shaders[0]->m_cBuffer;
	D3D11_MAPPED_SUBRESOURCE mappedResource;
	m_deviceContext->Map(shaderConsts, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	MatrixBuffer* dataPtr = static_cast<MatrixBuffer*>(mappedResource.pData);
	dataPtr->mProj	= proj;
	dataPtr->mView	= view;
	dataPtr->mWorld = world;
	m_deviceContext->Unmap(shaderConsts, 0);
	m_deviceContext->VSSetConstantBuffers(0, 1, &shaderConsts);

	// set viewport
	D3D11_VIEWPORT viewPort = { 0 };
	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = SCREEN_WIDTH;
	viewPort.Height = SCREEN_HEIGHT;
	m_deviceContext->RSSetViewports(1, &viewPort);

	// set vertex & index buffers & topology
	unsigned stride = sizeof(Vertex);
	unsigned offset = 0;
	m_deviceContext->IASetVertexBuffers(0, 1, &m_bufferObjects[0]->m_vertexBuffer, &stride, &offset);
	m_deviceContext->IASetIndexBuffer(m_bufferObjects[0]->m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);
	m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	// draw
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

