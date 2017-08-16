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
constexpr float		CUBE_DISTANCE		= 4.0f * 2.2f;

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

RoomScene::RoomScene(SceneManager& sceneMan)
	:
	m_sceneManager(sceneMan)
{}

void RoomScene::Load(Renderer* pRenderer, SerializedScene& scene)
{
	m_pRenderer = pRenderer;
	m_room.Initialize(pRenderer);
	InitializeLights(scene);
	InitializeObjectArrays();

	m_skybox = Skybox::s_Presets[SKYBOX_PRESETS::NIGHT_SKY];
}

void RoomScene::Update(float dt)
{
	for (auto& anim : m_animations) anim.Update(dt);
	UpdateCentralObj(dt);
}

void ExampleRender(Renderer* pRenderer, const XMMATRIX& viewProj);
void RoomScene::Render(Renderer* pRenderer, const XMMATRIX& viewProj) const
{
	const ShaderID selectedShader = m_sceneManager.GetSelectedShader();
	const bool bSendMaterialData = (
		   selectedShader == SHADERS::FORWARD_PHONG 
		|| selectedShader == SHADERS::UNLIT 
		|| selectedShader == SHADERS::NORMAL
		|| selectedShader == SHADERS::FORWARD_BRDF
		|| selectedShader == SHADERS::DEFERRED_GEOMETRY
	);

	if(selectedShader != SHADERS::DEFERRED_GEOMETRY)
		m_skybox.Render(viewProj);

	pRenderer->SetShader(selectedShader);
	
	if (selectedShader != SHADERS::DEFERRED_GEOMETRY)
		m_sceneManager.SendLightData();
	
	m_room.Render(m_pRenderer, viewProj, bSendMaterialData);
	
	//if (selectedShader == SHADERS::FORWARD_BRDF) m_pRenderer->SetRasterizerState((int)DEFAULT_RS_STATE::CULL_BACK);
	
	RenderCentralObjects(viewProj, bSendMaterialData);
	RenderLights(viewProj);
}


 		  
void RoomScene::InitializeLights(SerializedScene& scene)
{
	m_lights = std::move(scene.lights);
	m_lights[1]._color = m_lights[1]._model.m_material.color = vec3(m_lights[1]._color) * 3.0f;
	m_lights[2]._color = m_lights[2]._model.m_material.color = vec3(m_lights[2]._color) * 2.0f;
	m_lights[3]._color = vec3(m_lights[3]._color) * 1.5f;
	
	// hard-coded scales for now
	m_lights[0]._transform.SetUniformScale(0.8f);
	m_lights[1]._transform.SetUniformScale(0.3f);
	m_lights[2]._transform.SetUniformScale(0.4f);
	m_lights[3]._transform.SetUniformScale(0.5f);

	for (size_t i = 0; i < RAND_LIGHT_COUNT; i++)
	{
		unsigned rndIndex = rand() % Color::Palette().size();
		Color rndColor = Color::Palette()[rndIndex];
		Light l;
		float x = RandF(-20.0f, 20.0f);
		float y = RandF(-15.0f, 15.0f);
		float z = RandF(-10.0f, 20.0f);
		l._transform.SetPosition(x, y, z);
		l._transform.SetUniformScale(0.1f);
		l._model.m_mesh = GEOMETRY::SPHERE;
		l._model.m_material.color = l._color = rndColor;
		l.SetLightRange(static_cast<float>(rand() % 50 + 10));
		m_lights.push_back(l);
	}

	Animation anim;
	anim._fTracks.push_back(Track<float>(&m_lights[1]._brightness, 40.0f, 800.0f, 3.0f));
	m_animations.push_back(anim);
}
	 		  
