#pragma once
//class RenderCommands
//{
//public:
//	RenderCommands();
//	~RenderCommands();
//};


//#include "Renderer.h"


class Renderer;

#include "Shader.h"

struct TextureSetCommand
{
	void SetResource(Renderer* pRenderer);	// this can't be inlined due to circular include between this and renderer

	int texID;
	ShaderTexture shdTex;
};

