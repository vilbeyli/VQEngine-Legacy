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

#include "GameObject.h"
#include "Renderer.h"

GameObject::GameObject()
#ifdef ENABLE_PHY_CODE
	:
	m_rb(m_transform)
#endif
{
#ifdef ENABLE_PHY_CODE
	// these should be turned on explicitly
	m_rb.EnableGravity = false;
	m_rb.EnablePhysics = false;
#endif
}


GameObject::~GameObject()
{}

GameObject::GameObject(const GameObject & obj)
#ifdef ENABLE_PHY_CODE
	:
	m_rb(m_transform)
#endif
{
	m_transform = obj.m_transform;
	m_model = obj.m_model;
#ifdef ENABLE_PHY_CODE
	m_rb = obj.m_rb;
	m_rb.UpdateVertPositions();
#endif
}

GameObject& GameObject::operator=(const GameObject& obj)
{
	m_transform = obj.m_transform;
	m_model = obj.m_model;
#ifdef ENABLE_PHY_CODE
	m_rb		= RigidBody(m_transform);
#endif
	return *this;
}

void GameObject::Render(Renderer* pRenderer, const XMMATRIX& viewProj, bool UploadMaterialDataToGPU) const
{
	// TODO: set shader for material
	if(UploadMaterialDataToGPU) m_model.m_material.SetMaterialConstants(pRenderer);
	pRenderer->SetBufferObj(m_model.m_mesh);
	XMMATRIX world = m_transform.WorldTransformationMatrix();
	XMMATRIX wvp = world * viewProj;
	pRenderer->SetConstant4x4f("world", world);
	pRenderer->SetConstant4x4f("normalMatrix", m_transform.NormalMatrix(world));
	pRenderer->SetConstant4x4f("worldViewProj", wvp);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
}

void GameObject::RenderZ(Renderer * pRenderer) const
{
	pRenderer->SetBufferObj(m_model.m_mesh);
	XMMATRIX world = m_transform.WorldTransformationMatrix();
	pRenderer->SetConstant4x4f("world", world);
	pRenderer->SetConstant4x4f("normalMatrix", m_transform.NormalMatrix(world));
	pRenderer->Apply();
	pRenderer->DrawIndexed();
}


