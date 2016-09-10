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

#include "Light.h"
#include <vector>

class Renderer;
class Camera;

typedef int ShaderID;
struct RenderData
{
	ShaderID phongShader;
	ShaderID unlitShader;
	ShaderID texCoordShader;
	ShaderID normalShader;
	TextureID exampleTex;
	TextureID exampleNormMap;
};

class SceneManager
{
public:
	SceneManager();
	~SceneManager();

	void Initialize(Renderer* renderer, RenderData rData, Camera* cam);
	void Update(float dt);
	void Render(const XMMATRIX& view, const XMMATRIX& proj) ;	// todo: const

private:
	void InitializeBuilding();
	void InitializeLights();

	void RenderBuilding(const XMMATRIX& view, const XMMATRIX& proj) const;
	void RenderLights(const XMMATRIX& view, const XMMATRIX& proj) const;
	void RenderCentralObjects(const XMMATRIX& view, const XMMATRIX& proj); // todo: const

	void SendLightData() const;
private:
	Renderer*	m_renderer;
	Camera*		m_camera;

	// render data
	RenderData	m_renderData;
	ShaderID	m_selectedShader;
	bool		m_gammaCorrection;
	std::vector<Light> m_lights;

	// scene variables
	GameObject m_floor;
	GameObject m_wallL;
	GameObject m_wallR;
	GameObject m_wallF;
	GameObject m_ceiling;

	GameObject m_centralObj;
};

