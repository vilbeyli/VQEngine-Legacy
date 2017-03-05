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
#include "SystemDefs.h"
#include "PerfTimer.h"

class Renderer;
class Input;
class Camera;
class PathManager;
class PhysicsEngine;
class SceneManager;
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

	const Input* INP() const;
	const PerfTimer TIMER() const;

	static Engine* GetEngine();

private:
	Engine();

	void TogglePause();
	void CalcFrameStats();
//--------------------------------------------------------------
public:
	RenderData*		m_renderData;

private:
	Input*			m_input;		// ownership
	Renderer*		m_renderer;		// ownership
	PhysicsEngine*	m_physics;		// ownership
	Camera*			m_camera;		// ownership
	PathManager*	m_pathMan;		// ownership

	bool			m_isPaused;

	PerfTimer		m_timer;
	static Engine*	s_instance;

	// Scene
	SceneManager*	m_sceneMan;
};

#define ENGINE Engine::GetEngine()
