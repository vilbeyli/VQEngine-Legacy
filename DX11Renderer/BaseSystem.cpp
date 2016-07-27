#include "BaseSystem.h"

#include "Input.h"
#include "Renderer.h"

#include <new>


BaseSystem::BaseSystem()
{
	mp_Input = NULL;
	mp_Renderer = NULL;
}


BaseSystem::BaseSystem(const BaseSystem& other)
{

}

BaseSystem::~BaseSystem()
{
}

bool BaseSystem::Init()
{
	int width, height;
	bool b_gfxInit;

	InitWindows(width, height);

	// create subclasses
	mp_Input	= new Input();
	mp_Renderer = new Renderer();
	if (!mp_Input)		return false;
	if (!mp_Renderer)	return false;

	// initialize
	mp_Input->Init();
	b_gfxInit = mp_Renderer->Init(width, height, m_hwnd);
	return b_gfxInit;
}

void BaseSystem::Run()
{
	MSG msg;
	bool done, result;

	ZeroMemory(&msg, sizeof(MSG));

	done = false;
	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			result = Frame();
			if (!result)
			{
				done = true;
			}
		}
	}

	return;
}

void BaseSystem::Exit()
{
	if (mp_Input)
	{
		delete mp_Input;
	}

	if (mp_Renderer)
	{
		mp_Renderer->Exit();
		delete mp_Renderer;
	}

	ShutdownWindows();
	return;
}

LRESULT CALLBACK BaseSystem::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
	case WM_KEYDOWN:
	{
		mp_Input->KeyDown((KeyCode)wparam);
		return 0;
	}

	case WM_KEYUP:
	{
		mp_Input->KeyUp((KeyCode)wparam);
		return 0;
	}

	default:
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
}

bool BaseSystem::Frame()
{
	bool result;

	if (mp_Input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	result = mp_Renderer->MakeFrame();
	return result;
}

void BaseSystem::InitWindows(int& width, int& height)
{
	WNDCLASSEX wc;
	DEVMODE dmScreenSettings;
	int posX, posY;

	gp_appHandle	= this;						// global handle
	m_hInstance		= GetModuleHandle(NULL);	// instance of this application
	m_appName		= "DX11 Renderer";			

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
	if (Renderer::FULL_SCREEN)
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
		width = 800;
		height = 600;

		posX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	}

	// create window with screen settings
	m_hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		m_appName,
		m_appName,
		WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_POPUP,
		posX, posY, width, height,
		NULL, NULL, m_hInstance, NULL
		);

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

	if (Renderer::FULL_SCREEN)
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
