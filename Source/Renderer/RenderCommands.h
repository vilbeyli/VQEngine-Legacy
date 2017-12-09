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

#pragma once

#include "Shader.h"
#include "Utilities/utils.h"

class Renderer;


// inheritance?

struct SetTextureCommand
{
	void SetResource(Renderer* pRenderer);	// this can't be inlined due to circular include between this and renderer

	TextureID		textureID;
	ShaderTexture	shaderTexture;
};

struct SetSamplerCommand
{
	void SetResource(Renderer* pRenderer);	// this can't be inlined due to circular include between this and renderer

	SamplerID		samplerID;
	ShaderSampler	shaderSampler;
};

struct ClearCommand
{
	static ClearCommand Depth(float depthClearValue);
	static ClearCommand Color(const std::array<float, 4>& clearColorValues);
	static ClearCommand Color(float r, float g, float b, float a);
	
	ClearCommand(bool doClearColor                          , bool doClearDepth    , bool doClearStencil, 
				const std::array<float, 4>& clearColorValues, float clearDepthValue, unsigned char clearStencilValue) :
		bDoClearColor(doClearColor),
		bDoClearDepth(doClearDepth),
		bDoClearStencil(doClearStencil),
		clearColor(clearColorValues),
		clearDepth(clearDepthValue),
		clearStencil(clearStencilValue)
	{}


	bool					bDoClearColor;
	bool					bDoClearDepth;
	bool					bDoClearStencil;

	std::array<float, 4>	clearColor;
	float					clearDepth;
	unsigned char			clearStencil;
};

struct SetScissorsCommand
{	// currently not used
	int left, right, top, bottom;
};

struct DrawQuadOnScreenCommand
{
	vec2 dimensionsInPixels;
	vec2 bottomLeftCornerScreenCoordinates;	// (0, 0) is bottom left corner of the screen
	TextureID texture;
	bool bIsDepthTexture;
};