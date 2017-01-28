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

//#include <algorithm>
//#include <random>

#define MAX_LIGHTS 20
#define MAX_SPOTS 10
#define RAND_LIGHT_COUNT 0
#define DISCO_PERIOD 0.25

int TBNMode = 0;

SceneManager::SceneManager()
{}

SceneManager::~SceneManager()
{}

void SceneManager::Initialize(Renderer * renderer, const RenderData* rData, Camera* cam)
{
	m_renderer = renderer;
	m_renderData = rData;
	m_selectedShader = m_renderData->phongShader;
	m_gammaCorrection = true;
	m_camera = cam;
	InitializeBuilding();
	InitializeLights();
	
	//m_centralObj.m_model.m_mesh = MESH_TYPE::GRID;
	//m_centralObj.m_model.m_material.color = Color::blue;
	//m_centralObj.m_model.m_material.shininess = 40.0f;
	m_centralObj.m_model.m_mesh = MESH_TYPE::CUBE;
	m_centralObj.m_model.m_material.color = Color::white;
	m_centralObj.m_model.m_material.shininess = 90.0f;
	m_centralObj.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
	m_centralObj.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this

	// set skydome
	//m_skydome.Init(this, "skydomeTex.png", FAR_PLANE / 3.97f);
	//m_skydome.Init(this, "bluecloud_dn.jpg", FAR_PLANE / 3.97f);
	//m_skydome.Init(this, "browncloud_rt.jpg", FAR_PLANE / 3.97f);
	m_skydome.Init(m_renderer, "browncloud_lf.jpg", FAR_PLANE / 3.97f, m_renderData->unlitShader);
	
}

void SceneManager::InitializeBuilding()
{
	const float floorWidth = 20.0f;
	const float floorDepth = 20.0f;
	const float wallHieght = 15.0f;	// amount from middle to top and bottom: because gpu cube is 2 units in length
	const float YOffset = wallHieght - 3;

	// FLOOR
	{
		Transform& tf = m_floor.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, -wallHieght + YOffset, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
		//m_floor.m_model.m_material.color		= Color::green;
		//m_floor.m_model.m_material.shininess	= 40.0f;
		m_floor.m_model.m_material = Material::jade;
		//m_floor.m_model.m_material.diffuseMap = m_renderer->GetTexture(m_renderData.exampleTex);
		m_floor.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
		m_floor.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this
	}
	// CEILING
	{
		Transform& tf = m_ceiling.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, wallHieght + YOffset, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
		//m_ceiling.m_model.m_material.color		= Color::purple;
		//m_ceiling.m_model.m_material.shininess	= 10.0f;
		m_ceiling.m_model.m_material = Material::jade;
		m_ceiling.m_model.m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
		m_ceiling.m_model.m_material.diffuseMap.name = "bricks_n.png";	// todo: rethink this
		m_ceiling.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
		m_ceiling.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this
	}

	// RIGHT WALL
	{
		Transform& tf = m_wallR.m_transform;
		tf.SetScale(floorDepth, 0.1f, wallHieght);
		tf.SetPosition(floorWidth, YOffset, 0);
		tf.SetRotationDeg(90.0f, 0.0f, 90.0f);
		//m_wallR.m_model.m_material.color		= Color::gray;
		//m_wallR.m_model.m_material.shininess	= 120.0f;
		m_wallR.m_model.m_material = Material::bronze;
		m_wallR.m_model.m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
		m_wallR.m_model.m_material.normalMap.id  = m_renderer->AddTexture("bricks_n.png");
		m_wallR.m_model.m_material.normalMap.name  = "bricks_n.png";	// todo: rethink this
		m_wallR.m_model.m_material.diffuseMap.name = "bricks_d.png";	// todo: rethink this
	}

	// LEFT WALL
	{
		Transform& tf = m_wallL.m_transform;
		tf.SetScale(floorDepth, 0.1f, wallHieght);
		tf.SetPosition(-floorWidth, YOffset, 0);
		tf.SetRotationDeg(90.0f, 0.0f, -90.0f);
		//m_wallL.m_model.m_material.color		= Color::gray;
		//m_wallL.m_model.m_material.shininess	= 60.0f;
		m_wallL.m_model.m_material = Material::bronze;
		m_wallL.m_model.m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
		m_wallL.m_model.m_material.normalMap.id  = m_renderer->AddTexture("bricks_n.png");
		m_wallL.m_model.m_material.normalMap.name  = "bricks_n.png";	// todo: rethink this
		m_wallL.m_model.m_material.diffuseMap.name = "bricks_d.png";	// todo: rethink this
	}
	// WALL
	{
		Transform& tf = m_wallF.m_transform;
		tf.SetScale(floorWidth, 0.1f, wallHieght);
		tf.SetPosition(0, YOffset, floorDepth);
		tf.SetRotationDeg(-90.0f, 0.0f, 0.0f);
		//m_wallF.m_model.m_material.color		= Color::gray;
		//m_wallF.m_model.m_material.shininess	= 90.0f;
		m_wallF.m_model.m_material = Material::gold;
		m_wallF.m_model.m_material.diffuseMap.id = m_renderer->AddTexture("bricks_d.png");
		m_wallF.m_model.m_material.normalMap.id = m_renderer->AddTexture("bricks_n.png");
		m_wallF.m_model.m_material.normalMap.name = "bricks_n.png";	// todo: rethink this
		m_wallF.m_model.m_material.diffuseMap.name = "bricks_d.png";	// todo: rethink this
	}
}

