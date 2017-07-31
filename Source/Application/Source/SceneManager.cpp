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



void DepthShadowPass::Initialize(Renderer* pRenderer, ID3D11Device* device)
{
	this->_shadowMapDimension = 1024;

	// check feature support & error handle:
	// https://msdn.microsoft.com/en-us/library/windows/apps/dn263150

	// create shadow map texture for the pixel shader stage
	D3D11_TEXTURE2D_DESC shadowMapDesc;
	ZeroMemory(&shadowMapDesc, sizeof(D3D11_TEXTURE2D_DESC));
	shadowMapDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	shadowMapDesc.MipLevels = 1;
	shadowMapDesc.ArraySize = 1;
	shadowMapDesc.SampleDesc.Count = 1;
	shadowMapDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
	shadowMapDesc.Height = static_cast<UINT>(_shadowMapDimension);
	shadowMapDesc.Width = static_cast<UINT>(_shadowMapDimension);
	this->_shadowMap = pRenderer->CreateTexture2D(shadowMapDesc, false);

	// careful: removing const qualified from texture. rethink this
	Texture& shadowMap = const_cast<Texture&>(pRenderer->GetTextureObject(_shadowMap));

	// depth stencil view and shader resource view for the shadow map (^ BindFlags)
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;			// check format
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	this->_dsv = pRenderer->AddDepthStencil(dsvDesc, shadowMap._tex2D);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.Texture2D.MipLevels = 1;

	HRESULT hr = device->CreateShaderResourceView(shadowMap._tex2D, &srvDesc, &shadowMap._srv);
	if (FAILED(hr))
	{
		Log::Error(ERROR_LOG::CANT_CREATE_RESOURCE, "Cannot create SRV in InitilizeDepthPass\n");

	}

	// comparison sampler
	D3D11_SAMPLER_DESC comparisonSamplerDesc;
	ZeroMemory(&comparisonSamplerDesc, sizeof(D3D11_SAMPLER_DESC));
	comparisonSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	comparisonSamplerDesc.BorderColor[0] = 1.0f;
	comparisonSamplerDesc.BorderColor[1] = 1.0f;
	comparisonSamplerDesc.BorderColor[2] = 1.0f;
	comparisonSamplerDesc.BorderColor[3] = 1.0f;
	comparisonSamplerDesc.MinLOD = 0.f;
	comparisonSamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	comparisonSamplerDesc.MipLODBias = 0.f;
	comparisonSamplerDesc.MaxAnisotropy = 0;
	//comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;	// todo: set default sampler elsewhere
	comparisonSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	//comparisonSamplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
	comparisonSamplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	this->_shadowSampler = pRenderer->CreateSamplerState(comparisonSamplerDesc);

	// render states for front face culling 
	this->_shadowRenderState = pRenderer->AddRSState(RS_CULL_MODE::FRONT, RS_FILL_MODE::SOLID, true);
	this->_drawRenderState   = pRenderer->AddRSState(RS_CULL_MODE::BACK, RS_FILL_MODE::SOLID, true);

	// shader
	std::vector<InputLayout> layout = {
		{ "POSITION",	FLOAT32_3 },
		{ "NORMAL",		FLOAT32_3 },
		{ "TANGENT",	FLOAT32_3 },
		{ "TEXCOORD",	FLOAT32_2 } };
	this->_shadowShader = pRenderer->GetShader(pRenderer->AddShader("DepthShader", pRenderer->s_shaderRoot, layout, false));

	ZeroMemory(&_shadowViewport, sizeof(D3D11_VIEWPORT));
	_shadowViewport.Height = static_cast<float>(_shadowMapDimension);
	_shadowViewport.Width = static_cast<float>(_shadowMapDimension);
	_shadowViewport.MinDepth = 0.f;
	_shadowViewport.MaxDepth = 1.f;
}

