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

#include "SSAOTestScene.h"
#include "Renderer/Renderer.h"

#undef min
#undef max

constexpr int MODEL_COUNT = 10;
#define LOAD_MODELS 0
#define LOAD_CUBE_GRID 0

#if DO_NOT_LOAD_SCENES
void SSAOTestScene::Load(SerializedScene& scene) {}
void SSAOTestScene::Unload() {}
void SSAOTestScene::Update(float dt) {}
void SSAOTestScene::RenderUI() const {}
#else
void SSAOTestScene::Load(SerializedScene& scene)
{
	//SetEnvironmentMap(EEnvironmentMapPresets::MILKYWAY);
	mSkybox = Skybox::s_Presets[MILKYWAY];

#if LOAD_CUBE_GRID
	// grid arrangement ( (row * col) cubes that are 'CUBE_DISTANCE' apart from each other )
	constexpr size_t	CUBE_ROW_COUNT = 6;
	constexpr size_t	CUBE_COLUMN_COUNT = 4;
	constexpr size_t	CUBE_COUNT = CUBE_ROW_COUNT * CUBE_COLUMN_COUNT;
	constexpr float		CUBE_DISTANCE_X = CUBE_ROW_COUNT * 2.70f;
	constexpr float		CUBE_DISTANCE_Y = CUBE_ROW_COUNT * 2.60f;
	{	
		const std::vector<vec3>   rowRotations = { vec3::Zero, vec3::Up, vec3::Right, vec3::Forward };
		static std::vector<float> sCubeDelays = std::vector<float>(CUBE_COUNT, 0.0f);
		const std::vector<LinearColor>  colors = { LinearColor::white, LinearColor::red, LinearColor::green, LinearColor::blue, LinearColor::orange, LinearColor::light_gray, LinearColor::cyan };

		for (int i = 0; i < CUBE_ROW_COUNT; ++i)
		{
			//Color color = c_colors[i % c_colors.size()];
			LinearColor color = vec3(1, 1, 1) * std::max(0.08f, static_cast<float>(i)) / (float)(CUBE_ROW_COUNT - 1);
			const float height = 80.0f;

			for (int j = 0; j < CUBE_COLUMN_COUNT; ++j)
			{
				GameObject* pCube = Scene::CreateNewGameObject();

				// Transform
				Transform tf;
				float x, y, z;	// position
				x = i * CUBE_DISTANCE_X - CUBE_ROW_COUNT * CUBE_DISTANCE_X / 2;
				y = height + CUBE_DISTANCE_Y * j;
				z = j * CUBE_DISTANCE_X - CUBE_COLUMN_COUNT * CUBE_DISTANCE_X / 2 + 50;

				tf.SetPosition(x, y, z);
				tf.SetUniformScale(8.0f);
				pCube->SetTransform(tf);

				// Mesh
				pCube->AddMesh(EGeometry::CUBE);

				// Material
				const TextureID texNormalMap = mpRenderer->CreateTextureFromFile("simple_normalmap.png");
				BRDF_Material* pBRDF = static_cast<BRDF_Material*>(Scene::CreateNewMaterial(GGX_BRDF));
				pBRDF->diffuse = color;
				pBRDF->alpha = 1.0f;
				pBRDF->diffuseMap = -1;// AmbientOcclusionPass::whiteTexture4x4;
				pBRDF->normalMap = texNormalMap;
				pCube->AddMaterial(pBRDF);
				pCubes.push_back(pCube);
			}

			for (int j = CUBE_COLUMN_COUNT-1; j >= 0; --j)
			{
				GameObject* pCube = Scene::CreateNewGameObject();

				// Transform
				Transform tf;
				float x, y, z;	// position
				x = i * CUBE_DISTANCE_X - CUBE_ROW_COUNT * CUBE_DISTANCE_X / 2;
				y = height + CUBE_DISTANCE_Y * j;
				z = j * CUBE_DISTANCE_X - CUBE_COLUMN_COUNT * CUBE_DISTANCE_X / 2 + 50;

				tf.SetPosition(x, y, z);
				tf.SetUniformScale(8.0f);
				pCube->SetTransform(tf);

				// Mesh
				pCube->AddMesh(EGeometry::CUBE);

				// Material
				const TextureID texNormalMap = mpRenderer->CreateTextureFromFile("simple_normalmap.png");
				BRDF_Material* pBRDF = static_cast<BRDF_Material*>(Scene::CreateNewMaterial(GGX_BRDF));
				pBRDF->diffuse = color;
				pBRDF->alpha = 1.0f;
				pBRDF->diffuseMap = -1;// AmbientOcclusionPass::whiteTexture4x4;
				pBRDF->normalMap = texNormalMap;
				pCube->AddMaterial(pBRDF);
				pCubes.push_back(pCube);
			}
		}
	}
#endif

#if LOAD_MODELS
	// load nanosuit models
	for (int i = 0; i < MODEL_COUNT; ++i)
	{
		// linear position - left to right with
		const float stepLinear = 20.0f;
		const vec3 posLinear((i - MODEL_COUNT / 2) * stepLinear, 0, 0);

		// arc position - models make a pie / circle
		const float arcRange = 85.0f;	// how far away models are from center
		const float arcSweepAngle = 160.0f * DEG2RAD;
		const float arcInitialOffsetAngle = (180.0f - arcSweepAngle) * DEG2RAD * 0.1f;
		const float translationOffsetZ = -45.0f;	// get a bit closer to the camera
		const Quaternion arcRotation = Quaternion::FromAxisAngle(vec3::Up, i * (arcSweepAngle / MODEL_COUNT) + PI);
		const vec3 arcInitialPosition = Quaternion::FromAxisAngle(vec3::Up, arcInitialOffsetAngle).TransformVector(vec3::Right * arcRange);
		const vec3 posArc = arcRotation.TransformVector(arcInitialPosition) + vec3(0, 0, translationOffsetZ);

		const vec3 position(posArc);	// use linear/arc position
		const Quaternion rotation = Quaternion::Identity();
		const vec3 scale(3.5f);

		GameObject* pModel = Scene::CreateNewGameObject();
		pModel->SetModel(Scene::LoadModel("nanosuit/nanosuit.obj"));
		pModel->SetTransform(Transform(position, rotation, scale));
		pModel->GetTransform().RotateAroundGlobalYAxisDegrees(90.0f * (i % 4));
		pModels.push_back(pModel);
	}
#endif
}

void SSAOTestScene::Unload()
{
#if LOAD_CUBE_GRID
	pCubes.clear();
#endif

#if LOAD_MODELS
	pModels.clear();
#endif
}

void SSAOTestScene::Update(float dt)
{
	constexpr float ROTATION_SPEED_DEG_PER_SEC = 12.0f;
#if LOAD_MODELS
	for (GameObject* pModel : pModels)
	{
		//pModel->GetTransform().RotateAroundGlobalYAxisDegrees(dt * ROTATION_SPEED_DEG_PER_SEC);
	}
#endif
}
void SSAOTestScene::RenderUI() const {}
#endif