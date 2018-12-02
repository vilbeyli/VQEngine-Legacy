//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
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

#include "LightsScene.h"
#include "Engine/Engine.h"
#include "Application/Input.h"


// Scene-specific loading logic
//
void LightsScene::Load(SerializedScene& scene)
{
	pHelloObject = Scene::CreateNewGameObject();
	pHelloObject->AddMesh(EGeometry::CUBE);
	pHelloObject->AddMaterial(Scene::CreateRandomMaterialOfType(GGX_BRDF));
	pHelloObject->SetTransform(Transform(vec3(0, 15, -55)));

	BRDF_Material* pMat2 = static_cast<BRDF_Material*>(Scene::CreateRandomMaterialOfType(GGX_BRDF));
	pMat2->diffuse = vec3(0.05f, 0.45f, 0.89f);
	pMat2->metalness = 0.1f;
	pMat2->roughness = 0.4f;

	pHelloObject2 = Scene::CreateNewGameObject();
	pHelloObject2->AddMesh(EGeometry::CONE);
	pHelloObject2->AddMaterial(pMat2);
	pHelloObject2->SetTransform(Transform(
		  vec3(0, 75, 55)
		, Quaternion::FromAxisAngle(vec3(1, 1, 1).normalized(), 15.2f)
		//, Quaternion::Identity()
		, vec3(5.0f)));
}

// Scene-specific unloading logic
//
void LightsScene::Unload()
{

}

// Update() is called each frame
//
void LightsScene::Update(float dt)
{
	pHelloObject->GetTransform().RotateAroundGlobalYAxisDegrees(dt * 45.0f);
	pHelloObject2->GetTransform().RotateAroundGlobalYAxisDegrees(dt * 75.0f);
}

// RenderUI() is called at the last stage of rendering before presenting the frame.
// Scene-specific UI rendering goes in here.
//
void LightsScene::RenderUI() const 
{
	
}
