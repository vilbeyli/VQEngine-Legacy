#include "SceneParser.h"
#include "utils.h"
#include "SceneManager.h"

#include <Windows.h>
#include <fstream>

#include "BaseSystem.h"

const char* file_root		= "Data\\";
const char* scene_file		= "RoomScene.scn";
const char* settings_file	= "settings.ini";	

SceneParser::SceneParser(){}

SceneParser::~SceneParser(){}

Settings::Window SceneParser::ReadSettings()
{
	Settings::Window s;
	
	std::string filePath = std::string(file_root) + std::string(settings_file);
	std::ifstream settingsFile(filePath.c_str());
	if (settingsFile.is_open())
	{
		std::string	line;
		while (getline(settingsFile, line))
		{
			if (line[0] == '/' || line[0] == '#')					// skip comments
				continue;

			std::vector<std::string> command = split(line, ' ');	// ignore whitespace
			ParseSetting(command, s);								// process command
		}
	}
	else
	{
		OutputDebugString("Error: Settings.ini can't be opened.");
	}

	return s;
}

void SceneParser::ParseSetting(const std::vector<std::string>& line, Settings::Window& settings)
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
		settings.width      = stoi(line[1]);
		settings.height     = stoi(line[2]);
		settings.fullscreen = stoi(line[3]);
		settings.vsync      = stoi(line[4]);
	}
	else
	{
		Log::Error("Setting Parser: Invalid command, expected ""screen""");
		return;
	}
}

void SceneParser::ReadScene(SceneManager* pSceneManager)
{
	Settings::Camera cam;
	std::string filePath = std::string(file_root) + std::string(scene_file);
	std::ifstream sceneFile(filePath.c_str());

	if (sceneFile.is_open())
	{
		std::string	line;
		while (getline(sceneFile, line))
		{
			if (line[0] == '/' || line[0] == '#' || line[0] == '\0')	// skip comments
				continue;

			std::vector<std::string> command = split(line, ' ');	// ignore whitespace
			ParseScene(command, pSceneManager, cam);					// process command
		}
	}
	else
	{
		Log::Error( "Parser: Unknown setting command\n");
	}

	sceneFile.close();
}

void SceneParser::ParseScene(const std::vector<std::string>& command, SceneManager* pSceneManager, Settings::Camera& cameraSettings)
{
	if (command.empty())
	{
		OutputDebugString("Empty Command.");
		assert(!command.empty());
		return;
	}
	
	const std::string& cmd = command[0];	// shorthand
	if (cmd == "camera")
	{
		if (command.size() != 4)
		{
			OutputDebugString("camera input parameter count != 4");
			assert(command.size() == 4);
		}

		// Parameters
		//--------------------------------------------------------------
		// |  Near Plane	| Far Plane	|	Field of View	
		//--------------------------------------------------------------
		cameraSettings.nearPlane	= stof(command[1]);
		cameraSettings.farPlane		= stof(command[2]);
		cameraSettings.fov			= stof(command[3]);
		
		pSceneManager->SetCameraSettings(cameraSettings, BaseSystem::s_windowSettings);	
	}
	
	else if (cmd == "light")
	{
		// Parameters
		//--------------------------------------------------------------
		// | Color	|  Light Type	| Spot.Angle OR Point.Range
		//--------------------------------------------------------------
		//mSceneSettings.light.color = Color::white;				// todo read color
		//mSceneSettings.light.type = Light::LightType::POINT;	// todo
		//mSceneSettings.light
		Log::Error("TODO: parse light info to build scene from file\n");
	}
	
	else
	{
		Log::Error("Parser: Unknown command\n");
	}
}
