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

#include <string>
#include <Utilities/Color.h>
#include "BufferObject.h"

class Renderer;

struct TextDrawDescription
{
	std::string text;
	LinearColor color;
	vec2 screenPosition;	// top-left (0,0) - bottom-right(1,1)
	float scale;
};

class TextRenderer
{
public:
	bool Initialize(Renderer* pRenderer);
	void Exit();

	void RenderText(const TextDrawDescription& drawDesc);

private:
	std::vector<vec4> mQuadVertexData;
	BufferID mQuadVertexBuffer;	// vertex buffer for rendering quad
	BlendStateID mAlphaBlendState;
private:
	static Renderer* pRenderer;
	static int		 shaderText;
};