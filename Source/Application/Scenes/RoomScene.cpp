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
#include "SceneManager.h"

#include "Renderer.h"
#include "Engine.h"
#include "Input.h"

#define ENABLE_POINT_LIGHTS
#define xROTATE_SHADOW_LIGHT	// todo: fix frustum for shadow

constexpr size_t	RAND_LIGHT_COUNT	= 0;

constexpr size_t	CUBE_ROW_COUNT		= 21;
constexpr size_t	CUBE_COLUMN_COUNT	= 4;
constexpr size_t	CUBE_COUNT			= CUBE_ROW_COUNT * CUBE_COLUMN_COUNT;
constexpr float		CUBE_DISTANCE		= 4.0f * 1.4f;

//constexpr size_t	CUBE_ROW_COUNT		= 20;
//constexpr size_t	CUBE_ROW_COUNT		= 20;

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
	
	// lights
	//---------------------------------------------------------------
	mLights = std::move(scene.lights);
	mLights[1]._color = vec3(mLights[1]._color) * 3.0f;// * 0;
	mLights[2]._color = vec3(mLights[2]._color) * 2.0f;// * 0;
	mLights[3]._color = vec3(mLights[3]._color) * 1.5f;// * 0;

	// hard-coded scales for now
	mLights[0]._transform.SetUniformScale(0.8f);
	mLights[1]._transform.SetUniformScale(0.3f);
	mLights[2]._transform.SetUniformScale(0.4f);
	mLights[3]._transform.SetUniformScale(0.5f);

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

	Animation anim;
	anim._fTracks.push_back(Track<float>(&mLights[1]._brightness, 40.0f, 800.0f, 3.0f));
	m_animations.push_back(anim);


	// objects
	//---------------------------------------------------------------
	const float sphHeight[2] = { 10.0f, 19.0f };
	{	// rotating spheres
		const float r = 30.0f;
		const size_t numSph = 15;

		const vec3 radius(r, sphHeight[0], 0.0f);
		const float rot = 2.0f * XM_PI / (numSph == 0 ? 1.0f : numSph);
		const vec3 axis = vec3::Up;
		for (size_t i = 0; i < numSph; i++)
		{
			// calc position
			Quaternion rotQ = Quaternion::FromAxisAngle(axis, rot * i);
			vec3 pos = rotQ.TransformVector(radius);

			GameObject sph;
			sph.mTransform = pos;
			sph.mTransform.Translate(vec3::Up * (sphHeight[1] * ((float)i / (float)numSph) + sphHeight[1]));
			sph.mModel.mMesh = EGeometry::SPHERE;

			// set materials blinn-phong
			sph.mModel.mBlinnPhong_Material = BlinnPhong_Material::gold;

			// set materials brdf
			const float baseSpecular = 5.0f;
			const float step = 15.0f;
			sph.mModel.mBRDF_Material.diffuse = LinearColor::gray;
			sph.mModel.mBRDF_Material.specular = vec3(baseSpecular + (static_cast<float>(i) / numSph) * step)._v;
			//sph.m_model.m_material.specular = i % 2 == 0 ? vec3((static_cast<float>(i) / numSph) * baseSpecular)._v : vec3((static_cast<float>(numSph - i) / numSph) * baseSpecular)._v;
			//sph.m_model.m_material.specular = i < numSph / 2 ? vec3(0.0f).v : vec3(90.0f).v;

			sph.mModel.mBRDF_Material.roughness = 0.2f;
			sph.mModel.mBRDF_Material.metalness = 1.0f - static_cast<float>(i) / static_cast<float>(numSph);

			spheres.push_back(sph);
		}
	}
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
			const vec3 offset = vec3(col * r, -1.0f, row * r) + vec3(offsetDim, 0.0f, offsetDim);

			const vec3 pos = origin + offset;

			GameObject sph;
			sph.mTransform = pos;
			sph.mTransform.SetUniformScale(2.5f);
			sph.mModel.mMesh = EGeometry::SPHERE;

			BRDF_Material&		 mat0 = sph.mModel.mBRDF_Material;
			// col(-x->+x) -> metalness [0.0f, 1.0f]
			sph.mModel.SetDiffuseColor(LinearColor(vec3(LinearColor::red) / 1.5f));
			mat0.metalness = colStep;

			// row(-z->+z) -> roughness [roughnessLowClamp, 1.0f]
			const float roughnessLowClamp = 0.065f;
			mat0.roughness = rowStep < roughnessLowClamp ? roughnessLowClamp : rowStep;

			BlinnPhong_Material& mat1 = sph.mModel.mBlinnPhong_Material;
			const float shininessMax = 150.f;
			const float shininessBase = shininessMax + 7.0f;
			mat1.shininess = shininessBase - rowStep * shininessMax;

			spheres.push_back(sph);
		}
	}

	// objects from scene file
	for (GameObject& obj : objects)
	{
		//if (obj.mModel.mMesh == EGeometry::SPHERE || obj.mModel.mMesh == EGeometry::CUBE)
			obj.mRenderSettings.bRenderTBN = true;
	}

	m_skybox = ESkyboxPreset::NIGHT_SKY;
}

