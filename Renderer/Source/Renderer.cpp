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
	bool result = m_Direct3D->Init(width, height, Renderer::VSYNC, m_hWnd, FULL_SCREEN, NEAR_PLANE, FAR_PLANE);
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

void Renderer::GeneratePrimitives()
{
	m_geom.SetDevice(m_device);
	m_bufferObjects[TRIANGLE]	= m_geom.Triangle();
	m_bufferObjects[QUAD]		= m_geom.Quad();
	m_bufferObjects[CUBE]		= m_geom.Cube();
}

void Renderer::PollShaderFiles()
{
	// todo: https://msdn.microsoft.com/en-us/library/aa365261(v=vs.85).aspx
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
	shd->AssignID(static_cast<int>(m_shaders.size()) - 1);
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
	m_activeBuffer = -1;
}


void Renderer::SetViewport(const unsigned width, const unsigned height)
{
	m_viewPort.TopLeftX = 0;
	m_viewPort.TopLeftY = 0;
	m_viewPort.Width	= static_cast<float>(width);
	m_viewPort.Height	= static_cast<float>(height);
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
		std::string err("Error: Constant not found: "); err += cName;
		OutputDebugString(err.c_str());
	}

	return;
}

void Renderer::Begin(const float clearColor[4])
{
	m_Direct3D->BeginFrame(clearColor);
}

void Renderer::End()
{
	m_Direct3D->EndFrame();
	++m_frameCount;
#ifdef _DEBUG
	if (m_frameCount % 60 == 0)	PollShaderFiles();
#endif
}


