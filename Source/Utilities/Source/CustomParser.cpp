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

#include "CustomParser.h"
#include "utils.h"
#include "Log.h"
#include "Color.h"

#include "Renderer/Renderer.h"

#include <fstream>
#include <unordered_map>
#include <algorithm>


const std::string file_root		= "Data\\";
const std::string scene_root	= "Data\\Scenes\\";

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
	const std::string filePath = file_root + settingsFileName;
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

			std::vector<std::string> command = split(line, ' ');	// ignore whitespace
			ParseSetting(command, setting);							// process command
		}
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
	if (cmd == "screen")
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
	else if (cmd == "bloom")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Bloom Threshold BRDF | Bloom Threshold PHONG | BlurPassCount
		//---------------------------------------------------------------
		settings.rendering.postProcess.bloom.threshold_brdf  = stof(line[1]);
		settings.rendering.postProcess.bloom.threshold_phong = stof(line[2]);
		settings.rendering.postProcess.bloom.blurPassCount   = stoi(line[3]);
	}
	else if (cmd == "shadowMap")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Shadow Map dimension
		//---------------------------------------------------------------
		settings.rendering.shadowMap.dimension = stoi(line[1]);
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
	}
	else if (cmd == "HDR")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Enabled?
		//---------------------------------------------------------------
		settings.rendering.postProcess.HDREnabled = sBoolTypeReflection.at(GetLowercased(line[1]));
	}
	else if (cmd == "level")
	{
		// Parameters
		//---------------------------------------------------------------
		// | Enabled?
		//---------------------------------------------------------------
		settings.levelToLoad = stoi(line[1]);
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

	if (sceneFile.is_open())
	{
		std::string	line;
		while (getline(sceneFile, line))
		{
			if (line[0] == '/' || line[0] == '#' || line[0] == '\0')	// skip comments
				continue;

			std::vector<std::string> command = split(line, ' ', '\t');	// ignore whitespace
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
static bool bIsReadingMaterial = false;
static GameObject obj;

enum MaterialType { UNKNOWN, BRDF, PHONG };
static MaterialType materialType = MaterialType::UNKNOWN;
static BRDF_Material brdf;
static BlinnPhong_Material phong;

using ParseFunctionType = void(__cdecl *)(const std::vector<std::string>&);
using ParseFunctionLookup = std::unordered_map<std::string, ParseFunctionType>;

static const std::unordered_map<std::string, Light::ELightType>	sLightTypeLookup
{ 
	{"s", Light::ELightType::SPOT },
	{"p", Light::ELightType::POINT},
	{"d", Light::ELightType::DIRECTIONAL}
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
	{"purple"    , LinearColor::s_palette[ static_cast<int>(EColorValue::PURPLE     )]}
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
	if (command.empty())
	{
		Log::Error("Empty Command.");
		assert(!command.empty());
		return;
	}
	
	const std::string& cmd = command[0];	// shorthand
	if (cmd == "camera")
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
		// #Parameters: 11
		//--------------------------------------------------------------
		// | Light Type	| Color	| Shadowing? |  Brightness | Spot.Angle OR Point.Range | Position3 | Rotation3
		//--------------------------------------------------------------


		const bool bCommandHasRotationEntry = command.size() > 10;
		const bool bCommandHasScaleEntry = command.size() > 12;

		const std::string lightType	 = GetLowercased(command[1]);	// lookups have lowercase keys
		const std::string colorValue = GetLowercased(command[2]);
		const std::string shadowing	 = GetLowercased(command[3]);
		const float rotX = bCommandHasRotationEntry ? stof(command[9])  : 0.0f;
		const float rotY = bCommandHasRotationEntry ? stof(command[10]) : 0.0f;
		const float rotZ = bCommandHasRotationEntry ? stof(command[11]) : 0.0f;
		const float scl  = bCommandHasScaleEntry ? stof(command[12]) : 1.0f;
		const float range      = stof(command[5]);
		const float brightness = stof(command[4]);
		const bool  bCastsShadows = sBoolTypeReflection.at(shadowing);

		if (lightType != "s" && lightType != "p" && lightType != "d")
		{	// check light types
			Log::Error("light type unknown: %s", command[1]);
			return;
		}
		
		Light l(	// let there be light
			sLightTypeLookup.at(lightType),
			sColorLookup.at(colorValue),
			range,
			brightness,
			range,	// = spot angle
			bCastsShadows
		);
		l._transform.SetPosition(stof(command[6]), stof(command[7]), stof(command[8]));
		l._transform.RotateAroundGlobalXAxisDegrees(rotX);
		l._transform.RotateAroundGlobalYAxisDegrees(rotY);
		l._transform.RotateAroundGlobalZAxisDegrees(rotZ);
		l._transform.SetUniformScale(scl);
		scene.lights.push_back(l);
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
			obj.Clear();
		}

		if (objCmd == "end")
		{
			if (!bIsReadingGameObject)
			{
				Log::Error(" expecting \"object begin\" before ending an object definition");
				return;
			}
			bIsReadingGameObject = false;
			scene.objects.push_back(obj);
		}
	}
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
		obj.mModel.mMesh = sMeshLookup.at(mesh);
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
				Log::Error(" Syntax Error: Already defining a phong material!");
				return;
			}

			materialType = UNKNOWN;
			bIsReadingMaterial = false;
			return;
		}

		bIsReadingMaterial = true;
		materialType = BRDF;
		obj.mModel.mBRDF_Material.Clear();
	}
	else if (cmd == "blinnphong")
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
		obj.mModel.mBlinnPhong_Material.Clear();
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
		if (IsImageName(firstParam))
		{
			const TextureID texDiffuse = pRenderer->CreateTextureFromFile(firstParam);
			obj.mModel.SetDiffuseMap(texDiffuse);
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
				obj.mModel.SetDiffuseAlpha(LinearColor(r, g, b), a);
			}
			else
			{
				obj.mModel.SetDiffuseColor(LinearColor(r, g, b));
			}
		}
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
		obj.mModel.mBRDF_Material.roughness = stof(command[1]);
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
		obj.mModel.mBRDF_Material.metalness = stof(command[1]);
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
		obj.mModel.mBlinnPhong_Material.shininess = stof(command[1]);
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
		const TextureID texDiffuse = pRenderer->CreateTextureFromFile(command[1]);
		obj.mModel.SetDiffuseMap(texDiffuse);

		if (command.size() > 2)
		{
			const TextureID texNormal = pRenderer->CreateTextureFromFile(command[2]);
			obj.mModel.SetNormalMap(texNormal);
		}

		if (command.size() > 3)
		{
			// add various maps (specular etc)
		}
	}

	else if (cmd == "transform")
	{
		// #Parameters: 7-9
		//--------------------------------------------------------------
		// Position(3), Rotation(3), UniformScale(1)/Scale(3)
		//--------------------------------------------------------------
		if (!bIsReadingGameObject)
		{
			Log::Error(" Creating Transform without defining a game object (missing cmd: \"%s\"", "object begin");
			return;
		}
		
		Transform& tf = obj.mTransform;
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
	}
	else if (cmd == "ao")
	{
		scene.settings.bAmbientOcclusionEnabled = sBoolTypeReflection.at(command[1]); 
		scene.settings.ambientFactor = stof(command[2]);
	}
	else
	{
		Log::Error("Parser: Unknown command \"%s\"", cmd.c_str());
	}
}
