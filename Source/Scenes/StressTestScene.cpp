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

#include "StressTestScene.h"

#include "Engine/Engine.h"
#include "Engine/Light.h"

#include "Application/Input.h"

#include "Utilities/utils.h"
#include "Utilities/Log.h"

#include "Renderer/Renderer.h"
#include "Renderer/TextRenderer.h"

#include <numeric>
#include <algorithm>

using std::begin;
using std::end;

#pragma region CONSTANTS
//----------------------------------------------------------------------------------------------
// CONSTANTS | SCENE CONFIG
//----------------------------------------------------------------------------------------------

// LIGHTS
constexpr size_t NUM_LIGHT = 30;

// OBJECTS
#ifdef _DEBUG
constexpr size_t NUM_OBJ = 500;
constexpr size_t DIV = 10;
#else
constexpr size_t NUM_OBJ = 5000;
constexpr size_t DIV = 25;
#endif

// TRANSFORMS
const vec3		ORIGIN_OFFSET	= vec3(-200, 20, 0);
constexpr float OFFSET_SCALING	= 20.0f;

#define RANDOMIZE_SCALE 1
constexpr float SCALE_LOW = 1.0f;
constexpr float SCALE_HI = 5.0f;

#define RANDOMIZE_ROTATION 1

// MATERIAL
#define NO_TEXTURES 1
constexpr int TEXTURED_OBJECT_PERCENTAGE = 45;

// MESH
//
// selects a random mesh among built-in meshes 
// when creating the object instances
#define RANDOMIZE_MESH 1

// MODEL
#define LOAD_MODELS 1			// toggle model loading
#define LOAD_MODELS_ASYNC 1		// 30-40% performance gain with the current test data
#define LOAD_FREE3D_MODELS 1	// use this flag if you have the assets (Models/free3D/)
#pragma endregion

#pragma region GLOBALS
//----------------------------------------------------------------------------------------------
// GLOBALS & PROTOTYPES
//----------------------------------------------------------------------------------------------
static Renderer* pRenderer = nullptr;

static int sObjectLayer = 0;

static std::vector<float> rotationSpeeds;

static vec3 centerOfMass;
static std::vector<vec3> objectDisplacements;

// ===============

void AddLights(std::vector<Light>&);
void RemoveLights(std::vector<Light>&);
#pragma endregion

//----------------------------------------------------------------------------------------------
#pragma region SCENE_INTERFACE

#if DO_NOT_LOAD_SCENES
void StressTestScene::Load(SerializedScene& scene) {}
void StressTestScene::Unload() {}
void StressTestScene::Update(float dt) {}
void StressTestScene::RenderUI() const {}
#else

