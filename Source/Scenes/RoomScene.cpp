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

#include "RoomScene.h"

#include "Application/Input.h"
#include "Renderer/Renderer.h"
#include "Engine/SceneManager.h"
#include "Engine/Engine.h"

#define ENABLE_POINT_LIGHTS
#define xROTATE_SHADOW_LIGHT	// todo: fix frustum for shadow

constexpr size_t	RAND_LIGHT_COUNT	= 0;

constexpr float		DISCO_PERIOD		= 0.25f;

enum class WALLS
{
	FLOOR = 0,
	LEFT,
	RIGHT,
	FRONT,
	BACK,
	CEILING
};

RoomScene::RoomScene(SceneManager& sceneMan, std::vector<Light>& lights)
	:
	Scene(sceneMan, lights)
{}

void RoomScene::Load(SerializedScene& scene)
{
	m_room.Initialize(mpRenderer);

	for (size_t i = 0; i < RAND_LIGHT_COUNT; i++)
	{
		unsigned rndIndex = rand() % LinearColor::Palette().size();
		LinearColor rndColor = LinearColor::Palette()[rndIndex];
		Light l;
		float x = RandF(-20.0f, 20.0f);
		float y = RandF(-15.0f, 15.0f);
		float z = RandF(-10.0f, 20.0f);
		l._transform.SetPosition(x, y, z);
		l._transform.SetUniformScale(0.1f);
		l._renderMesh = EGeometry::SPHERE;
		l._color = rndColor;
		l.SetLightRange(static_cast<float>(rand() % 50 + 10));
		mLights.push_back(l);
	}

	//Animation anim;
	//anim._fTracks.push_back(Track<float>(&mLights[1]._brightness, 40.0f, 800.0f, 3.0f));
	//m_animations.push_back(anim);


	// objects
	//---------------------------------------------------------------
	const float sphHeight[2] = { 10.0f, 19.0f };
	{	// sphere grid
		constexpr float r = 11.0f;
		constexpr size_t gridDimension = 7;
		constexpr size_t numSph = gridDimension * gridDimension;

		const vec3 origin = vec3::Zero;

		for (size_t i = 0; i < numSph; i++)
		{
			const size_t row = i / gridDimension;
			const size_t col = i % gridDimension;

			const float sphereStep = static_cast<float>(i) / numSph;
			const float rowStep = static_cast<float>(row) / ((numSph - 1) / gridDimension);
			const float colStep = static_cast<float>(col) / ((numSph - 1) / gridDimension);

			// offset to center the grid
			const float offsetDim = -static_cast<float>(gridDimension) * r / 2 + r / 2.0f;
			const vec3 offset = vec3(col * r, -1.0f, row * r) + vec3(offsetDim, 0.75f, offsetDim);

			const vec3 pos = origin + offset;

			GameObject sph;
			sph.mTransform = pos;
			sph.mTransform.SetUniformScale(2.5f);
			sph.mModel.mMesh = EGeometry::SPHERE;

			BRDF_Material&		 mat0 = sph.mModel.mBRDF_Material;
			// col(-x->+x) -> metalness [0.0f, 1.0f]
			sph.mModel.SetDiffuseColor(LinearColor(vec3(LinearColor::gold)));
			//sph.mModel.SetDiffuseColor(LinearColor(vec3(LinearColor::white) * rowStep));
			mat0.metalness = 1.0;

			// row(-z->+z) -> roughness [roughnessLowClamp, 1.0f]
			const float roughnessLowClamp = 0.07f;
			mat0.roughness = rowStep < roughnessLowClamp ? roughnessLowClamp : rowStep;
			mat0.roughness = (1.0f + roughnessLowClamp) - mat0.roughness;

			BlinnPhong_Material& mat1 = sph.mModel.mBlinnPhong_Material;
			const float shininessMax = 150.f;
			const float shininessBase = shininessMax + 7.0f;
			mat1.shininess = shininessBase - rowStep * shininessMax;

			mSpheres.push_back(sph);
		}
	}

	// objects from scene file
	for (GameObject& obj : mObjects)
	{
		//if (obj.mModel.mMesh == EGeometry::SPHERE || obj.mModel.mMesh == EGeometry::CUBE)
		;// obj.mRenderSettings.bRenderTBN = true;
	}

	mSkybox = Skybox::s_Presets[EEnvironmentMapPresets::MILKYWAY];
}

void RoomScene::Unload()
{
	mSpheres.clear();
	mAnimations.clear();
}

void RoomScene::Update(float dt)
{
	for (auto& anim : mAnimations) anim.Update(dt);
	UpdateCentralObj(dt);
}

void ExampleRender(Renderer* pRenderer, const XMMATRIX& viewProj);

int RoomScene::Render(const SceneView& sceneView, bool bSendMaterialData) const
{
	m_room.Render(mpRenderer, sceneView, bSendMaterialData);
	int numObj = 6;

	for (const auto& sph : mSpheres)
	{
		sph.Render(mpRenderer, sceneView, bSendMaterialData);
		++numObj;
	}
	return numObj;
}

void RoomScene::RenderUI() const{}


void RoomScene::GetShadowCasters(std::vector<const GameObject*>& casters) const
{
	Scene::GetShadowCasters(casters);
	casters.push_back(&m_room.floor);
	casters.push_back(&m_room.wallL);
	casters.push_back(&m_room.wallR);
	casters.push_back(&m_room.wallF);
	casters.push_back(&m_room.ceiling);
	for (const GameObject& obj : mSpheres)	casters.push_back(&obj);
}

