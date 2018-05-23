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

#include "ObjectsScene.h"

#include "Application/Input.h"
#include "Renderer/Renderer.h"
#include "Engine/Engine.h"

#define ENABLE_POINT_LIGHTS
#define xROTATE_SHADOW_LIGHT	// todo: fix frustum for shadow

enum class WALLS
{
	FLOOR = 0,
	LEFT,
	RIGHT,
	FRONT,
	BACK,
	CEILING
};

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

void ObjectsScene::Load(SerializedScene& scene)
{
	SetEnvironmentMap(EEnvironmentMapPresets::WALK_OF_FAME);

	//---------------------------------------------------------------
	// ROOM
	//---------------------------------------------------------------
	std::for_each(std::begin(m_room.walls), std::end(m_room.walls), [&](GameObject*& pObj) { pObj = CreateNewGameObject(); });
	m_room.Initialize(mMaterials, mpRenderer);

	//Animation anim;
	//anim._fTracks.push_back(Track<float>(&mLights[1]._brightness, 40.0f, 800.0f, 3.0f));
	//m_animations.push_back(anim);

	//---------------------------------------------------------------
	// SPHERES
	//---------------------------------------------------------------
	const float sphHeight[2] = { 10.0f, 19.0f };
	{	// sphere grid
		constexpr float r = 11.0f;
		constexpr size_t gridDimension = 7;
		constexpr size_t numSph = gridDimension * gridDimension;

		const vec3 origin = vec3::Zero;

		for (size_t i = 0; i < numSph; i++)
		{
			// TRANSFORM
			//
			const size_t row = i / gridDimension;
			const size_t col = i % gridDimension;
			const float sphereStep = static_cast<float>(i) / numSph;
			const float rowStep = static_cast<float>(row) / ((numSph - 1) / gridDimension);
			const float rowStepInv = 1.0f - rowStep;
			const float colStep = static_cast<float>(col) / ((numSph - 1) / gridDimension);
			const float offsetDim = -static_cast<float>(gridDimension) * r / 2 + r / 2.0f;	// offset to center the grid
			const vec3 offset = vec3(col * r, -1.0f, row * r) + vec3(offsetDim, 0.75f, offsetDim);
			const vec3 pos = origin + offset;
			Transform transformObj(pos, Quaternion::Identity(), 2.5f);


			// MATERIALS
			//
			//const float gradientStep = rowStepInv * rowStepInv;
			const float gradientStep = 1.0f;
			auto sRGBtoLinear = [](const vec3& sRGB)
			{
				// some sRGB references
				// ---------------------------------
				// 128 146 217 - lila
				// 44 107 171  - 'facebook' blue
				// 129 197 216 - turuaz/yesil
				// 41 113 133  - koyu yesil
				// 177 206 149 - acik yesil
				// 211 235 82  - lime
				// 247 198 51  - turuncu/sari
				// ---------------------------------
				return vec3(
					std::pow(sRGB.x(), 2.2f),
					std::pow(sRGB.y(), 2.2f),
					std::pow(sRGB.z(), 2.2f)
				);
			};
			
#if 0
			const float r = std::pow(44.0f * gradientStep / 255.0f, 2.2f);
			const float g = std::pow(107.0f* gradientStep / 255.0f, 2.2f);
			const float b = std::pow(171.0f* gradientStep / 255.0f, 2.2f);
#else
			const float r = std::pow(128.0f * gradientStep / 255.0f, 2.2f);
			const float g = std::pow(146.0f * gradientStep / 255.0f, 2.2f);
			const float b = std::pow(217.0f * gradientStep / 255.0f, 2.2f);
#endif

			
			BRDF_Material* pMat0 = static_cast<BRDF_Material*>(mMaterials.CreateAndGetMaterial(GGX_BRDF));
			pMat0->diffuse = LinearColor(r, g, b);	
			pMat0->metalness = 1.0;	// col(-x->+x) -> metalness [0.0f, 1.0f]

			const float roughnessLowClamp = 0.07f;
			pMat0->roughness = colStep * 1.2f < roughnessLowClamp ? roughnessLowClamp : colStep * 1.2f;
			pMat0->roughness = (1.0f + roughnessLowClamp) - pMat0->roughness;	// row(-z->+z) -> roughness [roughnessLowClamp, 1.0f]


			//shared_ptr<BlinnPhong_Material> pMat1 = std::static_pointer_cast<BlinnPhong_Material>(mMaterials.CreateAndGetMaterial(BLINN_PHONG));
			//const float shininessMax = 150.f;
			//const float shininessBase = shininessMax + 7.0f;
			//pMat1->shininess = shininessBase - rowStep * shininessMax;


			// GAME OBJECT
			//
			GameObject* pSphereObject = Scene::CreateNewGameObject();
			pSphereObject->SetTransform(transformObj);
			pSphereObject->AddMesh(EGeometry::SPHERE);
			pSphereObject->AddMaterial(pMat0->ID);
			//pSphereObject->AddMaterial(pMat1->ID);
			mSpheres.push_back(pSphereObject);
		}
	}
}