void DepthShadowPass::RenderDepth(Renderer* pRenderer, const std::vector<const Light*> shadowLights, const std::vector<GameObject*> ZPassObjects) const
{
	const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	pRenderer->UnbindRenderTarget();						// no render target
	pRenderer->BindDepthStencil(_dsv);						// only depth stencil buffer
	pRenderer->SetRasterizerState(_shadowRenderState);		// shadow render state: cull front faces, fill solid, clip dep
	pRenderer->SetViewport(_shadowViewport);				// lights viewport 512x512
	pRenderer->SetShader(_shadowShader->ID());				// shader for rendering z buffer
	pRenderer->SetConstant4x4f("viewProj", shadowLights.front()->GetLightSpaceMatrix());
	pRenderer->SetConstant4x4f("view", shadowLights.front()->GetViewMatrix());
	pRenderer->SetConstant4x4f("proj", shadowLights.front()->GetProjectionMatrix());
	pRenderer->Apply();
	pRenderer->Begin(clearColor, 1.0f);
	size_t idx = 0;
	for (const GameObject* obj : ZPassObjects)
	{
		obj->RenderZ(pRenderer);
	}
}

void PostProcessPass::Initialize(Renderer * pRenderer, ID3D11Device * device)
{
	DXGI_SAMPLE_DESC smpDesc;
	smpDesc.Count = 1;
	smpDesc.Quality = 0;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D11_TEXTURE2D_DESC rtDesc;
	ZeroMemory(&rtDesc, sizeof(rtDesc));
	rtDesc.Width = pRenderer->WindowWidth();
	rtDesc.Height = pRenderer->WindowHeight();
	rtDesc.MipLevels = 1;
	rtDesc.ArraySize = 1;
	rtDesc.Format = format;
	rtDesc.Usage = D3D11_USAGE_DEFAULT;
	rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	rtDesc.CPUAccessFlags = 0;
	rtDesc.SampleDesc = smpDesc;
	rtDesc.MiscFlags = 0;

	D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
	ZeroMemory(&RTVDesc, sizeof(RTVDesc));
	RTVDesc.Format = format;
	RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	RTVDesc.Texture2D.MipSlice = 0;

	this->_finalRenderTarget   = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._colorRT  = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._brightRT = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._blurPingPong[0] = pRenderer->AddRenderTarget(rtDesc, RTVDesc);
	this->_bloomPass._blurPingPong[1] = pRenderer->AddRenderTarget(rtDesc, RTVDesc);

	D3D11_SAMPLER_DESC blurSamplerDesc;
	ZeroMemory(&blurSamplerDesc, sizeof(blurSamplerDesc));
	blurSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	blurSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	blurSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	blurSamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	this->_bloomPass._blurSampler = pRenderer->CreateSamplerState(blurSamplerDesc);
}

void PostProcessPass::Render(Renderer * pRenderer) const
{
	const TextureID worldTexture = pRenderer->GetDefaultRenderTargetTexture();

	pRenderer->SetShader(SHADERS::BLOOM);
	pRenderer->BindRenderTargets(_bloomPass._colorRT, _bloomPass._brightRT);
	pRenderer->UnbindDepthStencil();
	pRenderer->SetBufferObj(GEOMETRY::QUAD);
	pRenderer->Apply();
	pRenderer->SetTexture("worldRenderTarget", worldTexture);
	pRenderer->Apply();
	pRenderer->DrawIndexed();

	constexpr size_t BLUR_PASS_COUNT = 10;
	const TextureID brightTexture = pRenderer->GetRenderTargetTexture(_bloomPass._brightRT);

	pRenderer->SetShader(SHADERS::BLUR);
	for (size_t i = 0; i < BLUR_PASS_COUNT; i++)
	{
		const int isHorizontal = i % 2;
		const TextureID pingPong = pRenderer->GetRenderTargetTexture(_bloomPass._blurPingPong[1 - isHorizontal]);
		const int texWidth = pRenderer->GetTextureObject(pingPong)._width;
		const int texHeight = pRenderer->GetTextureObject(pingPong)._height;

		pRenderer->UnbindRenderTarget();
		pRenderer->Apply();
		pRenderer->BindRenderTarget(_bloomPass._blurPingPong[isHorizontal]);
		pRenderer->SetConstant1i("isHorizontal", 1 - isHorizontal);
		pRenderer->SetConstant1i("textureWidth" , texWidth);
		pRenderer->SetConstant1i("textureHeight", texHeight);
		pRenderer->SetTexture("InputTexture", i == 0 ? brightTexture : pingPong);
		pRenderer->SetSamplerState("BlurSampler", _bloomPass._blurSampler);
		pRenderer->Apply();
		pRenderer->DrawIndexed();
	}

	// todo: blend blurred blright color + world render target
	// blend state or shader????
	//pRenderer->BindRenderTarget(_finalRenderTarget);

}