void StressTestScene::Load(SerializedScene& scene)
{
	pRenderer = mpRenderer;

	AddLights(mLights);
	SetEnvironmentMap(EEnvironmentMapPresets::BARCELONA);

	AddObjects();

#if LOAD_MODELS

	std::vector<std::string> ModelLoadList = 
	{
		"sponza/sponza.obj",
		"nanosuit/nanosuit.obj",

		"platform.obj",
		"nanosuit/nanosuit.obj",	// test for multiple entry async loading

		"SaiNarayan/Wanderer/Wanderer_LP.obj",
		"SaiNarayan/CorinthianPillar/column.obj",

		"EgemenIlbeyli/sise2.obj",
		"EgemenIlbeyli/zen_orb.obj",

#if LOAD_FREE3D_MODELS
		"free3D/IronMan/IronMan.obj",
		"free3D/Spider/Only_Spider_with_Animations_Export.obj",

#if !_DEBUG
		"free3D/Room/Room.obj",
#endif
#endif // LOAD_FREE3D_MODELS

	};

	const Quaternion Q_I = Quaternion::Identity();
	const Quaternion Q_Pillar = Quaternion::FromAxisAngle(vec3::Up, DEG2RAD * 20.0f) * Quaternion::FromAxisAngle(vec3::Forward, DEG2RAD * 90.0f);
	const Quaternion Q_Platform = Quaternion::FromAxisAngle(vec3::Right, DEG2RAD * -45.0f);
	const Quaternion Q_Nanosuit = Quaternion::FromAxisAngle(vec3::Up, DEG2RAD * 180.0f);
	const int X_UNITS = 500;
	const int Z_UNITS = 250;
	std::vector<Transform> transforms = 
	{
		Transform(vec3(-X_UNITS, 20, 0), Q_I, vec3(0.08f)),					// sponza
		Transform(vec3(-X_UNITS, 0, Z_UNITS), Q_Nanosuit, vec3(5.0f)),		// nano

		Transform(vec3(+X_UNITS, 20, 0), Q_Platform, vec3(70.0f)),			// platform
		Transform(vec3(+X_UNITS, 0 , Z_UNITS), Q_I, vec3(15.0f)),			// nano (big)

		Transform(vec3(-X_UNITS, 0 , -Z_UNITS), Q_I, vec3(7.0f)),			// wanderer
		Transform(vec3(+X_UNITS, 25, -Z_UNITS), Q_Pillar, vec3(15.0f)),		// pillar

		Transform(vec3(-X_UNITS, 0 , -Z_UNITS* 2), Q_I, vec3(1.0f)),		// bottle
		Transform(vec3(+X_UNITS, 20, -Z_UNITS* 2), Q_I, vec3(1.0f)),		// zen ball

		Transform(vec3(+X_UNITS, 0, +Z_UNITS * 2), Q_I, vec3(1.0f)),		// ironman
		Transform(vec3(0, 10, -Z_UNITS), Q_I, vec3(1.0f)),					// spider
		
#if !_DBEUG
		Transform(vec3(-X_UNITS, 83, +Z_UNITS * 2), Q_I, vec3(0.7f)),		// room
#endif
	};

	int i = 0;
	std::for_each(RANGE(ModelLoadList), [&](const std::string& modelPath)
	{
		GameObject* pObj = Scene::CreateNewGameObject();
		pObj->SetTransform(transforms[i]);
#if LOAD_MODELS_ASYNC
		// Debug: 35s | Release: 4.9s
		Scene::LoadModel_Async(pObj, modelPath);
#else
		// Debug: 55s | Release: 7.3s
		pObj->SetModel(Scene::LoadModel(modelPath));
#endif
		mpLoadedModels.push_back(pObj);
		++i;
	});

#endif	// LOAD_MODELS

	if(!ENGINE->IsRenderingStatsOn()) ENGINE->ToggleRenderingStats();
	if (!ENGINE->IsProfileRenderingOn()) ENGINE->ToggleProfilerRendering();
}

void StressTestScene::Unload()
{
	mLights.clear();
	mpTestObjects.clear();
	objectDisplacements.clear();
	rotationSpeeds.clear();
}

void StressTestScene::Update(float dt)
{
	if (ENGINE->INP()->IsKeyTriggered("+"))
	{
		if (ENGINE->INP()->IsKeyDown("Shift"))
			AddLights(mLights);
		else
			AddObjects();
	}
	if (ENGINE->INP()->IsKeyTriggered("-"))
	{
		if (ENGINE->INP()->IsKeyDown("Shift"))
			RemoveLights(mLights);
		else
			RemoveObjects();
	}

	// old, inactive code
	//
	//if (rotationSpeeds.size() > 0)
	//{
	//	for (size_t i = 0; i < NUM_OBJ; ++i)
	//	{
	//		GameObject& o = mpTestObjects[i];
	//		o.mTransform.RotateAroundGlobalYAxisDegrees(dt * rotationSpeeds[i]);
	//	}
	//}
	
	std::for_each(mpTestObjects.begin(), mpTestObjects.end(), [&](GameObject* o)
	{
		o->RotateAroundGlobalYAxisDegrees(dt * 5);
	});
}


void StressTestScene::RenderUI() const
{
	// helper ui
	if (ENGINE->GetSettingShowControls())
	{
		const int NumObj = std::accumulate(RANGE(mpObjects), 0, [](int val, const GameObject* o) { return val + (o->mRenderSettings.bRender ? 1 : 0); });
		const int NumPointLights = std::accumulate(RANGE(mLights), 0, [](int val, const Light& l) { return val + ( (true/*l._bEnabled*/ && l._type == Light::POINT) ? 1 : 0); });
		const int NumSpotLights  = std::accumulate(RANGE(mLights), 0, [](int val, const Light& l) { return val + ( (true/*l._bEnabled*/ && l._type == Light::ELightType::SPOT) ? 1 : 0); });

		TextDrawDescription drawDesc;
		drawDesc.color = vec3(1, 1, 0.3f);
		drawDesc.scale = 0.35f;
		int numLine = 0;

		Settings::Engine sEngineSettings = ENGINE->GetSettings();

		const float X_POS = sEngineSettings.window.width * 0.72f;
		const float Y_POS = 0.05f * sEngineSettings.window.height;
		const float LINE_HEIGHT = 25.0f;
		vec2 screenPosition(X_POS, Y_POS);

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("CONTROLS");
		mpTextRenderer->RenderText(drawDesc);

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("=========================================");
		mpTextRenderer->RenderText(drawDesc);

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("Add/Remove Objects        : Numpad+/Numpad-");
		mpTextRenderer->RenderText(drawDesc);

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("Add/Remove Point Lights : Shift + Numpad+/Numpad-");
		mpTextRenderer->RenderText(drawDesc);

		++numLine;

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("SCENE STATS");
		mpTextRenderer->RenderText(drawDesc);

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("=========================================");
		mpTextRenderer->RenderText(drawDesc);

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("Objects: ") + std::to_string(NumObj);
		mpTextRenderer->RenderText(drawDesc);

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("Point Lights: ") + std::to_string(NumPointLights);
		mpTextRenderer->RenderText(drawDesc);

		drawDesc.screenPosition = vec2(screenPosition.x(), screenPosition.y() + numLine++ * LINE_HEIGHT);
		drawDesc.text = std::string("Spot Lights: ") + std::to_string(NumSpotLights);
		mpTextRenderer->RenderText(drawDesc);
	}
}
#pragma endregion
//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
#pragma region HELPER_FUNCTIONS

