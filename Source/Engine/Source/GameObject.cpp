//	VQEngine | DirectX11 Renderer
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
#include "Renderer/Renderer.h"
#include "RenderPasses.h"

#include "SceneResources.h"

#include "Engine.h"

GameObject::GameObject(Scene* pScene) : mpScene(pScene) {};

void GameObject::AddMesh(MeshID meshID)
{
	mModel.mData.mMeshIDs.push_back(meshID);
}


void GameObject::AddMaterial(Material * pMat)
{
	mModel.AddMaterialToMesh(mModel.mData.mMeshIDs.back(), pMat->ID, pMat->IsTransparent());
}


void GameObject::RenderOpaque(Renderer* pRenderer
	, const SceneView& sceneView
	, bool UploadMaterialDataToGPU
	, const MaterialPool& materialBuffer) const
{
	const EShaders shader = static_cast<EShaders>(pRenderer->GetActiveShader());
	const XMMATRIX world = mTransform.WorldTransformationMatrix();
	const XMMATRIX wvp = world * sceneView.viewProj;

	// SET MATRICES
	switch (shader)
	{
	case EShaders::TBN:
		pRenderer->SetConstant4x4f("world", world);
		pRenderer->SetConstant4x4f("viewProj", sceneView.viewProj);
		pRenderer->SetConstant4x4f("normalMatrix", mTransform.NormalMatrix(world));
		break;
	case EShaders::Z_PREPRASS:
	case EShaders::DEFERRED_GEOMETRY:
		pRenderer->SetConstant4x4f("worldView", world * sceneView.view);
		pRenderer->SetConstant4x4f("normalViewMatrix", mTransform.NormalMatrix(world) * sceneView.view);
		pRenderer->SetConstant4x4f("worldViewProj", wvp);
		break;
	case EShaders::NORMAL:
		pRenderer->SetConstant4x4f("normalMatrix", mTransform.NormalMatrix(world));
	case EShaders::UNLIT:
	case EShaders::TEXTURE_COORDINATES:
		pRenderer->SetConstant4x4f("worldViewProj", wvp);
		break;
	default:	// lighting shaders
		pRenderer->SetConstant4x4f("world", world);
		pRenderer->SetConstant4x4f("normalMatrix", mTransform.NormalMatrix(world));
		pRenderer->SetConstant4x4f("worldViewProj", wvp);
		break;
	}

	// SET GEOMETRY & MATERIAL, THEN DRAW
	//pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);
	for_each(mModel.mData.mMeshIDs.begin(), mModel.mData.mMeshIDs.end(), [&](MeshID id) 
	{
		const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(mpScene, id);

		// SET MATERIAL CONSTANTS
		if (UploadMaterialDataToGPU)
		{
			const bool bMeshHasMaterial = mModel.mData.mMaterialLookupPerMesh.find(id) != mModel.mData.mMaterialLookupPerMesh.end();

			if (bMeshHasMaterial)
			{
				const MaterialID materialID = mModel.mData.mMaterialLookupPerMesh.at(id);
				const Material* pMat = materialBuffer.GetMaterial_const(materialID);
				// #TODO: uncomment below when transparency is implemented.
				//if (pMat->IsTransparent())	// avoidable branching - perhaps keeping opaque and transparent meshes on separate vectors is better.
				//	return;
				pMat->SetMaterialConstants(pRenderer, shader, sceneView.bIsDeferredRendering);
			}
			else
			{
				materialBuffer.GetDefaultMaterial(GGX_BRDF)->SetMaterialConstants(pRenderer, shader, sceneView.bIsDeferredRendering);
			}
		}

		pRenderer->SetVertexBuffer(IABuffer.first);
		pRenderer->SetIndexBuffer(IABuffer.second);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
	});
}

