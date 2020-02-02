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

#pragma once

#include <unordered_map>
#include <string>

#define KEY_COUNT 256
#define ENABLE_RAW_INPUT

using KeyCode = unsigned int;

class Input
{
	using KeyMapping = std::unordered_map<std::string, KeyCode>;
	
	static const KeyMapping sKeyMap;
	friend class Application;
public:
	enum EMouseButtons
	{	// windows btn codes
		MOUSE_BUTTON_LEFT = 1,
		MOUSE_BUTTON_RIGHT = 2,
		MOUSE_BUTTON_MIDDLE = 16
	};

	Input();
	Input(const Input&);
	~Input();

	void Initialize();
	inline void ToggleInputBypassing() { m_bIgnoreInput = !m_bIgnoreInput; }

	// update state
	void KeyDown(KeyCode);
	void KeyUp(KeyCode);
	void ButtonDown(EMouseButtons);
	void ButtonUp(EMouseButtons);
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
	bool IsWheelUp() const;
	bool IsWheelDown() const;
	bool IsWheelPressed() const;

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


// SOME TEMPLATE FUN HERE --------------------------------------------------------------------
public:
	template<class Args> inline bool are_all_true(int argc, Args...)
	{
		bool bAreAllDown = true;
		va_list args;	// use this unpack function variadic parameters
		va_start(args, argc);
		for (int i = 0; i < argc; ++i)
		{
			bAreAllDown &= va_arg(args, bool);
		}
		va_end(args);
		return bAreAllDown;
	}
	template<class... Args> bool AreKeysDown(int keyCount, Args&&... args)
	{ 	//-------------------------------------------------------------------------------------
		// Note:
		//
		// We want to feed each argument to IsKeyDown() which is not a tmeplate function.
		//
		// IsKeyDown(args)...; -> we can't do this to expand and feed the args to the function. 
		//
		// but if we enclose it in a template function like this: 
		//     f(argc, IsKeyDown(args)...)
		//
		// we can expand the arguments one by one into the IsKeyDown() function.
		// now if the function f is a template function, all its arguments will be bools
		// as the return type of IsKeyDown() is bool. We can simply chain them together to
		// get the final result. f() in this case is 'are_all_true()'.
		//-------------------------------------------------------------------------------------
		assert(false);	// there's a bug. doesn't work in release mode. don't use this for now.
		return are_all_true(keyCount, IsKeyDown(args)...);
	}
};

