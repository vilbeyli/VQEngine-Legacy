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

Scene::Scene(SceneManager& sceneManager, std::vector<Light>& lights)
	:
	mSceneManager(sceneManager),
	mLights(lights),
	mpRenderer(nullptr)	// pRenderer is initialized at Load()
{}

void Scene::Load(Renderer * pRenderer, SerializedScene & scene)
{
	mpRenderer = pRenderer;
	objects = std::move(scene.objects);
	Load(scene);
}

void Scene::Render(const SceneView& sceneView) const
{
	const ShaderID selectedShader = ENGINE->GetSelectedShader();
	const bool bSendMaterialData = (
		selectedShader == EShaders::FORWARD_PHONG
		//|| selectedShader == EShaders::UNLIT
		//|| selectedShader == EShaders::NORMAL
		|| selectedShader == EShaders::FORWARD_BRDF
		|| selectedShader == EShaders::DEFERRED_GEOMETRY
		);

	for (const auto& obj : objects) obj.Render(mpRenderer, sceneView, bSendMaterialData);
	Render(sceneView, bSendMaterialData);
}

void Scene::GetShadowCasters(std::vector<const GameObject*>& casters) const
{
	for (const GameObject& obj : objects) 
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
	for (const GameObject& obj : objects)
		objs.push_back(&obj);
#endif
}