void ObjectsScene::Unload()
{
	mSpheres.clear();
	mAnimations.clear();
}

void ObjectsScene::Update(float dt)
{
	for (auto& anim : mAnimations) anim.Update(dt);
	UpdateCentralObj(dt);
}

void ObjectsScene::RenderUI() const{}


void ObjectsScene::UpdateCentralObj(const float dt)
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

#if 0
	float angle = (dt * XM_PI * 0.08f) + (sinf(t) * sinf(dt * XM_PI * 0.03f));
	size_t sphIndx = 0;
	for (auto& sph : spheres)
	{
		if (sphIndx < 15)	// don't rotate grid spheres
		{
			const vec3 rotAxis = vec3::Up;
			const vec3 rotPoint = vec3::Zero;
			sph.mTransform.RotateAroundPointAndAxis(rotAxis, angle, rotPoint);
		}
		++sphIndx;
	}
#endif

#if 0
	// assume first object is the cube
	const float cubeRotSpeed = 100.0f; // degs/s
	mObjects.front()->mTransform.RotateAroundGlobalYAxisDegrees(-cubeRotSpeed / 10 * dt);

#ifdef ROTATE_SHADOW_LIGHT
	mLights[0]._transform.RotateAroundGlobalYAxisDegrees(dt * cubeRotSpeed);
#endif
#endif
}

#if 1 
void ObjectsScene::ToggleFloorNormalMap() {}
#else
void ObjectsScene::ToggleFloorNormalMap()
{
	TextureID nMap = m_room.floor.mModel.mBRDF_Material.normalMap;

	nMap = nMap == mpRenderer->GetTexture("openart/185_norm.JPG") ? -1 : mpRenderer->CreateTextureFromFile("openart/185_norm.JPG");
	m_room.floor.mModel.mBRDF_Material.normalMap = nMap;
}
#endif


