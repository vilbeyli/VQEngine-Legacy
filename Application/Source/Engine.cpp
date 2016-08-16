#include "Engine.h"

#include "../Renderer/Source/Renderer.h"
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
	m_camera->SetPosition(0, 0, 1);
	m_renderer->SetCamera(m_camera);
	
	return true;
}

bool Engine::Load()
{
	m_renderer->AddShader("tex", "Data/Shaders/");

	return true;
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
	
	//-----------------------------------------------------------------------------------
	// TRANSLATION TEST
	if (m_input->IsKeyDown(VK_LEFT))	m_tf.Translate(Transform::Left);
	if (m_input->IsKeyDown(VK_RIGHT))	m_tf.Translate(Transform::Right);
	if (m_input->IsKeyDown(VK_UP))		m_tf.Translate(Transform::Forward);
	if (m_input->IsKeyDown(VK_DOWN))	m_tf.Translate(Transform::Backward);
	if (m_input->IsKeyDown(VK_SPACE))	m_tf.Translate(Transform::Up);
	if (m_input->IsKeyDown(VK_CONTROL))	m_tf.Translate(Transform::Down);
	
	{
		char info[128];
		XMFLOAT3 pos;
		XMStoreFloat3(&pos, m_tf.GetPosition());
		sprintf_s(info, 128, "Transform: %.2f, %.2f, %.2f\n", pos.x, pos.y, pos.z);
		OutputDebugString(info);
	}
	
	XMVECTOR pos = m_tf.GetPosition() * 0.1f;
	m_camera->SetPosition(pos.m128_f32[0], pos.m128_f32[1], pos.m128_f32[2]);

	// ROTATION TEST

	// SCALE TEST
}

void Engine::Render()
{
	m_renderer->MakeFrame();
}