void SceneManager::InitializeLights()
{
	// spot lights
	{
		Light l;
		l.lightType_ = Light::LightType::SPOT;
		l.tf.SetPosition(0.0f, 13.0f, 0.0f);
		l.tf.Rotate(XMVectorSet(180.0f*DEG2RAD, 0.0f, 0.0f, 0.0f));
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
		l.model.m_material.color = Color::red;
		l.color_ = Color::red;
		l.SetLightRange(50);
		m_lights.push_back(l);
	}
	{
		Light l;
		l.tf.SetPosition(8.0f, 3.0f, 17.0f);
		l.tf.SetScaleUniform(0.1f);
		l.model.m_mesh = MESH_TYPE::SPHERE;
		l.model.m_material.color = Color::yellow;
		l.color_ = Color::yellow;
		l.SetLightRange(60);
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

void SceneManager::Update(float dt)
{
	// MOVE OBJECTS
	//--------------------------------------------------------------------------------
	XMVECTOR rot = XMVectorZero();
	XMVECTOR tr = XMVectorZero();
	if (ENGINE->INP()->IsKeyDown('O')) rot += XMVectorSet(45.0f, 0.0f, 0.0f, 1.0f);
	if (ENGINE->INP()->IsKeyDown('P')) rot += XMVectorSet(0.0f, 45.0f, 0.0f, 1.0f);
	if (ENGINE->INP()->IsKeyDown('U')) rot += XMVectorSet(0.0f, 0.0f, 45.0f, 1.0f);

	if (ENGINE->INP()->IsKeyDown('L')) tr += XMVectorSet(45.0f, 0.0f, 0.0f, 1.0f);
	if (ENGINE->INP()->IsKeyDown('J')) tr += XMVectorSet(-45.0f, 0.0f, 0.0f, 1.0f);
	if (ENGINE->INP()->IsKeyDown('K')) tr += XMVectorSet(0.0f, 0.0f, -45.0f, 1.0f);
	if (ENGINE->INP()->IsKeyDown('I')) tr += XMVectorSet(0.0f, 0.0f, +45.0f, 1.0f);
	m_centralObj.m_transform.Rotate(rot * dt * 0.1f);
	if (!m_lights.empty()) m_lights[0].tf.Rotate(rot * dt * 0.1f);
	if (!m_lights.empty()) m_lights[0].tf.Translate(tr * dt * 0.2f);

	// SHADER CONFIGURATION
	//--------------------------------------------------------------------------------
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

	if (ENGINE->INP()->IsKeyTriggered(49)) TBNMode = 0;
	if (ENGINE->INP()->IsKeyTriggered(50)) TBNMode = 1;
	if (ENGINE->INP()->IsKeyTriggered(51)) TBNMode = 2;
	if (ENGINE->INP()->IsKeyTriggered(52)) TBNMode = 3;

	// ANIMATE LIGHTS
	//--------------------------------------------------------------------------------
	static double accumulator = 0;
	accumulator += dt;
	if (RAND_LIGHT_COUNT > 0 && accumulator > DISCO_PERIOD)
	{
		//// shuffling won't rearrange data, just the means of indexing.
		//char info[256];
		//sprintf_s(info, "Shuffle(L1:(%f, %f, %f)\tL2:(%f, %f, %f)\n",
		//	m_lights[0].tf.GetPositionF3().x, m_lights[0].tf.GetPositionF3().y,	m_lights[0].tf.GetPositionF3().z,
		//	m_lights[1].tf.GetPositionF3().x, m_lights[1].tf.GetPositionF3().y, m_lights[1].tf.GetPositionF3().z);
		//OutputDebugString(info);
		//static auto engine = std::default_random_engine{};
		//std::shuffle(std::begin(m_lights), std::end(m_lights), engine);

		//// randomize all lights
		//for (Light& l : m_lights)
		//{
		//	size_t i = rand() % Color::Palette().size();
		//	Color c = Color::Color::Palette()[i];
		//	l.color_ = c;
		//	l.model.m_material.color = c;
		//}

		// randomize all lights except 1 and 2
		for (unsigned j = 3; j<m_lights.size(); ++j)
		{
			Light& l = m_lights[j];
			size_t i = rand() % Color::Palette().size();
			Color c = Color::Color::Palette()[i];
			l.color_ = c;
			l.model.m_material.color = c;
		}

		accumulator = 0;
	}
}

void SceneManager::Render(const XMMATRIX& view, const XMMATRIX& proj) 
{
	m_skydome.Render(m_renderer, view, proj);
	RenderLights(view, proj);
	RenderBuilding(view, proj);
	RenderCentralObjects(view, proj);

	// TBN test
	auto prevShader = m_selectedShader;
	m_selectedShader = m_renderData->TNBShader;
	RenderCentralObjects(view, proj);
	m_selectedShader = prevShader;
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
		m_floor.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world = m_floor.m_transform.WorldTransformationMatrix();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		m_ceiling.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world = m_ceiling.m_transform.WorldTransformationMatrix();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		m_wallR.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world = m_wallR.m_transform.WorldTransformationMatrix();
		XMFLOAT3 color = m_wallR.m_model.m_material.color.Value();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		m_wallL.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world = m_wallL.m_transform.WorldTransformationMatrix();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		m_wallF.m_model.m_material.SetMaterialConstants(m_renderer);
		XMMATRIX world = m_wallF.m_transform.WorldTransformationMatrix();
		m_renderer->SetConstant4x4f("world", world);
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
	// 2 spheres
	m_renderer->SetBufferObj(MESH_TYPE::SPHERE);
	m_centralObj.m_transform.SetScale(XMFLOAT3(1, 1, 1));

	m_renderer->SetShader(m_selectedShader);
	if(m_selectedShader == m_renderData->phongShader) SendLightData();
	m_renderer->SetConstant4x4f("view", view);
	m_renderer->SetConstant4x4f("proj", proj);
	if(m_selectedShader == m_renderData->TNBShader)	  m_renderer->SetConstant1i("mode", TBNMode);
	if(m_selectedShader == m_renderData->phongShader) m_centralObj.m_model.m_material.shininess /= 3.0;
	if(m_selectedShader == m_renderData->phongShader) m_centralObj.m_model.m_material.SetMaterialConstants(m_renderer);
	if(m_selectedShader == m_renderData->phongShader) m_renderer->SetConstant1f("gammaCorrection", m_gammaCorrection == true ? 1.0f : 0.0f);

	m_centralObj.m_transform.Translate(XMVectorSet(10.0f, 0.0f, 0.0f, 0.0f));
	XMMATRIX world = m_centralObj.m_transform.WorldTransformationMatrix();
	m_renderer->SetConstant4x4f("world", world);
	m_renderer->Apply();
	m_renderer->DrawIndexed();

	m_renderer->SetBufferObj(MESH_TYPE::CUBE);
	if(m_selectedShader == m_renderData->phongShader) m_centralObj.m_model.m_material.shininess *= 3.0;
	if(m_selectedShader == m_renderData->phongShader) m_centralObj.m_model.m_material.SetMaterialConstants(m_renderer);
	m_centralObj.m_transform.Translate(XMVectorSet(-20.0f, 0.0f, 0.0f, 0.0f));
	world = m_centralObj.m_transform.WorldTransformationMatrix();
	m_renderer->SetConstant4x4f("world", world);
	m_renderer->Apply();
	m_renderer->DrawIndexed();

	// grid
	m_centralObj.m_transform.Translate(XMVectorSet(10.0f, 0.0f, 0.0f, 0.0f));
	m_renderer->SetBufferObj(m_centralObj.m_model.m_mesh);
	m_renderer->SetShader(m_renderData->texCoordShader);
	m_renderer->SetConstant4x4f("view", view);
	m_renderer->SetConstant4x4f("proj", proj);

	m_centralObj.m_transform.SetScaleUniform(2);
	//m_centralObj.m_transform.SetScale(XMFLOAT3(3 * 4, 5 * 4, 2 * 4));
	world = m_centralObj.m_transform.WorldTransformationMatrix();
	m_renderer->SetConstant4x4f("world", world);
	m_renderer->Apply();
	m_renderer->DrawIndexed();
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
