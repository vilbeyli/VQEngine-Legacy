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

void RoomScene::Load(Renderer* pRenderer)
{
	m_pRenderer = pRenderer;
	InitializeRoom();
	InitializeLights();
	InitializeObjectArrays();

	m_skybox = Skybox::s_Presets[SKYBOX_PRESETS::NIGHT_SKY];
	//m_skydome.Initialize(m_pRenderer, "skydomeTex.png", 1000.0f / 2.2f, SHADERS::UNLIT);
}

void RoomScene::Update(float dt)
{
	UpdateCentralObj(dt);
}

void RoomScene::Render(Renderer* pRenderer, const XMMATRIX& viewProj) const
{
	const ShaderID selectedShader = m_sceneManager.GetSelectedShader();
	const bool SendMaterialData = selectedShader == SHADERS::FORWARD_PHONG || selectedShader == SHADERS::UNLIT || selectedShader == SHADERS::NORMAL;

	m_skybox.Render(viewProj);

	pRenderer->SetShader(selectedShader);
	m_room.Render(m_pRenderer, viewProj, SendMaterialData);
	if (selectedShader == SHADERS::NORMAL) m_pRenderer->SetRasterizerState((int)DEFAULT_RS_STATE::CULL_BACK);
	RenderCentralObjects(viewProj);
	RenderLights(viewProj);
}

void RoomScene::InitializeRoom()
{
	const float floorWidth = 5 * 30.0f;
	const float floorDepth = 5 * 30.0f;
	const float wallHieght = 3.8*15.0f;	// amount from middle to top and bottom: because gpu cube is 2 units in length
	const float YOffset = wallHieght - 9.0f;

	// FLOOR
	{
		Transform& tf = m_room.floor.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, -wallHieght + YOffset, 0);

		//m_room.floor.m_model.m_material.shininess	= 40.0f;
		m_room.floor.m_model.m_material = Material::bronze;
		m_room.floor.m_model.m_material.diffuseMap = m_pRenderer->TextureFromFile("metal3.png");
		m_room.floor.m_model.m_material.normalMap = m_pRenderer->TextureFromFile("nrm_metal3.png");
	}
	// CEILING
	{
		Transform& tf = m_room.ceiling.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, wallHieght + YOffset, 0);

		//m_room.ceiling.m_model.m_material.color		= Color::purple;
		//m_room.ceiling.m_model.m_material.shininess	= 10.0f;
		m_room.ceiling.m_model.m_material = Material::gold;
		m_room.ceiling.m_model.m_material.diffuseMap = m_pRenderer->TextureFromFile("metal2.png");
		m_room.ceiling.m_model.m_material.normalMap = m_pRenderer->TextureFromFile("nrm_metal2.png");
	}

	// RIGHT WALL
	{
		Transform& tf = m_room.wallR.m_transform;
		tf.SetScale(floorDepth, 0.1f, wallHieght);
		tf.SetPosition(floorWidth, YOffset, 0);
		tf.SetXRotationDeg(90.0f);
		tf.RotateAroundGlobalYAxisDegrees(-90);
		//tf.RotateAroundGlobalXAxisDegrees(-180.0f);

		//m_room.wallR.m_model.m_material.color		= Color::gray;
		//m_room.wallR.m_model.m_material.shininess	= 120.0f;
		m_room.wallR.m_model.m_material = Material::bronze;

		m_room.wallR.m_model.m_material.diffuseMap = m_pRenderer->TextureFromFile("metal2.png");
		m_room.wallR.m_model.m_material.normalMap = m_pRenderer->TextureFromFile("nrm_metal2.png");
	}

	// LEFT WALL
	{
		Transform& tf = m_room.wallL.m_transform;
		tf.SetScale(floorDepth, 0.1f, wallHieght);
		tf.SetPosition(-floorWidth, YOffset, 0);
		tf.SetXRotationDeg(-90.0f);
		tf.RotateAroundGlobalYAxisDegrees(-90.0f);
		//tf.SetRotationDeg(90.0f, 0.0f, -90.0f);
		//m_room.wallL.m_model.m_material.color		= Color::gray;
		//m_room.wallL.m_model.m_material.shininess	= 60.0f;
		m_room.wallL.m_model.m_material = Material::bronze;
		m_room.wallL.m_model.m_material.diffuseMap = m_pRenderer->TextureFromFile("metal2.png");;
		m_room.wallL.m_model.m_material.normalMap = m_pRenderer->TextureFromFile("nrm_metal2.png");;
	}
	// WALL
	{
		Transform& tf = m_room.wallF.m_transform;
		tf.SetScale(floorWidth, 0.1f, wallHieght);
		tf.SetPosition(0, YOffset, floorDepth);
		tf.SetXRotationDeg(-90.0f);
		//m_room.wallF.m_model.m_material.color		= Color::gray;
		m_room.wallF.m_model.m_material = Material::gold;
		//m_room.wallF.m_model.m_material.shininess	= 90.0f;
		m_room.wallF.m_model.m_material.diffuseMap = m_pRenderer->TextureFromFile("metal2.png");
		m_room.wallF.m_model.m_material.normalMap = m_pRenderer->TextureFromFile("nrm_metal2.png");
	}

	m_room.floor.m_model.m_mesh   = MESH_TYPE::CUBE;
	m_room.wallL.m_model.m_mesh   = MESH_TYPE::CUBE;
	m_room.wallR.m_model.m_mesh   = MESH_TYPE::CUBE;
	m_room.wallF.m_model.m_mesh   = MESH_TYPE::CUBE;
	m_room.ceiling.m_model.m_mesh = MESH_TYPE::CUBE;
}
	 		  
