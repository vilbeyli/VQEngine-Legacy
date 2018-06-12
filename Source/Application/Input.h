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

#include <unordered_map>
#include <string>

#define KEY_COUNT 256
#define ENABLE_RAW_INPUT

using KeyCode = unsigned int;

class Input
{
	static const std::unordered_map<const char*, KeyCode> sKeyMap;
	friend class BaseSystem;
public:
	Input();
	Input(const Input&);
	~Input();

	void Initialize();
	inline void ToggleInputBypassing() { m_bIgnoreInput = !m_bIgnoreInput; }

	// update state
	void KeyDown(KeyCode);
	void KeyUp(KeyCode);
	void ButtonDown(KeyCode);
	void ButtonUp(KeyCode);
	void UpdateMousePos(long x, long y, short scroll);

	bool IsKeyDown(KeyCode) const;
	bool IsKeyDown(const char*) const;
	bool IsKeyDown(const std::string&) const;
	bool IsKeyUp(const char*) const;
	bool IsMouseDown(KeyCode) const;
	//bool IsMouseDown(const char*) const;
	//bool IsMouseDown(const std::string&) const;
	bool IsKeyTriggered(KeyCode) const;
	bool IsKeyTriggered(const char*) const;
	bool IsKeyTriggered(const std::string&) const;
	int  MouseDeltaX() const;
	int  MouseDeltaY() const;
	bool IsScrollUp() const;
	bool IsScrollDown() const;

	void PostUpdate();
	const long* GetDelta() const;

private:
	// state
	bool m_bIgnoreInput;

	// keyboard
	bool m_keys[KEY_COUNT];
	bool m_prevKeys[KEY_COUNT];

	// mouse
	bool m_buttons[17];
	long m_mouseDelta[2];
	long m_mousePos[2];
	short m_mouseScroll;
};

