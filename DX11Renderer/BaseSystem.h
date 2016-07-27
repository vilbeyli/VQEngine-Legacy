#ifndef _SYSTEMCLASS_H_
#define _SYSTEMCLASS_H_

#define WIN32_LEAN_AND_MEAN		// speeds up build process

#include <windows.h>

// forward declarations
class Input;
class Renderer;

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
	bool Frame();
	void InitWindows(int&, int&);
	void ShutdownWindows();

private:
	LPCSTR		m_appName;
	HINSTANCE	m_hInstance;
	HWND		m_hwnd;

	Input*		mp_Input;
	Renderer*	mp_Renderer;
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