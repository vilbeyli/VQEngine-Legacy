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

#ifdef _DEBUG
#include <string>
#include <Windows.h>
#endif

Input::Input()
{
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

#ifdef _DEBUG
	std::string info("KeyDown: "); info += std::to_string(key); info += "\n";
	OutputDebugString(info.c_str());
#endif
}

void Input::KeyUp(KeyCode key)
{
#ifdef _DEBUG
	std::string info("KeyReleased: "); info += std::to_string(key); info += "\n";
	OutputDebugString(info.c_str());
#endif
	m_keys[key] = false;
}

bool Input::IsKeyDown(KeyCode key) const
{
	return m_keys[key];
}
