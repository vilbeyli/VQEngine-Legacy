//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
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

#define NOMINMAX
#include "Application.h"
#include "Input.h"

//#import 

#include "Engine/Engine.h"
#include "Engine/Settings.h"

#include "Utilities/utils.h"
#include "Utilities/CustomParser.h"
#include "Utilities/Log.h"
#include "Utilities/Profiler.h"

#include <strsafe.h>
#include <vector>
#include <new>

#ifdef _DEBUG
#include <cassert>
#endif

#define xLOG_WINDOW_EVENTS

std::string Application::s_WorkspaceDirectory = "";
std::string Application::s_ShaderCacheDirectory = "";

Application::Application()
	:
	m_appName("VQEngine Demo"),
	m_bMouseCaptured(false),
	m_bAppWantsExit(false),
	m_threadPool(VQEngine::ThreadPool::sHardwareThreadCount - 2)
{
	m_hInstance		= GetModuleHandle(NULL);	// instance of this application
}

Application::~Application(){}


void Application::Exit()
{
	ENGINE->Exit();
	ShutdownCOMInterface();
	ShutdownWindows();
}

bool Application::Init()
{
	// SETTINGS
	//
	s_WorkspaceDirectory = DirectoryUtil::GetSpecialFolderPath(DirectoryUtil::ESpecialFolder::APPDATA) + "/VQEngine";
	Settings::Engine& settings = const_cast<Settings::Engine&>(Engine::ReadSettingsFromFile());	// namespace doesn't make sense.

	// LOG
	//
	Log::Initialize(settings.logger);
	
	// WINDOW
	//
	InitWindow(settings.window);
	ShowWindow(m_hwnd, SW_SHOW);
	this->CaptureMouse(true);

#ifdef ENABLE_RAW_INPUT
	// INPUT
	//
	InitRawInputDevices();
#endif

	// UI
	// 
	std::string errMsg;
	if (!InitCOMInterface(errMsg))
	{
		Log::Error(errMsg);
		//return false;
	}

	// ENGINE
	//
	if (!ENGINE->Initialize(m_hwnd))
	{
		Log::Error("Could not initialize VQEngine. Exiting...");
		return false;
	}
	
	if (!ENGINE->Load(&m_threadPool))
	{
		Log::Error("Could not load VQEngine. Exiting...");
		return false;
	}

	return true;
}	

void Application::Run()
{
	ENGINE->mpTimer->Reset();
	ENGINE->mpTimer->Start();
	MSG msg = { };
	
	while (!m_bAppWantsExit)
	{
		// todo: keep dragging main window
		// game engine architecture
		// http://advances.realtimerendering.com/s2016/index.html
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);		// Translates virtual-key messages into character messages
			DispatchMessage(&msg);		// indirectly causes Windows to invoke WndProc
		}

		if (ENGINE->INP()->IsKeyUp("ESC"))
		{
			if (m_bMouseCaptured)		  { this->CaptureMouse(false); }
			else if(!ENGINE->IsLoading()) 
			{ 
				m_bAppWantsExit = true;    
			}
		}

		if (msg.message == WM_QUIT && !ENGINE->IsLoading())
		{
			m_bAppWantsExit = true;
		}
		
		ENGINE->SimulateAndRenderFrame();
		const_cast<Input*>(ENGINE->INP())->PostUpdate();	// update previous state after frame;
	}
}


