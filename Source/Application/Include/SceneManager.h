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
#include "Renderer.h"

#include "SceneParser.h"

#include <vector>
#include <memory>

using std::shared_ptr;

// todo: move scene headers to cpp
// scenes
#include "../Scenes/RoomScene.h"
#include "../Scenes/SSAOTestScene.h"
#include "../Scenes/IBLTestScene.h"

class Camera;
class PathManager;

struct SceneView;
struct Path;


class SceneManager
{
	friend class SceneParser;
	friend class Engine;	// temp hack until gameObject array refactoring
public:
	SceneManager(std::vector<Light>& lights);	// lights passed down to RoomScene
	~SceneManager() = default;

	ESkyboxPreset GetSceneSkybox() const;
	const Camera& GetMainCamera() const;

	bool Load(Renderer* renderer, PathManager* pathMan, const Settings::Engine& settings, std::vector<const GameObject*>& zPassObjects);

	void Update(float dt);
	void Render(Renderer* pRenderer, const SceneView& sceneView) const;

	bool LoadScene(Renderer* pRenderer, const Settings::Engine& settings, std::vector<const GameObject*>& zPassObjects);
	void ReloadScene(Renderer* pRenderer, std::vector<const GameObject*>& ZPassObjects);
	void ResetMainCamera();
private:
	void LoadScene(int level);
	void HandleInput();

private:
	SerializedScene			mSerializedScene;
	Scene*					mpActiveScene;
	int						mCurrentLevel;

	// current design for adding new scenes is as follows:
	// - add the .scn scene file to Data/Levels directory
	// - Create a class for your scene and inherit from Scene base class
	// - override functions as necessary. 
	// - edit settings.ini to start with your scene
	// - remember to add scene name in scenemanager.cpp
	RoomScene				mRoomScene;
	SSAOTestScene			mSSAOTestScene;
	IBLTestScene			mIBLTestScene;

};