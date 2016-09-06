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

SceneManager::SceneManager()
{}

SceneManager::~SceneManager()
{}

void SceneManager::Initialize(Renderer * renderer, RenderData rData, Camera* cam)
{
	m_renderer = renderer;
	m_renderData = rData;
	m_camera = cam;
	InitializeBuilding();
	InitializeLights();
	
	m_centralObj.m_model.m_mesh = MESH_TYPE::GRID;
}

void SceneManager::Update(float dt)
{
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
	m_lights[0].tf.Translate(tr * dt * 0.2f);
}

void SceneManager::Render(const XMMATRIX& view, const XMMATRIX& proj) 
{
	RenderLights(view, proj);
	RenderBuilding(view, proj);

	// central obj
	m_renderer->SetShader(m_renderData.texCoordShader);
	//m_renderer->SetViewport(m_renderer->WindowWidth(), m_renderer->WindowHeight());
	m_renderer->SetConstant4x4f("view", view);
	m_renderer->SetConstant4x4f("proj", proj);

	m_renderer->SetBufferObj(MESH_TYPE::GRID);
	m_centralObj.m_transform.SetScale(XMFLOAT3(3 * 4, 5 * 4, 2 * 4));
	XMMATRIX world = m_centralObj.m_transform.WorldTransformationMatrix();
	m_renderer->SetConstant4x4f("world", world);
	m_renderer->Apply();
	m_renderer->DrawIndexed();

	m_renderer->SetBufferObj(MESH_TYPE::SPHERE);
	m_centralObj.m_transform.Translate(XMVectorSet(10.0f, 0.0f, 0.0f, 0.0f));
	m_centralObj.m_transform.SetScale(XMFLOAT3(1, 1, 1));
	world = m_centralObj.m_transform.WorldTransformationMatrix();
	m_renderer->SetConstant4x4f("world", world);
	m_renderer->Apply();
	m_renderer->DrawIndexed();
	m_centralObj.m_transform.Translate(XMVectorSet(-10.0f, 0.0f, 0.0f, 0.0f));
}

