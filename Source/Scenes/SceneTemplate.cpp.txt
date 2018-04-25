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

#include "SceneTemplate.h"
#include "Engine/Engine.h"
#include "Application/Input.h"


SceneTemplate::SceneTemplate(SceneManager& sceneMan, std::vector<Light>& lights)
	:
	Scene(sceneMan, lights)
{
}

void SceneTemplate::Load(SerializedScene& scene)
{

}

void SceneTemplate::Unload()
{

}

void SceneTemplate::Update(float dt)
{

}

int SceneTemplate::Render(const SceneView & sceneView, bool bSendMaterialData) const
{

}

void SceneTemplate::GetShadowCasters(std::vector<const GameObject*>& casters) const
{
	Scene::GetShadowCasters(casters);

	// add the objects declared in the header. 
}

void SceneTemplate::GetSceneObjects(std::vector<const GameObject*>& objs) const
{
	Scene::GetSceneObjects(objs);

	// add the objects declared in the header. 
}
