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

#pragma once

#include <windows.h>
#include <memory>
using std::shared_ptr;
using std::unique_ptr;

// forward decl
class Renderer;
class Input;
class PerfTimer;
class SceneParser;
class SceneManager;

class PathManager;		// unused
class PhysicsEngine;	// unused

struct RenderData;

class Engine
{
	friend class BaseSystem;

public:
	~Engine();

	bool Initialize(HWND hWnd, int scr_width, int scr_height);
	bool Load();
	bool Run();

	void Pause();
	void Unpause();
	float TotalTime() const;

	void Update(float dt);
	void Render();

	void Exit();

	shared_ptr<const Input>		 INP() const;
	shared_ptr<const PerfTimer>  TIMER() const;

	static Engine* GetEngine();

private:
	Engine();

	void TogglePause();
	void CalcFrameStats();

//--------------------------------------------------------------

public:
	RenderData*						m_pRenderData;

private:
	static Engine*					s_instance;
	// TODO : TYEPDEF POINTER TYPES...
	
	std::shared_ptr<Input>			m_input;
	std::shared_ptr<Renderer>		m_renderer;
//	std::unique_ptr<PhysicsEngine>	m_physics;
//	std::unique_ptr<PathManager>	m_pathMan;
	std::shared_ptr<SceneManager>	m_scene_manager;
	std::shared_ptr<PerfTimer>		m_timer;

	bool							m_isPaused;
};

#define ENGINE Engine::GetEngine()
