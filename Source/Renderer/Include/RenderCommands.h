#pragma once

class Renderer;

#include "Shader.h"

struct SetTextureCommand
{
	void SetResource(Renderer* pRenderer);	// this can't be inlined due to circular include between this and renderer

	int texID;
	ShaderTexture shdTex;
};

struct SetSamplerCommand
{
	void SetResource(Renderer* pRenderer);	// this can't be inlined due to circular include between this and renderer

	int samplerID;
	ShaderSampler shdSampler;
};

