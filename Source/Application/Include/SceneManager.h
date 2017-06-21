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

#define xENABLE_ANIMATION	
#define xENABLE_VPHYSICS
#ifdef ENABLE_ANIMATION
#include "../Animation/Include/AnimatedModel.h"
#endif
#ifdef ENABLE_VPHYSICS
#include "PhysicsEngine.h"
#endif

#include "GameObject.h"
#include "Light.h"
#include "Skydome.h"

#include "Settings.h"

#include <vector>
#include <memory>

using std::shared_ptr;
using std::unique_ptr;

class Renderer;
class Camera;
class PathManager;
struct Path;

typedef int ShaderID;
struct RenderData;


class SceneManager
{
	using pRenderer = std::shared_ptr<Renderer>;	// ?

public:
	SceneManager();
	~SceneManager();

	void Initialize(pRenderer renderer, const RenderData* rData, PathManager* pathMan);
	void SetCameraSettings(const Settings::Camera& cameraSettings);
	void Update(float dt);
	void Render();	// todo: const

private:
	void InitializeBuilding();
	void InitializeLights();
	void InitializeObjectArrays();

	void UpdateCentralObj(const float dt);

#ifdef ENABLE_VPHYSICS
	void InitializePhysicsObjects();
	void UpdateAnchors(float dt);
#endif
#ifdef ENABLE_ANIMATION
	void UpdateAnimatedModel(const float dt);
#endif

	void RenderBuilding(const XMMATRIX& view, const XMMATRIX& proj) const;
	void RenderLights(const XMMATRIX& view, const XMMATRIX& proj) const;
	void RenderAnimated(const XMMATRIX& view, const XMMATRIX& proj) const;
	void RenderCentralObjects(const XMMATRIX& view, const XMMATRIX& proj); // todo: const

	void SendLightData() const;

private:
	pRenderer			m_pRenderer;
	shared_ptr<Camera>	m_pCamera;
	PathManager*		m_pPathManager; // unused
	Skydome				m_skydome;

	// render data
	const RenderData*	m_renderData;
	ShaderID			m_selectedShader;
	bool				m_gammaCorrection;
	std::vector<Light>	m_lights;

	// scene variables
	struct Building {
		GameObject floor;
		GameObject wallL;
		GameObject wallR;
		GameObject wallF;
		GameObject ceiling;
	} m_building;

	std::vector<GameObject> spheres;
	std::vector<GameObject> cubes;

#ifdef ENABLE_ANIMATION
	// hierarchical model
	AnimatedModel m_model;
#endif

#ifdef ENABLE_VPHYSICS
	// physics simulation
	GameObject				m_anchor1;
	GameObject				m_anchor2;
	GameObject				m_physObj;
	std::vector<GameObject> m_vPhysObj;

	SpringSystem m_springSys;
#endif
};

