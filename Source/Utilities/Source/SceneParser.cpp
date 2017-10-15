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

#include "SceneParser.h"
#include "utils.h"
#include "Log.h"

#include "Color.h"

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


SceneParser::SceneParser(){}

SceneParser::~SceneParser(){}

Settings::Engine SceneParser::ReadSettings(const std::string& settingsFileName)
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

void SceneParser::ParseSetting(const std::vector<std::string>& line, Settings::Engine& settings)
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

SerializedScene SceneParser::ReadScene(const std::string& sceneFileName)
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
			ParseScene(command, scene);									// process command
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
void SceneParser::ParseScene(const std::vector<std::string>& command, SerializedScene& scene)
{
	// state tracking
	static bool bIsReadingGameObject = false;	
	static GameObject obj;

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

		const bool bCommandHasRotationEntry = command.size() > 9;

		const std::string lightType	 = GetLowercased(command[1]);	// lookups have lowercase keys
		const std::string colorValue = GetLowercased(command[2]);
		const std::string shadowing	 = GetLowercased(command[3]);
		const float rotX = bCommandHasRotationEntry ? stof(command[9])  : 0.0f;
		const float rotY = bCommandHasRotationEntry ? stof(command[10]) : 0.0f;
		const float rotZ = bCommandHasRotationEntry ? stof(command[11]) : 0.0f;
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
			obj.Clear();
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
		Log::Info("Todo: brdf mat");
		return;
		// #Parameters: todo
		//--------------------------------------------------------------
		// todo
		//--------------------------------------------------------------
		if (!bIsReadingGameObject)
		{
			Log::Error(" Creating BRDF Material without defining a game object (missing cmd: \"%s\"", "object begin");
			return;
		}

		BRDF_Material& mat = obj.mModel.mBRDF_Material;
		//const std::string mesh = GetLowercased(command[1]);


		
	}
	else if (cmd == "blinnphong")
	{
		Log::Info("Todo: blinnphong mat");
		return;
		// #Parameters: todo
		//--------------------------------------------------------------
		// todo
		//--------------------------------------------------------------
		if (!bIsReadingGameObject)
		{
			Log::Error(" Creating BlinnPhong Material without defining a game object (missing cmd: \"%s\"", "object begin");
			return;
		}

		BlinnPhong_Material& mat = obj.mModel.mBlinnPhong_Material;
		//const std::string mesh = GetLowercased(command[1]);

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
	else
	{
		Log::Error("Parser: Unknown command \"%s\"", cmd);
	}
}
