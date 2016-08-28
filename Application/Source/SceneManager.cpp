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

#include "../../Renderer/Source/Renderer.h"

SceneManager::SceneManager()
{
}


SceneManager::~SceneManager()
{
}

void SceneManager::Initialize(Renderer * renderer)
{
	m_renderer = renderer;

	// FLOOR
	{
		Transform& tf = m_floor.m_transform;
		tf.SetScale(10.0f, 0.0f, 10.0f);
		tf.SetPosition(0, -5, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 0.0f);
	}
	
	// RIGHT WALL
	{
		Transform& tf = m_wallR.m_transform;
		tf.SetScale(10.0f, 0.0f, 10.0f);
		tf.SetPosition(10, 0, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 90.0f);
	}

	// LEFT WALL
	{
		Transform& tf = m_wallL.m_transform;
		tf.SetScale(10.0f, 0.0f, 10.0f);
		tf.SetPosition(-10, 0, 0);
		tf.SetRotationDeg(0.0f, 0.0f, 90.0f);
	}
	// WALL
	{
		Transform& tf = m_wallF.m_transform;
		tf.SetScale(10.0f, 0.0f, 10.0f);
		tf.SetPosition(0, 0, 10);
		tf.SetRotationDeg(90.0f, 0.0f, 0.0f);
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
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		XMMATRIX world = m_wallR.m_transform.WorldTransformationMatrix();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		XMMATRIX world = m_wallL.m_transform.WorldTransformationMatrix();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
	{
		XMMATRIX world = m_wallF.m_transform.WorldTransformationMatrix();
		m_renderer->SetConstant4x4f("world", world);
		m_renderer->Apply();
		m_renderer->DrawIndexed();
	}
}
