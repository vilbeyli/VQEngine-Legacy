//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
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

#include "LODTestScene.h"
#include "Engine/Engine.h"
#include "Engine/Material.h"
#include "Application/Input.h"
#include "Utilities/Log.h"


// Scene-specific loading logic
//
void LODTestScene::Load(SerializedScene& scene)
{
	// some sick wireframe rendering material here
	Material* pWireframeMeshMaterial = this->CreateNewMaterial(EMaterialType::GGX_BRDF);
	pWireframeMeshMaterial->diffuse = LinearColor::green;
	pWireframeMeshMaterial->emissiveColor = LinearColor::green;
	pWireframeMeshMaterial->emissiveIntensity = 2000.0f;


	// we know 1st object is the ground plane, and the remaining are the LOD objects.
	// here we create a copy of them and change some render settings / material 
	// to make the mesh used for the LOD object visible above the LOD objects.
	const size_t numObjs = this->mpObjects.size();
	this->mNumLODObjects = numObjs - 1;
	for (size_t i = 1; i < numObjs; ++i)
	{
		const GameObject* pObjToCopy = this->mpObjects[i];
		GameObject* pNewObject = this->CreateNewGameObject();
		
		pNewObject->SetTransform(pObjToCopy->GetTransform());
		Transform& tf = pNewObject->GetTransform();
		tf._position.y() += 50.0f;

		pNewObject->SetModel(pObjToCopy->GetModel());
		pNewObject->SetMeshMaterials(pWireframeMeshMaterial);

		pNewObject->mRenderSettings.bCastShadow = false;
		pNewObject->mRenderSettings.bRender = false;

		const MeshID LODMeshID = pNewObject->GetModelData().mMeshIDs.back();
		const Mesh&  LODMesh = this->mMeshes[LODMeshID];


		Mesh WireframeLODMesh = LODMesh;
		WireframeLODMesh.SetRenderMode(Mesh::MeshRenderSettings::EMeshRenderMode::WIREFRAME);

		this->mMeshes.push_back(WireframeLODMesh);
		mpLODWireframeObjects.push_back(pNewObject);
	}
}

// Scene-specific unloading logic
//
void LODTestScene::Unload()
{

}

// Update() is called each frame
//
void LODTestScene::Update(float dt)
{
	this->mSelectedLODObjectPrev = this->mSelectedLODObject;
	if (this->mSelectedLODObject != -1)
	{
		if (ENGINE->INP()->IsKeyTriggered("UpArrow"))
		{
			// ++ lod 
		}
		if (ENGINE->INP()->IsKeyTriggered("DownArrow"))
		{
			// -- lod
		}
	}

	if (ENGINE->INP()->IsKeyTriggered("RightArrow"))
	{
		MathUtil::ClampedIncrementOrDecrement(this->mSelectedLODObject, 1, 0, static_cast<int>(this->mNumLODObjects));
	}
	if (ENGINE->INP()->IsKeyTriggered("LeftArrow"))
	{
		MathUtil::ClampedIncrementOrDecrement(this->mSelectedLODObject, -1, 0, static_cast<int>(this->mNumLODObjects));
	}


	if (ENGINE->INP()->IsKeyTriggered("Numpad0"))
	{
		this->mSelectedLODObject = -1;
	}


	if (this->mSelectedLODObject != this->mSelectedLODObjectPrev)
		OnSelectedLODObjectChange();

}

// RenderUI() is called at the last stage of rendering before presenting the frame.
// Scene-specific UI rendering goes in here.
//
void LODTestScene::RenderUI() const 
{
	TextDrawDescription drawDesc;
	drawDesc.color = LinearColor::green;
	drawDesc.scale = 0.28f;
	drawDesc.screenPosition = vec2(0.5f, 0.5f);
	drawDesc.text = "Test 123123";
	mpTextRenderer->RenderText(drawDesc);
}

size_t LODTestScene::GetSelectedLODObjectIndex() const
{
	return mSelectedLODObject == -1
		? 0

		// - we know mpObjects[0] is the ground plane
		// - we know the next N objects are the LOD objects.
		// : we can just add one to the index to offset the ground plane
		//   to get the mpObject[] index of the selected LOD object.
		: mSelectedLODObject + 1; 
}

void LODTestScene::OnSelectedLODObjectChange()
{
	if (mSelectedLODObject == -1)
	{
		// don't do anything if we just unselected
	}
	else
	{
		mLightsStatic[this->mSelectedLODObject].mColor = LinearColor::medium_pruple;
		mpLODWireframeObjects[this->mSelectedLODObject]->mRenderSettings.bRender = true;
		//mpLODWireframeObjects[this->mSelectedLODObject]->GetModel().mMaterialAssignmentQueue;

	}

	if (this->mSelectedLODObjectPrev != -1)
	{
		mLightsStatic[this->mSelectedLODObjectPrev].mColor = LinearColor::white;
		mpLODWireframeObjects[this->mSelectedLODObjectPrev]->mRenderSettings.bRender = false;
	}
}
