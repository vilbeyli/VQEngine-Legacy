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

#include "TextRenderer.h"
#include "Renderer.h"

#include "Utilities/Log.h"
#include "Utilities/utils.h"

#include <map>
#include <sstream>

#include "ft2build.h"
#include FT_FREETYPE_H

Renderer* TextRenderer::pRenderer = nullptr;
ShaderID TextRenderer::shaderText = -1;

struct Character
{
	TextureID tex;	// ID handle of the glyph texture
	vec2 size;		// Size of glyph
	vec2 bearing;	// Offset from baseline to left/top of glyph
	int advance;	// Offset to advance to next glyph
};

static std::map<char, Character> sCharacters;

// https://learnopengl.com/#!In-Practice/Text-Rendering
bool TextRenderer::Initialize(Renderer* pRenderer)
{
	TextRenderer::pRenderer = pRenderer;	// set static pRenderer
	TextRenderer::shaderText = pRenderer->AddShader("Text");

	FT_Error err; 

	// init font lib
	FT_Library ft;
	err = FT_Init_FreeType(&ft);
	if (err != 0)
	{
		Log::Error("Could not initialize FreeType Library. Error Code: %d", err);
		return false;
	}

	// load font
	const char* fontDir = "Data\\Fonts\\arial.ttf";
	FT_Face face; 
	err = FT_New_Face(ft, fontDir, 0, &face);
	if (err != 0)
	{
		Log::Error("Could not load Font (Err=%d): %s", err, fontDir);
		return false;
	}

	// set font width-height
	const int fontSize = 48;
	FT_Set_Pixel_Sizes(face, 0, fontSize);

	// create character-texture map
	for (char c = 0; c < 127; ++c)
	{
		// load character glyph
		err = FT_Load_Char(face, c, FT_LOAD_RENDER);
		if (err != 0)
		{
			Log::Error("Couldn't load character glyph (%d): %c", c, c);
		}

		int w = face->glyph->bitmap.width;
		int h = face->glyph->bitmap.rows;
		const int l = face->glyph->bitmap_left;
		const int t = face->glyph->bitmap_top;

		w = w == 0 ? 1 : w;
		h = h == 0 ? 1 : h;
		//if (w == 0 || h == 0)
		//	continue;

		TextureDesc texDesc;
		texDesc.format = EImageFormat::R8UN;
		texDesc.width = w;
		texDesc.height = h;
		texDesc.pData = face->glyph->bitmap.buffer;
		texDesc.dataPitch = face->glyph->bitmap.pitch;
		texDesc.dataSlicePitch = 0;

		TextureID charTexture = pRenderer->CreateTexture2D(texDesc);
		Character character = 
		{
			charTexture,
			vec2(w, h),
			vec2(l, t),
			face->glyph->advance.x
		};
		sCharacters.insert(std::pair<char, Character>(c, character));
	}

	// cleanup
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// create vertex buffer
	BufferDesc bufDesc;
	bufDesc.mElementCount = 6;
	bufDesc.mStride = sizeof(vec4);
	bufDesc.mType = VERTEX_BUFER;
	bufDesc.mUsage = DYNAMIC;
	mQuadVertexBuffer = pRenderer->CreateBuffer(bufDesc);

	// todo: blend state desc
	mAlphaBlendState = pRenderer->AddBlendState(); 
	return true;
}

void TextRenderer::Exit()
{
}

void TextRenderer::RenderText(const TextDrawDescription& drawDesc)
{
	assert(pRenderer);
	const vec2 windowSizeXY = pRenderer->GetWindowDimensionsAsFloat2();
	const XMMATRIX proj = XMMatrixOrthographicLH(windowSizeXY.x(), windowSizeXY.y(), 0.1f, 1000.0f);
	
	
	std::stringstream ss;
	ss << "RenderText: " << drawDesc.text;

	pRenderer->BeginEvent(ss.str());
	pRenderer->SetShader(shaderText);
	pRenderer->SetConstant4x4f("projection", proj);
	pRenderer->SetConstant3f("color", drawDesc.color);
	pRenderer->SetSamplerState("samText", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);
	pRenderer->SetBlendState(mAlphaBlendState);
	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);

	// offset with half window size, so that (0,0) is top left corner
	      float x =  drawDesc.screenPosition.x() - windowSizeXY.x() / 2;
	const float y = -drawDesc.screenPosition.y() + windowSizeXY.y() / 2;

	for (const char& c : drawDesc.text)
	{
		const Character& ch = sCharacters.at(c);

		const float xpos = x + ch.bearing.x() * drawDesc.scale;
		const float ypos = y - (ch.size.y() - ch.bearing.y()) * drawDesc.scale;

		const float w = ch.size.x() * drawDesc.scale;
		const float h = ch.size.y() * drawDesc.scale;

		float vertices[6][4] = 
		{
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
		};
		pRenderer->UpdateBuffer(mQuadVertexBuffer, vertices);

		pRenderer->SetTexture("textMap", ch.tex);
		
		pRenderer->SetVertexBuffer(mQuadVertexBuffer);
		pRenderer->Apply();
		pRenderer->Draw(6, EPrimitiveTopology::TRIANGLE_LIST);
		x += (ch.advance >> 6) * drawDesc.scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}
	pRenderer->SetBlendState(EDefaultBlendState::DISABLED);

	pRenderer->EndEvent();
}
