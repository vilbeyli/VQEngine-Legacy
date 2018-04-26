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

#include "StressTestScene.h"
#include "Engine/Engine.h"
#include "Application/Input.h"
#include "Utilities/utils.h"
#include "Renderer/Light.h"
#include "Renderer/Renderer.h"

constexpr size_t NUM_OBJ = 8000;
constexpr size_t DIV = 20;
const vec3 ORIGIN_OFFSET = vec3(-40, 20, -90);
constexpr float SCALING = 9.0f;


StressTestScene::StressTestScene(SceneManager& sceneMan, std::vector<Light>& lights)
	:
	Scene(sceneMan, lights)
{
}

void StressTestScene::Load(SerializedScene& scene)
{
	for (size_t i = 0; i < NUM_OBJ; ++i)
	{
		GameObject obj;
		Transform& tf = obj.mTransform;
		
		// MESH
		//
		obj.mModel.mMesh = static_cast<EGeometry>(RandI(0, EGeometry::MESH_TYPE_COUNT));
				
		// TRANSFORM
		//
		const float RandScale = RandF(1.0f, 1.5f) * (obj.mModel.mMesh == EGeometry::GRID ? 5.0f : 1.0f);
		tf.SetUniformScale(RandScale);

		tf.SetPosition(vec3((i / DIV % DIV), static_cast<float>(i%DIV), i / (DIV*DIV))* SCALING + ORIGIN_OFFSET);
		tf.RotateAroundAxisDegrees(vec3::Rand(), RandF(0, 360));

		// MATERIAL
		//
		BRDF_Material& m = obj.mModel.mBRDF_Material;
		if (RandI(0, 100) < 5)
		{
			obj.mModel.SetDiffuseColor(LinearColor::gold);
			m.metalness = 0.98f;
			m.roughness = RandF(0.04f, 0.15f);
		}
		else
		{
			m = Material::RandomBRDFMaterial();
		}
		if (RandI(0, 100) < 95)
		{
			const std::string fileNameDiffuse = "openart/" + std::to_string(RandI(151, 200)) + ".JPG";
			const std::string fileNameNormal  = "openart/" + std::to_string(RandI(151, 200)) + "_norm.JPG";
			m.diffuseMap = mpRenderer->CreateTextureFromFile(fileNameDiffuse);
			m.normalMap = mpRenderer->CreateTextureFromFile(fileNameNormal);
		}


		objs.push_back(obj);
		rotationSpeeds.push_back(RandF(5.0f, 15.0f) * (RandF(0, 1) < 0.5f ? 1.0f : -1.0f));
	}

	constexpr size_t NUM_LIGHT = 30;

	for (size_t i = 0; i < NUM_LIGHT; ++i)
	{
		const Light::ELightType type = Light::ELightType::POINT;
		const LinearColor color = LinearColor::white;
		const float range = RandF(300, 1500.f);
		const float bright = RandF(100, 300);

		Light l = Light(type, color, range, bright, 0.0f, false);
		l._transform.SetPosition(RandF(-50, 50), RandF(30, 90), RandF(-40, -90));
		l._transform.SetUniformScale(0.1f);
		mLights.push_back(l);
	}
	for (size_t i = 0; i < 3; ++i)
	{
		const Light::ELightType type = Light::ELightType::POINT;
		const LinearColor color = [&]() 
		{
			switch (i)
			{
			case 0: return (LinearColor)(vec3(LinearColor::red) * 3.0f);
			case 1: return LinearColor::green;
			case 2: return LinearColor::blue;
			}
		}();
		const float range = RandF(800, 1500.f);
		const float bright = RandF(2500, 5500);

		Light l = Light(type, color, range, bright, 0.0f, false);
		l._transform.SetPosition(RandF(-70, 40), RandF(30, 90), RandF(-40, -90));
		l._transform.SetUniformScale(0.3f);
		mLights.push_back(l);
	}


	SetEnvironmentMap(EEnvironmentMapPresets::BARCELONA);
}

void StressTestScene::Unload()
{

}

void StressTestScene::Update(float dt)
{
	//if (ENGINE->INP()->IsKeyDown("+"))


	for (size_t i = 0; i < NUM_OBJ; ++i)
	{
		GameObject& o = objs[i];
		o.mTransform.RotateAroundGlobalYAxisDegrees(dt * rotationSpeeds[i]);
	}

	//std::for_each(objs.begin(), objs.end(), [&](GameObject& o)
	//{
	//	o.mTransform.RotateAroundGlobalYAxisDegrees(dt * 5);
	//});
}

int StressTestScene::Render(const SceneView & sceneView, bool bSendMaterialData) const
{
	std::for_each(objs.begin(), objs.end(), [&](const GameObject& o) 
	{ 
		o.Render(mpRenderer, sceneView, bSendMaterialData);
	});
	return static_cast<int>(objs.size());
}

void StressTestScene::GetShadowCasters(std::vector<const GameObject*>& casters) const
{
	Scene::GetShadowCasters(casters);

	// add the objects declared in the header. 
}

void StressTestScene::GetSceneObjects(std::vector<const GameObject*>& objs) const
{
	Scene::GetSceneObjects(objs);

	// add the objects declared in the header. 
}
