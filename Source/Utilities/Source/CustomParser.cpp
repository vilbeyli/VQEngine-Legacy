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

#include "CustomParser.h"
#include "utils.h"
#include "Log.h"
#include "Color.h"

#include "Renderer/Renderer.h"

#include <fstream>
#include <unordered_map>
#include <algorithm>


const std::string file_root		= "Data\\";
const std::string scene_root	= "Data\\ScenesFiles\\";

const std::unordered_map<std::string, bool> sBoolTypeReflection
{
	{"true", true},		{"false", false},
	{"yes", true},		{"no", false},
	{"1", true},		{"0", false}
};

std::string GetLowercased(const std::string& str)
{
	std::string lowercase(str);
	std::transform(str.begin(), str.end(), lowercase.begin(), ::tolower);
	return lowercase;
}


Parser::Parser(){}

Parser::~Parser(){}

Settings::Engine Parser::ReadSettings(const std::string& settingsFileName)
{
	const std::string filePath = settingsFileName;
	Settings::Engine setting;

	std::ifstream settingsFile(filePath.c_str());
	if (settingsFile.is_open())
	{
		std::string	line;
		while (getline(settingsFile, line))
		{
			if (line.empty()) continue;
			if (line[0] == '/' || line[0] == '#')					// skip comments
				continue;

			std::vector<std::string> command = StrUtil::split(line, ' ');	// ignore whitespace
			ParseSetting(command, setting);							// process command
		}
		Log::Info("Initialized engine settings.");
	}
	else
	{
		Log::Error("Settings.ini can't be opened.");
	}

	return setting;
}

void Parser::ParseSetting(const std::vector<std::string>& line, Settings::Engine& settings)
{
	if (line.empty())
	{
		Log::Error("Empty Command in ParseSettings().");
		return;
	}

	const std::string& cmd = line[0];
	if (cmd == "window")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Window Width	|  Window Height	| Fullscreen?	| VSYNC?
		//---------------------------------------------------------------
		settings.window.width      = stoi(line[1]);
		settings.window.height     = stoi(line[2]);
		settings.window.fullscreen = stoi(line[3]);
		settings.window.vsync      = stoi(line[4]);
	}
	else if (cmd == "logger" || cmd == "logging" || cmd == "log")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Logger	|  Use Console Window	| Use File in AppData\VQEngine
		//---------------------------------------------------------------
		settings.logger.bConsole = sBoolTypeReflection.at(line[1]);
		settings.logger.bFile    = sBoolTypeReflection.at(line[2]);
	}
	else if (cmd == "shadowMap")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Shadow Map dimension
		//---------------------------------------------------------------
		settings.rendering.shadowMap.spotShadowMapDimensions = stoi(line[1]);
		settings.rendering.shadowMap.directionalShadowMapDimensions = stoi(line[2]);
		settings.rendering.shadowMap.pointShadowMapDimensions = stoi(line[3]);
	}
	else if (cmd == "lightingModel")
	{
		// Parameters
		//---------------------------------------------------------------
		// | phong/brdf
		//---------------------------------------------------------------
		const std::string lightingModel = GetLowercased(line[1]);
		settings.rendering.bUseBRDFLighting =  lightingModel == "brdf";
	}
	else if (cmd == "deferredRendering")
	{
		settings.rendering.bUseDeferredRendering = sBoolTypeReflection.at(line[1]);
	}
	else if (cmd == "ambientOcclusion")
	{
		settings.rendering.bAmbientOcclusion= sBoolTypeReflection.at(line[1]);
	}
	else if (cmd == "tonemapping")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Exposure
		//---------------------------------------------------------------
		settings.rendering.postProcess.toneMapping.exposure = stof(line[1]);
	}
	else if (cmd == "environmentMapping")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Environment Mapping enabled?
		//---------------------------------------------------------------
		settings.rendering.bEnableEnvironmentLighting = sBoolTypeReflection.at(GetLowercased(line[1]));
		settings.rendering.bPreLoadEnvironmentMaps	  = sBoolTypeReflection.at(GetLowercased(line[2]));
		if(line.size() > 3)
			settings.bCacheEnvironmentMapsOnDisk          = sBoolTypeReflection.at(GetLowercased(line[3]));
