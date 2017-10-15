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

// todo: remove this dependency
static const char* sceneNames[] = 
{
	"Room.scn",
	"SSAOTest.scn",
	"IBLTest.scn"
};

SceneManager::SceneManager(shared_ptr<Camera> pCam, std::vector<Light>& lights)
	:
	mpSceneCamera(pCam),
	mRoomScene(*this, lights),
	mSSAOTestScene(*this, lights),
	mIBLTestScene(*this, lights),
	mpActiveScene(nullptr)
{}

ESkyboxPreset SceneManager::GetSceneSkybox() const
{
	//return mRoomScene.GetSkybox();
	return ESkyboxPreset::NIGHT_SKY;
}

void SceneManager::ReloadLevel()
{
	// todo: rethink this
	const SerializedScene&		scene			= mSerializedScene;
	const Settings::Engine      engineSettings	= Engine::ReadSettingsFromFile();
	
	Log::Info("Reloading Level...");

	mpSceneCamera->ConfigureCamera(scene.cameraSettings, engineSettings.window);
	// only camera reset for now
	// todo: unload and reload scene, initialize depth pass...
}

bool SceneManager::Load(Renderer* renderer, PathManager* pathMan, const Settings::Engine& settings, shared_ptr<Camera> pCamera)
{
	constexpr size_t numScenes = sizeof(sceneNames) / sizeof(sceneNames[0]);
	assert(settings.levelToLoad < numScenes);

	mpSceneCamera = pCamera;
	mSerializedScene = SceneParser::ReadScene(sceneNames[settings.levelToLoad]);
	if (mSerializedScene.loadSuccess == '1')
	{
		mpSceneCamera->ConfigureCamera(mSerializedScene.cameraSettings, settings.window);
		switch (settings.levelToLoad)
		{
		case 0:	mpActiveScene = &mRoomScene; break;
		case 1:	mpActiveScene = &mSSAOTestScene; break;
		case 2:	mpActiveScene = &mIBLTestScene; break;
		default:	break;
		}
		mpActiveScene->Load(renderer, mSerializedScene);
		return true;
	}
	return false;
}


void SceneManager::HandleInput()
{

}

void SceneManager::Update(float dt)
{
	HandleInput();
	mpActiveScene->Update(dt);
}
//-----------------------------------------------------------------------------------------------------------------------------------------

void SceneManager::Render(Renderer* pRenderer, const SceneView& sceneView) const
{
	mpActiveScene->Render(sceneView);
}