void RoomScene::Update(float dt)
{
	for (auto& anim : m_animations) anim.Update(dt);
	UpdateCentralObj(dt);
}

void ExampleRender(Renderer* pRenderer, const XMMATRIX& viewProj);

void RoomScene::Render(const SceneView& sceneView, bool bSendMaterialData) const
{
	m_room.Render(mpRenderer, sceneView, bSendMaterialData);
	for (const auto& sph : spheres) sph.Render(mpRenderer, sceneView, bSendMaterialData);
}

void RoomScene::GetShadowCasters(std::vector<const GameObject*>& casters) const
{
	Scene::GetShadowCasters(casters);
	casters.push_back(&m_room.floor);
	casters.push_back(&m_room.wallL);
	casters.push_back(&m_room.wallR);
	casters.push_back(&m_room.wallF);
	casters.push_back(&m_room.ceiling);
	for (const GameObject& obj : spheres)	casters.push_back(&obj);
}

void RoomScene::GetSceneObjects(std::vector<const GameObject*>& objs) const
{
	Scene::GetSceneObjects(objs);
	objs.push_back(&m_room.floor);
	objs.push_back(&m_room.wallL);
	objs.push_back(&m_room.wallR);
	objs.push_back(&m_room.wallF);
	objs.push_back(&m_room.ceiling);
	for (const GameObject& obj : spheres)	objs.push_back(&obj);
}

