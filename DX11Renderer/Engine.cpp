#include "Engine.h"

#include "Renderer.h"
#include "Input.h"

Engine* Engine::s_instance = nullptr;

Engine::Engine()
{
	m_renderer	= nullptr;
	m_input		= nullptr;
}

Engine::~Engine(){}

bool Engine::Initialize(HWND hWnd, int scr_width, int scr_height)
{
	m_renderer = new Renderer();
	if (!m_renderer) return false;

	m_input = new Input();
	if (!m_input)	return false;
	
	
	// initialize systems
	m_input->Init();
	if(!m_renderer->Init(scr_width, scr_height, hWnd)) 
		return false;

	
	return true;
}

bool Engine::Run()
{
	Update();
	
	if (m_input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	Render();

	return true;
}

void Engine::Update()
{
}

void Engine::Render()
{
	m_renderer->MakeFrame();
}

void Engine::Exit()
{
	if (m_input)
	{
		delete m_input;
		m_input = nullptr;
	}

	if (m_renderer)
	{
		delete m_renderer;
		m_renderer = nullptr;
	}

	if (s_instance)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

Engine * Engine::GetEngine()
{
	if (s_instance == nullptr)
	{
		s_instance = new Engine();
	}

	return s_instance;
}
