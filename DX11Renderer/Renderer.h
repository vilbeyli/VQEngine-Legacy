#pragma once

#include <windows.h>

class D3DManager;

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Init(int width, int height, HWND hwnd);
	void Exit();
	bool MakeFrame();

	void EnableAlphaBlending(bool enable);
	void EnableZBuffer(bool enable);

	ID3D11Device*			GetDevice();
	ID3D11DeviceContext*	GetDeviceContext();

private:
	bool Render();

public:
	static const bool FULL_SCREEN  = false;
	static const bool VSYNC = true;
private:

	D3DManager* m_Direct3D;
	HWND m_hWnd;
};


// FUNCTION PROTOTYPES
//----------------------------------------------------------------------


// GLOBALS
//----------------------------------------------------------------------
const float g_farPlane		= 1000.0f;
const float g_nearPlane		= 0.1f;

 