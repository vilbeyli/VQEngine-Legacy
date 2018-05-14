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
#include "Renderer/Renderer.h"
#include "RenderPasses.h"

void GameObject::Render(Renderer* pRenderer, const SceneView& sceneView, bool UploadMaterialDataToGPU) const
{
	const EShaders shader = static_cast<EShaders>(pRenderer->GetActiveShader());
	const XMMATRIX world = mTransform.WorldTransformationMatrix();
	const XMMATRIX wvp = world * sceneView.viewProj;
	
	const Material* mat = [&]() -> const Material*{
		if(sceneView.bIsPBRLightingUsed)
			return &mModel.mBRDF_Material;
		return &mModel.mBlinnPhong_Material;
	}();

	if (UploadMaterialDataToGPU)
	{
		mat->SetMaterialConstants(pRenderer, shader, sceneView.bIsDeferredRendering);
	}

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

	//pRenderer->SetVertexBuffer();
	//pRenderer->SetIndexBuffer();
	pRenderer->SetGeometry(static_cast<EGeometry>(mModel.mMesh));
	pRenderer->Apply();
	pRenderer->DrawIndexed();
}

void GameObject::RenderZ(Renderer * pRenderer) const
{
	const XMMATRIX world = mTransform.WorldTransformationMatrix();
	const bool bIs2DGeometry = 
		mModel.mMesh == EGeometry::TRIANGLE || 
		mModel.mMesh == EGeometry::QUAD || 
		mModel.mMesh == EGeometry::GRID;
	const RasterizerStateID rasterizerState = bIs2DGeometry ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;

	pRenderer->SetGeometry(static_cast<EGeometry>(mModel.mMesh));
	pRenderer->SetRasterizerState(rasterizerState);
	pRenderer->SetConstant4x4f("world", world);
	pRenderer->SetConstant4x4f("normalMatrix", mTransform.NormalMatrix(world));
	pRenderer->Apply();
	pRenderer->DrawIndexed();
}

void GameObject::Clear()
{
	mTransform = Transform();
	mModel = Model();
	mRenderSettings = GameObjectRenderSettings();
}


