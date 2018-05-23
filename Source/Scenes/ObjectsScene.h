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

#include "Engine/Scene.h"
#include "Engine/Animation.h"

class ObjectsScene : public Scene
{
public:
	void Load(SerializedScene& scene) override;
	void Unload() override;
	void Update(float dt) override;
	void RenderUI() const override;

	ObjectsScene() = default;
	~ObjectsScene() = default;

private:
	void InitializeLights(SerializedScene& scene);
	void UpdateCentralObj(const float dt);
	void RenderCentralObjects(const SceneView& sceneView, bool sendMaterialData) const;
// ----------------------------------------------------------------------
	
	void ToggleFloorNormalMap();
	
	struct Room 
	{
		friend class ObjectsScene;
		void Render(Renderer* pRenderer, const SceneView& sceneView, bool sendMaterialData) const;
		void Initialize(MaterialBuffer& materials, Renderer* pRenderer);

		union 
		{
			struct 
			{
				GameObject* floor;
				GameObject* wallL;
				GameObject* wallR;
				GameObject* wallF;
				GameObject* ceiling;
			};
			GameObject* walls[5];
		};
	} m_room;

	std::vector<GameObject*> mSpheres;
	std::vector<Animation> mAnimations;
};