GameObject* StressTestScene::CreateRandomGameObject()
{
	const size_t i = mpTestObjects.size() - 1;
	GameObject* pObj = Scene::CreateNewGameObject();

	// MESH
	//
#if RANDOMIZE_MESH
	const EGeometry meshType = static_cast<EGeometry>(RandI(0, EGeometry::MESH_TYPE_COUNT));
#else
	const EGeometry meshType = EGeometry::SPHERE;
#endif

	// TRANSFORM
	//
	Transform tf;
#if RANDOMIZE_SCALE
	const float RandScale = RandF(SCALE_LOW, SCALE_HI) * (meshType == EGeometry::GRID ? 5.0f : 1.0f);
#else
	const float RandScale = 1.0f;
#endif

	vec3 position = vec3(
		static_cast<float>(i / DIV % DIV),			// X
		static_cast<float>(i%DIV),					// Y
		static_cast<float>(i / (DIV*DIV))			// Z
	)* OFFSET_SCALING
		+ ORIGIN_OFFSET;// +(vec3(0, 20, 0) * (sObjectLayer / 3));

	tf.SetUniformScale(RandScale);
	tf.SetPosition(position);
#if RANDOMIZE_ROTATION
	tf.RotateAroundAxisDegrees(vec3::Rand(), RandF(0, 360));
#endif



	// MATERIAL
	//
	Material* pMat = nullptr;
	if (RandI(0, 100) < 15)
	{
		BRDF_Material* pBRDF = static_cast<BRDF_Material*>(Scene::CreateNewMaterial(GGX_BRDF));
		pBRDF->diffuse = LinearColor::gold;
		pBRDF->metalness = 0.98f;
		pBRDF->roughness = RandF(0.04f, 0.15f);
		pMat = static_cast<Material*>(pBRDF);
	}
	else
	{
		pMat = Scene::CreateRandomMaterialOfType(GGX_BRDF);
	}
	if (RandI(0, 100) < TEXTURED_OBJECT_PERCENTAGE)
	{
		const std::string fileNameDiffuse = "openart/" + std::to_string(RandI(151, 200)) + ".JPG";
		const std::string fileNameNormal = "openart/" + std::to_string(RandI(151, 200)) + "_norm.JPG";
#if !NO_TEXTURES
		m.diffuseMap = pRenderer->CreateTextureFromFile(fileNameDiffuse);
		m.normalMap = pRenderer->CreateTextureFromFile(fileNameNormal);
#endif
	}


	// GAME OBJECT
	//
	pObj->AddMesh(meshType);
	pObj->SetTransform(tf);
	pObj->AddMaterial(pMat);
	return pObj;
}

