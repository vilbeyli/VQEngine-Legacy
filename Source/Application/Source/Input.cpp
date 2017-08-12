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

#include <algorithm>

#define LOG


const std::unordered_map<const char*, KeyCode> Input::sKeyMap = []() {
	// keyboard mapping for windows keycodes. 
	// #define LOG to see each keycode when you press on output window
	// to be used for this->IsKeyDown("F4")
	std::unordered_map<const char*, KeyCode> m;
	m["F1"] = 112;	m["F2"] = 113;	m["F3"] = 114;	m["F4"] = 115;
	m["F5"] = 116;	m["F6"] = 117;	m["F7"] = 118;	m["F8"] = 119;
	m["F9"] = 120;	m["F10"] = 121;	m["F11"] = 122;	m["F12"] = 123;
	
	m["R"] = 82;		m["r"] = 82;

	m["\\"] = 220;		m[";"] = 186;
	m["'"] = 222;
	m["Shift"] = 16;	m["shift"] = 16;
	m["Enter"] = 13;
	return m;
}();


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

void Input::Initialize()
{
	memset(m_keys,	   false, sizeof(bool) * KEY_COUNT);
	memset(m_prevKeys, false, sizeof(bool) * KEY_COUNT);
}

void Input::KeyDown(KeyCode key)
{
	m_keys[key] = true;

#if defined(_DEBUG) && defined(LOG)
	Log::Info("KeyDown: %u", key);
#endif
}


void Input::KeyUp(KeyCode key)
{
#if defined(_DEBUG) && defined(LOG)
	Log::Info("KeyReleased: %u", key);
#endif
	m_keys[key] = false;
}

void Input::ButtonDown(KeyCode btn)
{
#if defined(_DEBUG) && defined(LOG)
	Log::Info("Mouse Button Down: %d", btn);
#endif

	m_buttons[btn] = true;
}

void Input::ButtonUp(KeyCode btn)
{
#if defined(_DEBUG) && defined(LOG)
	Log::Info("Mouse Button Up: %d", btn);
#endif

	m_buttons[btn] = false;
}

void Input::UpdateMousePos(long x, long y)
{
#ifdef ENABLE_RAW_INPUT
	m_mouseDelta[0] = x;
	m_mouseDelta[1] = y;

	// unused for now
	m_mousePos[0] = 0;
	m_mousePos[1] = 0;
#else
	m_mouseDelta[0] = max(-1, min(x - m_mousePos[0], 1));
	m_mouseDelta[1] = max(-1, min(y - m_mousePos[1], 1));

	m_mousePos[0] = x;
	m_mousePos[1] = y;
#endif

#if defined(_DEBUG) && defined(LOG)
	Log::Info("Mouse Delta: (%d, %d)\tMouse Position: (%d, %d)", 
		m_mouseDelta[0], m_mouseDelta[1],
		m_mousePos[0], m_mousePos[1]);
#endif
	m_isConsumed = false;
}

bool Input::IsKeyDown(KeyCode key) const
{
	return m_keys[key];
}

bool Input::IsKeyDown(const char * key) const
{
	const KeyCode code = sKeyMap.at(key);
	return m_keys[code];
}

bool Input::IsKeyDown(const std::string& key) const
{
	const KeyCode code = sKeyMap.at(key.c_str());
	return m_keys[code];
}

bool Input::IsMouseDown(KeyCode btn) const
{
	return m_buttons[btn];
}

bool Input::IsKeyTriggered(KeyCode key) const
{
	return !m_prevKeys[key] && m_keys[key];
}

bool Input::IsKeyTriggered(const char *key) const
{
	const KeyCode code = sKeyMap.at(key);
	return !m_prevKeys[code] && m_keys[code];
}

bool Input::IsKeyTriggered(const std::string & key) const
{
	const KeyCode code = sKeyMap.at(key.data());
	return !m_prevKeys[code] && m_keys[code];
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