void GameObject::RenderZ(Renderer * pRenderer) const
{
	const XMMATRIX world = mTransform.WorldTransformationMatrix();
	pRenderer->SetConstant4x4f("world", world);
	pRenderer->SetConstant4x4f("normalMatrix", mTransform.NormalMatrix(world));
	for_each(mModel.mData.mMeshIDs.begin(), mModel.mData.mMeshIDs.end(), [&](MeshID id)
	{
		// TODO: #Optimization
		// changing state for each mesh is not a good idea -> sort and render | render list.
		const bool bIs2DGeometry =
			id == EGeometry::TRIANGLE ||
			id == EGeometry::QUAD ||
			id == EGeometry::GRID;
		const RasterizerStateID rasterizerState = bIs2DGeometry ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;
		pRenderer->SetRasterizerState(rasterizerState);

		const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(mpScene, id);
		pRenderer->SetVertexBuffer(IABuffer.first);
		pRenderer->SetIndexBuffer(IABuffer.second);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
	});
}

void GameObject::RenderTransparent(
	  Renderer * pRenderer
	, const SceneView& sceneView
	, bool UploadMaterialDataToGPU
	, const MaterialPool& materialBuffer) const
{
	const EShaders shader = static_cast<EShaders>(pRenderer->GetActiveShader());
	const XMMATRIX world = mTransform.WorldTransformationMatrix();
	const XMMATRIX wvp = world * sceneView.viewProj;

	// SET MATRICES
	switch (shader)
	{
	case EShaders::TBN:
		pRenderer->SetConstant4x4f("world", world);
		pRenderer->SetConstant4x4f("viewProj", sceneView.viewProj);
		pRenderer->SetConstant4x4f("normalMatrix", mTransform.NormalMatrix(world));
		break;
	case EShaders::Z_PREPRASS:
	case EShaders::DEFERRED_GEOMETRY:
		pRenderer->SetConstant4x4f("worldView", world * sceneView.view);
		pRenderer->SetConstant4x4f("normalViewMatrix", mTransform.NormalMatrix(world) * sceneView.view);
		pRenderer->SetConstant4x4f("worldViewProj", wvp);
		break;
	case EShaders::NORMAL:
		pRenderer->SetConstant4x4f("normalMatrix", mTransform.NormalMatrix(world));
	case EShaders::UNLIT:
	case EShaders::TEXTURE_COORDINATES:
		pRenderer->SetConstant4x4f("worldViewProj", wvp);
		break;
	default:	// lighting shaders
		pRenderer->SetConstant4x4f("world", world);
		pRenderer->SetConstant4x4f("normalMatrix", mTransform.NormalMatrix(world));
		pRenderer->SetConstant4x4f("worldViewProj", wvp);
		break;
	}

	// SET GEOMETRY & MATERIAL, THEN DRAW
	for_each(mModel.mData.mTransparentMeshIDs.begin(), mModel.mData.mTransparentMeshIDs.end(), [&](MeshID id)
	{
		const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(mpScene, id);

		// SET MATERIAL CONSTANTS
		if (UploadMaterialDataToGPU)
		{
			const bool bMeshHasMaterial = mModel.mData.mMaterialLookupPerMesh.find(id) != mModel.mData.mMaterialLookupPerMesh.end();

			if (bMeshHasMaterial)
			{
				const MaterialID materialID = mModel.mData.mMaterialLookupPerMesh.at(id);
				const Material* pMat = materialBuffer.GetMaterial_const(materialID);
				pMat->SetMaterialConstants(pRenderer, shader, sceneView.bIsDeferredRendering && shader != FORWARD_BRDF);
			}
			else
			{
				materialBuffer.GetDefaultMaterial(GGX_BRDF)->SetMaterialConstants(pRenderer, shader, sceneView.bIsDeferredRendering);
			}
		}

		pRenderer->SetVertexBuffer(IABuffer.first);
		pRenderer->SetIndexBuffer(IABuffer.second);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
	});
}

void GameObject::Clear()
{
	mTransform = Transform();
	mModel = Model();
	mRenderSettings = GameObjectRenderSettings();
}


