#pragma once

// DX11 library linking
#pragma comment(lib, "d3d11.lib")		// contains all the Direct3D functionality
#pragma comment(lib, "dxgi.lib")		// tools to interface with the hardware
#pragma comment(lib, "d3dcompiler.lib")	// functionality for compiling shaders

#include <d3d11.h>
#include <DirectXMath.h>

using namespace DirectX;


class D3DManager
{
public:
	D3DManager();
	~D3DManager();

	bool Init(int width, int height, const bool VSYNC, HWND m_hwnd, const bool FULL_SCREEN, const float g_nearPlane, const float g_farPlane);
	void Shutdown();

	void BeginFrame(const float* clearColor);
	void EndFrame();

	void EnableAlphaBlending(bool enable);
	void EnableZBuffer(bool enable);

	ID3D11Device*			GetDevice()			{ return m_device; }
	ID3D11DeviceContext*	GetDeviceContext()	{ return m_deviceContext;}

	void GetProjMatrix(XMMATRIX&)		;//{return m_mProj;}
	void GetWorldMatrix(XMMATRIX&)		;//{return m_mProj;}
	void GetOrthoMatrix(XMMATRIX&)		;//{return m_mProj;}

	void GetVideoCardInfo(char*, int&);
private:
	bool InitSwapChain(HWND hwnd, bool fullscreen, int scrWidth, int scrHeight, unsigned numerator, unsigned denominator);
	bool InitDepthBuffer(int scrWidth, int scrHeight);
	bool InitDepthStencilBuffer();
	bool InitRasterizerState();
	bool InitStencilView();
	void InitViewport(int scrWidth, int scrHeight);
	bool InitAlphaBlending();
	bool InitZBuffer();

private:
	bool						m_vsync_enabled;
	int							m_VRAM;
	char						m_GPUDescription[128];
	IDXGISwapChain*				m_swapChain;
	ID3D11Device*				m_device;
	ID3D11DeviceContext*		m_deviceContext;
	ID3D11RenderTargetView*		m_RTV;
	ID3D11Texture2D*			m_depthStencilBuffer;
	ID3D11DepthStencilView*		m_depthStencilView;
	
	//-----------------------------------------------
	// State Management
	ID3D11RasterizerState*		m_rasterState;
	ID3D11DepthStencilState*	m_depthStencilState;
	ID3D11BlendState*			m_alphaEnableBlendState;
	ID3D11BlendState*			m_alphaDisableBlendState;
	ID3D11DepthStencilState*	m_depthDisabledStencilState;

	XMMATRIX					m_mProj;
	XMMATRIX					m_mWorld;
	XMMATRIX					m_mOrtho;
};

