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

#include "SSAOTestScene.h"

#include "Engine.h"

SSAOTestScene::SSAOTestScene(SceneManager& sceneMan, std::vector<Light>& lights)
	:
	Scene(sceneMan, lights)
{}

void SSAOTestScene::Load(Renderer* pRenderer, SerializedScene& scene)
{
	mpRenderer = pRenderer;
	objects = std::move(scene.objects);
	for (GameObject& obj : objects)
	{
		obj.mRenderSettings.bRenderTBN = true;
	}

	//m_skybox = ESkyboxPreset::NIGHT_SKY;
}

void SSAOTestScene::Update(float dt)
{
		
}

void SSAOTestScene::Render(Renderer* pRenderer, const SceneView& sceneView) const
{
	const ShaderID selectedShader = ENGINE->GetSelectedShader();
	const bool bSendMaterialData = (
		selectedShader == EShaders::FORWARD_PHONG
		|| selectedShader == EShaders::UNLIT
		|| selectedShader == EShaders::NORMAL
		|| selectedShader == EShaders::FORWARD_BRDF
		|| selectedShader == EShaders::DEFERRED_GEOMETRY
		);
	
	for (const auto& obj : objects) obj.Render(mpRenderer, sceneView, bSendMaterialData);

	//m_room.Render(mpRenderer, sceneView, bSendMaterialData);
	//for (const auto& sph : spheres) sph.Render(mpRenderer, sceneView, bSendMaterialData);
}

void SSAOTestScene::GetShadowCasterObjects(std::vector<GameObject*>& casters)
{
	//casters.push_back(&m_room.floor);
	//casters.push_back(&m_room.wallL);
	//casters.push_back(&m_room.wallR);
	//casters.push_back(&m_room.wallF);
	//casters.push_back(&m_room.ceiling);
	//for (GameObject& obj : objects) casters.push_back(&obj);
	//for (GameObject& obj : spheres)	casters.push_back(&obj);
}
