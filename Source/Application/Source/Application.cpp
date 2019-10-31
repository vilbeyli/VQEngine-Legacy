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


#include "Engine/Engine.h"
#include "Engine/Settings.h"

#include "Utilities/utils.h"
#include "Utilities/CustomParser.h"
#include "Utilities/Log.h"
#include "Utilities/Profiler.h"

#include <strsafe.h>
#include <vector>
#include <new>

#include <shellapi.h> // command-line argument parsing

#ifdef _DEBUG
#include <cassert>
#endif

#define xLOG_WINDOW_EVENTS

 // to be assigned on App::init();
// used to redirect message handling from WndProc to class MessageHandler().
//
static Application* pApplication = nullptr;

static std::unordered_map<Application::EDirectories, std::string> DEFAULT_WORKSPACE_DIRECTORIES =
{
	  { Application::EDirectories::APP_DATA            , DirectoryUtil::GetSpecialFolderPath(DirectoryUtil::ESpecialFolder::LOCALAPPDATA) + "/VQEngine" }
	, { Application::EDirectories::LOGS                , DirectoryUtil::GetSpecialFolderPath(DirectoryUtil::ESpecialFolder::LOCALAPPDATA) + "/VQEngine/Logs" }
	, { Application::EDirectories::SHADER_BINARY_CACHE , DirectoryUtil::GetSpecialFolderPath(DirectoryUtil::ESpecialFolder::LOCALAPPDATA) + "/VQEngine/ShaderCache" }
	, { Application::EDirectories::SHADER_SOURCE       , "./Source/Shaders/" }
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------
	, { Application::EDirectories::MESHES              , "./Data/Meshes" }
	, { Application::EDirectories::MATERIALS           , "./Data/Materials" }
	, { Application::EDirectories::MODELS              , "./Data/Models" }
	, { Application::EDirectories::SCENES              , "./Data/Scenes" }
	, { Application::EDirectories::FONTS               , "./Data/Fonts" }
	, { Application::EDirectories::TEXTURES            , "./Data/Textures/" }
	, { Application::EDirectories::ENVIRONMENT_MAPS    , "./Data/Textures/EnvironmentMaps/"}
};

std::unordered_map<Application::EDirectories, std::string> Application::mWorkspaceDirectoryLookup = DEFAULT_WORKSPACE_DIRECTORIES;

const std::string& Application::GetDirectory(EDirectories dirEnum)
{
#if _DEBUG
	assert(mWorkspaceDirectoryLookup.find(dirEnum) != mWorkspaceDirectoryLookup.end());
#endif
	return mWorkspaceDirectoryLookup.at(dirEnum);
}

Application::Application(const char* psAppName)
	:
	m_appName(psAppName),
	m_bAppWantsExit(false)
{
	m_hInstance		= GetModuleHandle(NULL);	// instance of this application
}

Application::~Application(){}


void Application::Exit()
{
	ENGINE->Exit();
	ShutdownWindows();
	pApplication = nullptr;
}

bool Application::Init(HINSTANCE hInst, HINSTANCE hPrevInst, PSTR pScmdl, int iCmdShow)
{
	pApplication = this;

	//
	// COMMAND-LINE ARGUMENTS
	//
	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	this->ParseCommandLineArguments(argv, argc);
	LocalFree(argv);

	//
	// SETTINGS, LOGGING & DIRECTORIES
	//
	Settings::Engine& settings = const_cast<Settings::Engine&>(Engine::ReadSettingsFromFile());	// namespace doesn't make sense.
	Log::Initialize(settings.logger);
	DirectoryUtil::CreateFolderIfItDoesntExist(Application::GetDirectory(Application::EDirectories::SHADER_BINARY_CACHE));

	//
	// WINDOW
	//
	InitWindow(settings.window);
	ShowWindow(m_hwnd, SW_SHOW);

#ifdef ENABLE_RAW_INPUT
	//
	// INPUT
	//
	InitRawInputDevices();
#endif

	//
	// ENGINE
	//
	if (!ENGINE->Initialize(this))
	{
		Log::Error("Could not initialize VQEngine. Exiting...");
		return false;
	}
	
	return true;
}	


void Application::Run()
{
	if (!ENGINE->Load())
	{
		Log::Error("Could not load VQEngine. Exiting...");
		m_bAppWantsExit = true;
	}

	MSG msg = { };
	while (msg.message != WM_QUIT)
	{
		// Process any messages in the queue.
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (m_bAppWantsExit)
		{
			PostQuitMessage(0);
		}
	}

	Exit();
}