#if _DEBUG
		settings.rendering.bPreLoadEnvironmentMaps = false;
#endif
	}
	else if (cmd == "HDR")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Enabled?
		//---------------------------------------------------------------
		settings.rendering.postProcess.HDREnabled = sBoolTypeReflection.at(GetLowercased(line[1]));
	}
	else if (cmd == "levels")
	{
		const size_t countScenes = line.size() - 1;
		for (size_t i = 0; i < countScenes; ++i)
		{
			// all lines except the las one will contain a ',' in the end, e.g. "Objects.scn,"
			// the following will remove the commas except for the last scene in the line[] array
			settings.sceneNames.push_back(i == countScenes - 1
				? line[i + 1]
				: StrUtil::split(line[i + 1], ',')[0]	
			);
		}
	}
	else if (cmd == "level")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Enabled?
		//---------------------------------------------------------------
		settings.levelToLoad = stoi(line[1]) - 1;	// input file assumes 1 as first index
		if (settings.levelToLoad == -1) settings.levelToLoad = 0;
	}
	else
	{
		Log::Error("Setting Parser: Unknown command: %s", cmd);
		return;
	}
}

SerializedScene Parser::ReadScene(Renderer* pRenderer, const std::string& sceneFileName)
{
	SerializedScene scene;
	std::string filePath = scene_root + sceneFileName;
	std::ifstream sceneFile(filePath.c_str());

	scene.materials.Clear();
	scene.materials.Initialize(4096);
	scene.directionalLight.mbEnabled = false;

	if (sceneFile.is_open())
	{
		std::string	line;
		while (getline(sceneFile, line))
		{
			if (line[0] == '/' || line[0] == '#' || line[0] == '\0')	// skip comments
				continue;

			std::vector<std::string> command = StrUtil::split(line, ' ', '\t');	// ignore whitespace
			ParseScene(pRenderer, command, scene);						// process command
		}
		scene.loadSuccess = '1';
	}
	else
	{
		Log::Error("Cannot open scene file: %s", filePath.c_str());
		scene.loadSuccess = '0';
	}

	sceneFile.close();
	return scene;
}


// Scene File Formatting:
// ---------------------------------------------------------------------------------------------------------------
// - all lowercase
// - '//' starts a comment

// Object initializations
// ---------------------------------------------------------------------------------------------------------------
// Transform	: pos(3), rot(3:euler), scale(1:uniform|3:xyz)
// Camera		: near far vfov  pos(3:xyz)  yaw pitch
// Light		: [p]oint/[s]pot,  color,   shadowing?  brightness,  range/angle,      pos(3),            rot(X>Y>Z)
// BRDF			:
// Phong		:
// Object		: transform, brdf/phong, mesh
// ---------------------------------------------------------------------------------------------------------------

// state tracking
static bool bIsReadingGameObject = false;
static bool bIsReadingLight = false;
static bool bIsReadingMaterial = false;

enum MaterialType { UNKNOWN, BRDF, PHONG };
static MaterialType materialType = MaterialType::UNKNOWN;
static Material* pMaterial = nullptr;
static GameObject* pObject = nullptr;

Light sLight = Light();

using ParseFunctionType = void(__cdecl *)(const std::vector<std::string>&);
using ParseFunctionLookup = std::unordered_map<std::string, ParseFunctionType>;

static const std::unordered_map<std::string, Light::ELightType>	sLightTypeLookup
{ 
	{"s", Light::ELightType::SPOT },
	{"p", Light::ELightType::POINT},
	{"d", Light::ELightType::DIRECTIONAL},

	{"spot", Light::ELightType::SPOT },
	{"point", Light::ELightType::POINT},
	{"directional", Light::ELightType::DIRECTIONAL},
};

