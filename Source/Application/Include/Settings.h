#pragma once

#include "Components/Transform.h"

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
		float		fov;
		float		nearPlane;
		float		farPlane;
		float		x, y, z;
		float		yaw, pitch;
	};

	struct ShadowMap {
		size_t	dimension;
	};

	struct PostProcess {
		struct Bloom {
			float	threshold_brdf;
			float	threshold_phong;
			int     blurPassCount;
		} bloom;

		struct Tonemapping {
			float	exposure;
		} toneMapping;

		bool HDREnabled = false;
	};
	
	struct Renderer{
		Window		window;
		ShadowMap	shadowMap;
		bool		bUseDeferredRendering;
		PostProcess postProcess;
	};
};