void RoomScene::InitializeObjectArrays(SerializedScene& scene)
{
#if 0
	int i = 3;
	const float xCoord = 85.0f;
	const float distToEachOther = -30.0f;
	grid.mTransform = scene.objects.back().mTransform;
	--i;

	cube.mTransform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	cube.mTransform.SetUniformScale(5.0f);

	//cubes.front().m_transform.Translate(0, 30, 0);
	cube.mTransform.SetPosition(0, 65, 0);
	cube.mTransform.SetXRotationDeg(90);
	cube.mTransform.RotateAroundGlobalXAxisDegrees(30);
	cube.mTransform.RotateAroundGlobalYAxisDegrees(30);
	cube.mModel.mBRDF_Material.normalMap = mpRenderer->CreateTextureFromFile("BumpMapTexturePreview.png");
	cube.mModel.mBRDF_Material.metalness = 0.6f;
	cube.mModel.mBRDF_Material.roughness = 0.15f;

	--i;

	cylinder.mTransform.SetPosition(xCoord, 0.0f, distToEachOther * i);
	cylinder.mTransform.SetUniformScale(7.0f);
	--i;


	sphere.mTransform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	//sphere.m_transform.SetPosition(0, 20.0f, 0);
	//sphere.m_transform.SetUniformScale(5.0f);
	--i;

	triangle.mTransform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	triangle.mTransform.SetXRotationDeg(30.0f);
	triangle.mTransform.RotateAroundGlobalYAxisDegrees(30.0f);
	triangle.mTransform.SetUniformScale(4.0f);
	--i;

	quad.mTransform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	quad.mTransform.SetXRotationDeg(30);
	quad.mTransform.RotateAroundGlobalYAxisDegrees(30);
	quad.mTransform.SetUniformScale(4.0f);

	grid.mModel.mMesh = scene.objects.back().mModel.mMesh;
	cylinder.mModel.mMesh = EGeometry::CYLINDER;
	triangle.mModel.mMesh = EGeometry::TRIANGLE;
	    quad.mModel.mMesh = EGeometry::QUAD;
		cube.mModel.mMesh = EGeometry::CUBE;
	 sphere.mModel.mMesh = EGeometry::SPHERE;

		grid.mModel.mBRDF_Material.roughness = 0.02f;
		grid.mModel.mBRDF_Material.metalness = 0.6f;
		grid.mModel.mBlinnPhong_Material.shininess = 40.f;
	cylinder.mModel.mBRDF_Material.roughness = 0.3f;
	cylinder.mModel.mBRDF_Material.metalness = 0.3f;
		cube.mModel.mBRDF_Material.roughness = 0.6f;
		cube.mModel.mBRDF_Material.metalness = 0.6f;
	  sphere.mModel.mBRDF_Material.roughness = 0.3f;
	  sphere.mModel.mBRDF_Material.metalness = 1.0f;
	  sphere.mModel.SetDiffuseColor(LinearColor(0.04f, 0.04f, 0.04f));
	  sphere.mTransform.SetUniformScale(2.5f);
#endif
#if 0 // todo: move to ssao scene
	  Plane2.mModel.mMesh = EGeometry::QUAD;
	  Plane2.mTransform.SetPosition(-300, 0, 0);
	  Plane2.mTransform.SetUniformScale(100);
	  Plane2.mTransform.RotateAroundGlobalXAxisDegrees(90);

	  obj2.mModel.mMesh = EGeometry::SPHERE;
	  obj2.mTransform.SetPosition(-300, 10, 0);
	  obj2.mTransform.SetUniformScale(10);

	  {	// grid arrangement ( (row * col) cubes that are 'CUBE_DISTANCE' apart from each other )
		  const std::vector<vec3>   rowRotations = { vec3::Zero, vec3::Up, vec3::Right, vec3::Forward };
		  static std::vector<float> sCubeDelays = std::vector<float>(CUBE_COUNT, 0.0f);
		  const std::vector<LinearColor>  colors = { LinearColor::white, LinearColor::red, LinearColor::green, LinearColor::blue, LinearColor::orange, LinearColor::light_gray, LinearColor::cyan };

		  for (int i = 0; i < CUBE_ROW_COUNT; ++i)
		  {
			  //Color color = c_colors[i % c_colors.size()];
			  LinearColor color = vec3(1, 1, 1) * static_cast<float>(i) / (float)(CUBE_ROW_COUNT - 1);

			  for (int j = 0; j < CUBE_COLUMN_COUNT; ++j)
			  {
				  GameObject cube;

				  // set transform
				  float x, y, z;	// position
				  x = i * CUBE_DISTANCE - CUBE_ROW_COUNT * CUBE_DISTANCE / 2;
				  y = 5.0f + cubes.size();
				  z = j * CUBE_DISTANCE - CUBE_COLUMN_COUNT * CUBE_DISTANCE / 2;
				  cube.mTransform.SetPosition(x, y, z + 350);
				  cube.mTransform.SetUniformScale(4.0f);

				  // set material
				  cube.mModel.SetDiffuseColor(color);
				  cube.mModel.SetNormalMap(mpRenderer->CreateTextureFromFile("simple_normalmap.png"));

				  // set model
				  cube.mModel.mMesh = EGeometry::CUBE;

				  cubes.push_back(cube);
			  }
		  }
	  }
#endif
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

	// assume first object is the cube
	const float cubeRotSpeed = 100.0f; // degs/s
	objects.front().mTransform.RotateAroundGlobalYAxisDegrees(-cubeRotSpeed / 10 * dt);

	for (int i = 0; i <  CUBE_ROW_COUNT; ++i)
	{
		for (int j = 0; j < CUBE_COLUMN_COUNT; ++j)
		{
			if (i == 0 && j == 0) continue;
			//if (j > 4)
			{	// exclude diagonal
				//cubes[j* CUBE_ROW_COUNT + i].m_transform.RotateAroundAxisDegrees(vec3::XAxis, dt * cubeRotSpeed);
			}
		}
	}

#ifdef ROTATE_SHADOW_LIGHT
	mLights[0]._transform.RotateAroundGlobalYAxisDegrees(dt * cubeRotSpeed);
#endif
}