static const std::unordered_map<std::string, const LinearColor&>		sColorLookup
{
	{"orange"    , LinearColor::s_palette[ static_cast<int>(EColorValue::ORANGE     )]},
	{"black"     , LinearColor::s_palette[ static_cast<int>(EColorValue::BLACK      )]},
	{"white"     , LinearColor::s_palette[ static_cast<int>(EColorValue::WHITE      )]},
	{"red"       , LinearColor::s_palette[ static_cast<int>(EColorValue::RED        )]},
	{"green"     , LinearColor::s_palette[ static_cast<int>(EColorValue::GREEN      )]},
	{"blue"      , LinearColor::s_palette[ static_cast<int>(EColorValue::BLUE       )]},
	{"yellow"    , LinearColor::s_palette[ static_cast<int>(EColorValue::YELLOW     )]},
	{"magenta"   , LinearColor::s_palette[ static_cast<int>(EColorValue::MAGENTA    )]},
	{"cyan"      , LinearColor::s_palette[ static_cast<int>(EColorValue::CYAN       )]},
	{"gray"      , LinearColor::s_palette[ static_cast<int>(EColorValue::GRAY       )]},
	{"light_gray", LinearColor::s_palette[ static_cast<int>(EColorValue::LIGHT_GRAY )]},
	{"orange"    , LinearColor::s_palette[ static_cast<int>(EColorValue::ORANGE     )]},
	{"purple"    , LinearColor::s_palette[ static_cast<int>(EColorValue::PURPLE     )]},
	{"sun"       , LinearColor::s_palette[ static_cast<int>(EColorValue::SUN        )]}
};

// todo: get rid of else ifs for cmd == "" comparison... 
// perhaps use a function lookup?
// or define static functions in corresponding objects?
// might as well use some preprocessor magic... 

//static const ParseFunctionLookup sParseCommandFn =
//{	
//	{"camera", [](const std::vector<std::string>& cmds) -> void 
//	{
//		const auto& cmd = cmds[0];
//		
//	}
//	}
//};

