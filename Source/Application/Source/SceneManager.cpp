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

#include "SceneManager.h"	
#include "Engine.h"			
#include "Input.h"
#include "Renderer.h"
#include "Camera.h"
#include "utils.h"
#include "Light.h"

#define KEY_TRIG(k) ENGINE->INP()->IsKeyTriggered(k)


constexpr int		MAX_LIGHTS = 20;
constexpr int		MAX_SPOTS = 10;


SceneManager::SceneManager()
	:
	m_pCamera(new Camera()),
	m_roomScene(*this)
{}

SceneManager::~SceneManager()
{}

void SceneManager::Initialize(Renderer* renderer, PathManager* pathMan)
{
	//m_pPathManager		= pathMan;

	m_pRenderer			= renderer;
	m_selectedShader	= SHADERS::FORWARD_BRDF;
	m_debugRender		= false;

	Skybox::InitializePresets(m_pRenderer);

	m_roomScene.Load(m_pRenderer);

	// todo: static game object array in gameobj.h 
	m_depthPass.Initialize(m_pRenderer, m_pRenderer->m_device);

	//renderer->m_Direct3D->ReportLiveObjects();
	m_ZPassObjects.push_back(&m_roomScene.m_room.floor);
	m_ZPassObjects.push_back(&m_roomScene.m_room.wallL);
	m_ZPassObjects.push_back(&m_roomScene.m_room.wallR);
	m_ZPassObjects.push_back(&m_roomScene.m_room.wallF);
	m_ZPassObjects.push_back(&m_roomScene.m_room.ceiling);
	m_ZPassObjects.push_back(&m_roomScene.quad);
	m_ZPassObjects.push_back(&m_roomScene.grid);
	m_ZPassObjects.push_back(&m_roomScene.cylinder);
	m_ZPassObjects.push_back(&m_roomScene.triangle);
	m_ZPassObjects.push_back(&m_roomScene.cube);
	m_ZPassObjects.push_back(&m_roomScene.sphere);
	for (GameObject& obj : m_roomScene.cubes)	m_ZPassObjects.push_back(&obj);
	for (GameObject& obj : m_roomScene.spheres)	m_ZPassObjects.push_back(&obj);
	
	m_postProcessPass.Initialize(m_pRenderer, m_pRenderer->m_device);
}


void SceneManager::SetCameraSettings(const Settings::Camera & cameraSettings)
{
	assert(m_pRenderer);
	const auto& NEAR_PLANE = cameraSettings.nearPlane;
	const auto& FAR_PLANE = cameraSettings.farPlane;
	m_pCamera->SetOthoMatrix(m_pRenderer->WindowWidth(), m_pRenderer->WindowHeight(), NEAR_PLANE, FAR_PLANE);
	m_pCamera->SetProjectionMatrix((float)XM_PIDIV4, m_pRenderer->AspectRatio(), NEAR_PLANE, FAR_PLANE);
	m_pCamera->SetPosition(0, 50, -190);
	m_pCamera->Rotate(0.0f, 15.0f * DEG2RAD, 1.0f);
	m_pRenderer->SetCamera(m_pCamera.get());
}


void SceneManager::Update(float dt)
{
	m_pCamera->Update(dt);
	m_roomScene.Update(dt);
	
	//-------------------------------------------------------------------------------- SHADER CONFIGURATION ----------------------------------------------------------
	//----------------------------------------------------------------------------------------------------------------------------------------------------------------
	if (ENGINE->INP()->IsKeyTriggered("F1")) m_selectedShader = SHADERS::TEXTURE_COORDINATES;
	if (ENGINE->INP()->IsKeyTriggered("F2")) m_selectedShader = SHADERS::NORMAL;
	//if (ENGINE->INP()->IsKeyTriggered(114)) m_selectedShader = SHADERS::TANGENT;
	if (ENGINE->INP()->IsKeyTriggered("F3")) m_roomScene.ToggleFloorNormalMap();
	if (ENGINE->INP()->IsKeyTriggered("F4")) m_selectedShader = SHADERS::BINORMAL;
	
	if (ENGINE->INP()->IsKeyTriggered("F5")) m_selectedShader = SHADERS::UNLIT;
	if (ENGINE->INP()->IsKeyTriggered("F6")) m_selectedShader = m_selectedShader == SHADERS::FORWARD_PHONG ? SHADERS::FORWARD_BRDF : SHADERS::FORWARD_PHONG;
	if (ENGINE->INP()->IsKeyTriggered("F7")) m_debugRender = !m_debugRender;

	if (ENGINE->INP()->IsKeyTriggered("F9")) m_postProcessPass._bloomPass.ToggleBloomPass();

}
//-----------------------------------------------------------------------------------------------------------------------------------------

