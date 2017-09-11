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
#include "RenderPasses.h"

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

void GameObject::Render(Renderer* pRenderer, const SceneView& sceneView, bool UploadMaterialDataToGPU) const
{
	const EShaders shader = static_cast<EShaders>(pRenderer->GetActiveShader());
	const XMMATRIX world = m_transform.WorldTransformationMatrix();
	const XMMATRIX wvp = world * sceneView.viewProj;

	if(UploadMaterialDataToGPU) 
		m_model.m_material.SetMaterialConstants(pRenderer, shader);

	switch (shader)
	{
	case EShaders::TBN:
		pRenderer->SetConstant4x4f("world", world);
		pRenderer->SetConstant4x4f("viewProj", sceneView.viewProj);
		pRenderer->SetConstant4x4f("normalMatrix", m_transform.NormalMatrix(world));
		break;
	case EShaders::DEFERRED_GEOMETRY:
		pRenderer->SetConstant4x4f("worldView", world * sceneView.view);
		pRenderer->SetConstant4x4f("normalViewMatrix", m_transform.NormalMatrix(world) * sceneView.view);
		pRenderer->SetConstant4x4f("worldViewProj", wvp);
		break;
	default:
		pRenderer->SetConstant4x4f("world", world);
		pRenderer->SetConstant4x4f("normalMatrix", m_transform.NormalMatrix(world));
		pRenderer->SetConstant4x4f("worldViewProj", wvp);
		break;

	}

	pRenderer->SetBufferObj(m_model.m_mesh);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
}

void GameObject::RenderZ(Renderer * pRenderer) const
{
	const XMMATRIX world = m_transform.WorldTransformationMatrix();
	const bool bIs2DGeometry = false;	// todo: fix self shadowing 2D geometry
		//m_model.m_mesh == GEOMETRY::TRIANGLE || 
		//m_model.m_mesh == GEOMETRY::QUAD || 
		//m_model.m_mesh == GEOMETRY::GRID;
	const RasterizerStateID rasterizerState = bIs2DGeometry ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;

	pRenderer->SetBufferObj(m_model.m_mesh);
	pRenderer->SetRasterizerState(rasterizerState);
	pRenderer->SetConstant4x4f("world", world);
	pRenderer->SetConstant4x4f("normalMatrix", m_transform.NormalMatrix(world));
	pRenderer->Apply();
	pRenderer->DrawIndexed();
}


