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

#include "utils.h"

#include <map>
#include <vector>
#include <array>

using namespace DirectX;

enum class COLOR_VLAUE
{
	BLACK = 0,
	WHITE,
	RED,
	GREEN,
	BLUE,
	YELLOW,
	MAGENTA,
	CYAN,
	GRAY,
	LIGHT_GRAY,
	ORANGE,
	PURPLE,

	COUNT
};


struct Color
{
	using ColorPalette = std::array < const Color, static_cast<int>(COLOR_VLAUE::COUNT)>;

public:
	Color();
	Color(const vec3&);
	Color(float r, float g, float b);
	Color& operator=(const Color&);
	Color& operator=(const vec3&);

	vec3 Value() const { return value; }
	static const ColorPalette Palette();
	static vec3 RandColorF3();
	static XMVECTOR RandColorV();
	static Color	RandColor();

	operator vec3() const { return value; }

	//static Color GetColorByName(std::string);
	//static std::string GetNameByColor(Color c);

public:
	static const Color black, white, red, green, blue, magenta, yellow, cyan, gray, light_gray, orange, purple;
	static const ColorPalette s_palette;
private:
	vec3 value;
};

