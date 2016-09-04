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

#include <DirectXMath.h>

#include <map>
#include <vector>

using namespace DirectX;

class Color
{
public:
	Color();
	~Color();
	Color(const XMFLOAT3);
	Color(float r, float g, float b);
	Color& operator=(const Color&);

	XMFLOAT3 Value() const { return value; }
	static const std::vector<Color> Palette();

	//static Color GetColorByName(std::string);
	//static std::string GetNameByColor(Color c);

public:
	static const Color black, white, red, green, blue, magenta, yellow, cyan, gray, orange, purple;
	static std::vector<Color> colorPalette;
private:
	//typedef std::map<const std::string, Color> ColorMap;
	//static ColorMap colorRefTable;//created for easier deserialization in Mesh

	XMFLOAT3 value;
};