void StressTestScene::AddObjects()
{
	// TOGGLE VISIBILITY
	//
	bool bAllObjectsRendered = std::all_of(RANGE(mpTestObjects), [](const GameObject* o) { return o->mRenderSettings.bRender; });
	if (!bAllObjectsRendered)
	{
		auto itNonRenderingFirst = std::find_if(RANGE(mpTestObjects), [](GameObject* o) { return !o->mRenderSettings.bRender; });
		for (size_t i = 0; i < NUM_OBJ; ++i)
		{
			(*itNonRenderingFirst)->mRenderSettings.bRender = true;
			++itNonRenderingFirst;
		}
		++sObjectLayer;
		return;
	}
	//------------------------------------------------------------------------------------


	// ADD NEW OBJECTS
	//
	std::vector<vec3> initialPositions;
	for (size_t i = 0; i < NUM_OBJ; ++i)
	{
		GameObject* pObj = CreateRandomGameObject();
		mpTestObjects.push_back(pObj);

		rotationSpeeds.push_back(RandF(5.0f, 15.0f) * (RandF(0, 1) < 0.5f ? 1.0f : -1.0f));
		initialPositions.push_back(pObj->GetPosition());
	}
	std::for_each(mpTestObjects.begin(), mpTestObjects.end(), [&](GameObject* o)
	{
		o->mRenderSettings.bCastShadow = false;
	});

	// CENTER OF MASS
	//
	vec3 total;
	for (size_t i = 0; i < NUM_OBJ; ++i)
	{
		total += initialPositions[i];
	}
	centerOfMass = total / static_cast<float>(initialPositions.size());

	// DISPLACEMENT
	//
	for (size_t i = 0; i < NUM_OBJ; ++i)
	{
		//objectDisplacements.push_back(mpTestObjects[i].mTransform._position - centerOfMass);
		//mpTestObjects[i].mTransform._position += objectDisplacements.back();
	}

	++sObjectLayer;
	//return initialPositions;
}


// Toggles bRender of NUM_OBJ objects starting from the end of the game obj vector going backwards
//
void StressTestScene::RemoveObjects()
{
	// range check
	auto itRenderToggleOn = std::find_if(RRANGE(mpTestObjects), [](GameObject* o) { return o->mRenderSettings.bRender; });
	if (itRenderToggleOn == mpTestObjects.rend())
		return;
	
	for (size_t i = 0; i < NUM_OBJ; ++i)
	{
		(*itRenderToggleOn)->mRenderSettings.bRender = false;
		++itRenderToggleOn;
	}

	--sObjectLayer;
}


Light CreateRandomPointLight()
{
	const Light::ELightType type = Light::ELightType::POINT;
	const LinearColor color = LinearColor::white;
	const float range = RandF(300, 1500.f);
	const float bright = RandF(100, 300);

	Light l = Light(type, color, range, bright, 0.0f, false);
	l._transform.SetPosition(RandF(-50, 50), RandF(30, 90), RandF(-40, -90));
	l._transform.SetUniformScale(0.1f);

	return l;
}


void AddLights(std::vector<Light>& mLights)
{
	return;
	//bool bAllLightsEnabled = std::all_of(RANGE(mLights), [](const Light& l) { return l._bEnabled; });
	//if (!bAllLightsEnabled)
	//{
	//	auto itDisabledLight = std::find_if(RANGE(mLights), [](Light& l) { return !l._bEnabled; });
	//	for (size_t i = 0; i < NUM_LIGHT + 3; ++i)
	//	{
	//		itDisabledLight->_bEnabled = true;
	//		++itDisabledLight;
	//	}
	//	return;
	//}
	//----------------------------------

	if (mLights.size() + NUM_LIGHT > NUM_POINT_LIGHT)
	{
		Log::Warning("Maximum light count reached: %d", NUM_POINT_LIGHT);
		return;
	}

	for (size_t i = 0; i < NUM_LIGHT; ++i)
	{
		mLights.push_back(CreateRandomPointLight());
	}
	for (size_t i = 0; i < 3; ++i)
	{
		const Light::ELightType type = Light::ELightType::POINT;
		const LinearColor color = [&]()
		{
			switch (i)
			{
			case 0: return (LinearColor)(vec3(LinearColor::red));
			case 1: return LinearColor::green;
			case 2: return LinearColor::blue;
			}
			return LinearColor::white;
		}();
		const float range = RandF(800, 1500.f);
		const float bright = RandF(2500, 5500);

		Light l = Light(type, color, range, bright, 0.0f, false);
		l._transform.SetPosition(RandF(-70, 40), RandF(30, 90), RandF(-40, -90));
		l._transform.SetUniformScale(0.3f);
		mLights.push_back(l);
	}
}

void RemoveLights(std::vector<Light>& mLights)
{
	return;
	//auto itEnabledLight = std::find_if(RRANGE(mLights), [](Light& l) { return l._bEnabled && l._type == Light::POINT; });
	//if (itEnabledLight == mLights.rend())
	//	return;
	//
	//for (size_t i = 0; i < NUM_LIGHT+3; ++i)
	//{
	//	itEnabledLight->_bEnabled = false;
	//	++itEnabledLight;
	//}
}
#pragma endregion
//----------------------------------------------------------------------------------------------

#endif