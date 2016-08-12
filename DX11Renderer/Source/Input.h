#pragma once

#define KEY_COUNT 256

typedef unsigned int KeyCode;

class Input
{
public:
	Input();
	Input(const Input&);
	~Input();

	void Init();

	void KeyDown(KeyCode);
	void KeyUp(KeyCode);
	bool IsKeyDown(KeyCode);

private:
	bool m_keys[KEY_COUNT];
};

