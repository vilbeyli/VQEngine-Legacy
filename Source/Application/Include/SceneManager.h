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
#include "Settings.h"
#include "Renderer.h"
#include <vector>
#include <memory>

// scenes
#include "RoomScene.h"

#include "RenderPasses.h"


using std::shared_ptr;

class Camera;
class PathManager;

struct Path;



class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	void Initialize(Renderer* renderer, PathManager* pathMan);
	void SetCameraSettings(const Settings::Camera& cameraSettings);

	void Update(float dt);
	void Render() const;

	void SendLightData() const;
	inline ShaderID GetSelectedShader() const { return m_selectedShader; }

private:
	friend struct DepthShadowPass;

private:
	RoomScene			m_roomScene;	// todo: rethink scene management

	Renderer*			m_pRenderer;
	shared_ptr<Camera>	m_pCamera;
	//PathManager*		m_pPathManager; // unused

	// rendering passes
	// lighting pass?
	SamplerID			m_normalSampler;
	DepthShadowPass		m_depthPass;
	PostProcessPass		m_postProcessPass;

	ShaderID			m_selectedShader;
	bool				m_debugRender;

	std::vector<GameObject*> m_ZPassObjects;
};