void SceneManager::InitializeBuilding()
{
	const float floorWidth = 19.0f;
	const float floorDepth = 30.0f;
	const float wallHieght = 15.0f;	// amount from middle to top and bottom: because gpu cube is 2 units in length
	
	// FLOOR
	{
		Transform& tf = m_floor.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, -wallHieght, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
		m_floor.m_model.m_material.color = Color::green;
	}
	// CEILING
	{
		Transform& tf = m_ceiling.m_transform;
		tf.SetScale(floorWidth, 0.1f, floorDepth);
		tf.SetPosition(0, wallHieght, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
		m_ceiling.m_model.m_material.color = Color::purple;
	}

	// RIGHT WALL
	{
		Transform& tf = m_wallR.m_transform;
		tf.SetScale(wallHieght, 0.1f, floorDepth);
		tf.SetPosition(floorWidth, 0, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 90.0f);
		m_wallR.m_model.m_material.color = Color::gray;
	}

	// LEFT WALL
	{
		Transform& tf = m_wallL.m_transform;
		tf.SetScale(wallHieght, 0.1f, floorDepth);
		tf.SetPosition(-floorWidth, 0, 0);
		tf.SetRotationDeg(0.0f, 0.0f, -90.0f);
		m_wallL.m_model.m_material.color = Color::gray;
	}
	// WALL
	{
		Transform& tf = m_wallF.m_transform;
		tf.SetScale(floorWidth, 0.1f, wallHieght);
		tf.SetPosition(0, 0, floorDepth);
		tf.SetRotationDeg(90.0f, 0.0f, 0.0f);
		m_wallF.m_model.m_material.color = Color::white;
	}
}

void SceneManager::InitializeLights()
{
	Light l;
	l.tf.SetPosition(-8.0f, 10.0f, 0);
	l.tf.SetScaleUniform(0.1f);
	l.model.m_material.color = Color::orange;
	l.color_ = Color::orange;
	l.SetLightRange(50);
	m_lights.push_back(l);
}

void SceneManager::RenderBuilding(const XMMATRIX& view, const XMMATRIX& proj) const
{
	//XMFLOAT4 camPos  = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	//XMFLOAT4 camPos2 = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	//XMFLOAT4 camPos3 = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	//XMFLOAT4 camPos4 = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	//XMStoreFloat4(&camPos, -view.r[0]);
	//XMStoreFloat4(&camPos2, -view.r[1]);
	//XMStoreFloat4(&camPos3, -view.r[2]);
	//XMStoreFloat4(&camPos4, -view.r[3]);

	//char info[128];
	//sprintf_s(info, "camPos: (%f, %f, %f, %f)\n", camPos.x, camPos.y, camPos.z, camPos.w);
	//OutputDebugString(info);
	//sprintf_s(info, "camPos2: (%f, %f, %f, %f)\n", camPos2.x, camPos2.y, camPos2.z, camPos2.w);
	//OutputDebugString(info);
	//sprintf_s(info, "camPos3: (%f, %f, %f, %f)\n", camPos3.x, camPos3.y, camPos3.z, camPos3.w);
	//OutputDebugString(info);
	//sprintf_s(info, "camPos4: (%f, %f, %f, %f)\n", camPos4.x, camPos4.y, camPos4.z, camPos4.w);
	//OutputDebugString(info);
	

	XMFLOAT3 camPos_real = m_camera->GetPositionF();
	//sprintf_s(info, "camPos_real: (%f, %f, %f)\n\n", camPos_real.x, camPos_real.y, camPos_real.z);
	//OutputDebugString(info);

	m_renderer->SetShader(m_renderData.phongShader); 
	m_renderer->SetConstant4x4f("view", view);
	m_renderer->SetConstant4x4f("proj", proj);
	m_renderer->SetBufferObj(MESH_TYPE::CUBE);

	ShaderLight sl = m_lights[0].ShaderLightStruct();
	m_renderer->SetConstantStruct("light", static_cast<void*>(&sl), sizeof(sl));
	m_renderer->SetConstant3f("cameraPos", camPos_real);
	//m_renderer->SetConstant3f("cameraPos", camPos);
	{
		XMMATRIX world = m_floor.m_transform.WorldTransformationMatrix();
		XMFLOAT3 color = m_floor.m_model.m_material.color.Value();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant3f("color", color);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		XMMATRIX world = m_ceiling.m_transform.WorldTransformationMatrix();
		XMFLOAT3 color = m_ceiling.m_model.m_material.color.Value();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant3f("color", color);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		XMMATRIX world = m_wallR.m_transform.WorldTransformationMatrix();
		XMFLOAT3 color = m_wallR.m_model.m_material.color.Value();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant3f("color", color);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		XMMATRIX world = m_wallL.m_transform.WorldTransformationMatrix();
		XMFLOAT3 color = m_wallL.m_model.m_material.color.Value();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant3f("color", color);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		XMMATRIX world = m_wallF.m_transform.WorldTransformationMatrix();
		XMFLOAT3 color = m_wallF.m_model.m_material.color.Value();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant3f("color", color);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
}

void SceneManager::RenderLights(const XMMATRIX& view, const XMMATRIX& proj) const
{
	m_renderer->Reset();
	m_renderer->SetShader(m_renderData.unlitShader);
	m_renderer->SetConstant4x4f("view", view);
	m_renderer->SetConstant4x4f("proj", proj);
	m_renderer->SetBufferObj(MESH_TYPE::SPHERE);
	for (const Light& light : m_lights)
	{
		XMMATRIX world = light.tf.WorldTransformationMatrix();
		XMFLOAT3 color = light.model.m_material.color.Value();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->SetConstant3f("color", color);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
}
