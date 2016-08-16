#pragma once

#include <windows.h>
#include "SystemDefs.h"

#include "Components/Transform.h"

class Renderer;
class Input;
class Camera;

class Engine
{
	friend class BaseSystem;

public:
	~Engine();

	bool Initialize(HWND hWnd, int scr_width, int scr_height);
	bool Load();
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

	Camera*			m_camera;

	Transform	m_tf;	//test

	static Engine*	s_instance;
};

#define ENGINE Engine::GetEngine()
