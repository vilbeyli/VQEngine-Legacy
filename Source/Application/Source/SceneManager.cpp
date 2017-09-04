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

#include "SceneManager.h"	
#include "SceneParser.h"
#include "Engine.h"			
#include "Input.h"
#include "utils.h"
#include "BaseSystem.h"	// only for renderer settings... maybe store in renderer?
#include "Log.h"


#include "Renderer.h"
#include "Camera.h"
#include "Light.h"


#define KEY_TRIG(k) ENGINE->INP()->IsKeyTriggered(k)

SceneManager::SceneManager(shared_ptr<Camera> pCam, std::vector<Light>& lights)
	:
	m_sceneCamera(pCam),
	m_roomScene(*this, lights)
{}

SceneManager::~SceneManager()
{}

ESkyboxPresets SceneManager::GetSceneSkybox() const
{
	return m_roomScene.m_skybox;
}


void SceneManager::ReloadLevel()
{
	const SerializedScene&		scene            =  m_serializedScenes.back();	// load last level
	const Settings::Renderer&	rendererSettings = Engine::InitializeRendererSettingsFromFile();
	Log::Info("Reloading Level...");

	m_sceneCamera->ConfigureCamera(scene.cameraSettings, rendererSettings.window);
	// only camera reset for now
	// todo: unload and reload scene, initialize depth pass...
}

void SceneManager::Load(Renderer* renderer, PathManager* pathMan, const Settings::Renderer& rendererSettings, shared_ptr<Camera> pCamera)
{
#if 0
	m_pPathManager		= pathMan;
#endif
	m_sceneCamera = pCamera;
	m_serializedScenes.push_back(SceneParser::ReadScene());
	
	SerializedScene& scene = m_serializedScenes.back();	// non-const because lights are std::move()d for roomscene loading
	m_roomScene.Load(renderer, scene);
	m_sceneCamera->ConfigureCamera(scene.cameraSettings, rendererSettings.window);
}


void SceneManager::HandleInput()
{

}

void SceneManager::Update(float dt)
{
	HandleInput();
	m_roomScene.Update(dt);
}
//-----------------------------------------------------------------------------------------------------------------------------------------

void SceneManager::Render(Renderer* pRenderer, const SceneView& sceneView) const
{
	m_roomScene.Render(pRenderer, sceneView);
}