void RoomScene::InitializeLights()
{
	// todo: read from file
	// spot lights
	{
		Light l;
		l._type = Light::LightType::SPOT;
		l._transform.SetPosition(0.0f, 3.6f*25.0f, 0.0f);
		l._transform.RotateAroundGlobalXAxisDegrees(200.0f);
		l._transform.SetUniformScale(0.8f);
		l._model.m_mesh = MESH_TYPE::CYLINDER;
		l._model.m_material.color = Color::white;
		l._color = Color::white;
		//l._range = 200.f;
		l._spotAngle = 70.0f;
		l._castsShadow = true;
		m_lights.push_back(l);
	}

#ifdef ENABLE_POINT_LIGHTS
	// point lights
	{
		Light l;
		l._transform.SetPosition(-8.0f, 22.0f, 0);
		l._transform.SetUniformScale(0.3f);
		l._model.m_mesh = MESH_TYPE::SPHERE;
		l._model.m_material.color = l._color = Color::blue;
		l.SetLightRange(180);
		m_lights.push_back(l);
	}
	{
		Light l;
		l._transform.SetPosition(28.0f, 15.0f, -17.0f);
		l._transform.SetUniformScale(0.4f);
		l._model.m_mesh = MESH_TYPE::SPHERE;
		l._model.m_material.color = Color::orange;
		l._color = l._model.m_material.color;
		l.SetLightRange(250);
		m_lights.push_back(l);
	}
	{
		Light l;
		l._transform.SetPosition(-140.0f, 100.0f, 140.0f);
		l._transform.SetUniformScale(0.5f);
		l._model.m_mesh = MESH_TYPE::SPHERE;
		l._model.m_material.color = l._color = Color::red;
		l.SetLightRange(40);
		m_lights.push_back(l);
	}

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
		l._model.m_mesh = MESH_TYPE::SPHERE;
		l._model.m_material.color = l._color = rndColor;
		l.SetLightRange(static_cast<float>(rand() % 50 + 10));
		m_lights.push_back(l);
	}
#endif
}
	 		  
