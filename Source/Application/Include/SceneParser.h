#pragma once

#include <vector>
#include <string>
#include <memory>

#include "Settings.h"

using std::shared_ptr;
using std::unique_ptr;

class SceneManager;
class Camera;

static class SceneParser
{
public:
	SceneParser();
	~SceneParser();

	static Settings::Window ReadSettings();
	static void ParseSetting(const std::vector<std::string>& line, Settings::Window& settings);

	static void ReadScene(shared_ptr<SceneManager> scene_man);
	static void ParseScene(const std::vector<std::string>& command, shared_ptr<SceneManager> scene_man, Settings::Camera& cameraSettings);
private:

} s_SceneParser;

