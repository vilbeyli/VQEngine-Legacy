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

#include "SceneTemplate.h"
#include "Engine/Engine.h"
#include "Application/Input.h"


// Scene-specific loading logic
//
void SceneTemplate::Load(SerializedScene& scene)
{
	pHelloObject = Scene::CreateNewGameObject();
	pHelloObject->AddMesh(EGeometry::CUBE);
	pHelloObject->AddMaterial(Scene::CreateRandomMaterialOfType(GGX_BRDF));
}

// Scene-specific unloading logic
//
void SceneTemplate::Unload()
{

}

// Update() is called each frame
//
void SceneTemplate::Update(float dt)
{
	pHelloObject->RotateAroundGlobalYAxisDegrees(dt * 45.0f);
}

// RenderUI() is called at the last stage of rendering before presenting the frame.
// Scene-specific UI rendering goes in here.
//
void SceneTemplate::RenderUI() const 
{
	
}
