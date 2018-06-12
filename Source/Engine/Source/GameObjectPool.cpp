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

#include "GameObjectPool.h"

#include "Engine/Scene.h"
#include "Engine/GameObject.h"

void GameObjectPool::Initialize(size_t poolSize)
{
	mObjects.resize(poolSize, GameObject(nullptr));
	pNextAvailable = &mObjects[0];
	for (size_t i = 0; i < mObjects.size() - 1; ++i)
	{
		mObjects[i].pNextFreeObject = &mObjects[i + 1];
	}
	mObjects.back().pNextFreeObject = nullptr;
}

GameObject* GameObjectPool::Create(Scene* pScene)
{
	assert(pNextAvailable);
	GameObject* pNewObj = pNextAvailable;
	pNextAvailable = pNextAvailable->pNextFreeObject;
	pNewObj->mpScene = pScene;
	return pNewObj;
}

void GameObjectPool::Destroy(GameObject* pObj)
{
	pObj->pNextFreeObject = pNextAvailable;
	pNextAvailable = pObj;
}

void GameObjectPool::Cleanup()
{
	mObjects.clear();
	pNextAvailable = nullptr;
}