void RoomScene::ToggleFloorNormalMap()
{
	TextureID nMap = m_room.floor.mModel.mBRDF_Material.normalMap;

	nMap = nMap == mpRenderer->GetTexture("185_norm.JPG") ? -1 : mpRenderer->CreateTextureFromFile("185_norm.JPG");
	m_room.floor.mModel.mBRDF_Material.normalMap = nMap;
}

void RoomScene::Room::Render(Renderer* pRenderer, const SceneView& sceneView, bool sendMaterialData) const
{
	floor.Render(pRenderer, sceneView, sendMaterialData);
	wallL.Render(pRenderer, sceneView, sendMaterialData);
	wallR.Render(pRenderer, sceneView, sendMaterialData);
	wallF.Render(pRenderer, sceneView, sendMaterialData);
	ceiling.Render(pRenderer, sceneView, sendMaterialData);
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

		floor.mModel.mBlinnPhong_Material.shininess = 20.0f;
		floor.mModel.mBRDF_Material.roughness = 0.8f;
		floor.mModel.mBRDF_Material.metalness = 0.0f;

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

		ceiling.mModel.mBlinnPhong_Material.shininess = 20.0f;
	}

	// RIGHT WALL
	{
		Transform& tf = wallR.mTransform;
		tf.SetScale(floorDepth, thickness, wallHieght);
		tf.SetPosition(floorWidth, YOffset, 0);
		tf.SetXRotationDeg(90.0f);
		tf.RotateAroundGlobalYAxisDegrees(-90);
		//tf.RotateAroundGlobalXAxisDegrees(-180.0f);

		wallR.mModel.SetDiffuseMap(pRenderer->CreateTextureFromFile("190.JPG"));
		wallR.mModel.SetNormalMap(pRenderer->CreateTextureFromFile("190_norm.JPG"));
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
		wallL.mModel.SetDiffuseMap(pRenderer->CreateTextureFromFile("190.JPG"));
		wallL.mModel.SetNormalMap(pRenderer->CreateTextureFromFile("190_norm.JPG"));
	}
	// WALL
	{
		Transform& tf = wallF.mTransform;
		tf.SetScale(floorWidth, thickness, wallHieght);
		tf.SetPosition(0, YOffset, floorDepth);
		tf.SetXRotationDeg(-90.0f);

		wallF.mModel.mBlinnPhong_Material.shininess = 90.0f;
		wallF.mModel.SetDiffuseMap(pRenderer->CreateTextureFromFile("190.JPG"));
		wallF.mModel.SetNormalMap(pRenderer->CreateTextureFromFile("190_norm.JPG"));
	}

	wallL.mModel.mMesh = EGeometry::CUBE;
	wallR.mModel.mMesh = EGeometry::CUBE;
	wallF.mModel.mMesh = EGeometry::CUBE;
	ceiling.mModel.mMesh = EGeometry::CUBE;
#endif
	floor.mModel.mMesh = EGeometry::CUBE;
}


template<typename T>
void Track<T>::Update(float dt)
{
	const float prevNormalizedTime = _totalTime - static_cast<int>(_totalTime / _period) * _period;
	const float prev_t = prevNormalizedTime / _period;

	_totalTime += dt;

	const float normalizedTime = _totalTime - static_cast<int>(_totalTime / _period) * _period;
	const float t = normalizedTime / _period;	// [0, 1] for lerping
	//const float prev_t = (*_data - _valueBegin) / (_valueEnd - _valueBegin);
	if (prev_t > t)
	{
		std::swap(_valueBegin, _valueEnd);
	}

	*_data = _valueBegin * (1.0f - t) + _valueEnd * t;


}