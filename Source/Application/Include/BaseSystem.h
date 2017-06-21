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

#ifndef _SYSTEMCLASS_H_
#define _SYSTEMCLASS_H_

#define WIN32_LEAN_AND_MEAN		// speeds up build process

#include <windows.h>
#include "Settings.h"

class BaseSystem
{
public:
	BaseSystem();
	BaseSystem(const BaseSystem&);
	~BaseSystem();

	bool Init();
	void Run();
	void Exit();

	LRESULT CALLBACK MessageHandler(HWND, UINT, WPARAM, LPARAM);

private:
	void InitWindow(int&, int&);
	void ShutdownWindows();
	void InitRawInputDevices();

private:
	LPCSTR				m_appName;
	HINSTANCE			m_hInstance;
	HWND				m_hwnd;
	Settings::Window	m_settings;
};

// The WndProc function and ApplicationHandle pointer are also included in this class file so we can redirect 
// the windows system messaging into our MessageHandler function inside the system class.

// FUNCTION PROTOTYPES
//----------------------------------------------------------------------
static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// GLOBALS
//----------------------------------------------------------------------
static BaseSystem* gp_appHandle = 0;

#endif	// _SYSTEMCLASS_H_