LRESULT CALLBACK Application::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	const Settings::Window& setting = Engine::sEngineSettings.window;
	switch (umsg)
	{
	case WM_CLOSE:		// Check if the window is being closed.
		Log::Info("[WANT EXIT X]");
		m_bAppWantsExit = true;
		PostQuitMessage(0);
		break;

	case WM_SIZE:
#ifdef LOG_WINDOW_EVENTS
		Log::Info("[WM_SIZE]");
#endif
		break;

	case WM_ACTIVATE:	// application active/inactive
		if (LOWORD(wparam) == WA_INACTIVE)
		{
#ifdef LOG_WINDOW_EVENTS
			Log::Info("WM_ACTIVATE::WA_INACTIVE");
#endif
			//this->CaptureMouse(false);
			// paused = true
			// timer stop
		}
		else
		{
#ifdef LOG_WINDOW_EVENTS
			Log::Info("WM_ACTIVATE::WA_ACTIVE");
#endif
			this->CaptureMouse(true);
			// paused = false
			// timer start
		}
		break;

	// resize bar grab-release
	case WM_ENTERSIZEMOVE:
#ifdef LOG_WINDOW_EVENTS
		Log::Info("WM_ENTERSIZEMOVE");
#endif
		// paused = true
		// resizing = true
		// timer.stop()
		break;

	case WM_EXITSIZEMOVE:
#ifdef LOG_WINDOW_EVENTS
		Log::Info("WM_EXITSIZEMOVE");
#endif
		// paused = false
		// resizing= false
		// timer.start()
		// onresize()
		break;

	// prevent window from becoming too small
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lparam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lparam)->ptMinTrackSize.y = 200;
		break;

	// keyboard
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	{
#ifdef LOG_WINDOW_EVENTS
		Log::Info("[WM_KEYDOWN]");// :\t MouseCaptured = %s", m_bMouseCaptured ? "True" : "False");
#endif
		if ( wparam == VK_ESCAPE)
		{
			if (!m_bMouseCaptured && !ENGINE->IsLoading())
			{
				m_bAppWantsExit = true;
				Log::Info("[WANT EXIT ESC]");
			}
		}

		ENGINE->mpInput->KeyDown((KeyCode)wparam);
		break;
	}

	case WM_KEYUP:
	{
#ifdef LOG_WINDOW_EVENTS
		Log::Info("WM_UP");
#endif
		ENGINE->mpInput->KeyUp((KeyCode)wparam);
		break;
	}

	// mouse buttons
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	{
		if (m_bMouseCaptured)	ENGINE->mpInput->ButtonDown((Input::EMouseButtons)wparam);
		else					this->CaptureMouse(true);
		break;
	}

	// todo: wparam is 0 for all cases? 
	case WM_MBUTTONUP:
	{
		if (m_bMouseCaptured)
			ENGINE->mpInput->ButtonUp((Input::EMouseButtons)16);
		break;
	}
	case WM_RBUTTONUP:
	{
		if (m_bMouseCaptured)
			ENGINE->mpInput->ButtonUp((Input::EMouseButtons)2);
		break;
	}
	case WM_LBUTTONUP:
	{
		if (m_bMouseCaptured)
			ENGINE->mpInput->ButtonUp((Input::EMouseButtons)1);
		break;
	}

#ifdef ENABLE_RAW_INPUT
	// raw input for mouse - see: https://msdn.microsoft.com/en-us/library/windows/desktop/ee418864.aspx
	case WM_INPUT:	
	{
		UINT rawInputSize = 48;
		LPBYTE inputBuffer[48];
		ZeroMemory(inputBuffer, rawInputSize);

		GetRawInputData(
			(HRAWINPUT)lparam, 
			RID_INPUT,
			inputBuffer, 
			&rawInputSize, 
			sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)inputBuffer;

		if (raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
		{
			if (m_bMouseCaptured)
			{
				ENGINE->mpInput->UpdateMousePos(raw->data.mouse.lLastX, raw->data.mouse.lLastY, raw->data.mouse.usButtonData);
				//SetCursorPos(setting.width / 2, setting.height / 2);
			}
			
#ifdef LOG
			char szTempOutput[1024];
			StringCchPrintf(szTempOutput, STRSAFE_MAX_CCH, TEXT("%u  Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"),
				rawInputSize,
				raw->data.mouse.usFlags,
				raw->data.mouse.ulButtons,
				raw->data.mouse.usButtonFlags,
				raw->data.mouse.usButtonData,
				raw->data.mouse.ulRawButtons,
				raw->data.mouse.lLastX,
				raw->data.mouse.lLastY,
				raw->data.mouse.ulExtraInformation);
			OutputDebugString(szTempOutput);
#endif
		}
		break;
	}

#else
	// client area mouse - not good for first person camera
	case WM_MOUSEMOVE:
	{
		ENGINE->mpInput->UpdateMousePos(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), scroll);
		break;
	}
#endif

	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}

	return 0;
}

