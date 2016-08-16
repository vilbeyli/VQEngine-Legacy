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