void RoomScene::InitializeObjectArrays()
{
	{	// grid arrangement ( (row * col) cubes that are 'CUBE_DISTANCE' apart from each other )
		const std::vector<vec3>   rowRotations = { vec3::Zero, vec3::Up, vec3::Right, vec3::Forward };
		static std::vector<float> sCubeDelays = std::vector<float>(CUBE_COUNT, 0.0f);
		const std::vector<Color>  colors = { Color::white, Color::red, Color::green, Color::blue, Color::orange, Color::light_gray, Color::cyan };

		for (int i = 0; i < CUBE_ROW_COUNT; ++i)
		{
			//Color color = c_colors[i % c_colors.size()];
			Color color = vec3(1,1,1) * static_cast<float>(i) / (float)(CUBE_ROW_COUNT-1);

			for (int j = 0; j < CUBE_COLUMN_COUNT; ++j)
			{
				GameObject cube;

				// set transform
				float x, y, z;	// position
				x = i * CUBE_DISTANCE - CUBE_ROW_COUNT * CUBE_DISTANCE / 2;		y = 5.0f;	z = j * CUBE_DISTANCE - CUBE_COLUMN_COUNT * CUBE_DISTANCE / 2;
				cube.m_transform.SetPosition(x, y, z + 550);
				cube.m_transform.SetUniformScale(4.0f);

				// set material
				cube.m_model.m_material.color = color;
				cube.m_model.m_mesh = GEOMETRY::CUBE;
				//cube.m_model.m_material = Material::RandomMaterial();
				//cube.m_model.m_material.normalMap.id = m_pRenderer->AddTexture("bricks_n.png");
				cube.m_model.m_material.normalMap = m_pRenderer->CreateTextureFromFile("simple_normalmap.png");

				cubes.push_back(cube);
			}
		}
	}
	
	// circle arrangement
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
			sph.m_transform = pos;
			sph.m_transform.Translate(vec3::Up * (sphHeight[1] * ((float)i / (float)numSph) + sphHeight[1]));
			sph.m_model.m_mesh = GEOMETRY::SPHERE;
			sph.m_model.m_material = Material::gold;

			// set materials
			const float baseSpecular = 5.0f;
			const float step = 15.0f;
			sph.m_model.m_material.specular = vec3( baseSpecular + (static_cast<float>(i) / numSph) * step)._v;
			//sph.m_model.m_material.specular = i % 2 == 0 ? vec3((static_cast<float>(i) / numSph) * baseSpecular)._v : vec3((static_cast<float>(numSph - i) / numSph) * baseSpecular)._v;
			//sph.m_model.m_material.specular = i < numSph / 2 ? vec3(0.0f).v : vec3(90.0f).v;

			sph.m_model.m_material.roughness = 0.2f;
			sph.m_model.m_material.metalness = 1.0f - static_cast<float>(i) / static_cast<float>(numSph);

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
			const float rowStep = static_cast<float>(row) / ((numSph-1) / gridDimension);
			const float colStep = static_cast<float>(col) / ((numSph-1) / gridDimension);

			// offset to center the grid
			const float offsetDim = -static_cast<float>(gridDimension) * r / 2 + r/2.0f;
			const vec3 offset = vec3(col * r, 0.0f, row * r) + vec3(offsetDim, 0.0f, offsetDim);

			const vec3 pos = origin + offset;

			GameObject sph;
			sph.m_transform = pos;
			sph.m_transform.SetUniformScale(2.5f);
			sph.m_model.m_mesh = GEOMETRY::SPHERE;

			Material& mat = sph.m_model.m_material;

			// col(-x->+x) -> metalness [0.0f, 1.0f]
			mat.color = vec3(Color::red) / 2.0f;
			mat.metalness = colStep;

			// row(-z->+z) -> roughness [roughnessLowClamp, 1.0f]
			const float roughnessLowClamp = 0.025f;
			mat.roughness = rowStep < roughnessLowClamp ? roughnessLowClamp : rowStep;

			spheres.push_back(sph);
		}
	}

	int i = 3;
	const float xCoord = 85.0f;
	const float distToEachOther = -30.0f;
	grid.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	grid.m_transform.SetUniformScale(30);
	grid.m_transform.RotateAroundGlobalZAxisDegrees(45);
	--i;

	cube.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	cube.m_transform.SetUniformScale(5.0f);
	--i;

	cylinder.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	cylinder.m_transform.SetUniformScale(4.0f);
	--i;


	sphere.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	//sphere.m_transform.SetPosition(0, 20.0f, 0);
	//sphere.m_transform.SetUniformScale(5.0f);
	--i;

	triangle.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	triangle.m_transform.SetXRotationDeg(30.0f);
	triangle.m_transform.RotateAroundGlobalYAxisDegrees(30.0f);
	triangle.m_transform.SetUniformScale(4.0f);
	--i;

	quad.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	quad.m_transform.SetXRotationDeg(30);
	quad.m_transform.RotateAroundGlobalYAxisDegrees(30);
	quad.m_transform.SetUniformScale(4.0f);


	    grid.m_model.m_mesh = GEOMETRY::GRID;
	cylinder.m_model.m_mesh = GEOMETRY::CYLINDER;
	triangle.m_model.m_mesh = GEOMETRY::TRIANGLE;
	    quad.m_model.m_mesh = GEOMETRY::QUAD;
		cube.m_model.m_mesh = GEOMETRY::CUBE;
	 sphere.m_model.m_mesh = GEOMETRY::SPHERE;

	    grid.m_model.m_material = Material();
		grid.m_model.m_material.roughness = 0.02f;
		grid.m_model.m_material.metalness = 0.6f;
	cylinder.m_model.m_material = Material();
	cylinder.m_model.m_material.roughness = 0.3f;
	cylinder.m_model.m_material.metalness = 0.3f;
	triangle.m_model.m_material = Material();
	    quad.m_model.m_material = Material();
		cube.m_model.m_material = Material();
		cube.m_model.m_material.roughness = 0.6f;
		cube.m_model.m_material.metalness = 0.6f;
	  sphere.m_model.m_material = Material();
	  sphere.m_model.m_material.roughness = 0.3f;
	  sphere.m_model.m_material.metalness = 1.0f;
	  //sphere.m_model.m_material.color = vec3(0.04f, 61.0f / 255.0f, 19.0f / 255.0f);
	  sphere.m_model.m_material.color = vec3(0.04f, 0.04f, 0.04f);
	  sphere.m_transform.SetUniformScale(2.5f);



	  //cubes.front().m_transform.Translate(0, 30, 0);
	  cube.m_transform.SetPosition(0, 15, 0);
	  cube.m_transform.SetXRotationDeg(90);
	  cube.m_transform.RotateAroundGlobalXAxisDegrees(30);
	  cube.m_transform.RotateAroundGlobalYAxisDegrees(30);
	  cube.m_model.m_material.normalMap = m_pRenderer->CreateTextureFromFile("BumpMapTexturePreview.png");
	  cube.m_model.m_material.metalness = 0.6f;
	  cube.m_model.m_material.roughness = 0.15f;
}
	 		  
	 		  
	 		  
