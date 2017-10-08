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

#include "Scene.h"
#include "Animation.h"
#include "GameObject.h"
#include "Skybox.h"


class RoomScene : public Scene
{
public:
	void Load(Renderer* pRenderer, SerializedScene& scene) override;
	void Update(float dt) override;
	void Render(Renderer* pRenderer, const SceneView& sceneView) const override;
	void GetShadowCasterObjects(std::vector<GameObject*>& casters) override;

	RoomScene(SceneManager& sceneMan, std::vector<Light>& lights);
	~RoomScene() = default;

	inline ESkyboxPreset GetSkybox() const { return m_skybox; }

private:
	void InitializeLights(SerializedScene& scene);
	void InitializeObjectArrays(SerializedScene& scene);
	void UpdateCentralObj(const float dt);
	void RenderCentralObjects(const SceneView& sceneView, bool sendMaterialData) const;
// ----------------------------------------------------------------------
	
	void ToggleFloorNormalMap();

	ESkyboxPreset		m_skybox;

	struct Room {
		friend class RoomScene;
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

	std::vector<GameObject> spheres;
	std::vector<Animation> m_animations;



	/// todo: move to ao test scene
	// std::vector<GameObject> cubes; 
	//GameObject Plane2;
	//GameObject obj2;
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