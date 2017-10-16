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

#include "IBLTestScene.h"
#include "Engine.h"
#include "Input.h"

#include "Renderer.h"

IBLTestScene::IBLTestScene(SceneManager& sceneMan, std::vector<Light>& lights)
	:
	Scene(sceneMan, lights)
{
}

void IBLTestScene::Load(SerializedScene& scene)
{
	{	// sphere grid
		constexpr float r = 11.0f;
		constexpr size_t gridDimension[2] = { 4, 4 };
		constexpr size_t numSph = gridDimension[0] * gridDimension[1];

		const vec3 origin = vec3::Zero;

		for (size_t i = 0; i < numSph; i++)
		{
			const size_t row = i / gridDimension[0];
			const size_t col = i % gridDimension[1];

			const float sphereStep = static_cast<float>(i) / numSph;
			const float rowStep = static_cast<float>(row) / ((numSph - 1) / gridDimension[0]);
			const float colStep = static_cast<float>(col) / ((numSph - 1) / gridDimension[1]);

			// offset to center the grid
			const float offsetDim[2] = {
				-static_cast<float>(gridDimension[0]) * r / 2 + r / 2.0f,
				-static_cast<float>(gridDimension[1]) * r / 2 + r / 2.0f
			};
			const vec3 offset = vec3(col * r, 1.0f, row * r) + vec3(offsetDim[0], 0.0f, offsetDim[1]);

			const vec3 pos = origin + offset;

			GameObject sph;
			sph.mTransform.SetPosition(pos);
			sph.mTransform.SetUniformScale(2.3f);
			sph.mModel.mMesh = EGeometry::SPHERE;

			BRDF_Material&		 mat0 = sph.mModel.mBRDF_Material;
			// col(-x->+x) -> metalness [0.0f, 1.0f]
			sph.mModel.SetDiffuseColor(LinearColor(vec3(LinearColor::white)));
			mat0.metalness = 0;

			// row(-z->+z) -> roughness [roughnessLowClamp, 1.0f]
			const float roughnessLowClamp = 0.12f;
			mat0.roughness = sphereStep < roughnessLowClamp ? roughnessLowClamp : sphereStep;

			BlinnPhong_Material& mat1 = sph.mModel.mBlinnPhong_Material;
			const float shininessMax = 150.f;
			const float shininessBase = shininessMax + 7.0f;
			mat1.shininess = shininessBase - rowStep * shininessMax;

			spheres.push_back(sph);
		}
	}

	//{	// spot/directional light
	//	Light l;
	//	l._type = Light::SPOT;
	//	l._transform.SetPosition(0, 80, 0);
	//	l._transform.RotateAroundGlobalXAxisDegrees(220);
	//	l._castsShadow = true;
	//	l._brightness = 500;
	//	l._color = LinearColor::white;
	//	l._renderMesh = EGeometry::CYLINDER;
	//	l._spotAngle = 50;
	//	mLights.push_back(l);
	//}

	const std::string sIBLDirectory = Renderer::sHDRTextureRoot + std::string("sIBL/");

	TextureID hdrTex = mpRenderer->CreateHDRTexture("Milkyway/Milkyway_small.hdr", sIBLDirectory);
	testQuad.mModel.SetDiffuseMap(hdrTex);

	const float scl = 40;
	testQuad.mTransform.SetPosition(0, 40, 120);
	testQuad.mTransform.SetScale(scl * 1.77f, scl, 1);
	testQuad.mModel.mMesh = EGeometry::QUAD;
}

void IBLTestScene::Unload()
{
	spheres.clear();
}

void IBLTestScene::Update(float dt)
{
	float t = ENGINE->GetTotalTime();
	const float moveSpeed = 45.0f;
	const float rotSpeed = XM_PI;

	XMVECTOR rot = XMVectorZero();
	XMVECTOR tr = XMVectorZero();
	if (ENGINE->INP()->IsKeyDown("Numpad6")) tr += vec3::Right;
	if (ENGINE->INP()->IsKeyDown("Numpad4")) tr += vec3::Left;
	if (ENGINE->INP()->IsKeyDown("Numpad8")) tr += vec3::Forward;
	if (ENGINE->INP()->IsKeyDown("Numpad2")) tr += vec3::Back;
	if (ENGINE->INP()->IsKeyDown("Numpad9")) tr += vec3::Up;
	if (ENGINE->INP()->IsKeyDown("Numpad3")) tr += vec3::Down;
	mLights[0]._transform.Translate(dt * tr * moveSpeed);
}

void IBLTestScene::Render(const SceneView & sceneView, bool bSendMaterialData) const
{
	for (const auto& sph : spheres) sph.Render(mpRenderer, sceneView, bSendMaterialData);
	testQuad.Render(mpRenderer, sceneView, bSendMaterialData);
}

void IBLTestScene::GetShadowCasters(std::vector<const GameObject*>& casters) const
{
	Scene::GetShadowCasters(casters);
	for (const GameObject& obj : spheres)	casters.push_back(&obj);
}

void IBLTestScene::GetSceneObjects(std::vector<const GameObject*>& objs) const
{
	Scene::GetSceneObjects(objs);
	for (const GameObject& obj : spheres)	objs.push_back(&obj);
	objs.push_back(&testQuad);
}
