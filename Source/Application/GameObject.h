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

#pragma once

#include "Components/Transform.h"
#include "Renderer/Model.h"	// todo: engine? model's kinda component...

class Renderer;
struct SceneView;

struct GameObjectRenderSettings
{
	bool bRenderTBN = false;
	bool bRenderDepth = true;
};

class GameObject
{
public:
	void Render(Renderer* pRenderer, const SceneView& sceneView, bool UploadMaterialDataToGPU) const;
	void RenderZ(Renderer* pRenderer) const;
	void Clear();

	Transform					mTransform;
	Model						mModel;
	GameObjectRenderSettings	mRenderSettings;
};

