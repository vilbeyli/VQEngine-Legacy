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

#include "SceneResourceView.h"
#include "Scene.h"

std::pair<BufferID, BufferID> SceneResourceView::GetVertexAndIndexBufferIDsOfMesh(const Scene* pScene, MeshID meshID)
{
	return pScene->mMeshes[meshID].GetIABuffers(pScene->mLODManager.GetLODValue(/*TODO*/ 0, meshID));
}

std::pair<BufferID, BufferID> SceneResourceView::GetVertexAndIndexBufferIDsOfMesh(const Scene* pScene, MeshID meshID, const GameObject* pObj)
{
	return pScene->mMeshes[meshID].GetIABuffers(pScene->mLODManager.GetLODValue(pObj, meshID));
}

std::pair<BufferID, BufferID> SceneResourceView::GetBuiltinMeshVertexAndIndexBufferID(EGeometry builtInGeometry, int lod)
{
	return Scene::GetGeometryVertexAndIndexBuffers(builtInGeometry, lod);
}

const Material* SceneResourceView::GetMaterial(const Scene* pScene, MaterialID materialID)
{
	return pScene->mMaterials.GetMaterial_const(materialID);
}

const MeshRenderSettings::EMeshRenderMode SceneResourceView::GetMeshRenderMode(const Scene* pScene, const GameObject* pObj, MeshID meshID)
{
	const MeshRenderSettingsLookup& meshRenderSettingsLookup = pObj->GetModelData().mMeshRenderSettingsLookup;
	const bool bRenderSettingsFound = meshRenderSettingsLookup.find(meshID) != meshRenderSettingsLookup.end();

	return bRenderSettingsFound
		? meshRenderSettingsLookup.at(meshID).renderMode
		: MeshRenderSettings::DefaultRenderSettings();
}

