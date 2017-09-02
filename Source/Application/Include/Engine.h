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
#include "WorkerPool.h"

#include "RenderPasses.h"
#include "Light.h"

#include <memory>
using std::shared_ptr;
using std::unique_ptr;


namespace Settings { struct Renderer; }

class Renderer;
class Input;
class SceneParser;
class SceneManager;

class PathManager;		// unused
class PhysicsEngine;	// unused


constexpr int MAX_POINT_LIGHT_COUNT = 20;
constexpr int MAX_SPOT_LIGHT_COUNT = 10;
struct SceneLightData
{
	std::array<LightShaderSignature, MAX_POINT_LIGHT_COUNT> pointLights;
	std::array<LightShaderSignature, MAX_SPOT_LIGHT_COUNT>  spotLights;
	size_t pointLightCount;
	size_t spotLightCount;
};


class Engine
{
	friend class BaseSystem;

public:
	static const Settings::Renderer& InitializeRendererSettingsFromFile();
	static Engine*	GetEngine();

	~Engine();

	bool			Initialize(HWND hwnd);
	bool			Load();

	void			Pause();
	void			Unpause();
	float			TotalTime() const;

	bool			UpdateAndRender();
	void			Render();

	void			Exit();

	const Input*	INP() const;
	inline float	GetTotalTime() const { return m_timer->TotalTime(); }
	inline ShaderID GetSelectedShader() const { return m_selectedShader; }
	void			ToggleLightingModel();	// BRDF / Phong


private:
	Engine();

	void TogglePause();
	void CalcFrameStats();
	bool HandleInput();
	void PreRender();

	void SendLightData() const;
	void RenderLights() const;

//--------------------------------------------------------------

private:
	static Engine*					s_instance;
	static Settings::Renderer		s_rendererSettings;
	
	Input*							m_input;
	Renderer*						m_pRenderer;
	SceneManager*					m_sceneManager;
	PerfTimer*						m_timer;
	shared_ptr<Camera>				m_pCamera;	// support only 1 main camera for now

	bool							m_isPaused;
	WorkerPool						m_workerPool;

	SceneView						m_sceneView;
	SceneLightData					m_sceneLights;
	std::vector<Light>				m_lights;

	bool							m_bUsePaniniProjection;
	//PathManager*					m_pPathManager; // unused

	// rendering passes
	SamplerID						m_normalSampler;
	ShadowMapPass					m_shadowMapPass;
	PostProcessPass					m_postProcessPass;
	// Lighting pass?
	bool							m_useDeferredRendering;
	bool							m_isAmbientOcclusionOn;
	DeferredRenderingPasses			m_deferredRenderingPasses;

	ShaderID						m_selectedShader;
	bool							m_debugRender;

	std::vector<GameObject*>		m_ZPassObjects;
};

#define ENGINE Engine::GetEngine()
