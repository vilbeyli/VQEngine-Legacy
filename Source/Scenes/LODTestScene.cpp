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
#include "Renderer/Renderer.h"


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

		// copy the geometry and assign the new mesh with its render settings
		pNewObject->AddMesh(LODMeshID, { MeshRenderSettings::EMeshRenderMode::WIREFRAME });

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

	// handle LOD level change input if LOD object is selected
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

	// handle LOD Object selection key presses
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

	// handle selected object change event
	if (this->mSelectedLODObject != this->mSelectedLODObjectPrev)
		OnSelectedLODObjectChange();

	// rotate the selected object
	constexpr float OBJ_ROTATION_SPEED = 3.14f;
	if (this->mSelectedLODObject >= 0)
	{
		const float rotationAmount = dt * OBJ_ROTATION_SPEED;
		mpLODWireframeObjects[mSelectedLODObject]->GetTransform().RotateAroundGlobalYAxisDegrees(rotationAmount);
		mpObjects[mSelectedLODObject + 1]->GetTransform().RotateAroundGlobalYAxisDegrees(rotationAmount);;
	}

	// update the LOD stats
	for (int currLODObject = 0; currLODObject < this->mNumLODObjects; ++currLODObject)
	{
		const GameObject* pObj = mpLODWireframeObjects[currLODObject];
		const MeshID meshID = pObj->GetModelData().mMeshIDs.back();
		const int currentLODValue = mLODManager.GetLODValue(pObj, meshID);
		auto VB_IB_IDs = mMeshes[meshID].GetIABuffers(currentLODValue);
		const BufferDesc bufDescVB = mpRenderer->GetBufferDesc(EBufferType::VERTEX_BUFFER, VB_IB_IDs.first);
		const BufferDesc bufDescIB = mpRenderer->GetBufferDesc(EBufferType::INDEX_BUFFER , VB_IB_IDs.second);
		const LODManager::LODSettings& lod = mLODManager.GetMeshLODSettings(pObj, meshID);

		vec3 distSq = pObj->GetTransform()._position - this->GetActiveCamera().GetPositionF();
		distSq = XMVector3Dot(distSq, distSq);
		const float distance = std::sqrtf(distSq.x());

		mSceneStats.objStats[currLODObject].lod = currentLODValue;
		mSceneStats.objStats[currLODObject].numVert = bufDescVB.mElementCount;
		mSceneStats.objStats[currLODObject].numTri = bufDescIB.mElementCount / 3;
		mSceneStats.objStats[currLODObject].lodLevelDistanceThreshold = lod.distanceThresholds[currentLODValue];
		mSceneStats.objStats[currLODObject].currDistance = distance;
	}
}

