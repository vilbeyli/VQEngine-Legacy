#include "Renderer.h"
#include "DXInterface.h"

#include <cassert>

Renderer::Renderer()
{
	m_Direct3D = nullptr;
}


Renderer::~Renderer()
{
}

bool Renderer::Init(int width, int height, HWND m_hWnd)
{

	bool result;


	// Create the Direct3D object.
	m_Direct3D = new DXInterface();
	if (!m_Direct3D)
	{
		assert(false);
		return false;
	}

	// Initialize the Direct3D object.
	result = m_Direct3D->Init(width, height, Renderer::VSYNC, m_hWnd, FULL_SCREEN, g_nearPlane, g_farPlane);
	if (!result)
	{
		MessageBox(m_hWnd, "Could not initialize Direct3D", "Error", MB_OK);
		return false;
	}

	return true;
}

void Renderer::Exit()
{
	// Release the Direct3D object.
	if (m_Direct3D)
	{
		m_Direct3D->Shutdown();
		delete m_Direct3D;
	}

	return;
}

bool Renderer::MakeFrame()
{
	bool success = Render();

	return success;
}

bool Renderer::Render()
{
	const float clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	m_Direct3D->BeginFrame(clearColor);

	m_Direct3D->EndFrame();

	return true;
}
