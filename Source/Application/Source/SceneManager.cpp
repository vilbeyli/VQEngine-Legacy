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
#include "Engine.h"			
#include "Input.h"
#include "BaseSystem.h"	// only for renderer settings... maybe store in renderer?
#include "Camera.h"

#include "Utilities/CustomParser.h"
#include "Utilities/utils.h"
#include "Utilities/Log.h"

#include "Renderer/Renderer.h"
#include "Renderer/Light.h"

// todo: remove this dependency
static const char* sceneNames[] = 
{
	"Room.scn",
	"SSAOTest.scn",
	"IBLTest.scn"
};

SceneManager::SceneManager(std::vector<Light>& lights)
	:
	mRoomScene(*this, lights),
	mSSAOTestScene(*this, lights),
	mIBLTestScene(*this, lights),
	mpActiveScene(nullptr)
{}

ESkyboxPreset SceneManager::GetSceneSkybox() const
{
	return mpActiveScene->GetSkybox();
}

const Camera& SceneManager::GetMainCamera() const
{
	return mpActiveScene->GetActiveCamera();
}


void SceneManager::ReloadScene(Renderer* pRenderer, std::vector<const GameObject*>& ZPassObjects)
{	
	const auto& settings = Engine::GetSettings();
	Log::Info("Reloading Scene (%d)...", settings.levelToLoad);

	mpActiveScene->UnloadScene();
	ZPassObjects.clear();

	LoadScene(pRenderer, settings, ZPassObjects);
}

void SceneManager::ResetMainCamera()
{
	mpActiveScene->ResetActiveCamera();
}

bool SceneManager::LoadScene(Renderer* pRenderer, const Settings::Engine& settings, std::vector<const GameObject*>& zPassObjects)
{
	mSerializedScene = Parser::ReadScene(sceneNames[settings.levelToLoad]);
	if (mSerializedScene.loadSuccess == '0') return false;
	
	mCurrentLevel = settings.levelToLoad;
	switch (settings.levelToLoad)
	{
	case 0:	mpActiveScene = &mRoomScene; break;
	case 1:	mpActiveScene = &mSSAOTestScene; break;
	case 2:	mpActiveScene = &mIBLTestScene; break;
	default:	break;
	}
	mpActiveScene->LoadScene(pRenderer, mSerializedScene, settings.window);
	mpActiveScene->GetShadowCasters(zPassObjects);
	return true;
}


bool SceneManager::Load(Renderer* renderer, PathManager* pathMan, const Settings::Engine& settings, std::vector<const GameObject*>& zPassObjects)
{
	constexpr size_t numScenes = sizeof(sceneNames) / sizeof(sceneNames[0]);
	assert(settings.levelToLoad < numScenes);
	bool bLoadSuccess = LoadScene(renderer, settings, zPassObjects);
	return bLoadSuccess;
}

void SceneManager::LoadScene(int level)
{
	Renderer* mpRenderer = ENGINE->mpRenderer;
	auto& mZPassObjects = ENGINE->mZPassObjects;

	mpActiveScene->UnloadScene();
	mZPassObjects.clear();
	Engine::sEngineSettings.levelToLoad = level;
	LoadScene(mpRenderer, Engine::sEngineSettings, mZPassObjects);
}

void SceneManager::HandleInput()
{
	const Input* mpInput = ENGINE->INP();
	Renderer* mpRenderer = ENGINE->mpRenderer;
	auto& mZPassObjects  = ENGINE->mZPassObjects;
	
	if (mpInput->IsKeyTriggered("R"))
	{
		if (mpInput->IsKeyDown("Shift")) ReloadScene(mpRenderer, mZPassObjects);
		else							 ResetMainCamera();
	}

	if (mpInput->IsKeyTriggered("0"))	LoadScene(0);
	if (mpInput->IsKeyTriggered("1"))	LoadScene(1);
	if (mpInput->IsKeyTriggered("2"))	LoadScene(2);
}

void SceneManager::Update(float dt)
{
	HandleInput();
	mpActiveScene->UpdateScene(dt);
}
//-----------------------------------------------------------------------------------------------------------------------------------------

void SceneManager::Render(Renderer* pRenderer, const SceneView& sceneView) const
{
	mpActiveScene->Render(sceneView);
}



