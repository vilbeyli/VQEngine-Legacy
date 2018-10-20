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

#include "Input.h"
#include "SystemDefs.h"

#include <algorithm>

#include <winuser.rh>

// #define LOG to see each keycode when you press on output window
#define xLOG


// keyboard mapping for windows keycodes.
// use case: this->IsKeyDown("F4")
const Input::KeyMapping Input::sKeyMap = []() 
{	
	KeyMapping m;
	m["F1"] = 112;	m["F2"] = 113;	m["F3"] = 114;	m["F4"] = 115;
	m["F5"] = 116;	m["F6"] = 117;	m["F7"] = 118;	m["F8"] = 119;
	m["F9"] = 120;	m["F10"] = 121;	m["F11"] = 122;	m["F12"] = 123;
	
	m["0"] = 48;		m["1"] = 49;	m["2"] = 50;	m["3"] = 51;
	m["4"] = 52;		m["5"] = 53;	m["6"] = 54;	m["7"] = 55;
	m["8"] = 56;		m["9"] = 57;
	
	m["A"] = 65;		m["a"] = 65;	m["B"] = 66;	m["b"] = 66;
	m["C"] = 67;		m["c"] = 67;	m["N"] = 78;	m["n"] = 78;
	m["R"] = 82;		m["r"] = 82;	m["T"] = 'T';	m["t"] = 'T';
	m["F"] = 'F';		m["f"] = 'F';
	

	m["\\"] = 220;		m[";"] = 186;
	m["'"] = 222;
	m["Shift"] = 16;	m["shift"] = 16;
	m["Enter"] = 13;	m["enter"] = 13;
	m["Backspace"] = 8; m["backspace"] = 8;
	m["Escape"] = 0x1B; m["escape"] = 0x1B; m["ESC"] = 0x1B; m["esc"] = 0x1B;
	m["PageUp"] = 33;	m["PageDown"] = 34;
	m["Space"] = VK_SPACE; m["space"] = VK_SPACE;

	m["ctrl"] = VK_CONTROL;  m["Ctrl"] = VK_CONTROL;
	m["rctrl"] = VK_RCONTROL; m["RCtrl"] = VK_RCONTROL; m["rCtrl"] = VK_RCONTROL;
	m["alt"] = VK_MENU;	m["Alt"] = VK_MENU;


	m["Numpad7"] = 103;		m["Numpad8"] = 104;			m["Numpad9"] = 105;
	m["Numpad4"] = 100;		m["Numpad5"] = 101;			m["Numpad6"] = 102;
	m["Numpad1"] = 97 ;		m["Numpad2"] = 98 ;			m["Numpad3"] = 99;
	m["Numpad+"] = VK_ADD;	m["Numpad-"] = VK_SUBTRACT;	
	m["+"]		 = VK_ADD;	m["-"]		 = VK_SUBTRACT;	
	return std::move(m);
}();


Input::Input()
	:
	m_mouseScroll(0),
	m_bIgnoreInput(false)
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

void Input::UpdateMousePos(long x, long y, short scroll)
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
	Log::Info("Mouse Delta: (%d, %d)\tMouse Position: (%d, %d)\tMouse Scroll: (%d)", 
		m_mouseDelta[0], m_mouseDelta[1],
		m_mousePos[0], m_mousePos[1],
		(int)scroll);
#endif
	m_mouseScroll = scroll;
}

bool Input::IsScrollUp() const
{
	return m_mouseScroll > 0 && !m_bIgnoreInput;
}

bool Input::IsScrollDown() const
{
	return m_mouseScroll < 0 && !m_bIgnoreInput;
}

bool Input::IsKeyDown(KeyCode key) const
{
	return m_keys[key] && !m_bIgnoreInput;
}

bool Input::IsKeyDown(const char * key) const
{
	const KeyCode code = sKeyMap.at(key);
	return m_keys[code] && !m_bIgnoreInput;
}

bool Input::IsKeyUp(const char * key) const
{
	const KeyCode code = sKeyMap.at(key);
	return (!m_keys[code] && m_prevKeys[code]) && !m_bIgnoreInput;
}

bool Input::IsKeyDown(const std::string& key) const
{
	const KeyCode code = sKeyMap.at(key.c_str());
	return m_keys[code] && !m_bIgnoreInput;
}

bool Input::IsMouseDown(KeyCode btn) const
{
	return m_buttons[btn] && !m_bIgnoreInput;
}

bool Input::IsKeyTriggered(KeyCode key) const
{
	return !m_prevKeys[key] && m_keys[key] && !m_bIgnoreInput;
}

bool Input::IsKeyTriggered(const char *key) const
{
	const KeyCode code = sKeyMap.at(key);
	return !m_prevKeys[code] && m_keys[code] && !m_bIgnoreInput;
}

bool Input::IsKeyTriggered(const std::string & key) const
{
	const KeyCode code = sKeyMap.at(key.data());
	return !m_prevKeys[code] && m_keys[code] && !m_bIgnoreInput;
}

int Input::MouseDeltaX() const
{
	return !m_bIgnoreInput ? m_mouseDelta[0] : 0;
}

int Input::MouseDeltaY() const
{
	return !m_bIgnoreInput ? m_mouseDelta[1] : 0;
}

// called at the end of the frame
void Input::PostUpdate()
{
	memcpy(m_prevKeys, m_keys, sizeof(bool) * KEY_COUNT);
	m_mouseDelta[0] = m_mouseDelta[1] = 0;
	m_mouseScroll = 0;
}

const long * Input::GetDelta() const
{
	return m_mouseDelta;
}
