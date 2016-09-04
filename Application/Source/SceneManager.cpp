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

#include "Renderer.h"

SceneManager::SceneManager()
{
}


SceneManager::~SceneManager()
{
}

void SceneManager::Initialize(Renderer * renderer)
{
	m_renderer = renderer;

	const float floorWidth = 19.0f;
	const float floorDepth = 30.0f;
	const float wallHieght = 12.0f;	// amount from middle to top and bottom: because gpu cube is 2 units in length

	// FLOOR
	{
		Transform& tf = m_floor.m_transform;
		tf.SetScale(floorWidth, 0.0f, floorDepth);
		tf.SetPosition(0, -wallHieght, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
		m_floor.m_model.m_material.color = Color::green;
	}
	// CEILING
	{
		Transform& tf = m_ceiling.m_transform;
		tf.SetScale(floorWidth, 0.0f, floorDepth);
		tf.SetPosition(0, wallHieght, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
		m_ceiling.m_model.m_material.color = Color::purple;
	}

	// RIGHT WALL
	{
		Transform& tf = m_wallR.m_transform;
		tf.SetScale(wallHieght, 0.0f, floorDepth);
		tf.SetPosition(floorWidth, 0, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 90.0f);
		m_wallR.m_model.m_material.color = Color::orange;
	}

	// LEFT WALL
	{
		Transform& tf = m_wallL.m_transform;
		tf.SetScale(wallHieght, 0.0f, floorDepth);
		tf.SetPosition(-floorWidth, 0, 0);
		tf.SetRotationDeg(0.0f, 0.0f, -90.0f);
		m_wallL.m_model.m_material.color = Color::orange;
	}
	// WALL
	{
		Transform& tf = m_wallF.m_transform;
		tf.SetScale(floorWidth, 0.0f, wallHieght);
		tf.SetPosition(0, 0, floorDepth);
		tf.SetRotationDeg(90.0f, 0.0f, 0.0f);
		m_wallF.m_model.m_material.color = Color::orange;
	}
}

void SceneManager::Update(float dt)
{
}

void SceneManager::Render() const
{
	m_renderer->SetBufferObj(MESH_TYPE::CUBE);
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