//-----------------------------------------------------------------------------
// The WndProc function is where windows sends its messages to. You'll notice 
// we tell windows the name of it when we initialize the window class with 
// wc.lpfnWndProc = WndProc in the InitializeWindows function above.
LRESULT CALLBACK WndProc(HWND hwnd, UINT umessage, WPARAM wparam, LPARAM lparam)
{
	switch (umessage)
	{
	case WM_DESTROY:	// Check if the window is being destroyed.
		PostQuitMessage(0);
		return 0;
		
	case WM_QUIT:		// Check if the window is being closed.
		PostQuitMessage(0);
		return 0;
	default: // All other messages pass to the message handler in the system class.
		return gp_appHandle->MessageHandler(hwnd, umessage, wparam, lparam);
	}
}



void Application::UpdateWindowDimensions(int w, int h)
{
	m_windowHeight = h;
	m_windowWidth = w;
}

void Application::LaunchControlPanelUI()
{

}

void Application::ShutdownControlPanelUI()
{

}

void Application::InitWindow(Settings::Window& windowSettings)
{
	int width, height;
	int posX, posY;				// window position
	gp_appHandle = this;		// global handle		

	// default settings for windows class
	WNDCLASSEX wc;
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = m_hInstance;
	wc.hIcon = LoadIcon(NULL, "VQEngine0.ico");
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = m_appName;
	wc.cbSize = sizeof(WNDCLASSEX);
	RegisterClassEx(&wc);

	// get clients desktop resolution
	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	// set screen settings
	if (windowSettings.fullscreen)
	{
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);
		dmScreenSettings.dmPelsWidth = (unsigned long)width;
		dmScreenSettings.dmPelsHeight = (unsigned long)height;
		dmScreenSettings.dmBitsPerPel = 32;
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
		ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN);

		posX = posY = 0;
	}
	else
	{
		width = std::min(windowSettings.width, width);
		height = std::min(windowSettings.height, height);

		if (width != windowSettings.width || height != windowSettings.height)
		{
			Log::Warning("Resolution not supported (%dx%d): Fallback to (%dx%d)"
				, windowSettings.width, windowSettings.height
				, width, height
			);

			windowSettings.width = width;
			windowSettings.height = height;
		}

		posX = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
		posY = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
	}

	// create window with screen settings
	m_hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,			// Forces a top-level window onto the taskbar when the window is visible.
		m_appName,					// class name
		m_appName,					// Window name
		WS_POPUP,					// Window style
		posX, posY, width, height,	// Window position and dimensions
		NULL, NULL,					// parent, menu
		m_hInstance, NULL
	);

	if (m_hwnd == NULL)
	{
		MessageBox(NULL, "CreateWindowEx() failed", "Error", MB_OK);
		ENGINE->Exit();
		PostQuitMessage(0);
		return;
	}
	return;
}

//extern "C" __declspec(dllimport) void TestFn();
bool Application::InitCOMInterface(std::string& errMsg)
{
	// =================================================================================================================================
	// https://docs.microsoft.com/en-us/windows/desktop/learnwin32/initializing-the-com-library
	// HR CoInitializeEx();
	// enum tagCOINIT: 
	// =================================================================================================================================
	// - COINIT_APARTMENTTHREADED: 
	//   App must guarantee the following:
	//     1) You will access each COM object from a single thread; you will not share COM interface pointers between multiple threads.
	//     2) The thread will have a message loop.
	//
	// - COINIT_MULTITHREADED
	//   If no guarantee to any of the above.
	// =================================================================================================================================
	tagCOINIT COM_mode = COINIT_APARTMENTTHREADED;
	HRESULT hr = CoInitializeEx(NULL /*reserved, must be NULL*/, COM_mode);
	if (FAILED(hr))
	{
		errMsg = std::string("Could not initialize COM interface, error=") + std::to_string(hr);
		return false;
	}
	Log::Info("COM Interface initialized successfully.");
	return true;
}

