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

#include "SceneManager.h"	
#include "Engine.h"			
#include "Input.h"
#include "Renderer.h"
#include "Camera.h"
#include "utils.h"

#ifdef ENABLE_VPHYSICS
#include "PhysicsEngine.h"
#endif

#ifdef ENABLE_ANIMATION
#include "../Animation/Include/PathManager.h"
#endif

//#include <algorithm>
//#include <random>

#define KEY_TRIG(k) ENGINE->INP()->IsKeyTriggered(k)

#define MAX_LIGHTS 20
#define MAX_SPOTS 10
#define RAND_LIGHT_COUNT 0
#define DISCO_PERIOD 0.25

#define LOAD_ANIMS
//#undef LOAD_ANIMS	// disable

int TBNMode = 0;

SceneManager::SceneManager()
#ifdef ENABLE_VPHYSICS
	:
	m_springSys(m_anchor1, m_anchor2, &m_anchor2)
#endif
{}

SceneManager::~SceneManager()
{}

void SceneManager::Initialize(Renderer * renderer, const RenderData* rData, Camera* cam, PathManager* pathMan)
{
	m_pathMan			= pathMan;
	m_renderer			= renderer;
	m_renderData		= rData;
	m_selectedShader	= m_renderData->phongShader;
	m_gammaCorrection	= true;
	m_camera			= cam;
	InitializeBuilding();
	InitializeLights();
	
	//m_centralObj.m_model.m_mesh = MESH_TYPE::GRID;
	//m_centralObj.m_model.m_material.color = Color::blue;
	//m_centralObj.m_model.m_material.shininess = 40.0f;

#ifdef ENABLE_VPHYSICS
	InitializePhysicsObjects();
#endif

	// set skydome
	m_skydome.Init(m_renderer, "browncloud_lf.jpg", FAR_PLANE/2.2f, m_renderData->unlitShader);


#if defined( ENABLE_ANIMATION ) && defined( LOAD_ANIMS )
	m_pathMan->Init();
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

	m_model.SetPath(m_pathMan->m_paths[0]);
	m_model.m_pathLapTime = 12.8f;

	AnimatedModel::renderer = renderer;
#endif
}

enum WALLS
{
	FLOOR = 0,
	LEFT,
	RIGHT,
	FRONT,
	BACK,
	CEILING
};

