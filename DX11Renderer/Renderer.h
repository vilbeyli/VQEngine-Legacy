#pragma once

#include <windows.h>

class Renderer
{
public:
	Renderer();
	~Renderer();
	bool Init(int width, int height, HWND m_hwnd);
	void Exit();
	bool MakeFrame();

private:
	bool Render();

public:
	static const bool FULL_SCREEN  = false;
	static const bool VSYNC = true;
private:

};


// FUNCTION PROTOTYPES
//----------------------------------------------------------------------


// GLOBALS
//----------------------------------------------------------------------
const float g_farPlane		= 1000.0f;
const float g_nearPlane		= 0.1f;