void RoomScene::InitializeObjectArrays()
{
	{	// grid arrangement ( (row * col) cubes that are 'r' apart from each other )
		const int	row = 6,
			col = 6;
		const float r = 4.0f * 6;

		const std::vector<vec3> c_rowRotations = { vec3::Zero, vec3::Up, vec3::Right, vec3::Forward };
		static std::vector<float> s_cubeDelays = std::vector<float>(row*col, 0.0f);
		const std::vector<Color> c_colors = { Color::white, Color::red, Color::green, Color::blue, Color::orange, Color::light_gray, Color::cyan };
		for (int i = 0; i < row; ++i)
		{
			Color color = c_colors[i % c_colors.size()];

			for (int j = 0; j < col; ++j)
			{
				GameObject cube;

				// set transform
				float x, y, z;	// position
				x = i * r - row * r / 2;		y = 5.0f;	z = j * r - col * r / 2;
				cube.m_transform.SetPosition(x, y, z);
				if (RandF(0, 1) < 0.5f)	cube.m_transform.SetScale(1, 6, 3);
				else					cube.m_transform.SetUniformScale(RandF(3.0f, 5.0f));
				if (j == 3)	cube.m_transform.SetUniformScale(RandF(6.0f, 9.0f));
				cube.m_model.m_material.color = color;
				//if (i == j) cube.m_transform.SetRotationDeg(90.0f / 4 * sqrt(i*j), 0, 0);
				//if (i == j) cube.m_transform.RotateAroundAxisDegrees(c_rowRotations[1], 90.0f);
				if (j == 0 && col != 1) cube.m_transform.RotateAroundAxisDegrees(vec3::XAxis, (static_cast<float>(i) / (col - 1)) * 90.0f);
				if (j == 1 && col != 1) cube.m_transform.RotateAroundAxisDegrees(vec3::YAxis, (static_cast<float>(i) / (col - 1)) * 90.0f);

				if (j == 3 && col != 1) cube.m_transform.RotateAroundAxisDegrees(vec3::ZAxis, (static_cast<float>(i) / (col - 1)) * 90.0f);

				cube.m_transform.RotateAroundAxisDegrees(vec3::YAxis, -90.0f);



				// set material
				cube.m_model.m_mesh = MESH_TYPE::CUBE;
				//cube.m_model.m_material = Material::RandomMaterial();
				//cube.m_model.m_material.normalMap.id = m_pRenderer->AddTexture("bricks_n.png");
				cube.m_model.m_material.normalMap = m_pRenderer->TextureFromFile("simple_normalmap.png");

				cubes.push_back(cube);
			}
		}
	}
	cubes.front().m_transform.Translate(0, 10, 0);
	cubes.front().m_transform.SetXRotationDeg(90);
	cubes.front().m_transform.RotateAroundGlobalYAxisDegrees(-90);

	// circle arrangement
	const float sphHeight[2] = { 60.0f, 45.0f };
	{	// large circle
		const float r = 30.0f;
		const size_t numSph = 15;

		const vec3 radius(r, sphHeight[0], 0.0f);
		const float rot = 2.0f * XM_PI / numSph == 0 ? 1.0f : numSph;
		const vec3 axis = vec3::Up;
		for (size_t i = 0; i < numSph; i++)
		{
			// calc position
			Quaternion rotQ = Quaternion::FromAxisAngle(axis, rot * i);
			vec3 pos = rotQ.TransformVector(radius);

			GameObject sph;
			sph.m_transform = pos;
			sph.m_model.m_mesh = MESH_TYPE::SPHERE;
			sph.m_model.m_material.specular = i % 2 == 0 ? vec3((static_cast<float>(i) / numSph) * 50.0f)._v : vec3((static_cast<float>(numSph - i) / numSph) * 50.0f)._v;
			//sph.m_model.m_material.specular = i < numSph / 2 ? vec3(0.0f).v : vec3(90.0f).v;

			spheres.push_back(sph);
		}
	}
	{	// small circle
		const float r = 15.0f;
		const size_t numSph = 5;

		const float rot = 2.0f * XM_PI / numSph == 0 ? 1.0f : numSph;
		const vec3 radius(r, sphHeight[1], 0.0f);
		const auto axis = vec3::Up;
		for (size_t i = 0; i < numSph; i++)
		{
			// calc position
			Quaternion rotQ = Quaternion::FromAxisAngle(axis, rot * i);
			vec3 pos = rotQ.TransformVector(radius);

			GameObject sph;
			sph.m_transform = pos;
			sph.m_model.m_mesh = MESH_TYPE::SPHERE;
			sph.m_model.m_material = Material::RandomMaterial();

			spheres.push_back(sph);
		}
	}

	int i = 3;
	const float xCoord = 85.0f;
	const float distToEachOther = -30.0f;
	grid.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	grid.m_transform.SetUniformScale(30);
	--i;

	cylinder.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	cylinder.m_transform.SetUniformScale(4.0f);

	--i;
	triangle.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	triangle.m_transform.SetXRotationDeg(30.0f);
	triangle.m_transform.SetUniformScale(4.0f);

	--i;
	quad.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);
	quad.m_transform.SetXRotationDeg(30);
	quad.m_transform.SetUniformScale(4.0f);

	--i;
	cube.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);

	--i;
	sphere.m_transform.SetPosition(xCoord, 5.0f, distToEachOther * i);

	    grid.m_model.m_mesh = MESH_TYPE::GRID;
	cylinder.m_model.m_mesh = MESH_TYPE::CYLINDER;
	triangle.m_model.m_mesh = MESH_TYPE::TRIANGLE;
	    quad.m_model.m_mesh = MESH_TYPE::QUAD;
		cube.m_model.m_mesh = MESH_TYPE::CUBE;
	 sphere.m_model.m_mesh = MESH_TYPE::SPHERE;

	    grid.m_model.m_material = Material();
	cylinder.m_model.m_material = Material();
	triangle.m_model.m_material = Material();
	    quad.m_model.m_material = Material();
		cube.m_model.m_material = Material();
	  sphere.m_model.m_material = Material();
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
		const vec3 rotAxis = vec3::Up;
		const vec3 rotPoint = vec3::Zero;
		sph.m_transform.RotateAroundPointAndAxis(rotAxis, angle, rotPoint);
		const vec3 pos = sph.m_transform._position;
		const float sinx = sinf(pos._v.x / 3.5f);
		const float y = 10.0f + 2.5f * sinx;

		sph.m_transform._position = pos;
		++sphIndx;
	}

	// rotate cubes
	const int	row = 6,
		col = 6;
	const float cubeRotSpeed = 100.0f; // degs/s
	for (int i = 0; i < row; ++i)
	{
		for (int j = 0; j < col; ++j)
		{
			if (i == 0 && j == 0) continue;
			//if (j > 4)
			{	// exclude diagonal

				cubes[i*row + j].m_transform.RotateAroundAxisDegrees(vec3::XAxis, dt * cubeRotSpeed);
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

void RoomScene::RenderCentralObjects(const XMMATRIX& viewProj) const
{
	const ShaderID shd = m_sceneManager.GetSelectedShader();
	const bool sendMaterialData = shd == SHADERS::FORWARD_PHONG || shd == SHADERS::UNLIT || shd == SHADERS::NORMAL;

	for (const auto& cube : cubes) cube.Render(m_pRenderer, viewProj, sendMaterialData);
	for (const auto& sph : spheres) sph.Render(m_pRenderer, viewProj, sendMaterialData);

	grid.Render(m_pRenderer, viewProj, sendMaterialData);
	quad.Render(m_pRenderer, viewProj, sendMaterialData);
	triangle.Render(m_pRenderer, viewProj, sendMaterialData);
	cylinder.Render(m_pRenderer, viewProj, sendMaterialData);
	cube.Render(m_pRenderer, viewProj, sendMaterialData);
	sphere.Render(m_pRenderer, viewProj, sendMaterialData);
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

	m_anchor1.m_model.m_mesh = MESH_TYPE::SPHERE;
	m_anchor1.m_model.m_material.color = Color::white;
	m_anchor1.m_model.m_material.shininess = 90.0f;
	m_anchor1.m_model.m_material.normalMap._id = m_pRenderer->TextureFromFile("bricks_n.png");
	m_anchor1.m_model.m_material.normalMap._name = "bricks_n.png";	// todo: rethink this
	m_anchor1.m_transform.SetRotationDeg(15.0f, .0f, .0f);
	m_anchor1.m_transform.SetPosition(-20.0f, 25.0f, 2.0f);
	m_anchor1.m_transform.SetUniformScale(anchorScale);
	//m_anchor2 = m_anchor1;
	//m_anchor2.m_model.m_material.color = Color::orange;
	//m_anchor2.m_transform.SetPosition(-10.0f, 4.0f, +4.0f);

	m_anchor2.m_model.m_mesh = MESH_TYPE::SPHERE;
	m_anchor2.m_model.m_material.color = Color::white;
	m_anchor2.m_model.m_material.shininess = 90.0f;
	//m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
	m_anchor2.m_model.m_material.normalMap._id = m_pRenderer->TextureFromFile("bricks_n.png");
	m_anchor2.m_model.m_material.normalMap._name = "bricks_n.png";	// todo: rethink this
	m_anchor2.m_transform.SetRotationDeg(15.0f, .0f, .0f);
	m_anchor2.m_transform.SetPosition(20.0f, 25.0f, 2.0f);
	m_anchor2.m_transform.SetUniformScale(anchorScale);

	// PHYSICS OBJECT REFERENCE
	auto& mat = m_physObj.m_model.m_material;
	auto& msh = m_physObj.m_model.m_mesh;
	auto& tfm = m_physObj.m_transform;
	msh = MESH_TYPE::CUBE;
	mat.color = Color::white;
	//mat.normalMap.id	= m_renderer->AddTexture("bricks_n.png");
	mat.diffuseMap._id = m_pRenderer->TextureFromFile("bricks_d.png");
	mat.normalMap._name = "bricks_n.png";	// todo: rethink this
	mat.shininess = 65.0f;
	tfm.SetPosition(0.0f, 10.0f, 0.0f);
	tfm.SetRotationDeg(15.0f, 0.0f, 0.0f);
	tfm.SetScale(1.5f, 0.5f, 0.5f);
	m_physObj.m_rb.InitPhysicsVertices(MESH_TYPE::CUBE, m_physObj.m_transform.GetScaleF3());
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
m_anchor1.m_model.m_material.SetMaterialConstants(m_pRenderer);
XMMATRIX world = m_anchor1.m_transform.WorldTransformationMatrix();
XMMATRIX nrmMatrix = m_anchor1.m_transform.NormalMatrix(world);
m_pRenderer->SetConstant4x4f("world", world);
m_pRenderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
m_pRenderer->Apply();
m_pRenderer->DrawIndexed();

// draw anchor 2 sphere
m_pRenderer->SetBufferObj(m_anchor2.m_model.m_mesh);
if (m_selectedShader == m_renderData->phongShader || m_selectedShader == m_renderData->normalShader)
m_anchor2.m_model.m_material.SetMaterialConstants(m_pRenderer);
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
m_physObj.m_model.m_material.SetMaterialConstants(m_pRenderer);

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