void Parser::ParseScene(Renderer* pRenderer, const std::vector<std::string>& command, SerializedScene& scene)
{
	if (command.empty()){return;}
	const std::string& cmd = command[0];	// shorthand
	if (cmd == "camera") // there's a lot of else's... :( (todo: use a map of function pointers...)
	{
		if (command.size() != 9)
		{
			Log::Info("camera input parameter count != 4");
			assert(command.size() == 9);
		}
		// #Parameters: 3
		//--------------------------------------------------------------
		// |  Near Plane	| Far Plane	|	Field of View	| Position | Rotation
		//--------------------------------------------------------------
		Settings::Camera camSettings;
		camSettings.nearPlane	= stof(command[1]);
		camSettings.farPlane	= stof(command[2]);
		camSettings.fovV		= stof(command[3]);
		camSettings.x           = stof(command[4]);
		camSettings.y           = stof(command[5]);
		camSettings.z           = stof(command[6]);
		camSettings.yaw         = stof(command[7]);
		camSettings.pitch       = stof(command[8]);
		scene.cameras.push_back(camSettings);
	}
	else if (cmd == "light")
	{
		// #Parameters: 2
		//--------------------------------------------------------------
		// begin/end
		//--------------------------------------------------------------
		const std::string objCmd = GetLowercased(command[1]);
		if (objCmd == "begin")
		{
			if (bIsReadingLight)
			{
				Log::Error(" expecting \"light end\" before starting a new light definition");
				return;
			}
			bIsReadingLight = true; 
			sLight = Light();
		}

		if (objCmd == "end")
		{
			if (!bIsReadingLight)
			{
				Log::Error(" expecting \"light begin\" before ending a light definition");
				return;
			}
			bIsReadingLight = false;
			
			if (sLight.mType == Light::ELightType::DIRECTIONAL)
				scene.directionalLight = sLight;
			else
				scene.lights.push_back(sLight);
		}
	}

	else if (cmd == "object")
	{
		// #Parameters: 2
		//--------------------------------------------------------------
		// begin/end
		//--------------------------------------------------------------
		const std::string objCmd = GetLowercased(command[1]);
		if (objCmd == "begin")
		{
			if (bIsReadingGameObject)
			{
				Log::Error(" expecting \"object end\" before starting a new object definition");
				return;
			}
			bIsReadingGameObject = true;
			pObject = scene.CreateNewGameObject();
		}

		if (objCmd == "end")
		{
			if (!bIsReadingGameObject)
			{
				Log::Error(" expecting \"object begin\" before ending an object definition");
				return;
			}
			bIsReadingGameObject = false;
			pObject = nullptr;
		}
	}


	// material
	else if (cmd == "mesh")
	{
		// #Parameters: 1
		//--------------------------------------------------------------
		// Mesh Name: Cube/Quad/Sphere/Grid/...
		//--------------------------------------------------------------
		if (!bIsReadingGameObject)
		{
			Log::Error(" Creating mesh without defining a game object (missing cmd: \"%s\"", "object begin");
			return;
		}
		static const std::unordered_map<std::string, EGeometry>		sMeshLookup
		{
			{"triangle" , EGeometry::TRIANGLE	},
			{"quad"     , EGeometry::QUAD		},
			{"cube"     , EGeometry::CUBE		},
			{"sphere"   , EGeometry::SPHERE		},
			{"grid"     , EGeometry::GRID		},
			{"cylinder" , EGeometry::CYLINDER	}
			// todo: assimp mesh
		};
		const std::string mesh = GetLowercased(command[1]);

		
		pObject->AddMesh(sMeshLookup.at(mesh));
		pObject->mModel.mbLoaded = true; // assume model is already loaded as we're using a builtin mesh.
		//obj.mModel.mMeshID = sMeshLookup.at(mesh);
	}
	else if (cmd == "brdf")
	{
		// #Parameters: 0
		//--------------------------------------------------------------
		if (!bIsReadingGameObject)
		{
			Log::Error(" Creating BRDF Material without defining a game object (missing cmd: \"%s\"", "object begin");
			return;
		}
		if (bIsReadingMaterial)
		{
			if (materialType != BRDF)
			{
				Log::Error(" Syntax Error: Already defining a Phong material!");
				return;
			}

			materialType = UNKNOWN;
			bIsReadingMaterial = false;
			return;
		}

		bIsReadingMaterial = true;
		materialType = BRDF;
		pMaterial = scene.materials.CreateAndGetMaterial(GGX_BRDF);
		pObject->AddMaterial(pMaterial);
	}
	else if (cmd == "blinnphong" || cmd == "phong")
	{
		Log::Info("Todo: blinnphong mat");
		return;
		// #Parameters: 0
		//--------------------------------------------------------------
		if (!bIsReadingGameObject)
		{
			Log::Error(" Creating BlinnPhong Material without defining a game object (missing cmd: \"%s\"", "object begin");
			return;
		}
		if (bIsReadingMaterial)
		{
			if (materialType != PHONG)
			{
				Log::Error(" Syntax Error: Already defining a brdf material!");
				return;
			}

			materialType = UNKNOWN;
			bIsReadingMaterial = false;
			return;
		}

		bIsReadingMaterial = true;
		materialType = PHONG;
		pMaterial = scene.materials.CreateAndGetMaterial(BLINN_PHONG);
		pObject->AddMaterial(pMaterial);
	}

	else if (cmd == "diffuse" || cmd == "albedo")
	{
		if (!bIsReadingMaterial)
		{
			Log::Error(" Cannot define Material Property: %s", cmd.c_str());
			return;
		}

		// #Parameters: 4 (1 optional)
		//--------------------------------------------------------------
		// r g b a
		//--------------------------------------------------------------
		const std::string firstParam = GetLowercased(command[1]);
		if (DirectoryUtil::IsImageName(firstParam))
		{
			const TextureID texDiffuse = pRenderer->CreateTextureFromFile(firstParam);
			pMaterial->diffuseMap = texDiffuse;
		}
		else
		{
			assert(command.size() >= 4); // albedo r g b a(optional)
			const float r = stof(command[1]);
			const float g = stof(command[2]);
			const float b = stof(command[3]);
			
			if (command.size() == 5)
			{
				const float a = stof(command[4]);
				pMaterial->diffuse = LinearColor(r, g, b);
				pMaterial->alpha = a;
			}
			else
			{
				pMaterial->diffuse = LinearColor(r, g, b);
			}
		}
	}
	else if (cmd == "tiling")
	{
		if (!bIsReadingMaterial)
		{
			Log::Error(" Cannot define Material Property: %s", cmd.c_str());
			return;
		}

		// #Parameters: 2 (1 optional)
		//--------------------------------------------------------------
		// tiling(@parm1, @param1) | OR | tiling(@param1, @param2)
		//--------------------------------------------------------------
		const float tiling1 = stof(command[1]);
		const float tiling2 = command.size() > 2 ? stof(command[2]) : tiling1;
		pMaterial->tiling = vec2(tiling1, tiling2);
	}
	else if (cmd == "roughness")
	{
		if (!bIsReadingMaterial || materialType != BRDF)
		{
			Log::Error(" Cannot define Material Property: roughness ");
			return;
		}
		// #Parameters: 1
		//--------------------------------------------------------------
		// roughness [0.0f, 1.0f]
		//--------------------------------------------------------------
		static_cast<BRDF_Material*>(pMaterial)->roughness = stof(command[1]);
	}
	else if (cmd == "metalness")
	{
		if (!bIsReadingMaterial || materialType != BRDF)
		{
			Log::Error(" Cannot define Material Property: metalness ");
			return;
		}
		// #Parameters: 1
		//--------------------------------------------------------------
		// metalness [0.0f, 1.0f]
		//--------------------------------------------------------------
		static_cast<BRDF_Material*>(pMaterial)->metalness = stof(command[1]);
	}
	else if (cmd == "shininess")
	{
		if (!bIsReadingMaterial || materialType != PHONG)
		{
			Log::Error(" Cannot define Material Property: shininess ");
			return;
		}
		// #Parameters: 1
		//--------------------------------------------------------------
		// shininess [0.04 - inf]
		//--------------------------------------------------------------
		static_cast<BlinnPhong_Material*>(pMaterial)->shininess = stof(command[1]);
	}
	else if (cmd == "textures")
	{
		if (!bIsReadingMaterial)
		{
			Log::Error(" Cannot define Material Property: textures ");
			return;
		}

		// #Parameters: 2 (1 optional)
		//--------------------------------------------------------------
		// albedoMap normalMap
		//--------------------------------------------------------------
		if (command[1] != "\"\"")
		{
			const TextureID texDiffuse = pRenderer->CreateTextureFromFile(command[1]);
			pMaterial->diffuseMap = texDiffuse;
		}

		if (command.size() > 2)
		{
			const TextureID texNormal = pRenderer->CreateTextureFromFile(command[2]);
			pMaterial->normalMap= texNormal;
		}

		if (command.size() > 3)
		{
			// add various maps (specular etc)
		}
	}

	// light
	else if (cmd == "type")
	{
		std::string lowercaseTypeValue = command[1];
		std::transform(RANGE(lowercaseTypeValue), lowercaseTypeValue.begin(), ::tolower);
		if (sLightTypeLookup.find(lowercaseTypeValue) == sLightTypeLookup.end())
		{
			Log::Warning("Invalid light type: %s", command[1]);
		}
		else
		{
			sLight.mType = sLightTypeLookup.at(lowercaseTypeValue);
		}
	}
	else if (cmd == "color")
	{
		std::string lowercaseTypeValue = command[1];
		std::transform(RANGE(lowercaseTypeValue), lowercaseTypeValue.begin(), ::tolower);
		if (sColorLookup.find(lowercaseTypeValue) == sColorLookup.end())
		{
			Log::Warning("Unknown color: %s", command[1]);
		}
		else
		{
			sLight.mColor = sColorLookup.at(lowercaseTypeValue);
		}
	}
	else if (cmd == "brightness")
	{
		sLight.mBrightness = stof(command[1]);
	}
	else if (cmd == "shadows")
	{
		sLight.mbCastingShadows = sBoolTypeReflection.at(command[1]);
		sLight.mDepthBias = command.size() > 2 ? stof(command[2]) : 0.15f;
		sLight.mNearPlaneDistance = command.size() > 3 ? stof(command[3]) : 0.01f;
		sLight.mFarPlaneDistance  = command.size() > 4 ? stof(command[4]) : 1000.0f;
	}
	else if (cmd == "range")
	{
		sLight.mRange = stof(command[1]);
	}
	else if (cmd == "spot")
	{
		sLight.mSpotOuterConeAngleDegrees = stof(command[1]);
		sLight.mSpotInnerConeAngleDegrees= stof(command[2]);
	}
	else if (cmd == "directional")
	{
		sLight.mViewportX = sLight.mViewportY = stof(command[1]);
		sLight.mDistanceFromOrigin = stof(command[2]);
	}
	else if (cmd == "attenuation")
	{
		sLight.mAttenuationConstant = stof(command[1]);
		if (command.size() > 2) sLight.mAttenuationLinear = stof(command[2]);
		if (command.size() > 3) sLight.mAttenuationQuadratic = stof(command[3]);
	}
	else if (cmd == "transform")
	{
		// #Parameters: 7-9
		//--------------------------------------------------------------
		// Position(3), Rotation(3), UniformScale(1)/Scale(3)
		//--------------------------------------------------------------
		if (!bIsReadingGameObject && !bIsReadingLight)
		{
			Log::Error(" Creating Transform without defining a game object (missing cmd: \"%s\"), or a light (missing cmd: \"light begin\")", "object begin");
			return;
		}
		
		Transform tf;
		float x	= stof(command[1]);
		float y = stof(command[2]);
		float z = stof(command[3]);
		tf.SetPosition(x, y, z);

		float rotX = stof(command[4]);
		float rotY = stof(command[5]);
		float rotZ = stof(command[6]);
		tf.RotateAroundGlobalXAxisDegrees(rotX);
		tf.RotateAroundGlobalYAxisDegrees(rotY);
		tf.RotateAroundGlobalZAxisDegrees(rotZ);

		float sclX = stof(command[7]);
		if (command.size() <= 8)
		{
			tf.SetUniformScale(sclX);
		}
		else
		{
			float sclY = stof(command[8]);
			float sclZ = stof(command[9]);
			tf.SetScale(sclX, sclY, sclZ);
		}

		if(bIsReadingGameObject)
			pObject->SetTransform(tf);

		if (bIsReadingLight)
			sLight.mTransform = tf;
	}
	else if (cmd == "model")
	{
		if (!bIsReadingGameObject)
		{
			Log::Error(" Creating Transform without defining a game object (missing cmd: \"%s\"", "object begin");
			return;
		}

		Model m;
		m.mbLoaded = false;
		m.mModelDirectory = "";
		m.mModelName = command[1];
		pObject->SetModel(m);
	}
	else if (cmd == "ao")
	{
		Settings::SSAO& ssao = scene.settings.ssao;
		ssao.bEnabled		= sBoolTypeReflection.at(command[1]); 
		ssao.ambientFactor	= stof(command[2]);
		ssao.radius			= command.size() > 3 ? stof(command[3]) : 7.0f;	// 7 units - arbitrary.
		ssao.intensity		= command.size() > 4 ? stof(command[4]) : 1.0f;
	}
	else if (cmd == "skylight")
	{
		scene.settings.bSkylightEnabled= sBoolTypeReflection.at(command[1]);
	}
	else if (cmd == "bloom")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Bloom Threshold | BlurPassCount
		//---------------------------------------------------------------
		Settings::Bloom& bloom = scene.settings.bloom;
	
		bloom.bEnabled = sBoolTypeReflection.at(GetLowercased(command[1]));
		bloom.brightnessThreshold = command.size() > 2 ? stof(command[2]) : 1.5f;
		bloom.blurStrength = command.size() > 3 ? stoi(command[3]) : 3;	// 3 default blur stregth;
		// bloom.blurPassCount = stoi(command[3]);	// in case bloom settings should be more flexible
	}
	else
	{
		auto tokens = StrUtil::split(cmd, ' ', '\t');
		std::string strCmd = std::string(cmd);
		if (strCmd.find("//") == 0)
			;
		else
			Log::Error("Parser: Unknown command \"%s\"", cmd.c_str());
	}
}