static std::unordered_map<UINT, Input::EMouseButtons> sWindowsMessageToMouseButtonMapping =
{
	  { WM_MBUTTONUP, (Input::EMouseButtons)16 }
	, { WM_RBUTTONUP, (Input::EMouseButtons)2 }
	, { WM_LBUTTONUP, (Input::EMouseButtons)1 }	
};

LRESULT CALLBACK Application::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	const Settings::Window& setting = Engine::sEngineSettings.window;
	switch (umsg)
	{

	//-------------------------------------------------------------------
	// WINDOW
	//-------------------------------------------------------------------
	case WM_PAINT:
		ENGINE->SimulateAndRenderFrame();
		break;

	case WM_CLOSE:		// Check if the window is being closed.
		Log::Info("[WANT EXIT X]");
		m_bAppWantsExit = true;
		PostQuitMessage(0);
		break;

	case WM_ACTIVATE:	// application active/inactive
		LOWORD(wparam) == WA_INACTIVE
			? ENGINE->OnWindowLoseFocus()
			: ENGINE->OnWindowGainFocus();
		break;

	case WM_MOVE:
		ENGINE->OnWindowMove();
		break;

	// window resizing
	case WM_ENTERSIZEMOVE:
		Log::Info("WM_ENTERSIZEMOVE");
		break;
	case WM_EXITSIZEMOVE:
		Log::Info("WM_EXITSIZEMOVE");
		break;
	case WM_SIZE:
		ENGINE->OnWindowResize();
		break;

	// prevent window from becoming too small
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lparam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lparam)->ptMinTrackSize.y = 200;
		break;

	//-------------------------------------------------------------------
	// KEYBOARD
	//-------------------------------------------------------------------
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		ENGINE->OnKeyDown((KeyCode)wparam);
		break;

	case WM_KEYUP:
		ENGINE->OnKeyUp((KeyCode)wparam);
		break;
	
	//-------------------------------------------------------------------
	// MOUSE
	//-------------------------------------------------------------------
	case WM_MBUTTONDOWN:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		ENGINE->OnMouseDown((Input::EMouseButtons)wparam);
		break;
	
	// wparam seems to be 0 for all *MBUTTONUP events -> hardcoding the mapping in Input.h.
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONUP:
		ENGINE->OnMouseUp(sWindowsMessageToMouseButtonMapping.at(umsg));
		break;
	
#ifdef ENABLE_RAW_INPUT
	// raw input for mouse - see: https://msdn.microsoft.com/en-us/library/windows/desktop/ee418864.aspx
	case WM_INPUT:
	{
		// read raw input data ---------------------------------
		constexpr UINT RAW_INPUT_SIZE = 48;
		LPBYTE inputBuffer[RAW_INPUT_SIZE]; UINT rawInputSz = 48;
		ZeroMemory(inputBuffer, RAW_INPUT_SIZE);
		GetRawInputData(
			(HRAWINPUT)lparam,
			RID_INPUT,
			inputBuffer,
			&rawInputSz,
			sizeof(RAWINPUTHEADER));
		RAWINPUT* raw = (RAWINPUT*)inputBuffer;
		// read raw input data ---------------------------------

		if (raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
		{
			ENGINE->OnMouseMove(raw->data.mouse.lLastX, raw->data.mouse.lLastY, raw->data.mouse.usButtonData);
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

#else // ENABLE_RAW_INPUT
	// client area mouse - not good for first person camera
	case WM_MOUSEMOVE:
		ENGINE->OnMouseMove(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), scroll);
		break;
#endif // ENABLE_RAW_INPUT

	default:
	{
		return DefWindowProc(hwnd, umsg, wparam, lparam);
	}
	}

	return 0;
}
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
		return pApplication->MessageHandler(hwnd, umessage, wparam, lparam);
	}
}



void Application::UpdateWindowDimensions(int w, int h)
{
	m_windowHeight = h;
	m_windowWidth = w;
}

void Application::ParseCommandLineArguments(LPWSTR* argv, int argc)
{
	if (argc == 1) // no arguments, only exe path.
	{
		return;
	}

	// parse each argument
	for (int i = 1; i < argc; ++i)
	{
		LPWSTR arg = argv[i];

		// TODO: parse 
	}
}

void Application::InitWindow(Settings::Window& windowSettings)
{
	int width, height;
	int posX, posY;				// window position

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
		WS_OVERLAPPED | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SYSMENU, // Window style
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

	return;
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