void RoomScene::UpdateCentralObj(const float dt)
{
	float t = ENGINE->GetTotalTime();
	const float moveSpeed = 45.0f;
	const float rotSpeed = XM_PI;

	XMVECTOR rot = XMVectorZero();
	XMVECTOR tr = XMVectorZero();
	if (ENGINE->INP()->IsKeyDown(102)) tr += vec3::Right;	// Key: Numpad6
	if (ENGINE->INP()->IsKeyDown(100)) tr += vec3::Left;	// Key: Numpad4
	if (ENGINE->INP()->IsKeyDown(104)) tr += vec3::Forward;	// Key: Numpad8
	if (ENGINE->INP()->IsKeyDown(98))  tr += vec3::Back; 	// Key: Numpad2
	if (ENGINE->INP()->IsKeyDown(105)) tr += vec3::Up; 		// Key: Numpad9
	if (ENGINE->INP()->IsKeyDown(99))  tr += vec3::Down; 	// Key: Numpad3
	m_lights[0]._transform.Translate(dt * tr * moveSpeed);


	float angle = (dt * XM_PI * 0.08f) + (sinf(t) * sinf(dt * XM_PI * 0.03f));
	size_t sphIndx = 0;
	for (auto& sph : spheres)
	{
		//const vec3 largeSphereRotAxis = (vec3::Down + vec3::Back * 0.3f);
		//const vec3 rotPoint = sphIndx < 12 ? vec3() : vec3(0, 35, 0);
		//const vec3 rotAxis = sphIndx < 12 ? vec3::Up : largeSphereRotAxis.normalized();
		if (sphIndx < 15)	// don't rotate grid spheres
		{
			const vec3 rotAxis = vec3::Up;
			const vec3 rotPoint = vec3::Zero;
			sph.m_transform.RotateAroundPointAndAxis(rotAxis, angle, rotPoint);
		}
		//const vec3 pos = sph.m_transform._position;
		//const float sinx = sinf(pos._v.x / 3.5f);
		//const float y = 10.0f + 2.5f * sinx;
		//sph.m_transform._position = pos;

		++sphIndx;
	}

	// rotate cubes
	const float cubeRotSpeed = 100.0f; // degs/s
	cube.m_transform.RotateAroundGlobalYAxisDegrees(-cubeRotSpeed / 10 * dt);

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
	m_lights[0]._transform.RotateAroundGlobalYAxisDegrees(dt * cubeRotSpeed);
#endif
}




