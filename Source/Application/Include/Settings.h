#pragma once

//#include "Color.h"
//#include "Light.h"

namespace Settings
{
	//Settings() { memset(this, 0, sizeof(*this)); }

	//int read_err;
	struct Window {
		int		width;
		int		height;
		int		fullscreen;
		int		vsync;
	};

	struct Camera {
		float	fov;
		float	nearPlane;
		float	farPlane;
	};

	//struct LightSetting {
	//	Color				color;
	//	Light::LightType	type;
	//	float				angle_or_range;
	//} light;


	static Window window;
	static Camera camera;
};