//	Copyright(C) 2018  - Volkan Ilbeyli
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

#include "Application/Application.h"

#include "Utilities/Log.h"

using namespace VQEngine;


//------------------------------------------------------
// OS EVENTS
//------------------------------------------------------
void Engine::OnKeyDown(KeyCode key)
{
#ifdef LOG_WINDOW_EVENTS
	Log::Info("[WM_KEYDOWN]");// :\t MouseCaptured = %s", m_bMouseCaptured ? "True" : "False");
#endif
	if (key == VK_ESCAPE)
	{
		if (!IsMouseCaptured() && !IsLoading())
		{
			mbStartEngineShutdown = true;
			Log::Info("[WANT EXIT ESC]");
		}
	}

	ENGINE->mpInput->KeyDown(key);
}

void Engine::OnKeyUp(KeyCode key)
{
#ifdef LOG_WINDOW_EVENTS
	Log::Info("WM_KEYUP");
#endif
	mpInput->KeyUp(key);
}

void Engine::OnMouseMove(long x, long y, short scroll)
{
	if (mbMouseCaptured)
	{
		mpInput->UpdateMousePos(x, y, scroll);
	}
}

void Engine::OnMouseDown(const Input::EMouseButtons& btn)
{
	if (mbMouseCaptured)
	{
		mpInput->ButtonDown(btn);
	}
	else
	{
		mpApp->CaptureMouse(true);
		mbMouseCaptured = true;
	}
}

void Engine::OnMouseUp(const Input::EMouseButtons& btn)
{
	if (mbMouseCaptured)
	{
		mpInput->ButtonUp(btn);
	}
}

void Engine::OnWindowGainFocus()
{
	if (!mpApp)
		return;
#ifdef LOG_WINDOW_EVENTS
	Log::Info("WM_ACTIVATE::WA_ACTIVE");
#endif
	mpApp->CaptureMouse(true);
	mbMouseCaptured = true;

	// paused = false
	// timer start
}

void Engine::OnWindowLoseFocus()
{
#ifdef LOG_WINDOW_EVENTS
	Log::Info("WM_ACTIVATE::WA_INACTIVE");
#endif
	//this->CaptureMouse(false);
	// paused = true
	// timer stop
}

void Engine::OnWindowMove()
{
	Log::Info("MOVE");
}

void Engine::OnWindowResize()
{
	Log::Info("[WM_SIZE]");
}