#include "Input.h"



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
	// memset?
	for (size_t i = 0; i < KEY_COUNT; ++i)
		m_keys[i] = false;
}

void Input::KeyDown(KeyCode key)
{
	m_keys[key] = true;
}

void Input::KeyUp(KeyCode key)
{
	m_keys[key] = false;
}

bool Input::IsKeyDown(KeyCode key)
{
	return m_keys[key];
}
