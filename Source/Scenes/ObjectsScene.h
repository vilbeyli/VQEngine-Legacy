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

#include "Engine/Scene.h"
#include "Engine/Animation.h"
#include "Engine/GameObject.h"
#include "Engine/Skybox.h"


class ObjectsScene : public Scene
{
public:
	void Load(SerializedScene& scene) override;
	void Unload() override;
	void Update(float dt) override;
	int Render(const SceneView& sceneView, bool bSendMaterialData) const override;
	void RenderUI() const override;

	void GetShadowCasters(std::vector<const GameObject*>& casters) const override;	// todo: rename this... decide between depth and shadows
	void GetSceneObjects(std::vector<const GameObject*>&) const override;

	ObjectsScene(SceneManager& sceneMan, std::vector<Light>& lights);
	~ObjectsScene() = default;

private:
	void InitializeLights(SerializedScene& scene);
	void UpdateCentralObj(const float dt);
	void RenderCentralObjects(const SceneView& sceneView, bool sendMaterialData) const;
// ----------------------------------------------------------------------
	
	void ToggleFloorNormalMap();
	
	struct Room {
		friend class ObjectsScene;
		void Render(Renderer* pRenderer, const SceneView& sceneView, bool sendMaterialData) const;
		void Initialize(Renderer* pRenderer);
#if 0
		// union -> deleted dtor??
		union
		{
			struct 
			{
				GameObject floor;
				GameObject wallL;
				GameObject wallR;
				GameObject wallF;
				GameObject ceiling;
			};
			GameObject walls[5];
		};
#else
		GameObject floor;
		GameObject wallL;
		GameObject wallR;
		GameObject wallF;
		GameObject ceiling;
#endif
	} m_room;

	std::vector<GameObject> mSpheres;
	std::vector<Animation> mAnimations;
};






























// junkyard
#ifdef ENABLE_VPHYSICS
void InitializePhysicsObjects();
void UpdateAnchors(float dt);
#endif
#ifdef ENABLE_ANIMATION
void UpdateAnimatedModel(const float dt);
#endif
#define xENABLE_ANIMATION	
#define xENABLE_VPHYSICS
#ifdef ENABLE_ANIMATION
#include "../Animation/Include/AnimatedModel.h"
#endif
#ifdef ENABLE_VPHYSICS
#include "PhysicsEngine.h"
#endif

#ifdef ENABLE_VPHYSICS
// physics simulation
GameObject				m_anchor1;
GameObject				m_anchor2;
GameObject				m_physObj;
std::vector<GameObject> m_vPhysObj;

SpringSystem m_springSys;
#endif

#ifdef ENABLE_ANIMATION
// hierarchical model
AnimatedModel m_model;
#endif