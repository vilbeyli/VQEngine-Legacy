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
#include "PerfTimer.h"
#include "Settings.h"
#include "WorkerPool.h"

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

class Engine
{
	friend class BaseSystem;

public:
	~Engine();

	bool Initialize(HWND hwnd, const Settings::Window& windowSettings);
	bool Load();
	bool Run();

	void Pause();
	void Unpause();
	float TotalTime() const;

	void Update(float dt);
	void Render();

	void Exit();

	shared_ptr<const Input>		 INP() const;
	inline float GetTotalTime() const { return m_timer->TotalTime(); }

	static Engine* GetEngine();

private:
	Engine();

	void TogglePause();
	void CalcFrameStats();

//--------------------------------------------------------------

private:
	static Engine*					s_instance;
	
	std::shared_ptr<Input>			m_input;
	std::shared_ptr<Renderer>		m_renderer;
	std::shared_ptr<SceneManager>	m_sceneManager;
	std::shared_ptr<PerfTimer>		m_timer;

	bool							m_isPaused;
	WorkerPool						m_workerThreads;
};

#define ENGINE Engine::GetEngine()
