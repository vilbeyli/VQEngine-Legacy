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

#include "IBLTestScene.h"
#include "Application/Input.h"
#include "Engine/Engine.h"

#include "Renderer/Renderer.h"

#undef max

static const vec3 sSphereCenter = vec3(0, 1, 20);

void IBLTestScene::MakeSphereGrid()
{
	// todo
}

#if DO_NOT_LOAD_SCENES
void IBLTestScene::Load(SerializedScene& scene) {}
void IBLTestScene::Unload() {}
void IBLTestScene::Update(float dt) {}
void IBLTestScene::RenderUI() const {}
#else
void IBLTestScene::Load(SerializedScene& scene)
{
	mTimeAccumulator = 0.0f;

	// sphere grid
	constexpr float r = 22.0f;
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
		const float sphereGroupOffset = 12.0f + gridDimension[0] * r / 2;
		constexpr float SPHERE_SCALE = 5.65f;

		// TODO: Remove the copy paste... this is dumb.

		// GOLD / SILVER PAIR
		{
			GameObject* sph = Scene::CreateNewGameObject();

			Transform tf;
			pos.x() += sphereGroupOffset;
			tf.SetPosition(pos);
			tf.SetUniformScale(SPHERE_SCALE + 0 * sinf(sphereStep * PI));

			BRDF_Material* pBRDF = static_cast<BRDF_Material*>(Scene::CreateNewMaterial(GGX_BRDF));
			const float roughnessLowClamp = 0.03f;
			//pBRDF->roughness = powf(std::max(rowStep, 0.04f), 2.01f);	
			pBRDF->roughness = std::max(0.04f, rowStep);// roughness [roughnessLowClamp, 1.0f]
			pBRDF->metalness = 1.0f - colStep;			// metalness [0.0f, 1.0f]
			pBRDF->diffuse = LinearColor::white;
			//pBRDF->diffuseMap = mpRenderer->CreateTextureFromFile("openart/190.JPG");
			//pBRDF->normalMap = mpRenderer->CreateTextureFromFile("openart/190_norm.JPG");
			
			sph->AddMesh(EGeometry::SPHERE);
			sph->AddMaterial(pBRDF);
			sph->SetTransform(tf);
			spheres.push_back(sph);
		}
		{
			GameObject* sph = Scene::CreateNewGameObject();

			Transform tf;
			pos.x() -= 2 * sphereGroupOffset;
			tf.SetPosition(pos);
			tf.SetUniformScale(SPHERE_SCALE + 0 * sinf(sphereStep * PI));

			BRDF_Material* pBRDF = static_cast<BRDF_Material*>(Scene::CreateNewMaterial(GGX_BRDF));
			const float roughnessLowClamp = 0.1f;
			//pBRDF->roughness = // powf(std::max(rowStep, 0.04f), 2.01f);
			pBRDF->roughness = std::max(0.04f, 1.0f - rowStep);	// roughness [roughnessLowClamp, 1.0f];
			pBRDF->metalness = 1.0f - colStep;					// metalness [0.0f, 1.0f]
			pBRDF->diffuse = LinearColor::gold;
			
			sph->SetTransform(tf);
			sph->AddMesh(EGeometry::SPHERE);
			sph->AddMaterial(pBRDF);
			spheres.push_back(sph);
		}


		// RUBY / EMERALD PAIR
		offset = vec3(row * r, col * r, 1.0f) + vec3(offsetDim[0], offsetDim[1], 0);
		pos = origin + offset;
		pos.y() += offsetDim[1] * r / 5;
		{
			GameObject* sph = Scene::CreateNewGameObject();

			Transform tf;
			pos.x() += sphereGroupOffset;
			tf.SetPosition(pos);
			tf.SetUniformScale(SPHERE_SCALE + 0 * sinf(sphereStep * PI));
			sph->SetTransform(tf);

			sph->AddMesh(EGeometry::SPHERE);

			BRDF_Material* pBRDF = static_cast<BRDF_Material*>(Scene::CreateNewMaterial(GGX_BRDF));
			const float roughnessLowClamp = 0.1f;
			pBRDF->roughness = std::max(0.04f, rowStep);
			pBRDF->metalness = colStep;	
			pBRDF->diffuse = LinearColor::bp_ruby;
			sph->AddMaterial(pBRDF);

			spheres.push_back(sph);
		}
		{
			GameObject* sph = Scene::CreateNewGameObject();

			Transform tf;
			pos.x() -= 2 * sphereGroupOffset;
			tf.SetPosition(pos);
			tf.SetUniformScale(SPHERE_SCALE + 0 * sinf(sphereStep * PI));
			sph->SetTransform(tf);

			sph->AddMesh(EGeometry::SPHERE);

			BRDF_Material* pBRDF = static_cast<BRDF_Material*>(Scene::CreateNewMaterial(GGX_BRDF));
			const float roughnessLowClamp = 0.1f;
			pBRDF->roughness = 1.04f - std::max(0.04f, rowStep);// roughness [roughnessLowClamp, 1.0f]
			pBRDF->metalness = colStep;
			pBRDF->diffuse = vec3(0.04f);
			sph->AddMaterial(pBRDF);

			spheres.push_back(sph);
		}
	}

	if(!mLights.empty())
		mLights.back().mColor = vec3(255, 244, 221) / 255.0f;
	
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
	if(!mLights.empty()) mLights[0].mTransform.Translate(dt * tr * moveSpeed);

	for (GameObject* sphere : spheres)
	{
		const vec3& point = sSphereCenter;
		const float angle = rotSpeed;
		//sphere.mTransform.RotateAroundPointAndAxis(vec3::YAxis, angle, point);
	}


	// animate the grid object
	constexpr float PERIOD = 2.5f;
	constexpr float ROTATION_SPEED_DEG_PER_SEC = 10.0f;
	static float sDirection = 1.0f;
	mTimeAccumulator += dt;
	if (std::fabsf(mTimeAccumulator) > PERIOD)
	{
		mTimeAccumulator = mTimeAccumulator > 0.0f ? -PERIOD : PERIOD;
		sDirection *= -1.0f;
	}
	for (GameObject* pObj : mpObjects)
	{
		if (pObj->GetModelData().mMeshIDs[0] != EGeometry::GRID)
			continue;

		// rotate the grid obj
		//Quaternion qRotate = Quaternion::FromAxisAngle(vec3::ZAxis, DEG2RAD * 1.0f);


		pObj->GetTransform().RotateAroundGlobalZAxisDegrees(sDirection * ROTATION_SPEED_DEG_PER_SEC * dt);
	}
}

void IBLTestScene::RenderUI() const {}
#endif