void SceneManager::Render() const
{
	const float clearColor[4] = { 0.2f, 0.4f, 0.7f, 1.0f };
	const XMMATRIX view = m_pCamera->GetViewMatrix();
	const XMMATRIX proj = m_pCamera->GetProjectionMatrix();
	const XMMATRIX viewProj = view * proj;

#if 1
	// DEPTH PASS
	//------------------------------------------------------------------------
	// get shadow casters (todo: static/dynamic lights)
	std::vector<const Light*> _shadowCasters;	// warning: dynamic memory alloc. put in paramStruct { array start end } or C++11 equiv
	for (const Light& light : m_roomScene.m_lights)
	{
		if (light._castsShadow)
			_shadowCasters.push_back(&light);
	}

	m_depthPass.RenderDepth(m_pRenderer, _shadowCasters, m_ZPassObjects);
#endif

	// MAIN PASS
	//------------------------------------------------------------------------
	m_pRenderer->Reset();
	m_pRenderer->BindDepthStencil(0);
	m_pRenderer->BindRenderTarget(m_postProcessPass._worldRenderTarget);
	m_pRenderer->SetDepthStencilState(0); 
	m_pRenderer->SetRasterizerState(static_cast<int>(DEFAULT_RS_STATE::CULL_NONE));
	m_pRenderer->Begin(clearColor, 1.0f);
	
	m_pRenderer->SetViewport(m_pRenderer->WindowWidth(), m_pRenderer->WindowHeight());
	
	m_pRenderer->SetShader(m_selectedShader);
	m_pRenderer->SetConstant3f("cameraPos", m_pCamera->GetPositionF());

	constexpr int TBNMode = 0;
	if (m_selectedShader == SHADERS::TBN)	m_pRenderer->SetConstant1i("mode", TBNMode);

	m_roomScene.Render(m_pRenderer, viewProj);
	
#if 1
	// POST PROCESS PASS
	//------------------------------------------------------------------------
	m_postProcessPass.Render(m_pRenderer);


	// DEBUG PASS
	//------------------------------------------------------------------------
	if (m_debugRender)
	{
		// TBN test
		//auto prevShader = m_selectedShader;
		//m_selectedShader = m_renderData->TNBShader;
		//RenderCentralObjects(view, proj);
		//m_selectedShader = prevShader;

		m_pRenderer->SetShader(SHADERS::DEBUG);
		m_pRenderer->SetTexture("t_shadowMap", m_depthPass._shadowMap);	// todo: decide shader naming 
		m_pRenderer->SetBufferObj(GEOMETRY::QUAD);
		m_pRenderer->Apply();
		m_pRenderer->DrawIndexed();
	}
#endif
	m_pRenderer->End();
}


void SceneManager::SendLightData() const
{
	// SPOT & POINT LIGHTS
	//--------------------------------------------------------------
	const LightShaderSignature defaultLight = LightShaderSignature();
	std::vector<LightShaderSignature> lights(MAX_LIGHTS, defaultLight);
	std::vector<LightShaderSignature> spots(MAX_SPOTS, defaultLight);
	unsigned spotCount = 0;
	unsigned lightCount = 0;
	for (const Light& l : m_roomScene.m_lights)
	{
		switch (l._type)
		{
		case Light::LightType::POINT:
			lights[lightCount] = l.ShaderLightStruct();
			++lightCount;
			break;
		case Light::LightType::SPOT:
			spots[spotCount] = l.ShaderLightStruct();
			++spotCount;
			break;
		default:
			OutputDebugString("SceneManager::RenderBuilding(): ERROR: UNKOWN LIGHT TYPE\n");
			break;
		}
	}
	m_pRenderer->SetConstant1f("lightCount", static_cast<float>(lightCount));
	m_pRenderer->SetConstant1f("spotCount" , static_cast<float>(spotCount));
	m_pRenderer->SetConstantStruct("lights", static_cast<void*>(lights.data()));
	m_pRenderer->SetConstantStruct("spots" , static_cast<void*>(spots.data()));

	// SHADOW MAPS
	//--------------------------------------------------------------
	// first light is spot: single shadaw map support for now
	const Light& caster = m_roomScene.m_lights[0];
	TextureID shadowMap = m_depthPass._shadowMap;
	SamplerID shadowSampler = m_depthPass._shadowSampler;

	m_pRenderer->SetConstant4x4f("lightSpaceMat", caster.GetLightSpaceMatrix());
	m_pRenderer->SetTexture("gShadowMap", shadowMap);
	m_pRenderer->SetSamplerState("sShadowSampler", shadowSampler);

#ifdef _DEBUG
	if (lights.size() > MAX_LIGHTS)	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (spots.size() > MAX_SPOTS)	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
}
