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

#include "TextRenderer.h"
#include "Renderer.h"

#include "Utilities/Log.h"
#include "Utilities/utils.h"

#include <map>

#include "ft2build.h"
#include FT_FREETYPE_H

Renderer* TextRenderer::pRenderer = nullptr;
ShaderID TextRenderer::shaderText = -1;

struct Character
{
	TextureID tex;
	vec2 size;
	vec2 bearing;
	int advance;
};

static std::map<char, Character> sCharacters;

bool TextRenderer::Initialize(Renderer* pRenderer)
{
	// https://learnopengl.com/#!In-Practice/Text-Rendering
	TextRenderer::pRenderer = pRenderer;

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
			Log::Error("Couldn't load character glpyh (%d): %c", c, c);
		}

		const int w = face->glyph->bitmap.width;
		const int h = face->glyph->bitmap.rows;
		const int l = face->glyph->bitmap_left;
		const int t = face->glyph->bitmap_top;

		if (w == 0 || h == 0)
			continue;

		TextureDesc texDesc;
		texDesc.format = EImageFormat::R32U;
		texDesc.width = w;
		texDesc.height = h;
		texDesc.data = face->glyph->bitmap.buffer;

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

	const std::vector<InputLayout> layouts = { { "POSITION",	FLOAT32_3 }	};
	TextRenderer::shaderText = pRenderer->AddShader("Text", layouts);
	return true;
}

void TextRenderer::RenderText(const TextDrawDescription& drawDesc)
{
	//const XMFLOAT4X4 proj;
	//	
	//pRenderer->SetShader(shaderText);
	//pRenderer->SetConstant4x4f("projection");
	//pRenderer->Apply();
	//pRenderer->Draw();
}