//=======================================================================================

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
	m_selectedShader	= SHADERS::FORWARD_PHONG;
	m_gammaCorrection	= true;
	m_debugRender		= false;
	Skybox::InitializePresets(m_pRenderer);

	m_roomScene.Load(m_pRenderer);

	// todo: static game object array in gameobj.h 
	m_depthPass.Initialize(m_pRenderer, m_pRenderer->m_device);
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
	// F1-F4 | Debug Shaders
	if (ENGINE->INP()->IsKeyTriggered(112)) m_selectedShader = SHADERS::TEXTURE_COORDINATES;
	if (ENGINE->INP()->IsKeyTriggered(113)) m_selectedShader = SHADERS::NORMAL;
	//if (ENGINE->INP()->IsKeyTriggered(114)) m_selectedShader = SHADERS::TANGENT;
	if (ENGINE->INP()->IsKeyTriggered(114)) m_roomScene.ToggleFloorNormalMap();;
	if (ENGINE->INP()->IsKeyTriggered(115)) m_selectedShader = SHADERS::BINORMAL;
	
	// F5-F8 | Lighting Shaders								   
	if (ENGINE->INP()->IsKeyTriggered(116)) m_selectedShader = SHADERS::UNLIT;
	if (ENGINE->INP()->IsKeyTriggered(117)) m_selectedShader = SHADERS::FORWARD_PHONG;
	if (ENGINE->INP()->IsKeyTriggered(118)) m_debugRender = !m_debugRender;

	// F9-F12 | Shader Parameters
	if (ENGINE->INP()->IsKeyTriggered(120)) m_gammaCorrection = !m_gammaCorrection;
	if (ENGINE->INP()->IsKeyTriggered(120)) m_gammaCorrection = !m_gammaCorrection;
	if (ENGINE->INP()->IsKeyTriggered(122)) m_gammaCorrection = !m_gammaCorrection;
	//if (ENGINE->INP()->IsKeyTriggered(123)) 
		
	//if (ENGINE->INP()->IsKeyTriggered(52)) TBNMode = 3;

}
//-----------------------------------------------------------------------------------------------------------------------------------------

void SceneManager::Render() const
{
	const float clearColor[4] = { 0.2f, 0.4f, 0.7f, 1.0f };
	const XMMATRIX view = m_pCamera->GetViewMatrix();
	const XMMATRIX proj = m_pCamera->GetProjectionMatrix();
	const XMMATRIX viewProj = view * proj;


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
	
	// MAIN PASS
	//------------------------------------------------------------------------
	m_pRenderer->Reset();	// is necessary?
	m_pRenderer->BindDepthStencil(0); //todo, variable names or enums
	m_pRenderer->BindRenderTarget(0);
	m_pRenderer->SetDepthStencilState(0); 
	m_pRenderer->SetRasterizerState(static_cast<int>(DEFAULT_RS_STATE::CULL_NONE));
	m_pRenderer->Begin(clearColor, 1.0f);
	
	m_pRenderer->SetViewport(m_pRenderer->WindowWidth(), m_pRenderer->WindowHeight());
	
	m_pRenderer->SetShader(m_selectedShader);
	m_pRenderer->SetConstant1f("gammaCorrection", m_gammaCorrection ? 1.0f : 0.0f);
	m_pRenderer->SetConstant3f("cameraPos", m_pCamera->GetPositionF());

	constexpr int TBNMode = 0;
	if (m_selectedShader == SHADERS::TBN)	m_pRenderer->SetConstant1i("mode", TBNMode);

	m_roomScene.Render(m_pRenderer, viewProj);
	

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
