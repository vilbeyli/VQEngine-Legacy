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

#include "Application/Input.h"
#include "Application/BaseSystem.h"	// only for renderer settings... maybe store in renderer?

#include "Utilities/Camera.h"
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
	"IBLTest.scn",
	"StressTestScene.scn"
};

SceneManager::SceneManager(std::vector<Light>& lights)
	:
	mRoomScene(*this, lights),
	mSSAOTestScene(*this, lights),
	mIBLTestScene(*this, lights),
	mStressTestScene(*this, lights),
	mpActiveScene(nullptr)
{}

const Camera& SceneManager::GetMainCamera() const
{
	return mpActiveScene->GetActiveCamera();
}


void SceneManager::ReloadScene(Renderer* pRenderer, TextRenderer* pTextRenderer, std::vector<const GameObject*>& ZPassObjects)
{	
	const auto& settings = Engine::GetSettings();
	Log::Info("Reloading Scene (%d)...", settings.levelToLoad);

	mpActiveScene->UnloadScene();
	ZPassObjects.clear();

	LoadScene(pRenderer, pTextRenderer, settings, ZPassObjects);
}

void SceneManager::ResetMainCamera()
{
	mpActiveScene->ResetActiveCamera();
}

bool SceneManager::LoadScene(Renderer* pRenderer, TextRenderer* pTextRenderer, const Settings::Engine& settings, std::vector<const GameObject*>& zPassObjects)
{
	mSerializedScene = Parser::ReadScene(pRenderer, sceneNames[settings.levelToLoad]);
	if (mSerializedScene.loadSuccess == '0') return false;
	
	mCurrentLevel = settings.levelToLoad;
	switch (settings.levelToLoad)
	{
	case 0:	mpActiveScene = &mRoomScene; break;
	case 1:	mpActiveScene = &mSSAOTestScene; break;
	case 2:	mpActiveScene = &mIBLTestScene; break;
	case 3:	mpActiveScene = &mStressTestScene; break;
	default:	break;
	}
	mpActiveScene->LoadScene(pRenderer, pTextRenderer, mSerializedScene, settings.window);
	mpActiveScene->GetShadowCasters(zPassObjects);
	return true;
}


bool SceneManager::Load(Renderer* renderer, PathManager* pathMan, const Settings::Engine& settings, std::vector<const GameObject*>& zPassObjects)
{
	constexpr size_t numScenes = sizeof(sceneNames) / sizeof(sceneNames[0]);
	assert(settings.levelToLoad < numScenes);
	bool bLoadSuccess = LoadScene(renderer, ENGINE->mpTextRenderer, settings, zPassObjects);
	return bLoadSuccess;
}

void SceneManager::LoadScene(int level)
{
	Renderer* mpRenderer = ENGINE->mpRenderer;
	auto& mZPassObjects = ENGINE->mZPassObjects;

	mpActiveScene->UnloadScene();
	mZPassObjects.clear();
	Engine::sEngineSettings.levelToLoad = level;
	LoadScene(mpRenderer, ENGINE->mpTextRenderer, Engine::sEngineSettings, mZPassObjects);
}

void SceneManager::HandleInput()
{
	const Input* mpInput = ENGINE->INP();
	Renderer* mpRenderer = ENGINE->mpRenderer;
	TextRenderer* mpTextRenderer = ENGINE->mpTextRenderer;
	auto& mZPassObjects  = ENGINE->mZPassObjects;
	
	if (mpInput->IsKeyTriggered("R"))
	{
		if (mpInput->IsKeyDown("Shift")) ReloadScene(mpRenderer, mpTextRenderer, mZPassObjects);
		else							 ResetMainCamera();
	}

	if (mpInput->IsKeyTriggered("1"))	LoadScene(0);
	if (mpInput->IsKeyTriggered("2"))	LoadScene(1);
	if (mpInput->IsKeyTriggered("3"))	LoadScene(2);
	if (mpInput->IsKeyTriggered("4"))	LoadScene(3);


	// index using enums. first element of environment map presets starts with cubemap preset count, as if both lists were concatenated.
	const EEnvironmentMapPresets firstPreset = static_cast<EEnvironmentMapPresets>(CUBEMAP_PRESET_COUNT);
	const EEnvironmentMapPresets lastPreset = static_cast<EEnvironmentMapPresets>(
		static_cast<EEnvironmentMapPresets>(CUBEMAP_PRESET_COUNT) + ENVIRONMENT_MAP_PRESET_COUNT - 1
		);
	
	EEnvironmentMapPresets selectedEnvironmentMap = mpActiveScene->GetActiveEnvironmentMapPreset();
	if (ENGINE->INP()->IsKeyTriggered("PageUp"))	selectedEnvironmentMap = selectedEnvironmentMap == lastPreset ? firstPreset : static_cast<EEnvironmentMapPresets>(selectedEnvironmentMap + 1);
	if (ENGINE->INP()->IsKeyTriggered("PageDown"))	selectedEnvironmentMap = selectedEnvironmentMap == firstPreset ? lastPreset : static_cast<EEnvironmentMapPresets>(selectedEnvironmentMap - 1);
	mpActiveScene->SetEnvironmentMap(selectedEnvironmentMap);
}

void SceneManager::Update(float dt)
{
	HandleInput();
	mpActiveScene->UpdateScene(dt);
}
//-----------------------------------------------------------------------------------------------------------------------------------------

int SceneManager::Render(Renderer* pRenderer, const SceneView& sceneView) const
{
	return mpActiveScene->Render(sceneView);
}

void SceneManager::RenderUI() const
{
	mpActiveScene->RenderUI();
}