void RoomScene::RenderLights(const XMMATRIX& viewProj) const
{
	m_pRenderer->Reset();	// is reset necessary?
	m_pRenderer->SetShader(SHADERS::UNLIT);
	for (const Light& light : m_lights)
	{
		m_pRenderer->SetBufferObj(light._model.m_mesh);
		const XMMATRIX world = light._transform.WorldTransformationMatrix();
		const XMMATRIX worldViewProj = world  * viewProj;
		const vec3 color = light._model.m_material.color.Value();
		m_pRenderer->SetConstant4x4f("worldViewProj", worldViewProj);
		m_pRenderer->SetConstant3f("diffuse", color);
		m_pRenderer->SetConstant1f("isDiffuseMap", 0.0f);
		m_pRenderer->Apply();
		m_pRenderer->DrawIndexed();
	}
}

void RoomScene::RenderCentralObjects(const XMMATRIX& viewProj, bool bSendMaterialData) const
{
	const ShaderID shd = m_sceneManager.GetSelectedShader();

	for (const auto& cube : cubes) cube.Render(m_pRenderer, viewProj, bSendMaterialData);
	for (const auto& sph : spheres) sph.Render(m_pRenderer, viewProj, bSendMaterialData);

	grid.Render(m_pRenderer, viewProj, bSendMaterialData);
	quad.Render(m_pRenderer, viewProj, bSendMaterialData);
	triangle.Render(m_pRenderer, viewProj, bSendMaterialData);
	cylinder.Render(m_pRenderer, viewProj, bSendMaterialData);
	cube.Render(m_pRenderer, viewProj, bSendMaterialData);
	sphere.Render(m_pRenderer, viewProj, bSendMaterialData);
}

void RoomScene::ToggleFloorNormalMap()
{
	TextureID nMap = m_room.floor.m_model.m_material.normalMap;

	nMap = nMap == m_pRenderer->GetTexture("185_norm.JPG") ? -1 : m_pRenderer->CreateTextureFromFile("185_norm.JPG");
	m_room.floor.m_model.m_material.normalMap = nMap;
}

void RoomScene::RenderAnimated(const XMMATRIX& view, const XMMATRIX& proj) const
{
#ifdef ENABLE_ANIMATION
	// already coming from previous state
	// ----------------------------------
	//	m_renderer->SetShader(m_selectedShader);
	//	m_renderer->SetConstant4x4f("view", view);
	//	m_renderer->SetConstant4x4f("proj", proj);
	//	m_renderer->SetConstant1f("gammaCorrection", m_gammaCorrection ? 1.0f : 0.0f);
	//	m_renderer->SetConstant3f("cameraPos", camPos_real);
	//	SendLightData();
	// ----------------------------------
	const float time = ENGINE->TotalTime();
	const vec3 camPos = m_pCamera->GetPositionF();

	//m_renderer->SetShader(m_selectedShader);
	//m_renderer->SetConstant3f("cameraPos", camPos);
	//SendLightData();
	//m_animationLerp.Render(m_renderer, view, proj, time);

	m_pRenderer->Reset();
	m_pRenderer->SetShader(m_selectedShader);
	m_pRenderer->SetConstant3f("cameraPos", camPos);
	SendLightData();

	m_model.Render(m_pRenderer, view, proj);
#endif
}