// RenderUI() is called at the last stage of rendering before presenting the frame.
// Scene-specific UI rendering goes in here.
//
void LODTestScene::RenderUI() const 
{
	TextDrawDescription drawDesc;
	int numLine = 0;

	const Settings::Engine& sEngineSettings = ENGINE->GetSettings();
	constexpr int NUM_LOD_OBJECTS = 4; // todo: not hardcode
	

	//
	// DRAW DATA
	//
	static const char* LODObjectNames[] =
	{
		"CYLINDER"
		, "GRID"
		, "SPHERE"
		, "CONE"
	};
	static const char* LODStatEntryLabels[] =
	{
		"LOD: "
		, "Vertices: "
		, "Triangles: "
		, "LOD Distance: "
		, "Current Distance: "
	};
	constexpr size_t NUM_LOD_STAT_ENTRIES = sizeof(LODStatEntryLabels) / sizeof(char*);
	std::vector<std::string> LODValues;
	std::vector<std::string> NumVerts;
	std::vector<std::string> NumTriangles;
	std::vector<std::string> LODDistance;
	std::vector<std::string> CurrDistance;
	std::vector<const std::vector<std::string>*> LODStatEntries =
	{
		  &LODValues
		, &NumVerts
		, &NumTriangles
		, &LODDistance
		, &CurrDistance
	};

	// read LOD data and populate the containers which will be used to fill in draw desc data
	for (int currLODObject = 0; currLODObject < NUM_LOD_OBJECTS; ++currLODObject)
	{
		LODValues.push_back   (std::to_string(mSceneStats.objStats[currLODObject].lod));
		NumVerts.push_back    (std::to_string(mSceneStats.objStats[currLODObject].numVert));
		NumTriangles.push_back(std::to_string(mSceneStats.objStats[currLODObject].numTri));

		{
			std::stringstream ss; // format the distance value and use only 2 digits after the point
			ss << std::fixed << std::setprecision(2) << mSceneStats.objStats[currLODObject].lodLevelDistanceThreshold;
			LODDistance.push_back(ss.str());
		}
		{
			std::stringstream ss; // format the distance value and use only 2 digits after the point
			ss << std::fixed << std::setprecision(2) << mSceneStats.objStats[currLODObject].currDistance;
			CurrDistance.push_back(ss.str());
		}
	}


	//
	// Draw Settings
	//
	const float LOD_OBJ_STAT_X_MARGIN_PX = 0.1f * sEngineSettings.window.width;
	const float X_START_POS_PX = 0.35f * sEngineSettings.window.width;
	const float Y_START_POS_PX = 0.06f * sEngineSettings.window.height;
	const float LINE_HEIGHT = 25.0f;
	const vec2 screenPositionStart(X_START_POS_PX, Y_START_POS_PX);

	//
	// Render background
	// 
	constexpr float BACKGROUND_ALPHA = 0.800f;
	const vec2 sz(NUM_LOD_OBJECTS * 0.1f, (NUM_LOD_STAT_ENTRIES + 1) * 0.033f);
	VQEngine::UI::RenderBackground(mpRenderer, LinearColor::black, BACKGROUND_ALPHA, sz, screenPositionStart - vec2(30, 30));



	// ------------------------------------------------------------------------
	// Draw Table Header: Object Names
	// ------------------------------------------------------------------------
	int CURRENT_SCREEN_LINE = 0;
	drawDesc.scale = 0.33f;
	drawDesc.color = LinearColor::white;
	for (int currLODObject = 0; currLODObject < NUM_LOD_OBJECTS; ++currLODObject)
	{
		drawDesc.screenPosition = vec2(
			  screenPositionStart.x() + currLODObject * LOD_OBJ_STAT_X_MARGIN_PX
			, screenPositionStart.y() + CURRENT_SCREEN_LINE * LINE_HEIGHT
		);
		drawDesc.text = LODObjectNames[currLODObject];
		mpTextRenderer->RenderText(drawDesc);
	}
	++CURRENT_SCREEN_LINE;
	++CURRENT_SCREEN_LINE;

	// ------------------------------------------------------------------------
	// Draw Table Entries: LOD Values, num tris/verts, etc.
	// ------------------------------------------------------------------------
	drawDesc.color = LinearColor::light_blue;
	drawDesc.scale = 0.25f;
	for (size_t currStatEntry = 0; currStatEntry < NUM_LOD_STAT_ENTRIES; ++currStatEntry)
	{
		for (int currLODObject = 0; currLODObject < NUM_LOD_OBJECTS; ++currLODObject)
		{
			drawDesc.screenPosition = vec2(
				screenPositionStart.x() + currLODObject * LOD_OBJ_STAT_X_MARGIN_PX
				, screenPositionStart.y() + CURRENT_SCREEN_LINE * LINE_HEIGHT
			);
			drawDesc.text = LODStatEntryLabels[currStatEntry] + (*LODStatEntries[currStatEntry])[currLODObject];
			mpTextRenderer->RenderText(drawDesc);
		}
		++CURRENT_SCREEN_LINE;
	}
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
