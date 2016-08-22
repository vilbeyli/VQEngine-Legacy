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

#include "Input.h"
#include "SystemDefs.h"

#ifdef _DEBUG
#include <string>
#include <Windows.h>
#endif

#define LOG

Input::Input()
{
	memset(m_mouseDelta, 0, 2 * sizeof(int));
	m_mousePos[0] = SCREEN_WIDTH / 2;
	m_mousePos[1] = SCREEN_HEIGHT / 2;
}

Input::Input(const Input &)
{
}


Input::~Input()
{
}

void Input::Init()
{
	for (size_t i = 0; i < KEY_COUNT; ++i)
		m_keys[i] = false;
}

void Input::KeyDown(KeyCode key)
{
	m_keys[key] = true;

#if defined(_DEBUG) && defined(LOG)
	std::string info("KeyDown: "); info += std::to_string(key); info += "\n";
	OutputDebugString(info.c_str());
#endif
}

void Input::KeyUp(KeyCode key)
{
#if defined(_DEBUG) && defined(LOG)
	std::string info("KeyReleased: "); info += std::to_string(key); info += "\n";
	OutputDebugString(info.c_str());
#endif
	m_keys[key] = false;
}

void Input::ButtonDown(KeyCode btn)
{
#if defined(_DEBUG) && defined(LOG)
	char msg[128];
	sprintf_s(msg, "Mouse Button Down: %d\n", btn);
	OutputDebugString(msg);
#endif

	m_buttons[btn] = true;
}

void Input::ButtonUp(KeyCode btn)
{
#if defined(_DEBUG) && defined(LOG)
	char msg[128];
	sprintf_s(msg, "Mouse Button Up: %d\n", btn);
	OutputDebugString(msg);
#endif

	m_buttons[btn] = false;
}

void Input::UpdateMousePos(int x, int y)
{
	m_mouseDelta[0] = x - m_mousePos[0];
	m_mouseDelta[1] = y - m_mousePos[1];
	m_mousePos[0] = x;
	m_mousePos[1] = y;

#if defined(_DEBUG) && defined(LOG)
	char info[128];
	sprintf_s(info, "Mouse Delta: %d, %d\n", m_mouseDelta[0], m_mouseDelta[1]);
	OutputDebugString(info);
#endif
}

bool Input::IsKeyDown(KeyCode key) const
{
	return m_keys[key];
}

bool Input::IsMouseDown(KeyCode btn) const
{
	return m_buttons[btn];
}

int Input::MouseDeltaX() const
{
	return m_mouseDelta[0];
}

int Input::MouseDeltaY() const
{
	return m_mouseDelta[1];
}