void Application::ShutdownWindows()
{
	ShowCursor(true);

	if (Engine::sEngineSettings.window.fullscreen)
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


void Application::ShutdownCOMInterface()
{
	CoUninitialize();
}

void Application::InitRawInputDevices()
{
	// register mouse for raw input
	// https://msdn.microsoft.com/en-us/library/windows/desktop/ms645565.aspx
	RAWINPUTDEVICE Rid[1];
	Rid[0].usUsagePage = (USHORT)0x01;	// HID_USAGE_PAGE_GENERIC;
	Rid[0].usUsage = (USHORT)0x02;	// HID_USAGE_GENERIC_MOUSE;
	Rid[0].dwFlags = 0;
	Rid[0].hwndTarget = m_hwnd;
	if (FALSE == (RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]))))	// Cast between semantically different integer types : a Boolean type to HRESULT.
	{
		OutputDebugString("Failed to register raw input device!");
	}

	// get devices and print info
	//-----------------------------------------------------
	UINT numDevices = 0;
	GetRawInputDeviceList(
		NULL, &numDevices, sizeof(RAWINPUTDEVICELIST));
	if (numDevices == 0) return;

	std::vector<RAWINPUTDEVICELIST> deviceList(numDevices);
	GetRawInputDeviceList(
		&deviceList[0], &numDevices, sizeof(RAWINPUTDEVICELIST));

	std::vector<wchar_t> deviceNameData;
	std::wstring deviceName;
	for (UINT i = 0; i < numDevices; ++i)
	{
		const RAWINPUTDEVICELIST& device = deviceList[i];
		if (device.dwType == RIM_TYPEMOUSE)
		{
			char info[1024];
			sprintf_s(info, "Mouse: Handle=0x%08p\n", device.hDevice);
			OutputDebugString(info);

			UINT dataSize = 0;
			GetRawInputDeviceInfo(
				device.hDevice, RIDI_DEVICENAME, nullptr, &dataSize);
			if (dataSize)
			{
				deviceNameData.resize(dataSize);
				UINT result = GetRawInputDeviceInfo(
					device.hDevice, RIDI_DEVICENAME, &deviceNameData[0], &dataSize);
				if (result != UINT_MAX)
				{
					deviceName.assign(deviceNameData.begin(), deviceNameData.end());

					char info[1024];
					std::string ndeviceName(deviceName.begin(), deviceName.end());
					sprintf_s(info, "  Name=%s\n", ndeviceName.c_str());
					OutputDebugString(info);
				}
			}

			RID_DEVICE_INFO deviceInfo;
			deviceInfo.cbSize = sizeof deviceInfo;
			dataSize = sizeof deviceInfo;
			UINT result = GetRawInputDeviceInfo(
				device.hDevice, RIDI_DEVICEINFO, &deviceInfo, &dataSize);
			if (result != UINT_MAX)
			{
#ifdef _DEBUG
				assert(deviceInfo.dwType == RIM_TYPEMOUSE);
#endif
				char info[1024];
				sprintf_s(info,
					"  Id=%u, Buttons=%u, SampleRate=%u, HorizontalWheel=%s\n",
					deviceInfo.mouse.dwId,
					deviceInfo.mouse.dwNumberOfButtons,
					deviceInfo.mouse.dwSampleRate,
					deviceInfo.mouse.fHasHorizontalWheel ? "1" : "0");
				OutputDebugString(info);
			}
		}
	}
}


void Application::CaptureMouse(bool bDoCapture)
{
#ifdef LOG_WINDOW_EVENTS
	Log::Info("Capture Mouse: %d", bDoCapture ? 1 : 0);
#endif

	m_bMouseCaptured = bDoCapture;
	if (bDoCapture)
	{
		RECT rcClip;	GetWindowRect(m_hwnd, &rcClip);

		// keep clip cursor rect inside pixel area
		constexpr int PX_OFFSET = 15;
		constexpr int PX_WND_TITLE_OFFSET = 30;
		rcClip.left += PX_OFFSET;
		rcClip.right -= PX_OFFSET;
		rcClip.top += PX_OFFSET + PX_WND_TITLE_OFFSET;
		rcClip.bottom -= PX_OFFSET;

		while (ShowCursor(FALSE) >= 0);
		ClipCursor(&rcClip);
		GetCursorPos(&m_capturePosition);
		SetForegroundWindow(m_hwnd);
		SetFocus(m_hwnd);

		const_cast<Input*>(ENGINE->INP())->m_bIgnoreInput = false;
	}
	else
	{
		ClipCursor(nullptr);
		SetCursorPos(m_capturePosition.x, m_capturePosition.y);
		while (ShowCursor(TRUE) <= 0);
		SetForegroundWindow(NULL);
		// SetFocus(NULL);	// we still want to register events
		const_cast<Input*>(ENGINE->INP())->m_bIgnoreInput = true;
	}
}
