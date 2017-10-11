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

// https://en.wikipedia.org/wiki/Template_method_pattern
// https://stackoverflow.com/questions/9724371/force-calling-base-class-virtual-function
// template method seems like a good idea here.
// the idea is that base class takes care of the common tasks among all scenes and calls the 
// customized functions of the derived classes through pure virtual functions

// Base class for scenes
class Scene
{
	friend class Engine;
public:
	Scene(SceneManager& sceneManager, std::vector<Light>& lights);
	~Scene() = default;

	// sets mpRenderer and moves objects from serializedScene into objects vector
	void Load(Renderer* pRenderer, SerializedScene& scene);
	
	// calls .Render() on objects and calls derived class RenderSceneSpecific()
	void Render(const SceneView& sceneView) const;

	// puts objects into provided vector if the RenderSetting.bRenderDepth is true
	virtual void GetShadowCasters(std::vector<const GameObject*>& casters) const;

	// puts addresses of game objects in provided vector
	virtual void GetSceneObjects(std::vector<const GameObject*>& objs) const;

	virtual void Update(float dt) = 0;

protected:	// customization functions for derived classes
	virtual void Render(const SceneView& sceneView, bool bSendMaterialData) const = 0;
	virtual void Load(SerializedScene& scene) = 0;

	SceneManager&			mSceneManager;
	Renderer*				mpRenderer;
	std::vector<Light>&		mLights;
	std::vector<GameObject> objects;

	// maybe add a boolean to determine whether derived class will handle objects.Render()
};

struct SerializedScene
{
	Settings::Camera		cameraSettings;
	std::vector<Light>		lights;
	std::vector<GameObject> objects;
	char loadSuccess = '0';
};