void RoomScene::Room::Render(Renderer* pRenderer, const XMMATRIX& viewProj, bool sendMaterialData) const
{
	floor.Render(pRenderer, viewProj, sendMaterialData);
	wallL.Render(pRenderer, viewProj, sendMaterialData);
	wallR.Render(pRenderer, viewProj, sendMaterialData);
	wallF.Render(pRenderer, viewProj, sendMaterialData);
	ceiling.Render(pRenderer, viewProj, sendMaterialData);
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

	// FLOOR
	{
		Transform& tf = floor.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, -wallHieght + YOffset, 0);

		Material& mat = floor.m_model.m_material;
		mat.shininess = 20.0f;	// phong material
		mat.roughness = 0.8f;	// brdf  material
		mat.metalness = 0.0f;

		//mat = Material::bronze;
		mat.diffuseMap = pRenderer->CreateTextureFromFile("185.JPG");
		mat.normalMap = pRenderer->CreateTextureFromFile("185_norm.JPG");
		//mat.normalMap = pRenderer->CreateTextureFromFile("BumpMapTexturePreview.JPG");
	}
#if 1
	// CEILING
	{
		Transform& tf = ceiling.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, wallHieght + YOffset, 0);

		//ceiling.m_model.m_material.color		= Color::purple;
		ceiling.m_model.m_material.shininess = 20.0f;
		//ceiling.m_model.m_material = Material::gold;
		//ceiling.m_model.m_material.diffuseMap = m_pRenderer->CreateTextureFromFile("metal2.JPG");
		//ceiling.m_model.m_material.normalMap = m_pRenderer->CreateTextureFromFile("nrm_metal2.JPG");
	}

	// RIGHT WALL
	{
		Transform& tf = wallR.m_transform;
		tf.SetScale(floorDepth, 0.1f, wallHieght);
		tf.SetPosition(floorWidth, YOffset, 0);
		tf.SetXRotationDeg(90.0f);
		tf.RotateAroundGlobalYAxisDegrees(-90);
		//tf.RotateAroundGlobalXAxisDegrees(-180.0f);

		//wallR.m_model.m_material.color		= Color::gray;
		//wallR.m_model.m_material.shininess	= 120.0f;
		//wallR.m_model.m_material = Material::bronze;
		wallR.m_model.m_material.diffuseMap = pRenderer->CreateTextureFromFile("190.JPG");
		wallR.m_model.m_material.normalMap = pRenderer->CreateTextureFromFile("190_norm.JPG");
	}

	// LEFT WALL
	{
		Transform& tf = wallL.m_transform;
		tf.SetScale(floorDepth, 0.1f, wallHieght);
		tf.SetPosition(-floorWidth, YOffset, 0);
		tf.SetXRotationDeg(-90.0f);
		tf.RotateAroundGlobalYAxisDegrees(-90.0f);
		//tf.SetRotationDeg(90.0f, 0.0f, -90.0f);
		//wallL.m_model.m_material.color		= Color::gray;
		wallL.m_model.m_material.shininess = 60.0f;
		//wallL.m_model.m_material = Material::bronze;
		wallL.m_model.m_material.diffuseMap = pRenderer->CreateTextureFromFile("190.JPG");
		wallL.m_model.m_material.normalMap = pRenderer->CreateTextureFromFile("190_norm.JPG");
	}
	// WALL
	{
		Transform& tf = wallF.m_transform;
		tf.SetScale(floorWidth, 0.1f, wallHieght);
		tf.SetPosition(0, YOffset, floorDepth);
		tf.SetXRotationDeg(-90.0f);
		//wallF.m_model.m_material.color		= Color::gray;
		//wallF.m_model.m_material = Material::gold;
		wallF.m_model.m_material.shininess = 90.0f;
		wallF.m_model.m_material.diffuseMap = pRenderer->CreateTextureFromFile("190.JPG");
		wallF.m_model.m_material.normalMap = pRenderer->CreateTextureFromFile("190_norm.JPG");
	}

	wallL.m_model.m_mesh = GEOMETRY::CUBE;
	wallR.m_model.m_mesh = GEOMETRY::CUBE;
	wallF.m_model.m_mesh = GEOMETRY::CUBE;
	ceiling.m_model.m_mesh = GEOMETRY::CUBE;
#endif
	floor.m_model.m_mesh = GEOMETRY::CUBE;
}




