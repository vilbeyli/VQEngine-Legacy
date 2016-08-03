#pragma once

#include <windows.h>

class Renderer;
class Input;

class Engine
{
	friend class BaseSystem;

public:
	~Engine();

	bool Initialize(HWND hWnd, int scr_width, int scr_height);
	bool Run();

	void Update();
	void Render();

	void Exit();

	static Engine* GetEngine();

private:
	Engine();

private:
	Renderer*		m_renderer;
	Input*			m_input;
	static Engine*	s_instance;
};

#define ENGINE Engine::GetEngine()