void ObjectsScene::Room::Initialize(MaterialBuffer& materials, Renderer* pRenderer)
{
	const float floorWidth = 5 * 30.0f;
	const float floorDepth = 5 * 30.0f;
	const float wallHieght = 3.8*15.0f;	// amount from middle to top and bottom: because gpu cube is 2 units in length
	const float YOffset = wallHieght - 9.0f;
	const float thickness = 3.7f;

	// FLOOR
	{
		// Transform
		Transform tf;
		tf.SetPosition(0, -wallHieght + YOffset, 0);
		tf.SetScale(floorWidth, thickness, floorDepth);
		floor->SetTransform(tf);

		// Mesh
		floor->AddMesh(EGeometry::CUBE);

		// Materials
		BRDF_Material* pBRDF = static_cast<BRDF_Material*>(materials.CreateAndGetMaterial(GGX_BRDF));
		pBRDF->roughness = 0.8f;
		pBRDF->metalness = 0.0f;

		BlinnPhong_Material* pPhong = static_cast<BlinnPhong_Material*>(materials.CreateAndGetMaterial(BLINN_PHONG));
		pPhong->shininess = 40.0f;

		// set common material properties
		std::array<Material*, 2> materials{ pBRDF, pPhong };
		std::for_each(materials.begin(), materials.end(), [&](Material* pMat)
		{ 
			pMat->diffuse = LinearColor::white;
			pMat->alpha = 1.0f;
			pMat->tiling = vec2(10, 10);
			pMat->normalMap = pRenderer->CreateTextureFromFile("openart/161_norm.JPG");
			floor->AddMaterial(pMat->ID);
		});
	}

	const float ratio = floorWidth / wallHieght;
	const vec2 wallTiling = vec2(ratio, 1.3f) * 1.7f;
	const vec2 wallTilingInv = vec2(1.3f, ratio) * 1.7f;
	
	// RIGHT WALL
	{
		// Transform
		Transform tf;
		tf.SetScale(floorDepth, thickness, wallHieght);
		tf.SetPosition(floorWidth, YOffset, 0);
		tf.SetXRotationDeg(90.0f);
		tf.RotateAroundGlobalYAxisDegrees(-90);
		//tf.RotateAroundGlobalXAxisDegrees(-180.0f);
		wallR->SetTransform(tf);

		// Mesh
		wallR->AddMesh(EGeometry::CUBE);

		// Materials
		BRDF_Material* pBRDF = static_cast<BRDF_Material*>(materials.CreateAndGetMaterial(GGX_BRDF));
		pBRDF->roughness = 0.8f;
		pBRDF->metalness = 0.0f;

		BlinnPhong_Material* pPhong = static_cast<BlinnPhong_Material*>(materials.CreateAndGetMaterial(BLINN_PHONG));
		pPhong->shininess = 40.0f;

		// set common material properties
		std::array<Material*, 2> materials{ pBRDF, pPhong };
		std::for_each(materials.begin(), materials.end(), [&](Material* pMat)
		{
			pMat->diffuse = LinearColor::white;
			pMat->alpha = 1.0f;
			pMat->tiling = wallTiling;
			pMat->diffuseMap = pRenderer->CreateTextureFromFile("openart/190.JPG");
			pMat->normalMap = pRenderer->CreateTextureFromFile("openart/190_norm.JPG");
			wallR->AddMaterial(pMat->ID);
		});
	}


#if 0
	// This scene used to be a room scene, but now its an 'objects' scene with only floor and the right wall left
	// from the original scene. The code below is initialization code for the old room scene, without the
	// data oriented design refactoring applied, hence not compilable anymore. 
	//
	// CEILING
	{
		Transform& tf = ceiling.mTransform;
		tf.SetScale(floorWidth, thickness, floorDepth);
		tf.SetPosition(0, wallHieght + YOffset, 0);
		//ceiling.mModel.SetDiffuseAlpha(LinearColor::bp_bronze, 1.0f);

		ceiling.mModel.mBlinnPhong_Material.shininess = 20.0f;
	}



	// LEFT WALL
	{
		Transform& tf = wallL.mTransform;
		tf.SetScale(floorDepth, thickness, wallHieght);
		tf.SetPosition(-floorWidth, YOffset, 0);
		tf.SetXRotationDeg(-90.0f);
		tf.RotateAroundGlobalYAxisDegrees(-90.0f);
		//tf.SetRotationDeg(90.0f, 0.0f, -90.0f);

		wallL.mModel.mBlinnPhong_Material.shininess = 60.0f;
		wallL.mModel.SetDiffuseMap(pRenderer->CreateTextureFromFile("openart/190.JPG"));
		wallL.mModel.SetNormalMap(pRenderer->CreateTextureFromFile("openart/190_norm.JPG"));
		wallL.mModel.SetTextureTiling(wallTilingInv);
	}
	// WALL
	{
		Transform& tf = wallF.mTransform;
		tf.SetScale(floorWidth, thickness, wallHieght);
		tf.SetPosition(0, YOffset, floorDepth);
		tf.SetXRotationDeg(-90.0f);

		wallF.mModel.mBlinnPhong_Material.shininess = 90.0f;
		wallF.mModel.SetDiffuseMap(pRenderer->CreateTextureFromFile("openart/190.JPG"));
		wallF.mModel.SetNormalMap(pRenderer->CreateTextureFromFile("openart/190_norm.JPG"));
		wallL.mModel.SetTextureTiling(vec2(1, 3));
	}

	wallL->AddMesh(EGeometry::CUBE);
	wallR->AddMesh(EGeometry::CUBE);
	wallF->AddMesh(EGeometry::CUBE);
	ceiling->AddMesh(EGeometry::CUBE);
#endif
}
