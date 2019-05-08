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
#pragma once

#include "Application/HandleTypedefs.h"

#include "Mesh.h"
#include "Material.h"

#include <memory>

class Scene;

// https://en.wikibooks.org/wiki/More_C++_Idioms/Friendship_and_the_Attorney-Client
class SceneResourceView
{
#if 1
public:
	static std::pair<BufferID, BufferID> GetVertexAndIndexBuffersOfMesh(const Scene* pScene, MeshID meshID);
	static std::pair<BufferID, BufferID> GetBuiltinMeshVertexAndIndexBufferID(EGeometry builtInGeometry, int lod = 0);
	static const Material* GetMaterial(const Scene* pScene, MaterialID materialID);
	static const Mesh::MeshRenderSettings::EMeshRenderMode GetMeshRenderMode(const Scene* pScene, MeshID meshID);
#else
private:
	static std::pair<BufferID, BufferID> GetVertexAndIndexBuffersOfMesh(const Scene* pScene, MeshID meshID);
	static std::pair<BufferID, BufferID> GetBuiltinMeshVertexAndIndexBufferID(EGeometry builtInGeometry, int lod = 0);
	static const Material* GetMaterial(const Scene* pScene, MaterialID materialID);

	// accessor struct/class list 
	//
	friend class GameObject;
	friend class Renderer;
	friend class Engine;

	friend struct ShadowMapPass;
	friend struct DeferredRenderingPasses;
	friend struct ForwardLightingPass;
	friend struct ZPrePass;
#endif
};