void SceneManager::InitializeBuilding()
{
	const float floorWidth = 5*30.0f;
	const float floorDepth = 5*30.0f;
	const float wallHieght = 3.5*15.0f;	// amount from middle to top and bottom: because gpu cube is 2 units in length
	const float YOffset = wallHieght - 0.2f;

	//const size_t width = 1;
	//const size_t depth = 1;
	//const float scale = 10.0f;
	//std::vector<GameObject> walls[6];
	//for (size_t i = 0; i < width; i++)
	//{
	//	for (size_t j = 0; j < width; j++)
	//	{


	//	}
	//}
	


	// FLOOR
	{
		Transform& tf = m_building.floor.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, -wallHieght + YOffset, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
		//m_building.floor.m_model.m_material.color		= Color::green;
		//m_building.floor.m_model.m_material.shininess	= 40.0f;
		m_building.floor.m_model.m_material = Material::gold;
		//m_building.floor.m_model.m_material.diffuseMap = m_renderer->GetTexture(m_renderData.exampleTex);
		//m_building.floor.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
		//m_building.floor.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this
	}
	// CEILING
	{
		Transform& tf = m_building.ceiling.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, wallHieght + YOffset, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
		//m_building.ceiling.m_model.m_material.color		= Color::purple;
		//m_building.ceiling.m_model.m_material.shininess	= 10.0f;
		m_building.ceiling.m_model.m_material = Material::gold;
		//m_building.ceiling.m_model.m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
		//m_building.ceiling.m_model.m_material.diffuseMap.name = "bricks_n.png";	// todo: rethink this
		m_building.ceiling.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
		m_building.ceiling.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this
	}

	// RIGHT WALL
	{
		Transform& tf = m_building.wallR.m_transform;
		tf.SetScale(floorDepth, 0.1f, wallHieght);
		tf.SetPosition(floorWidth, YOffset, 0);
		tf.SetRotationDeg(90.0f, 0.0f, 90.0f);
		//m_building.wallR.m_model.m_material.color		= Color::gray;
		//m_building.wallR.m_model.m_material.shininess	= 120.0f;
		m_building.wallR.m_model.m_material = Material::bronze;
		m_building.wallR.m_model.m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
		m_building.wallR.m_model.m_material.normalMap.id  = m_renderer->AddTexture("bricks_n.png");
		m_building.wallR.m_model.m_material.normalMap.name  = "bricks_n.png";	// todo: rethink this
		m_building.wallR.m_model.m_material.diffuseMap.name = "bricks_d.png";	// todo: rethink this
	}

	// LEFT WALL
	{
		Transform& tf = m_building.wallL.m_transform;
		tf.SetScale(floorDepth, 0.1f, wallHieght);
		tf.SetPosition(-floorWidth, YOffset, 0);
		tf.SetRotationDeg(90.0f, 0.0f, -90.0f);
		//m_building.wallL.m_model.m_material.color		= Color::gray;
		//m_building.wallL.m_model.m_material.shininess	= 60.0f;
		m_building.wallL.m_model.m_material = Material::bronze;
		m_building.wallL.m_model.m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
		m_building.wallL.m_model.m_material.normalMap.id  = m_renderer->AddTexture("bricks_n.png");
		m_building.wallL.m_model.m_material.normalMap.name  = "bricks_n.png";	// todo: rethink this
		m_building.wallL.m_model.m_material.diffuseMap.name = "bricks_d.png";	// todo: rethink this
	}
	// WALL
	{
		Transform& tf = m_building.wallF.m_transform;
		tf.SetScale(floorWidth, 0.1f, wallHieght);
		tf.SetPosition(0, YOffset, floorDepth);
		tf.SetRotationDeg(-90.0f, 0.0f, 0.0f);
		//m_building.wallF.m_model.m_material.color		= Color::gray;
		//m_building.wallF.m_model.m_material.shininess	= 90.0f;
		m_building.wallF.m_model.m_material = Material::gold;
		m_building.wallF.m_model.m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
		m_building.wallF.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
		m_building.wallF.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this
		m_building.wallF.m_model.m_material.diffuseMap.name = "bricks_d.png";	// todo: rethink this
	}
}

void SceneManager::InitializeLights()
{
	// spot lights
	{
		Light l;
		l.lightType_ = Light::LightType::SPOT;
		l.tf.SetPosition(0.0f, 2.6f*13.0f, 0.0f);
		l.tf.RotateEulerRad(XMVectorSet(180.0f*DEG2RAD, 0.0f, 0.0f, 0.0f));
		l.tf.SetScaleUniform(0.3f);
		l.model.m_mesh = MESH_TYPE::CYLINDER;
		l.model.m_material.color = Color::white;
		l.color_ = Color::white;
		//l.SetLightRange(30);
		l.spotAngle_ = 70.0f;
		m_lights.push_back(l);
	}

	// point lights
	{
		Light l;
		l.tf.SetPosition(-8.0f, 10.0f, 0);
		l.tf.SetScaleUniform(0.1f);
		l.model.m_mesh = MESH_TYPE::SPHERE;
		l.model.m_material.color = l.color_ = Color::red;
		l.SetLightRange(50);
		m_lights.push_back(l);
	}
	{
		Light l;
		l.tf.SetPosition(8.0f, 30.0f, -17.0f);
		l.tf.SetScaleUniform(0.1f);
		l.model.m_mesh = MESH_TYPE::SPHERE;
		l.model.m_material.color = Color::yellow;
		l.color_ = Color::yellow;
		l.SetLightRange(60);
		m_lights.push_back(l);
	}
	{
		Light l;
		l.tf.SetPosition(-140.0f, 100.0f, 140.0f);
		l.tf.SetScaleUniform(0.5f);
		l.model.m_mesh				= MESH_TYPE::SPHERE;
		l.model.m_material.color	= Color::white;
		l.color_					= Color::white;
		l.SetLightRange(160);
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
		l.tf.SetPosition(x, y, z);
		l.tf.SetScaleUniform(0.1f);
		l.model.m_mesh = MESH_TYPE::SPHERE;
		l.model.m_material.color = l.color_ = rndColor;
		l.SetLightRange(static_cast<float>(rand() % 50 + 10));
		m_lights.push_back(l);
	}
}

