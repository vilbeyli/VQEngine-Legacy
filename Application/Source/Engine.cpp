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

#include "Engine.h"

#include "Renderer.h"
#include "Input.h"
#include "Camera.h"

#include "Mesh.h"

#include <sstream>

Engine* Engine::s_instance = nullptr;

Engine::Engine()
	:
	m_renderer(nullptr),
	m_input(nullptr),
	m_camera(nullptr),
	m_isPaused(false)
{}

Engine::~Engine(){}

bool Engine::Initialize(HWND hWnd, int scr_width, int scr_height)
{
	m_renderer = new Renderer();
	if (!m_renderer) return false;
	m_input = new Input();
	if (!m_input)	return false;
	m_camera = new Camera(m_input);
	if (!m_camera)	return false;
	
	// initialize systems
	m_input->Init();
	if(!m_renderer->Init(scr_width, scr_height, hWnd)) 
		return false;

	m_camera->SetOthoMatrix(m_renderer->WindowWidth(), m_renderer->WindowHeight(), NEAR_PLANE, FAR_PLANE);
	m_camera->SetProjectionMatrix((float)XM_PIDIV4, m_renderer->AspectRatio(), NEAR_PLANE, FAR_PLANE);
	m_camera->SetPosition(0, 0, -40);
	m_renderer->SetCamera(m_camera);

	return true;
}

bool Engine::Load()
{
	const char* shaderRoot	= "Data/Shaders/";
	const char* textureRoot	= "Data/Textures/";
	std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 },
	};
	m_renderData.unlitShader	= m_renderer->AddShader("UnlitTextureColor", shaderRoot, layout);
	m_renderData.texCoordShader = m_renderer->AddShader("TextureCoord", shaderRoot, layout);
	m_renderData.normalShader	= m_renderer->AddShader("Normal", shaderRoot, layout);
	m_renderData.phongShader	= m_renderer->AddShader("Forward_Phong", shaderRoot, layout);
	m_renderData.exampleTex		= m_renderer->AddTexture("bricks_d.png", textureRoot);
	m_renderData.exampleNormMap	= m_renderer->AddTexture("bricks_n.png", textureRoot);
	m_sceneMan.Initialize(m_renderer, m_renderData, m_camera);

	m_timer.Reset();
	return true;
}

void Engine::TogglePause()
{
	m_isPaused = !m_isPaused;
}

void Engine::CalcFrameStats()
{
	static long frameCount = 0;
	static float timeElaped = 0.0f;

	++frameCount;
	if (m_timer.TotalTime() - timeElaped >= 1.0f)
	{
		float fps = static_cast<float>(frameCount);	// #frames / 1.0f
		float frameTime = 1000.0f / fps;	// milliseconds

		std::ostringstream stats;
		stats.precision(6);
		stats << "VDemo | "
			<< "FPS: " << fps << " "
			<< "FrameTime: " << frameTime << "ms";
		SetWindowText(m_renderer->GetWindow(), stats.str().c_str());
		frameCount = 0;
		timeElaped += 1.0f;
	}
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
		m_renderer->Exit();
		delete m_renderer;
		m_renderer = nullptr;
	}

	if (s_instance)
	{
		delete s_instance;
		s_instance = nullptr;
	}
}

const Input* Engine::INP() const
{
	return m_input;
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
	//float begin = m_timer.CurrentTime();	// fps lock
	m_timer.Tick();
	if (m_input->IsKeyDown(VK_ESCAPE))
	{
		return false;
	}

	if (m_input->IsKeyTriggered(0x08)) // Key backspace
		TogglePause(); 

	if (!m_isPaused)
	{
		CalcFrameStats();
		Update(m_timer.DeltaTime());
		Render();
		
		// 70 fps block - won't work because of input lag
		//float end = 0.0f;
		//do 
		//{
		//	end = m_timer.CurrentTime();
		//	//char info[128];
		//	//sprintf_s(info, "end=%f, begin=%f, end-begin=%f\n", end, begin, end - begin);
		//	//OutputDebugString(info);
		//} while ((end-begin) < 0.01428f);		
	}

	m_input->Update();	// update previous state after frame;

#ifdef _DEBUG
	m_renderer->PollShaderFiles();
#endif
	return true;
}

void Engine::Pause()
{
	m_isPaused = true;
}

void Engine::Unpause()
{
	m_isPaused = false;
}

void Engine::Update(float dt)
{
	m_camera->Update(dt);
	m_sceneMan.Update(dt);
}

void Engine::Render()
{
	const float clearColor[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	XMMATRIX view = m_camera->GetViewMatrix();
	XMMATRIX proj = m_camera->GetProjectionMatrix();
	m_renderer->Begin(clearColor);
	m_renderer->SetViewport(m_renderer->WindowWidth(), m_renderer->WindowHeight());
	m_sceneMan.Render(view, proj);	// renders room
	m_renderer->End();
}

