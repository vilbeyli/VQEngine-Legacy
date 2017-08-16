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

#include "GameObject.h"
#include "Skydome.h"
#include "Skybox.h"
#include "Light.h"

struct SerializedScene;
class SceneManager;
class Renderer;

//-------------------------------------------------------------

// animation prototype, currently only used here.
// todo: animation class

template<typename T>
class Track
{
public:
	Track() = delete;
	Track(T* data, const T& beginVal, const T& endVal, float period)
		:
		_totalTime(0.0f),
		_period(period),
		_data(data),
		_valueBegin(beginVal),
		_valueEnd(endVal)
	{}

	void Update(float dt);

private:
	float _totalTime;
	float _period;

	T* _data;
	T _valueBegin;
	T _valueEnd;
};

struct Animation
{
	// lerping tracks
	std::vector<Track<float>> _fTracks;
	std::vector<Track<vec3>>  _vTracks;

	void Update(float dt)
	{
		for (auto& t : _fTracks) t.Update(dt);
		for (auto& t : _vTracks) t.Update(dt);
	}
};

//-------------------------------------------------------------

class RoomScene
{
public:
	void Load(Renderer* pRenderer, SerializedScene& scene);
	void Update(float dt);
	void Render(Renderer* pRenderer, const XMMATRIX& viewProj) const;

	RoomScene(SceneManager& sceneMan);
	~RoomScene() = default;

private:
	void InitializeLights(SerializedScene& scene);
	void InitializeObjectArrays();

	void UpdateCentralObj(const float dt);

	void RenderLights(const XMMATRIX& viewProj) const;
	void RenderAnimated(const XMMATRIX& view, const XMMATRIX& proj) const;
	void RenderCentralObjects(const XMMATRIX& viewProj, bool sendMaterialData) const;
// ----------------------------------------------------------------------
	friend class SceneManager;
	void ToggleFloorNormalMap();

	SceneManager&		m_sceneManager;
	Renderer*			m_pRenderer;
	Skydome				m_skydome;
	Skybox				m_skybox;
	std::vector<Light>	m_lights;

	struct Room {
		friend class RoomScene;
		void Render(Renderer* pRenderer, const XMMATRIX& viewProj, bool sendMaterialData) const;
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
	std::vector<GameObject> cubes;

	GameObject triangle;
	GameObject quad;
	GameObject grid;
	GameObject cylinder;
	GameObject cube;
	GameObject sphere;

	std::vector<Animation> m_animations;
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