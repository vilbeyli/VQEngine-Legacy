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

#include "BaseSystem.h"
#include "SystemDefs.h"
#include "Input.h"

#include <new>

BaseSystem::BaseSystem()
{
	m_hInstance		= GetModuleHandle(NULL);	// instance of this application
	m_appName		= "DX11 Renderer";
}


BaseSystem::BaseSystem(const BaseSystem& other){}

BaseSystem::~BaseSystem(){}

bool BaseSystem::Init()
{
	int width, height;
	InitWindow(width, height);

	// TODO: think error handling during initialization | engine/renderer separate init?
	if(!ENGINE->Initialize(m_hwnd, width, height))
		return false;

	if (!ENGINE->Load())	return false;

	return true;
}

void BaseSystem::Run()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	bool done = false;
	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);		// Translates virtual-key messages into character messages
			DispatchMessage(&msg);		// indirectly causes Windows to invoke WndProc
		}

		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			done = !Frame();
			//result = Frame();
			//if (!result)
			//{
			//	done = true;
			//}
		}
	}

	return;
}

void BaseSystem::Exit()
{
	ShutdownWindows();
}

LRESULT CALLBACK BaseSystem::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
	case WM_KEYDOWN:
	{
		ENGINE->m_input->KeyDown((KeyCode)wparam);
		return 0;
	}

	case WM_KEYUP:
	{
		ENGINE->m_input->KeyUp((KeyCode)wparam);
		return 0;
	}

	// TODO: PAINT ???

	default:
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
}

bool BaseSystem::Frame()
{
	return ENGINE->Run();
}

void BaseSystem::InitWindow(int& width, int& height)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	gp_appHandle	= this;						// global handle		

	// default settings for windows class
	wc.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc		= WndProc;
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= m_hInstance;
	wc.hIcon			= LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm			= wc.hIcon;
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground	= (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName		= NULL;
	wc.lpszClassName	= m_appName;
	wc.cbSize			= sizeof(WNDCLASSEX);

	RegisterClassEx(&wc);

	// get clients desktop resolution
	width	= GetSystemMetrics(SM_CXSCREEN);
	height	= GetSystemMetrics(SM_CYSCREEN);

	// set screen settings
	if (FULL_SCREEN)
	{
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize			= sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth	= (unsigned long)width;
		dmScreenSettings.dmPelsHeight	= (unsigned long)height;
		dmScreenSettings.dmBitsPerPel	= 32;
		dmScreenSettings.dmFields		= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		posX = posY = 0;
	}
	else
	{
		width = SCREEN_WIDTH;
		height = SCREEN_HEIGHT;

		posX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	}

	// create window with screen settings
	m_hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,		// Forces a top-level window onto the taskbar when the window is visible.
		m_appName,				// class name
		m_appName,				// Window name
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		posX, posY, width, height,
		NULL, NULL,				// parent, menu
		m_hInstance, NULL
		);

	if (m_hwnd == nullptr)
	{
		MessageBox(NULL, "CreateWindowEx() failed", "Error", MB_OK);
		ENGINE->Exit();
		PostQuitMessage(0);
		return;
	}

	// init engine here maybe?

	// focus window
	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	ShowCursor(false);

	return;
}

void BaseSystem::ShutdownWindows()
{
	ShowCursor(true);

	if (FULL_SCREEN)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(m_hwnd);
	m_hwnd = NULL;

	// Remove the application instance.
	UnregisterClass(m_appName, m_hInstance);
	m_hInstance = NULL;

	// Release the pointer to this class.
	gp_appHandle = NULL;

	return;
}

//-----------------------------------------------------------------------------
// The WndProc function is where windows sends its messages to. You'll notice 
// we tell windows the name of it when we initialize the window class with 
// wc.lpfnWndProc = WndProc in the InitializeWindows function above.
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
	// Check if the window is being destroyed.
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}

	// Check if the window is being closed.
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
	}

	// All other messages pass to the message handler in the system class.
	default:
	{
		return gp_appHandle->MessageHandler(hwnd, umessage, wparam, lparam);
	}
	}
}
