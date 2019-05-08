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
#include "Engine/Scene.h"
class LODTestScene : public Scene
{
public:

	void Load(SerializedScene& scene) override;
	void Unload() override;
	void Update(float dt) override;
	void RenderUI() const override;

	LODTestScene(const BaseSceneParams& params) : Scene(params) {}
	~LODTestScene() = default;

private:
	// [0, 3] : indexes the LOD object in the scene
	//          and is not the same array index in mpObjects[]
	int mSelectedLODObject = -1;
	int mSelectedLODObjectPrev = -1;
	size_t mNumLODObjects = 0;

	size_t GetSelectedLODObjectIndex() const;
	void OnSelectedLODObjectChange();

	std::vector<GameObject*> mpLODWireframeObjects;
	Material* mpWireframeMaterial;
	Material* mpWireframeMaterialHighlighted;
};