#ifdef ENABLE_VPHYSICS
void SceneManager::InitializePhysicsObjects()
{
	// ANCHOR POINTS
	const float anchorScale = 0.3f;

	m_anchor1.m_model.m_mesh = MESH_TYPE::SPHERE;
	m_anchor1.m_model.m_material.color = Color::white;
	m_anchor1.m_model.m_material.shininess = 90.0f;
	m_anchor1.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
	m_anchor1.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this
	m_anchor1.m_transform.SetRotationDeg(15.0f, .0f, .0f);
	m_anchor1.m_transform.SetPosition(-20.0f, 25.0f, 2.0f);
	m_anchor1.m_transform.SetScaleUniform(anchorScale);
	//m_anchor2 = m_anchor1;
	//m_anchor2.m_model.m_material.color = Color::orange;
	//m_anchor2.m_transform.SetPosition(-10.0f, 4.0f, +4.0f);

	m_anchor2.m_model.m_mesh = MESH_TYPE::SPHERE;
	m_anchor2.m_model.m_material.color = Color::white;
	m_anchor2.m_model.m_material.shininess = 90.0f;
	//m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
	m_anchor2.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
	m_anchor2.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this
	m_anchor2.m_transform.SetRotationDeg(15.0f, .0f, .0f);
	m_anchor2.m_transform.SetPosition(20.0f, 25.0f, 2.0f);
	m_anchor2.m_transform.SetScaleUniform(anchorScale);

	// PHYSICS OBJECT REFERENCE
	auto& mat = m_physObj.m_model.m_material;
	auto& msh = m_physObj.m_model.m_mesh;
	auto& tfm = m_physObj.m_transform;
	msh					= MESH_TYPE::CUBE;
	mat.color			= Color::white;
	//mat.normalMap.id	= m_renderer->AddTexture("bricks_n.png");
	mat.diffuseMap.id	= m_renderer->AddTexture("bricks_d.png");
	mat.normalMap.name	= "bricks_n.png";	// todo: rethink this
	mat.shininess		= 65.0f;
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

//-----------------------------------------------------------------------------------------------------------------------------------------
void SceneManager::UpdateCentralObj(const float dt)
{
	// TODO: move these to somewhere (utils.h maybe?)
	static const XMVECTOR global_L = XMVectorSet(-1.0f, +0.0f, +0.0f, +0.0f);
	static const XMVECTOR global_R = XMVectorSet(+1.0f, +0.0f, +0.0f, +0.0f);
	static const XMVECTOR global_F = XMVectorSet(+0.0f, +0.0f, +1.0f, +0.0f);
	static const XMVECTOR global_B = XMVectorSet(+0.0f, +0.0f, -1.0f, +0.0f);
	static const XMVECTOR global_U = XMVectorSet(+0.0f, +1.0f, +0.0f, +0.0f);
	static const XMVECTOR global_D = XMVectorSet(+0.0f, -1.0f, +0.0f, +0.0f);

	const float moveSpeed	= 15.0f;
	const float rotSpeed	= XM_PI;

	XMVECTOR rot = XMVectorZero();
	XMVECTOR tr  = XMVectorZero();
	if (ENGINE->INP()->IsKeyDown(102)) tr += global_R;	// Key: Numpad6
	if (ENGINE->INP()->IsKeyDown(100)) tr += global_L;	// Key: Numpad4
	if (ENGINE->INP()->IsKeyDown(104)) tr += global_F;	// Key: Numpad8
	if (ENGINE->INP()->IsKeyDown(98))  tr += global_B; 	// Key: Numpad2
	if (ENGINE->INP()->IsKeyDown(105)) tr += global_U; 	// Key: Numpad9
	if (ENGINE->INP()->IsKeyDown(99))  tr += global_D; 	// Key: Numpad3
	
#ifdef ENABLE_VPHYSICS
	// ANCHOR MOVEMENT
	if(!ENGINE->INP()->IsKeyDown(17))	m_anchor1.m_transform.Translate(tr * dt * moveSpeed); // Key: Ctrl
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

	// temp
	//if (!m_lights.empty()) m_lights[0].tf.Translate(tr * dt * 0.2f);
}

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

void SceneManager::Update(float dt)
{
	//-------------------------------------------------------------------------------- MOVE OBJECTS ------------------------------------------------------------------
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef ENABLE_ANIMATION
	m_model.Update(dt);
	UpdateAnimatedModel(dt);
#endif

	UpdateCentralObj(dt);

#ifdef ENABLE_VPHYSICS
	UpdateAnchors(dt);
#endif

	//-------------------------------------------------------------------------------- SHADER CONFIGURATION ----------------------------------------------------------
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------
	// F1-F4 | Debug Shaders
	if (ENGINE->INP()->IsKeyTriggered(112)) m_selectedShader = m_renderData->texCoordShader;
	if (ENGINE->INP()->IsKeyTriggered(113)) m_selectedShader = m_renderData->normalShader;
	if (ENGINE->INP()->IsKeyTriggered(114)) m_selectedShader = m_renderData->tangentShader;
	if (ENGINE->INP()->IsKeyTriggered(115)) m_selectedShader = m_renderData->binormalShader;
	
	// F5-F8 | Lighting Shaders
	if (ENGINE->INP()->IsKeyTriggered(116)) m_selectedShader = m_renderData->unlitShader;
	if (ENGINE->INP()->IsKeyTriggered(117)) m_selectedShader = m_renderData->phongShader;

	// F9-F12 | Shader Parameters
	if (ENGINE->INP()->IsKeyTriggered(120)) m_gammaCorrection = !m_gammaCorrection;
	//if (ENGINE->INP()->IsKeyTriggered(52)) TBNMode = 3;


	//-------------------------------------------------------------------------------- ANIMATE LIGHTS ----------------------------------------------------------------
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------
	//static double accumulator = 0;
	//accumulator += dt;
	//if (RAND_LIGHT_COUNT > 0 && accumulator > DISCO_PERIOD)
	//{
	//	//// shuffling won't rearrange data, just the means of indexing.
	//	//char info[256];
	//	//sprintf_s(info, "Shuffle(L1:(%f, %f, %f)\tL2:(%f, %f, %f)\n",
	//	//	m_lights[0].tf.GetPositionF3().x, m_lights[0].tf.GetPositionF3().y,	m_lights[0].tf.GetPositionF3().z,
	//	//	m_lights[1].tf.GetPositionF3().x, m_lights[1].tf.GetPositionF3().y, m_lights[1].tf.GetPositionF3().z);
	//	//OutputDebugString(info);
	//	//static auto engine = std::default_random_engine{};
	//	//std::shuffle(std::begin(m_lights), std::end(m_lights), engine);

	//	//// randomize all lights
	//	//for (Light& l : m_lights)
	//	//{
	//	//	size_t i = rand() % Color::Palette().size();
	//	//	Color c = Color::Color::Palette()[i];
	//	//	l.color_ = c;
	//	//	l.model.m_material.color = c;
	//	//}

	//	// randomize all lights except 1 and 2
	//	for (unsigned j = 3; j<m_lights.size(); ++j)
	//	{
	//		Light& l = m_lights[j];
	//		size_t i = rand() % Color::Palette().size();
	//		Color c = Color::Color::Palette()[i];
	//		l.color_ = c;
	//		l.model.m_material.color = c;
	//	}

	//	accumulator = 0;
	//}

}
//-----------------------------------------------------------------------------------------------------------------------------------------

void SceneManager::Render(const XMMATRIX& view, const XMMATRIX& proj) 
{
	m_skydome.Render(m_renderer, view, proj);
	RenderLights(view, proj);
	RenderAnimated(view, proj);
	RenderBuilding(view, proj);
	RenderCentralObjects(view, proj);

	// TBN test
	//auto prevShader = m_selectedShader;
	//m_selectedShader = m_renderData->TNBShader;
	//RenderCentralObjects(view, proj);
	//m_selectedShader = prevShader;
}

void SceneManager::RenderBuilding(const XMMATRIX& view, const XMMATRIX& proj) const
{
	m_renderer->SetShader(m_selectedShader);
	m_renderer->SetConstant4x4f("view", view);
	m_renderer->SetConstant4x4f("proj", proj);
	m_renderer->SetBufferObj(MESH_TYPE::CUBE);
	m_renderer->SetConstant1f("gammaCorrection", m_gammaCorrection ? 1.0f : 0.0f);
	m_renderer->SetConstant3f("cameraPos", m_camera->GetPositionF());

	SendLightData();
	{
		m_building.floor.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world		= m_building.floor.m_transform.WorldTransformationMatrix();
		XMMATRIX nrmMatrix	= m_building.floor.m_transform.NormalMatrix(world);
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		m_building.ceiling.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world		= m_building.ceiling.m_transform.WorldTransformationMatrix();
		XMMATRIX nrmMatrix	= m_building.ceiling.m_transform.NormalMatrix(world);
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		m_building.wallR.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world		= m_building.wallR.m_transform.WorldTransformationMatrix();
		XMFLOAT3 color		= m_building.wallR.m_model.m_material.color.Value();
		XMMATRIX nrmMatrix	= m_building.wallR.m_transform.NormalMatrix(world);
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		m_building.wallL.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world		= m_building.wallL.m_transform.WorldTransformationMatrix();
		XMMATRIX nrmMatrix	= m_building.wallL.m_transform.NormalMatrix(world);
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		m_building.wallF.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world		= m_building.wallF.m_transform.WorldTransformationMatrix();
		XMMATRIX nrmMatrix	= m_building.wallF.m_transform.NormalMatrix(world);
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
}

void SceneManager::RenderLights(const XMMATRIX& view, const XMMATRIX& proj) const
{
	m_renderer->Reset();
	m_renderer->SetShader(m_renderData->unlitShader);
	m_renderer->SetConstant4x4f("view", view);
	m_renderer->SetConstant4x4f("proj", proj);
	for (const Light& light : m_lights)
	{
		m_renderer->SetBufferObj(light.model.m_mesh);
		XMMATRIX world = light.tf.WorldTransformationMatrix();
		XMFLOAT3 color = light.model.m_material.color.Value();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant3f("diffuse", color);
		m_renderer->SetConstant1f("isDiffuseMap", 0.0f);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
}

void SceneManager::RenderCentralObjects(const XMMATRIX& view, const XMMATRIX& proj) 
{
	// set shader and send constants
	m_renderer->SetShader(m_selectedShader);	
	if(m_selectedShader == m_renderData->phongShader) SendLightData();
	m_renderer->SetConstant4x4f("view", view);
	m_renderer->SetConstant4x4f("proj", proj);
	if(m_selectedShader == m_renderData->TNBShader)	  m_renderer->SetConstant1i("mode", TBNMode);
	if(m_selectedShader == m_renderData->phongShader) m_renderer->SetConstant1f("gammaCorrection", m_gammaCorrection == true ? 1.0f : 0.0f);

#ifdef ENABLE_VPHYSICS
	// draw anchor 1 sphere
	m_renderer->SetBufferObj(m_anchor1.m_model.m_mesh);
	if (m_selectedShader == m_renderData->phongShader || m_selectedShader == m_renderData->normalShader)
		m_anchor1.m_model.m_material.SetMaterialConstants(m_renderer);
	XMMATRIX world		= m_anchor1.m_transform.WorldTransformationMatrix();
	XMMATRIX nrmMatrix	= m_anchor1.m_transform.NormalMatrix(world);
	m_renderer->SetConstant4x4f("world", world);
	m_renderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
	m_renderer->Apply();
	m_renderer->DrawIndexed();

	// draw anchor 2 sphere
	m_renderer->SetBufferObj(m_anchor2.m_model.m_mesh);
	if (m_selectedShader == m_renderData->phongShader || m_selectedShader == m_renderData->normalShader)
		m_anchor2.m_model.m_material.SetMaterialConstants(m_renderer);
	world	  = m_anchor2.m_transform.WorldTransformationMatrix();
	nrmMatrix = m_anchor2.m_transform.NormalMatrix(world);
	m_renderer->SetConstant4x4f("world", world);
	m_renderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
	m_renderer->Apply();
	m_renderer->DrawIndexed();

	// DRAW GRAVITY TEST OBJ
	//----------------------
	// materials
	m_renderer->SetBufferObj(m_physObj.m_model.m_mesh);
	m_physObj.m_model.m_material.SetMaterialConstants(m_renderer);

	// render bricks
	for (const auto& pObj : m_vPhysObj)
	{
		world	  = pObj.m_transform.WorldTransformationMatrix();
		nrmMatrix = pObj.m_transform.NormalMatrix(world);
		m_renderer->SetConstant4x4f("nrmMatrix", nrmMatrix);
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}

	m_springSys.RenderSprings(m_renderer, view, proj);
#endif
}

void SceneManager::RenderAnimated(const XMMATRIX& view, const XMMATRIX& proj) const
{
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
	const XMFLOAT3 camPos = m_camera->GetPositionF();

	//m_renderer->SetShader(m_selectedShader);
	//m_renderer->SetConstant3f("cameraPos", camPos);
	//SendLightData();
	//m_animationLerp.Render(m_renderer, view, proj, time);

	m_renderer->Reset();
	m_renderer->SetShader(m_selectedShader);
	m_renderer->SetConstant3f("cameraPos", camPos);
	SendLightData();

#ifdef ENABLE_ANIMATION
	m_model.Render(m_renderer, view, proj);
#endif
}

void SceneManager::SendLightData() const
{
	const ShaderLight defaultLight = ShaderLight();
	std::vector<ShaderLight> lights(MAX_LIGHTS, defaultLight);
	std::vector<ShaderLight> spots(MAX_SPOTS, defaultLight);
	unsigned spotCount = 0;
	unsigned lightCount = 0;
	for (const Light& l : m_lights)
	{
		switch (l.lightType_)
		{
		case Light::POINT:
			lights[lightCount] = l.ShaderLightStruct();
			++lightCount;
			break;
		case Light::SPOT:
			spots[spotCount] = l.ShaderLightStruct();
			++spotCount;
			break;
		default:
			OutputDebugString("SceneManager::RenderBuilding(): ERROR: UNKOWN LIGHT TYPE\n");
			break;
		}
	}
	m_renderer->SetConstant1f("lightCount", static_cast<float>(lightCount));
	m_renderer->SetConstant1f("spotCount", static_cast<float>(spotCount));
	m_renderer->SetConstantStruct("lights", static_cast<void*>(lights.data()));
	m_renderer->SetConstantStruct("spots", static_cast<void*>(spots.data()));

#ifdef _DEBUG
	if (lights.size() > MAX_LIGHTS)	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (spots.size() > MAX_SPOTS)	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
}
