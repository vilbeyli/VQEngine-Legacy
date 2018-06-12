//	VQEngine | DirectX11 Renderer
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

#pragma once 

// https://stackoverflow.com/questions/11040133/what-does-defining-win32-lean-and-mean-exclude-exactly
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx
#define WIN32_LEAN_AND_MEAN		// speeds up build process

#include <windows.h>
#include <string>

namespace Settings { struct Window; }

class BaseSystem
{
public: 
	static std::string s_WorkspaceDirectory;

public:
	BaseSystem();
	BaseSystem(const BaseSystem&);
	~BaseSystem();

	bool Init();
	void Run();
	void Exit();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);
	void UpdateWindowDimensions(int w, int h);

private:
	void InitRawInputDevices();
	void InitWindow(const Settings::Window& windowSettings);
	void ShutdownWindows();
	
	void CaptureMouse(bool bDoCapture);

private:
	LPCSTR		m_appName;
	HINSTANCE	m_hInstance;
	HWND		m_hwnd;
	int			m_windowWidth, m_windowHeight;

	// state
	bool		m_bMouseCaptured;
	bool		m_bAppWantsExit;
	POINT		m_capturePosition;
};

// The WndProc function and ApplicationHandle pointer are also included here so we can redirect 
// the windows system messaging into our MessageHandler function inside the system class.

static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
static BaseSystem* gp_appHandle = 0;