void RoomScene::GetSceneObjects(std::vector<const GameObject*>& objs) const
{
	Scene::GetSceneObjects(objs);
	objs.push_back(&m_room.floor);
	objs.push_back(&m_room.wallL);
	objs.push_back(&m_room.wallR);
	objs.push_back(&m_room.wallF);
	objs.push_back(&m_room.ceiling);
	for (const GameObject& obj : mSpheres)	objs.push_back(&obj);
}

void RoomScene::UpdateCentralObj(const float dt)
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

	// assume first object is the cube
	const float cubeRotSpeed = 100.0f; // degs/s
	mObjects.front().mTransform.RotateAroundGlobalYAxisDegrees(-cubeRotSpeed / 10 * dt);

#ifdef ROTATE_SHADOW_LIGHT
	mLights[0]._transform.RotateAroundGlobalYAxisDegrees(dt * cubeRotSpeed);
#endif
}

void RoomScene::ToggleFloorNormalMap()
{
	TextureID nMap = m_room.floor.mModel.mBRDF_Material.normalMap;

	nMap = nMap == mpRenderer->GetTexture("openart/185_norm.JPG") ? -1 : mpRenderer->CreateTextureFromFile("openart/185_norm.JPG");
	m_room.floor.mModel.mBRDF_Material.normalMap = nMap;
}

void RoomScene::Room::Render(Renderer* pRenderer, const SceneView& sceneView, bool sendMaterialData) const
{
	floor.Render(pRenderer, sceneView, sendMaterialData);
	//wallL.Render(pRenderer, sceneView, sendMaterialData);
	wallR.Render(pRenderer, sceneView, sendMaterialData);
	//wallF.Render(pRenderer, sceneView, sendMaterialData);
	//ceiling.Render(pRenderer, sceneView, sendMaterialData);
}

void RoomScene::Room::Initialize(Renderer* pRenderer)
{
	const float floorWidth = 5 * 30.0f;
	const float floorDepth = 5 * 30.0f;
	const float wallHieght = 3.8*15.0f;	// amount from middle to top and bottom: because gpu cube is 2 units in length
	const float YOffset = wallHieght - 9.0f;

#if 0
	constexpr size_t WallCount = 5;
	std::array<Material&, WallCount> mats = 
	{
		floor.m_model.m_material,
		wallL.m_model.m_material,
		wallR.m_model.m_material,
		wallF.m_model.m_material,
		ceiling.m_model.m_material
	};

	std::array<Transform&, WallCount> tfs =
	{

		  floor.m_transform,
		  wallL.m_transform,
		  wallR.m_transform,
		  wallF.m_transform,
		ceiling.m_transform
	};
#endif

	const float thickness = 3.7f;
	// FLOOR
	{
		Transform& tf = floor.mTransform;
		tf.SetScale(floorWidth, thickness, floorDepth);
		tf.SetPosition(0, -wallHieght + YOffset, 0);

		floor.mModel.mBlinnPhong_Material.shininess = 40.0f;
		floor.mModel.mBRDF_Material.roughness = 0.8f;
		floor.mModel.mBRDF_Material.metalness = 0.0f;
		floor.mModel.SetDiffuseAlpha(LinearColor::gray, 1.0f);
		floor.mModel.SetNormalMap(pRenderer->CreateTextureFromFile("openart/161_norm.JPG"));
		floor.mModel.SetTextureTiling(vec2(10, 10));
		//mat = Material::bronze;
		//floor.m_model.SetDiffuseMap(pRenderer->CreateTextureFromFile("185.JPG"));
		//floor.m_model.SetNormalMap(pRenderer->CreateTextureFromFile("185_norm.JPG"));
		//floor.m_model.SetNormalMap(pRenderer->CreateTextureFromFile("BumpMapTexturePreview.JPG"));
	}
#if 1
	// CEILING
	{
		Transform& tf = ceiling.mTransform;
		tf.SetScale(floorWidth, thickness, floorDepth);
		tf.SetPosition(0, wallHieght + YOffset, 0);
		//ceiling.mModel.SetDiffuseAlpha(LinearColor::bp_bronze, 1.0f);

		ceiling.mModel.mBlinnPhong_Material.shininess = 20.0f;
	}

	const float ratio = floorWidth / wallHieght;
	const vec2 wallTiling    = vec2(ratio, 1.3f) * 1.7f;
	const vec2 wallTilingInv = vec2(1.3f, ratio) * 1.7f;
	// RIGHT WALL
	{
		Transform& tf = wallR.mTransform;
		tf.SetScale(floorDepth, thickness, wallHieght);
		tf.SetPosition(floorWidth, YOffset, 0);
		tf.SetXRotationDeg(90.0f);
		tf.RotateAroundGlobalYAxisDegrees(-90);
		//tf.RotateAroundGlobalXAxisDegrees(-180.0f);

		wallR.mModel.SetDiffuseMap(pRenderer->CreateTextureFromFile("openart/190.JPG"));
		wallR.mModel.SetNormalMap(pRenderer->CreateTextureFromFile("openart/190_norm.JPG"));
		wallR.mModel.SetTextureTiling(wallTiling);
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

	wallL.mModel.mMesh = EGeometry::CUBE;
	wallR.mModel.mMesh = EGeometry::CUBE;
	wallF.mModel.mMesh = EGeometry::CUBE;
	ceiling.mModel.mMesh = EGeometry::CUBE;
#endif
	floor.mModel.mMesh = EGeometry::CUBE;
}

