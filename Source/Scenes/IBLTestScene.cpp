//	VQEngine | DirectX11 Renderer
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
#include "Application/Input.h"
#include "Engine/Engine.h"

#include "Renderer/Renderer.h"

#undef max

static const vec3 sSphereCenter = vec3(0, 1, 20);

#if DO_NOT_LOAD_SCENES
void IBLTestScene::Load(SerializedScene& scene) {}
void IBLTestScene::Unload() {}
void IBLTestScene::Update(float dt) {}
void IBLTestScene::RenderUI() const {}
#else
void IBLTestScene::Load(SerializedScene& scene)
{
	// sphere grid
	constexpr float r = 14.0f;
	constexpr size_t gridDimension[2] = { 8, 4 };
	constexpr size_t numSph = gridDimension[0] * gridDimension[1];
	TextureID cubeNormalMap = mpRenderer->CreateTextureFromFile("openart/185_norm.jpg");

	const vec3& origin = sSphereCenter;

	for (size_t i = 0; i < numSph; i++)
	{
		const size_t row = i / gridDimension[1];
		const size_t col = i % gridDimension[1];

		const float sphereStep = static_cast<float>(i) / (numSph - 1);
		const float rowStep = static_cast<float>(row) / (gridDimension[0]-1);
		const float colStep = static_cast<float>(col) / (gridDimension[1]-1);

#if 1
		// offset to center the grid
		const float offsetDim[2] = {
			-static_cast<float>(gridDimension[0]) * r / 2 + r / 2.0f,
			-static_cast<float>(gridDimension[1]) * r / 2 + r / 2.0f
		};
		vec3 offset = vec3(row * r, col * r, 1.0f) + vec3(offsetDim[0], offsetDim[1], 0);
#else
		const float R = 85.0f;
		const float archAngle = DEG2RAD * 61.9f;	// how much arch will cover
		const float archOffset = DEG2RAD * -15.0f;	// rotate arch by degrees

		const float phi = -sphereStep * archAngle + archOffset;
		const vec3 offset = R * vec3(cosf(phi), 0.15f * cosf((phi - archOffset) * 0.25f), sinf(phi));
		//const vec3 offset = vec3(2.1f * r * sphereStep - 1.1f * i * r, 0, 0);
#endif
		vec3 pos = origin + offset;
		const float sphereGroupOffset = gridDimension[0] * r / 2;

		// GOLD / SILVER PAIR
		{
			GameObject sph;
			pos.x() += sphereGroupOffset;
			sph.mTransform.SetPosition(pos);
			sph.mTransform.SetUniformScale(3.0f + 0 * sinf(sphereStep * PI));
			sph.mModel.mMeshID = EGeometry::SPHERE;

			BRDF_Material&		 mat0 = sph.mModel.mBRDF_Material;
			sph.mModel.SetDiffuseColor(LinearColor(vec3(1.0f)));
			mat0.metalness = 1.0f-colStep;	// metalness [0.0f, 1.0f]

			const float roughnessLowClamp = 0.03f;
			mat0.roughness = std::max(0.04f, rowStep);// powf(std::max(rowStep, 0.04f), 2.01f);	// roughness [roughnessLowClamp, 1.0f]

			BlinnPhong_Material& mat1 = sph.mModel.mBlinnPhong_Material;
			const float shininessMax = 150.f;
			const float shininessBase = shininessMax + 7.0f;
			mat1.shininess = shininessBase - rowStep * shininessMax;

			spheres.push_back(sph);
		}

		{
			pos.x() -= 2 * sphereGroupOffset;
			GameObject sph;
			sph.mTransform.SetPosition(pos);
			sph.mTransform.SetUniformScale(3.0f + 0 * sinf(sphereStep * PI));
			sph.mModel.mMeshID = EGeometry::SPHERE;

			BRDF_Material&		 mat0 = sph.mModel.mBRDF_Material;
			sph.mModel.SetDiffuseColor(LinearColor(vec3(LinearColor::gold)));
			mat0.metalness = 1.0f - colStep;	// metalness [0.0f, 1.0f]

			const float roughnessLowClamp = 0.1f;
			mat0.roughness = std::max(0.04f, 1.0f - rowStep);// powf(std::max(rowStep, 0.04f), 2.01f);	// roughness [roughnessLowClamp, 1.0f];

			BlinnPhong_Material& mat1 = sph.mModel.mBlinnPhong_Material;
			const float shininessMax = 150.f;
			const float shininessBase = shininessMax + 7.0f;
			mat1.shininess = shininessBase - rowStep * shininessMax;

			spheres.push_back(sph);
		}


		// RUBY / EMERALD PAIR
		offset = vec3(row * r, col * r, 1.0f) + vec3(offsetDim[0], offsetDim[1], 0);
		pos = origin + offset;
		pos.y() += offsetDim[1] * r / 5;
		{
			GameObject sph;
			pos.x() += sphereGroupOffset;
			sph.mTransform.SetPosition(pos);
			sph.mTransform.SetUniformScale(3.0f + 0 * sinf(sphereStep * PI));
			sph.mModel.mMeshID = EGeometry::SPHERE;

			BRDF_Material&		 mat0 = sph.mModel.mBRDF_Material;
			sph.mModel.SetDiffuseColor(LinearColor::bp_ruby);
			mat0.metalness = colStep;	// metalness [0.0f, 1.0f]

			const float roughnessLowClamp = 0.03f;
			mat0.roughness = std::max(0.04f, rowStep); // powf(std::max(rowStep, 0.04f), 2.55f);	// roughness [roughnessLowClamp, 1.0f]

			BlinnPhong_Material& mat1 = sph.mModel.mBlinnPhong_Material;
			const float shininessMax = 150.f;
			const float shininessBase = shininessMax + 7.0f;
			mat1.shininess = shininessBase - rowStep * shininessMax;

			spheres.push_back(sph);
		}

		{
			pos.x() -= 2 * sphereGroupOffset;
			GameObject sph;
			sph.mTransform.SetPosition(pos);
			sph.mTransform.SetUniformScale(3.0f + 0 * sinf(sphereStep * PI));
			sph.mModel.mMeshID = EGeometry::SPHERE;

			BRDF_Material&		 mat0 = sph.mModel.mBRDF_Material;
			sph.mModel.SetDiffuseColor(vec3(0.04f));
			mat0.metalness = colStep;	// metalness [0.0f, 1.0f]

			const float roughnessLowClamp = 0.1f;
			mat0.roughness = 1.0f - rowStep;// powf(std::max(rowStep, 0.04f), 0.75f);	// roughness [roughnessLowClamp, 1.0f]
			mat0.roughness = mat0.roughness == 0.0f ? 0.04f : mat0.roughness;

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


	//for (int gridX = 0; gridX < gridDimension[0]; ++gridX)
	//{
	//	for (int gridY = 0; gridY < gridDimension[1]; ++gridY)
	//	{
	//
	//	}
	//}
	if(!mLights.empty())
		mLights.back()._color = vec3(255, 244, 221) / 255.0f;
	
	SetEnvironmentMap(EEnvironmentMapPresets::BARCELONA);
}

void IBLTestScene::Unload()
{
	spheres.clear();
}

void IBLTestScene::Update(float dt)
{
	float t = ENGINE->GetTotalTime();
	const float moveSpeed = 45.0f;
	const float rotSpeed = XM_PI * sinf(t* 0.5f) * sinf((t+0.15f)* 0.5f) * dt * 0.01f;

	XMVECTOR rot = XMVectorZero();
	XMVECTOR tr = XMVectorZero();
	if (ENGINE->INP()->IsKeyDown("Numpad6")) tr += vec3::Right;
	if (ENGINE->INP()->IsKeyDown("Numpad4")) tr += vec3::Left;
	if (ENGINE->INP()->IsKeyDown("Numpad8")) tr += vec3::Forward;
	if (ENGINE->INP()->IsKeyDown("Numpad2")) tr += vec3::Back;
	if (ENGINE->INP()->IsKeyDown("Numpad9")) tr += vec3::Up;
	if (ENGINE->INP()->IsKeyDown("Numpad3")) tr += vec3::Down;
	if(!mLights.empty()) mLights[0]._transform.Translate(dt * tr * moveSpeed);

	for (GameObject& sphere : spheres)
	{
		const vec3& point = sSphereCenter;
		const float angle = rotSpeed;
		//sphere.mTransform.RotateAroundPointAndAxis(vec3::YAxis, angle, point);
	}
}

int IBLTestScene::Render(const SceneView & sceneView, bool bSendMaterialData) const
{
	int numObj = 0;
	for (const auto& sph : spheres)
	{
		sph.Render(mpRenderer, sceneView, bSendMaterialData);
		++numObj;
	}
	//testQuad.Render(mpRenderer, sceneView, bSendMaterialData);
	return numObj;
}

void IBLTestScene::RenderUI() const {}

void IBLTestScene::GetShadowCasters(std::vector<const GameObject*>& casters) const
{
	Scene::GetShadowCasters(casters);
	for (const GameObject& obj : spheres)	casters.push_back(&obj);
}

void IBLTestScene::GetSceneObjects(std::vector<const GameObject*>& objs) const
{
	Scene::GetSceneObjects(objs);
	for (const GameObject& obj : spheres)	objs.push_back(&obj);
	//objs.push_back(&testQuad);
}
#endif