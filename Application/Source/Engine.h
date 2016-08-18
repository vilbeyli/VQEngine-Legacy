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
