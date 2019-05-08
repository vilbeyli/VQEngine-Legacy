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
	Material* pMat = this->CreateNewMaterial(EMaterialType::GGX_BRDF);
	pMat->diffuse = LinearColor::black;
	pMat->emissiveColor = LinearColor::green;
	pMat->emissiveIntensity = 200.0f;
	mpWireframeMaterial = pMat;

	pMat = this->CreateNewMaterial(EMaterialType::GGX_BRDF);
	pMat->diffuse = LinearColor::black;
	pMat->emissiveColor = LinearColor::orange;
	pMat->emissiveIntensity = 200.0f;
	mpWireframeMaterialHighlighted = pMat;

	// we know 1st object is the ground plane, and the remaining are the LOD objects.
	// here we create a copy of them and change some render settings / material 
	// to make the mesh used for the LOD object visible above the LOD objects.
	const size_t numObjs = this->mpObjects.size();
	this->mNumLODObjects = numObjs - 1;
	for (size_t i = 1; i < numObjs; ++i)
	{
		// create a copy of the object
		const GameObject* pObjToCopy = this->mpObjects[i];
		GameObject* pNewObject = this->CreateNewGameObject();
		
		// copy transform
		pNewObject->SetTransform(pObjToCopy->GetTransform());
		Transform& tf = pNewObject->GetTransform();
		tf._position.y() += 75.0f;


		// generate wireframe mesh for the copied geometry
		const MeshID LODMeshID = pObjToCopy->GetModelData().mMeshIDs.back();
		Mesh WireframeLODMesh = this->mMeshes[LODMeshID];
		WireframeLODMesh.SetRenderMode(Mesh::MeshRenderSettings::EMeshRenderMode::WIREFRAME);
		this->mMeshes.push_back(WireframeLODMesh);

		// copy the geometry and assign the new mesh
		pNewObject->AddMesh(static_cast<MeshID>(this->mMeshes.size() - 1));

		// set material
		pNewObject->SetMeshMaterials(mpWireframeMaterial);

		// set render settings
		pNewObject->mRenderSettings.bCastShadow = false;
		pNewObject->mRenderSettings.bRender = true;

		// register the wireframe object to the scene
		mpLODWireframeObjects.push_back(pNewObject);
	}
}

// Scene-specific unloading logic
//
void LODTestScene::Unload()
{
	mpLODWireframeObjects.clear();
	this->mNumLODObjects = 0;
	this->mSelectedLODObject = this->mSelectedLODObjectPrev = -1;
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

	constexpr float OBJ_ROTATION_SPEED = 3.14f;
	if (this->mSelectedLODObject >= 0)
	{
		const float rotationAmount = dt * OBJ_ROTATION_SPEED;
		mpLODWireframeObjects[mSelectedLODObject]->GetTransform().RotateAroundGlobalYAxisDegrees(rotationAmount);
		mpObjects[mSelectedLODObject + 1]->GetTransform().RotateAroundGlobalYAxisDegrees(rotationAmount);;
	}
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
		mLightsStatic[this->mSelectedLODObject].mColor = LinearColor::orange;

		GameObject* pObj = mpLODWireframeObjects[this->mSelectedLODObject];
		pObj->GetModel().OverrideMaterials(mpWireframeMaterialHighlighted->ID);
		//mpLODWireframeObjects[this->mSelectedLODObject]->GetModel().mMaterialAssignmentQueue;
	}

	if (this->mSelectedLODObjectPrev != -1)
	{
		mLightsStatic[this->mSelectedLODObjectPrev].mColor = LinearColor::white;

		GameObject* pObj = mpLODWireframeObjects[this->mSelectedLODObjectPrev];
		pObj->GetModel().OverrideMaterials(mpWireframeMaterial->ID);
		pObj->GetTransform().ResetRotation();
		mpObjects[this->mSelectedLODObjectPrev + 1]->GetTransform().ResetRotation();
	}

	CalculateSceneBoundingBox();
}
