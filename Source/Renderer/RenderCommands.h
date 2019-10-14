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

#pragma once

#include "Shader.h"
#include "Utilities/vectormath.h"

class Renderer;

#if USE_DX12

#else

// inheritance... ?

constexpr size_t TEXTURE_ARRAY_SIZE = 32;
struct SetTextureCommand
{
	SetTextureCommand(TextureID texID, const TextureBinding& shaderBinding, unsigned sliceIn, bool bUA = false)
		: textureIDs({ texID })
		, binding(shaderBinding)
		, bUnorderedAccess(bUA)
		, slice(sliceIn)
		, numTextures(1)
	{}

	SetTextureCommand(const std::array<TextureID, TEXTURE_ARRAY_SIZE>& texIDs, unsigned numTexturesIn, const TextureBinding& shaderBinding, unsigned sliceIn, bool bUA = false)
		: textureIDs(texIDs)
		, binding(shaderBinding)
		, bUnorderedAccess(bUA)
		, slice(sliceIn)
		, numTextures(numTexturesIn)
	{}

	void SetResource(Renderer* pRenderer);	// this can't be inlined due to circular include between this and renderer

	const std::array<TextureID, TEXTURE_ARRAY_SIZE> textureIDs;
	const TextureBinding&	binding;
	const unsigned			slice;
	const unsigned			numTextures;
	bool bUnorderedAccess;
};

struct SetSamplerCommand
{
	SetSamplerCommand(SamplerID sampID, const SamplerBinding& shaderSamp) : samplerID(sampID), binding(shaderSamp) {}
	void SetResource(Renderer* pRenderer);	// this can't be inlined due to circular include between this and renderer

	const SamplerID			samplerID;
	const SamplerBinding&	binding;
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
	int numChannels = 3;
};
#endif