void ExampleRender(Renderer* pRenderer, const XMMATRIX& viewProj)
{
	// todo: show minimal demonstration of renderer
	GameObject obj;										// create object
	obj.m_model.m_mesh = GEOMETRY::SPHERE;				// set material
	obj.m_model.m_material.color = Color::cyan;
	obj.m_model.m_material.alpha = 1.0f;
	obj.m_model.m_material.specular = 90.0f;
	obj.m_model.m_material.diffuseMap = -1;		// empty texture
	obj.m_model.m_material.normalMap  = -1;
	//-------------------------------------------------------------------
	obj.m_transform.SetPosition(0,20,50);				
	//obj.m_transform.SetXRotationDeg(30.0f);
	obj.m_transform.SetUniformScale(5.0f);
	//-------------------------------------------------------------------
	pRenderer->SetShader(SHADERS::FORWARD_BRDF);
	obj.Render(pRenderer, viewProj, true);
	

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


























//======= JUNKYARD

#ifdef ENABLE_ANIMATION
void SceneManager::UpdateAnimatedModel(const float dt)
{
	if (ENGINE->INP()->IsKeyDown(16))	// Key: shift
	{
		if (ENGINE->INP()->IsKeyTriggered(49)) AnimatedModel::DISTANCE_FUNCTION = 0;	// Key:	1 + shift
		if (ENGINE->INP()->IsKeyTriggered(50)) AnimatedModel::DISTANCE_FUNCTION = 1;	// Key:	2 + shift
		if (ENGINE->INP()->IsKeyTriggered(51)) AnimatedModel::DISTANCE_FUNCTION = 2;	// Key:	3 + shift
	}
	else
	{
		if (ENGINE->INP()->IsKeyTriggered(49)) m_model.m_animMode = ANIM_MODE_LOOPING_PATH;								// Key:	1 
		if (ENGINE->INP()->IsKeyTriggered(50)) m_model.m_animMode = ANIM_MODE_LOOPING;									// Key:	2 
		if (ENGINE->INP()->IsKeyTriggered(51)) m_model.m_animMode = ANIM_MODE_IK;										// Key:	3 (default)

																														//if (ENGINE->INP()->IsKeyTriggered(13)) AnimatedModel::ENABLE_VQS = !AnimatedModel::ENABLE_VQS;				// Key: Enter
		if (ENGINE->INP()->IsKeyTriggered(13)) m_model.m_isAnimPlaying = !m_model.m_isAnimPlaying;						// Key: Enter
		if (ENGINE->INP()->IsKeyTriggered(32)) m_model.GoReachTheObject();												// Key: Space
		m_model.m_IKEngine.UpdateTargetPosition(m_anchor1.m_transform.GetPositionF3());
		if (ENGINE->INP()->IsKeyTriggered(107)) m_model.m_animSpeed += 0.1f;											// +
		if (ENGINE->INP()->IsKeyTriggered(109)) m_model.m_animSpeed -= 0.1f;											// -

		if (ENGINE->INP()->IsKeyDown(101))		IKEngine::inp = true;
	}
}
#endif

#ifdef ENABLE_VPHYSICS
void SceneManager::UpdateAnchors(float dt)
{
	// reset bricks velocities
	if (ENGINE->INP()->IsKeyTriggered('R'))
	{
		for (auto& brick : m_springSys.bricks)
		{
			brick->SetLinearVelocity(0.0f, 0.0f, 0.0f);
			brick->SetAngularVelocity(0.0f, 0.0f, 0.0f);
		}
	}
}
#endif


#ifdef ENABLE_VPHYSICS
void SceneManager::InitializePhysicsObjects()
{
	// ANCHOR POINTS
	const float anchorScale = 0.3f;

	m_anchor1.m_model.m_mesh = GEOMETRY::SPHERE;
	m_anchor1.m_model.m_material.color = Color::white;
	m_anchor1.m_model.m_material.shininess = 90.0f;
	m_anchor1.m_model.m_material.normalMap._id = m_pRenderer->CreateTextureFromFile("bricks_n.png");
	m_anchor1.m_model.m_material.normalMap._name = "bricks_n.png";	// todo: rethink this
	m_anchor1.m_transform.SetRotationDeg(15.0f, .0f, .0f);
	m_anchor1.m_transform.SetPosition(-20.0f, 25.0f, 2.0f);
	m_anchor1.m_transform.SetUniformScale(anchorScale);
	//m_anchor2 = m_anchor1;
	//m_anchor2.m_model.m_material.color = Color::orange;
	//m_anchor2.m_transform.SetPosition(-10.0f, 4.0f, +4.0f);

	m_anchor2.m_model.m_mesh = GEOMETRY::SPHERE;
	m_anchor2.m_model.m_material.color = Color::white;
	m_anchor2.m_model.m_material.shininess = 90.0f;
	//m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
	m_anchor2.m_model.m_material.normalMap._id = m_pRenderer->CreateTextureFromFile("bricks_n.png");
	m_anchor2.m_model.m_material.normalMap._name = "bricks_n.png";	// todo: rethink this
	m_anchor2.m_transform.SetRotationDeg(15.0f, .0f, .0f);
	m_anchor2.m_transform.SetPosition(20.0f, 25.0f, 2.0f);
	m_anchor2.m_transform.SetUniformScale(anchorScale);

	// PHYSICS OBJECT REFERENCE
	auto& mat = m_physObj.m_model.m_material;
	auto& msh = m_physObj.m_model.m_mesh;
	auto& tfm = m_physObj.m_transform;
	msh = GEOMETRY::CUBE;
	mat.color = Color::white;
	//mat.normalMap.id	= m_renderer->AddTexture("bricks_n.png");
	mat.diffuseMap._id = m_pRenderer->CreateTextureFromFile("bricks_d.png");
	mat.normalMap._name = "bricks_n.png";	// todo: rethink this
	mat.shininess = 65.0f;
	tfm.SetPosition(0.0f, 10.0f, 0.0f);
	tfm.SetRotationDeg(15.0f, 0.0f, 0.0f);
	tfm.SetScale(1.5f, 0.5f, 0.5f);
	m_physObj.m_rb.InitPhysicsVertices(GEOMETRY::CUBE, m_physObj.m_transform.GetScaleF3());
	m_physObj.m_rb.SetMassUniform(1.0f);
	m_physObj.m_rb.EnablePhysics = m_physObj.m_rb.EnableGravity = true;

	// instantiate bricks
	const size_t numBricks = 6;
	const float height = 10.0f;
	const float brickOffset = 5.0f;
	for (size_t i = 0; i < numBricks; i++)
	{
		m_physObj.m_transform.SetPosition(i * brickOffset - (numBricks / 2) * brickOffset, height, 0.0f);
		m_vPhysObj.push_back(m_physObj);
	}

	std::vector<RigidBody*> rbs;
	for (auto& obj : m_vPhysObj)	rbs.push_back(&obj.m_rb);

	m_springSys.SetBricks(rbs);
}
#endif

#ifdef ENABLE_VPHYSICS
// ANCHOR MOVEMENT
if (!ENGINE->INP()->IsKeyDown(17))	m_anchor1.m_transform.Translate(tr * dt * moveSpeed); // Key: Ctrl
else								m_anchor2.m_transform.Translate(tr * dt * moveSpeed);
//m_anchor1.m_transform.Rotate(global_U * dt * rotSpeed);

// ANCHOR TOGGLE
if (ENGINE->INP()->IsKeyTriggered(101)) // Key: Numpad5
{
	if (ENGINE->INP()->IsKeyDown(17))	m_springSys.ToggleAnchor(0);// ctrl
	else								m_springSys.ToggleAnchor(1);
}

m_springSys.Update();
#endif

#ifdef ENABLE_VPHYSICS
#include "PhysicsEngine.h"
#endif

#ifdef ENABLE_ANIMATION
#include "../Animation/Include/PathManager.h"
#endif


#ifdef ENABLE_VPHYSICS
InitializePhysicsObjects();
#endif

#if defined( ENABLE_ANIMATION ) && defined( LOAD_ANIMS )
m_pPathManager->Init();
// load animated model, set IK effector bones
// 	- from left hand to spine:
//		47 - 40 - 32 - 21 - 15 - 8
std::vector<std::string>	anims = { "Data/Animation/sylvanas_run.fbx", "Data/Animation/sylvanas_idle.fbx" };
std::vector<size_t>			effectorBones = { 60, 47 + 1, 40 + 1, 32 + 1, 21 + 1, 15 + 1, 8 + 1, 4 + 1 };
m_model.Load(anims, effectorBones, IKEngine::SylvanasConstraints());
m_model.SetScale(0.1f);	// scale actor down

						// set animation properties
m_model.SetQuaternionSlerp(false);
m_model.m_animMode = ANIM_MODE_IK;	// looping / path_looping / IK

m_model.SetPath(m_pPathManager->m_paths[0]);
m_model.m_pathLapTime = 12.8f;

AnimatedModel::renderer = renderer;
#endif
#ifdef ENABLE_ANIMATION
m_model.Update(dt);
UpdateAnimatedModel(dt);
#endif
#ifdef ENABLE_VPHYSICS
UpdateAnchors(dt);
#endif
#ifdef ENABLE_VPHYSICS
// draw anchor 1 sphere
m_pRenderer->SetBufferObj(m_anchor1.m_model.m_mesh);
if (m_selectedShader == m_renderData->phongShader || m_selectedShader == m_renderData->normalShader)
m_anchor1.m_model.m_material.SetMaterialConstants(m_pRenderer, TODO);
XMMATRIX world = m_anchor1.m_transform.WorldTransformationMatrix();
XMMATRIX nrmMatrix = m_anchor1.m_transform.NormalMatrix(world);
m_pRenderer->SetConstant4x4f("world", world);
m_pRenderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
m_pRenderer->Apply();
m_pRenderer->DrawIndexed();

// draw anchor 2 sphere
m_pRenderer->SetBufferObj(m_anchor2.m_model.m_mesh);
if (m_selectedShader == m_renderData->phongShader || m_selectedShader == m_renderData->normalShader)
m_anchor2.m_model.m_material.SetMaterialConstants(m_pRenderer, TODO);
world = m_anchor2.m_transform.WorldTransformationMatrix();
nrmMatrix = m_anchor2.m_transform.NormalMatrix(world);
m_pRenderer->SetConstant4x4f("world", world);
m_pRenderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
m_pRenderer->Apply();
m_pRenderer->DrawIndexed();

// DRAW GRAVITY TEST OBJ
//----------------------
// materials
m_pRenderer->SetBufferObj(m_physObj.m_model.m_mesh);
m_physObj.m_model.m_material.SetMaterialConstants(m_pRenderer, TODO);

// render bricks
for (const auto& pObj : m_vPhysObj)
{
	world = pObj.m_transform.WorldTransformationMatrix();
	nrmMatrix = pObj.m_transform.NormalMatrix(world);
	m_pRenderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
	m_pRenderer->SetConstant4x4f("world", world);
	m_pRenderer->Apply();
	m_pRenderer->DrawIndexed();
}

m_springSys.RenderSprings(m_pRenderer, view, proj);
#endif


//-------------------------------------------------------------------------------- ANIMATE LIGHTS ----------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------------------------------------
//static double accumulator = 0;
//accumulator += dt;
//if (RAND_LIGHT_COUNT > 0 && accumulator > DISCO_PERIOD)
//{
//	//// shuffling won't rearrange data, just the means of indexing.
//	//char info[256];
//	//sprintf_s(info, "Shuffle(L1:(%f, %f, %f)\tL2:(%f, %f, %f)\n",
//	//	m_lights[0]._transform.GetPositionF3().x, m_lights[0]._transform.GetPositionF3().y,	m_lights[0]._transform.GetPositionF3().z,
//	//	m_lights[1]._transform.GetPositionF3().x, m_lights[1]._transform.GetPositionF3().y, m_lights[1]._transform.GetPositionF3().z);
//	//OutputDebugString(info);
//	//static auto engine = std::default_random_engine{};
//	//std::shuffle(std::begin(m_lights), std::end(m_lights), engine);

//	//// randomize all lights
//	//for (Light& l : m_lights)
//	//{
//	//	size_t i = rand() % Color::Palette().size();
//	//	Color c = Color::Color::Palette()[i];
//	//	l.color_ = c;
//	//	l._model.m_material.color = c;
//	//}

//	// randomize all lights except 1 and 2
//	for (unsigned j = 3; j<m_lights.size(); ++j)
//	{
//		Light& l = m_lights[j];
//		size_t i = rand() % Color::Palette().size();
//		Color c = Color::Color::Palette()[i];
//		l.color_ = c;
//		l._model.m_material.color = c;
//	}

//	accumulator = 0;
//}

#ifdef ENABLE_VPHYSICS
:
m_springSys(m_anchor1, m_anchor2, &m_anchor2)	// test
#endif