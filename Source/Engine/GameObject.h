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

#pragma once

#include "Engine/Transform.h"
#include "Engine/Model.h"

#include <memory>

struct SceneView;
class Renderer;
class Scene;
class Parser;

struct GameObjectRenderSettings
{
	// #DataOptimization:
	// this can be reduced to a char with the help of bit manipulation
	bool bRender = true;
	bool bRenderTBN = false;
	bool bRenderDepth = true;
};

class GameObject
{
public:
	void Render(Renderer* pRenderer, const SceneView& sceneView, bool UploadMaterialDataToGPU, const MaterialBuffer& materialBuffer) const;
	void RenderZ(Renderer* pRenderer) const;
	void Clear();

	void SetTransform(const Transform& transform) { mTransform = transform; }

	void AddMesh(MeshID meshID);

	// Adds materialID to the newest meshID (meshes.back())
	//
	void AddMaterial(MaterialID materialID);

	// friend std::shared_ptr<GameObject> Scene::CreateNewGameObject();
	// friend std::shared_ptr<GameObject> SerializedScene::CreateNewGameObject();
	//friend void Parser::ParseScene(Renderer*, const std::vector<std::string>&, SerializedScene&);
	friend class Scene;
	friend struct SerializedScene;
	friend class Parser;
	friend class ObjectPool;
private:
	GameObject(Scene* pScene);

 private:
	Transform					mTransform;
	Model						mModel;
	GameObjectRenderSettings	mRenderSettings;

	// After a game object is created, we use the pointer field
	// as the Scene*. Otherwise, we keep a pointer for the object pool
	// to the next available object - a free list of GameObject pointers
	union
	{
		GameObject*				pNextFreeObject;
		Scene*					mpScene;
	};
};

