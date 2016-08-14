#include "Engine.h"

#include "Renderer.h"
#include "Input.h"
#include "Camera.h"

Engine* Engine::s_instance = nullptr;

Engine::Engine()
{
	m_renderer	= nullptr;
	m_input		= nullptr;
	m_camera	= nullptr;
}

Engine::~Engine(){}

bool Engine::Initialize(HWND hWnd, int scr_width, int scr_height)
{
	m_renderer = new Renderer();
	if (!m_renderer) return false;
	m_input = new Input();
	if (!m_input)	return false;
	m_camera = new Camera();
	if (!m_camera)	return false;
	
	// initialize systems
	m_input->Init();
	if(!m_renderer->Init(scr_width, scr_height, hWnd)) 
		return false;

	m_camera->SetOthoMatrix(scr_width, scr_height, NEAR_PLANE, FAR_PLANE);
	m_camera->SetProjectionMatrix((float)XM_PIDIV4, SCREEN_RATIO, NEAR_PLANE, FAR_PLANE);
	m_camera->SetPosition(0, 0, -1);
	
	return true;
}

bool Engine::Load()
{
	m_renderer->AddShader("tex", "Data/Shaders/");

	return true;
}

bool Engine::Run()
{
	// TODO: remove to input management?
	if (m_input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	Update();
	Render();

	return true;
}

void Engine::Update()
{
	m_camera->Update();
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
