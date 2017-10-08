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

#include "Settings.h"
#include "Light.h"
#include "GameObject.h"

struct SerializedScene;
class SceneManager;
class Renderer;
struct SceneView;

// Base class for scenes
class Scene
{
public:
	Scene(SceneManager& sceneManager, std::vector<Light>& lights);
	~Scene() = default;

	virtual void Load(Renderer* pRenderer, SerializedScene& scene) = 0;
	virtual void Update(float dt) = 0;
	virtual void Render(Renderer* pRenderer, const SceneView& sceneView) const = 0;

	SceneManager&		mSceneManager;
	Renderer*			mpRenderer;
	std::vector<Light>&	mLights;
};

struct SerializedScene
{
	Settings::Camera		cameraSettings;
	std::vector<Light>		lights;
	std::vector<GameObject> objects;
	char loadSuccess = '0';
};