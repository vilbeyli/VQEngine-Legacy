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

#include <string>

#ifdef _DEBUG
#include <Windows.h>
#endif

#define LOG

Input::Input()
	:
	m_isConsumed(false)
{
	memset(m_mouseDelta, 0, 2 * sizeof(long));
	memset(m_mousePos  , 0, 2 * sizeof(long));
}

Input::Input(const Input &)
{
}


Input::~Input()
{
}

void Input::Init()
{
	memset(m_keys,	   false, sizeof(bool) * KEY_COUNT);
	memset(m_prevKeys, false, sizeof(bool) * KEY_COUNT);
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

void Input::UpdateMousePos(long x, long y)
{
	m_mouseDelta[0] = x;
	m_mouseDelta[1] = y;

	// unused for now
	m_mousePos[0] = 0;
	m_mousePos[1] = 0;

#if defined(_DEBUG) && defined(N_LOG)
	char info[128];
	sprintf_s(info, "Mouse Delta: (%d, %d)\tMouse Pos: (%d, %d)\n", 
		m_mouseDelta[0], m_mouseDelta[1],
		m_mousePos[0], m_mousePos[1]);
	OutputDebugString(info);
#endif
	m_isConsumed = false;
}

bool Input::IsKeyDown(KeyCode key) const
{
	return m_keys[key];
}

bool Input::IsMouseDown(KeyCode btn) const
{
	return m_buttons[btn];
}

bool Input::IsKeyTriggered(KeyCode key) const
{
	return !m_prevKeys[key] && m_keys[key];
}

int Input::MouseDeltaX() const
{
	return m_mouseDelta[0];
}

int Input::MouseDeltaY() const
{
	return m_mouseDelta[1];
}

// called at the end of the frame
void Input::Update()
{
	memcpy(m_prevKeys, m_keys, sizeof(bool) * KEY_COUNT);
	m_mouseDelta[0] = m_mouseDelta[1] = 0;
	m_isConsumed = true;
}

const long * Input::GetDelta() const
{
	return m_mouseDelta;
}

bool Input::IsConsumed() const
{
	return m_isConsumed;
}