void Renderer::Apply()
{
	// Here, we make all the API calls for GPU data

	Shader* shader = m_shaders[m_activeShader];

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

	// set shader constants
	for (unsigned i = 0; i < shader->m_cBuffers.size(); ++i)
	{
		CBuffer& cbuf = shader->m_cBuffers[i];
		if (cbuf.dirty)	// if the CPU-side buffer is updated
		{
			ID3D11Buffer* bufferData = cbuf.data;
			D3D11_MAPPED_SUBRESOURCE mappedResource;

			// Map subresource to GPU - update contends - discard the subresource
			m_deviceContext->Map(bufferData, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
			char* bufferPos = static_cast<char*>(mappedResource.pData);	// char* so we can advance the pointer
			std::vector<CPUConstant>& cpuConsts = shader->m_constants[i];
			for (CPUConstant& c : cpuConsts)
			{
				memcpy(bufferPos, c.data, c.size);
				bufferPos += c.size;
			}
			m_deviceContext->Unmap(bufferData, 0);

			m_deviceContext->VSSetConstantBuffers(i, 1, &bufferData);
			cbuf.dirty = false;
		}
	}
}

void Renderer::DrawIndexed()
{
	m_deviceContext->DrawIndexed(m_bufferObjects[m_activeBuffer]->m_indexCount, 0, 0);
}





//*********************************************************************
// BoxDemo.cpp by Frank Luna (C) 2011 All Rights Reserved.
//
// Demonstrates rendering a colored box.
//
// Controls:
// Hold the left mouse button down and move the mouse to rotate.
// Hold the right mouse button down to zoom in and out.
//
//*********************************************************************
//#include "d3dApp.h"
//#include "d3dx11Effect.h"
//#include "MathHelper.h"
//struct Vertex
//{
//	XMFLOAT3 Pos;
//	XMFLOAT4 Color;
//};
//class BoxApp : public D3DApp
//{
//public:
//	BoxApp(HINSTANCE hInstance);
//	~BoxApp();
//	bool Init();
//	void OnResize();
//	void UpdateScene(float dt);
//	void DrawScene();
//	void OnMouseDown(WPARAM btnState, int x, int y);
//	void OnMouseUp(WPARAM btnState, int x, int y);
//	void OnMouseMove(WPARAM btnState, int x, int y);
//private:
//	void BuildGeometryBuffers();
//	void BuildFX();
//	void BuildVertexLayout();
//private:
//	ID3D11Buffer* mBoxVB;
//	ID3D11Buffer* mBoxIB;
//	ID3DX11Effect* mFX;
//	ID3DX11EffectTechnique* mTech;
//	ID3DX11EffectMatrixVariable* mfxWorldViewProj;
//	ID3D11InputLayout* mInputLayout;
//	XMFLOAT4X4 mWorld;
//	XMFLOAT4X4 mView;
//	XMFLOAT4X4 mProj;
//	float mTheta;
//	float mPhi;
//	float mRadius;
//	POINT mLastMousePos;
//};
//int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
//	PSTR cmdLine, int showCmd)
//{
//	// Enable run-time memory check for debug builds.
//#if defined(DEBUG) | defined(_DEBUG)
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//#endif
//	BoxApp theApp(hInstance);
//	if (!theApp.Init())
//		return 0;
//	return theApp.Run();
//}
//BoxApp::BoxApp(HINSTANCE hInstance)
//	: D3DApp(hInstance), mBoxVB(0), mBoxIB(0), mFX(0), mTech(0),
//	mfxWorldViewProj(0), mInputLayout(0),
//	mTheta(1.5f*MathHelper::Pi), mPhi(0.25f*MathHelper::Pi), mRadius(5.0f)
//{
//	mMainWndCaption = L"Box Demo";
//	mLastMousePos.x = 0;
//	mLastMousePos.y = 0;
//	XMMATRIX I = XMMatrixIdentity();
//	XMStoreFloat4x4(&mWorld, I);
//	XMStoreFloat4x4(&mView, I);
//	XMStoreFloat4x4(&mProj, I);
//}
//BoxApp::~BoxApp()
//{
//	ReleaseCOM(mBoxVB);
//	ReleaseCOM(mBoxIB);
//	ReleaseCOM(mFX);
//	ReleaseCOM(mInputLayout);
//}
//bool BoxApp::Init()
//{
//	if (!D3DApp::Init())
//		return false;
//	BuildGeometryBuffers();
//	BuildFX();
//	BuildVertexLayout();
//	return true;
//}
//void BoxApp::OnResize()
//{
//	D3DApp::OnResize();
//	// The window resized, so update the aspect ratio and recomputed
//	// the projection matrix.
//	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi,
//		AspectRatio(), 1.0f, 1000.0f);
//	XMStoreFloat4x4(&mProj, P);
//}
//void BoxApp::UpdateScene(float dt)
//{
//	// Convert Spherical to Cartesian coordinates.
//	float x = mRadius*sinf(mPhi)*cosf(mTheta);
//	float z = mRadius*sinf(mPhi)*sinf(mTheta);
//	float y = mRadius*cosf(mPhi);
//	// Build the view matrix.
//	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
//	XMVECTOR target = XMVectorZero();
//	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//	XMMATRIX V = XMMatrixLookAtLH(pos, target, up);
//	XMStoreFloat4x4(&mView, V);
//}
//void BoxApp::DrawScene()
//{
//	md3dImmediateContext->ClearRenderTargetView(mRenderTargetView,
//		reinterpret_cast<const float*>(&Colors::Blue));
//	md3dImmediateContext->ClearDepthStencilView(mDepthStencilView,
//		D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
//	md3dImmediateContext->IASetInputLayout(mInputLayout);
//	md3dImmediateContext->IASetPrimitiveTopology(
//		D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//	UINT stride = sizeof(Vertex);
//	UINT offset = 0;
//	md3dImmediateContext->IASetVertexBuffers(0, 1, &mBoxVB,
//		&stride, &offset);
//	md3dImmediateContext->IASetIndexBuffer(mBoxIB,
//		DXGI_FORMAT_R32_UINT, 0);
//	// Set constants
//	XMMATRIX world = XMLoadFloat4x4(&mWorld);
//	XMMATRIX view = XMLoadFloat4x4(&mView);
//	XMMATRIX proj = XMLoadFloat4x4(&mProj);
//	XMMATRIX worldViewProj = world*view*proj;
//	mfxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
//	D3DX11_TECHNIQUE_DESC techDesc;
//	mTech->GetDesc(&techDesc);
//	for (UINT p = 0; p < techDesc.Passes; ++p)
//	{
//		mTech->GetPassByIndex(p)->Apply(0, md3dImmediateContext);
//		// 36 indices for the box.
//		md3dImmediateContext->DrawIndexed(36, 0, 0);
//	}
//	HR(mSwapChain->Present(0, 0));
//}
//void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
//{
//	mLastMousePos.x = x;
//	mLastMousePos.y = y;
//	SetCapture(mhMainWnd);
//}
//void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
//{
//	ReleaseCapture();
//}
//void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
//{
//	if ((btnState & MK_LBUTTON) != 0)
//	{
//		// Make each pixel correspond to a quarter of a degree.
//		float dx = XMConvertToRadians(
//			0.25f*static_cast<float>(x - mLastMousePos.x));
//		float dy = XMConvertToRadians(
//			0.25f*static_cast<float>(y - mLastMousePos.y));
//		// Update angles based on input to orbit camera around box.
//		mTheta += dx;
//		mPhi += dy;
//		// Restrict the angle mPhi.
//		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
//	}
//	else if ((btnState & MK_RBUTTON) != 0)
//	{
//		// Make each pixel correspond to 0.005 unit in the scene.
//		float dx = 0.005f*static_cast<float>(x - mLastMousePos.x);
//		float dy = 0.005f*static_cast<float>(y - mLastMousePos.y);
//		// Update the camera radius based on input.
//		mRadius += dx - dy;
//		// Restrict the radius.
//		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
//	}
//	mLastMousePos.x = x;
//	mLastMousePos.y = y;
//}
//void BoxApp::BuildGeometryBuffers()
//{
//	// Create vertex buffer
//	Vertex vertices[] =
//	{
//		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), (const float*)&Colors::White },
//		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), (const float*)&Colors::Black },
//		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), (const float*)&Colors::Red },
//		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), (const float*)&Colors::Green },
//		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), (const float*)&Colors::Blue },
//		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), (const float*)&Colors::Yellow },
//		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), (const float*)&Colors::Cyan },
//		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), (const float*)&Colors::Magenta }
//	};
//	D3D11_BUFFER_DESC vbd;
//	vbd.Usage = D3D11_USAGE_IMMUTABLE;
//	vbd.ByteWidth = sizeof(Vertex) * 8;
//	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//	vbd.CPUAccessFlags = 0;
//	vbd.MiscFlags = 0;
//	vbd.StructureByteStride = 0;
//	D3D11_SUBRESOURCE_DATA vinitData;
//	vinitData.pSysMem = vertices;
//	HR(md3dDevice->CreateBuffer(&vbd, &vinitData, &mBoxVB));
//	// Create the index buffer
//	UINT indices[] = {
//		// front face
//		0, 1, 2,
//		0, 2, 3,
//		// back face
//		4, 6, 5,
//		4, 7, 6,
//		// left face
//		4, 5, 1,
//		4, 1, 0,
//		// right face
//		3, 2, 6,
//		3, 6, 7,
//		// top face
//		1, 5, 6,
//		1, 6, 2,
//		// bottom face
//		4, 0, 3,
//		4, 3, 7
//	};
//	D3D11_BUFFER_DESC ibd;
//	ibd.Usage = D3D11_USAGE_IMMUTABLE;
//	ibd.ByteWidth = sizeof(UINT) * 36;
//	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
//	ibd.CPUAccessFlags = 0;
//	ibd.MiscFlags = 0;
//	ibd.StructureByteStride = 0;
//	D3D11_SUBRESOURCE_DATA iinitData;
//	iinitData.pSysMem = indices;
//	HR(md3dDevice->CreateBuffer(&ibd, &iinitData, &mBoxIB));
//}
//void BoxApp::BuildFX()
//{
//	DWORD shaderFlags = 0;
//#if defined(DEBUG) || defined(_DEBUG)
//	shaderFlags |= D3D10_SHADER_DEBUG;
//	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
//#endif
//	ID3D10Blob* compiledShader = 0;
//	ID3D10Blob* compilationMsgs = 0;
//	HRESULT hr = D3DX11CompileFromFile(L"FX/color.fx", 0, 0, 0,
//		"fx_5_0", shaderFlags,
//		0, 0, &compiledShader, &compilationMsgs, 0);
//	// compilationMsgs can store errors or warnings.
//	if (compilationMsgs != 0)
//	{
//		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
//		ReleaseCOM(compilationMsgs);
//	}
//	// Even if there are no compilationMsgs, check to make sure there
//	// were no other errors.
//	if (FAILED(hr))
//	{
//		DXTrace(__FILE__, (DWORD)__LINE__, hr,
//			L"D3DX11CompileFromFile", true);
//	}
//	HR(D3DX11CreateEffectFromMemory(
//		compiledShader->GetBufferPointer(),
//		compiledShader->GetBufferSize(),
//		0, md3dDevice, &mFX));
//	// Done with compiled shader.
//	ReleaseCOM(compiledShader);
//	mTech = mFX->GetTechniqueByName("ColorTech");
//	mfxWorldViewProj = mFX->GetVariableByName(
//		"gWorldViewProj")->AsMatrix();
//}
//void BoxApp::BuildVertexLayout()
//{
//	// Create the vertex input layout.
//	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
//	{
//		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
//		D3D11_INPUT_PER_VERTEX_DATA, 0 },
//		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
//		D3D11_INPUT_PER_VERTEX_DATA, 0 }
//	};
//	// Create the input layout
//	D3DX11_PASS_DESC passDesc;
//	mTech->GetPassByIndex(0)->GetDesc(&passDesc);
//	HR(md3dDevice->CreateInputLayout(vertexDesc, 2,
//		passDesc.pIAInputSignature,
//		passDesc.IAInputSignatureSize, &mInputLayout));
//}
//
//class D3DApp
//{
//public:
//	D3DApp(HINSTANCE hInstance);
//	virtual ~D3DApp();
//	HINSTANCE AppInst()const;
//	HWND MainWnd()const;
//	float AspectRatio()const;
//	int Run();
//	// Framework methods. Derived client class overrides these methods to
//	// implement specific application requirements.
//	virtual bool Init();
//	virtual void OnResize();
//	virtual void UpdateScene(float dt) = 0;
//	virtual void DrawScene() = 0;
//	virtual LRESULT MsgProc(HWND hwnd, UINT msg,
//		WPARAM wParam, LPARAM lParam);
//	// Convenience overrides for handling mouse input.
//	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
//	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
//	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }
//protected:
//	bool InitMainWindow();
//	bool InitDirect3D();
//	void CalculateFrameStats();
//protected:
//	HINSTANCE mhAppInst; // application instance handle
//	HWND mhMainWnd; // main window handle
//	bool mAppPaused; // is the application paused?
//	bool mMinimized; // is the application minimized?
//	bool mMaximized; // is the application maximized?
//	bool mResizing; // are the resize bars being dragged?
//	UINT m4xMsaaQuality; // quality level of 4X MSAA
//						 // Used to keep track of the "delta-time" and game time (§4.3).
//	GameTimer mTimer;
//	// The D3D11 device (§4.2.1), the swap chain for page flipping
//	// (§4.2.4), the 2D texture for the depth/stencil buffer (§4.2.6),
//	// the render target (§4.2.5) and depth/stencil views (§4.2.6), and
//	// the viewport (§4.2.8).
//	ID3D11Device* md3dDevice;
//	ID3D11DeviceContext* md3dImmediateContext;
//	IDXGISwapChain* mSwapChain;
//	ID3D11Texture2D* mDepthStencilBuffer;
//	ID3D11RenderTargetView* mRenderTargetView;
//	ID3D11DepthStencilView* mDepthStencilView;
//	D3D11_VIEWPORT mScreenViewport;
//	// The following variables are initialized in the D3DApp constructor
//	// to default values. However, you can override the values in the
//	// derived class constructor to pick different defaults.
//	// Window title/caption. D3DApp defaults to "D3D11 Application".
//	std::wstring mMainWndCaption;
//	// Hardware device or reference device? D3DApp defaults to
//	// D3D_DRIVER_TYPE_HARDWARE.
//	D3D_DRIVER_TYPE md3dDriverType;
//	// Initial size of the window's client area. D3DApp defaults to
//	// 800x600. Note, however, that these values change at runtime
//	// to reflect the current client area size as the window is resized.
//	int mClientWidth;
//	int mClientHeight;
//	// True to use 4X MSAA (§4.1.8). The default is false.
//	bool mEnable4xMsaa;
//};
//void D3DApp::CalculateFrameStats()
//{
//	// Code computes the average frames per second, and also the
//	// average time it takes to render one frame. These stats
//	// are appeneded to the window caption bar.
//	static int frameCnt = 0;
//	static float timeElapsed = 0.0f;
//	frameCnt++;
//	// Compute averages over one second period.
//	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
//	{
//		float fps = (float)frameCnt; // fps = frameCnt / 1
//		float mspf = 1000.0f / fps;
//		std::wostringstream outs;
//		outs.precision(6);
//		outs << mMainWndCaption << L" "
//			<< L"FPS: " << fps << L" "
//			<< L"Frame Time: " << mspf << L" (ms)";
//		SetWindowText(mhMainWnd, outs.str().c_str());
//		// Reset for next average.
//		frameCnt = 0;
//		timeElapsed += 1.0f;
//	}
//}