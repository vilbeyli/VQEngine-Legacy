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

#include "Scene.h"
#include "Engine.h"
#include "Application/Input.h"

Scene::Scene(SceneManager& sceneManager, std::vector<Light>& lights)
	:
	mSceneManager(sceneManager)
	, mLights(lights)
	, mpRenderer(nullptr)	// pRenderer is initialized at Load()
	, mSelectedCamera(0)
	, mAmbientOcclusionFactor(1.0f)
{}

void Scene::LoadScene(Renderer* pRenderer, SerializedScene& scene, const Settings::Window& windowSettings)
{
	mpRenderer = pRenderer;
	mObjects = std::move(scene.objects);
	mLights = std::move(scene.lights);
	mAmbientOcclusionFactor = scene.aoFactor;

	for (const Settings::Camera& camSetting : scene.cameras)
	{
		Camera c;
		c.ConfigureCamera(camSetting, windowSettings);
		mCameras.push_back(c);
	}

	Load(scene);
}

void Scene::UnloadScene()
{
	mCameras.clear();
	mObjects.clear();
	mLights.clear();
	Unload();
}

void Scene::Render(const SceneView& sceneView) const
{
	const ShaderID selectedShader = ENGINE->GetSelectedShader();
	const bool bSendMaterialData = (
		selectedShader == EShaders::FORWARD_PHONG
		|| selectedShader == EShaders::UNLIT
		|| selectedShader == EShaders::NORMAL
		|| selectedShader == EShaders::FORWARD_BRDF
		|| selectedShader == EShaders::DEFERRED_GEOMETRY
		|| selectedShader == EShaders::Z_PREPRASS
		);

	for (const auto& obj : mObjects) obj.Render(mpRenderer, sceneView, bSendMaterialData);
	Render(sceneView, bSendMaterialData);
}

void Scene::GetShadowCasters(std::vector<const GameObject*>& casters) const
{
	for (const GameObject& obj : mObjects) 
		if(obj.mRenderSettings.bRenderDepth)
			casters.push_back(&obj);
}

void Scene::GetSceneObjects(std::vector<const GameObject*>& objs) const
{
#if 0
	for (size_t i = 0; i < objects.size(); i++)
	{
		objs[i] = &objects[i];
	}
#else
	for (const GameObject& obj : mObjects)
		objs.push_back(&obj);
#endif
}

void Scene::ResetActiveCamera()
{
	mCameras[mSelectedCamera].Reset();
}

void Scene::UpdateScene(float dt)
{
	if (ENGINE->INP()->IsKeyTriggered("C"))
	{
		mSelectedCamera = (mSelectedCamera + 1) % mCameras.size();
	}

	mCameras[mSelectedCamera].Update(dt);
	Update(dt);
}
