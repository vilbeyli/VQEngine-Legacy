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


#include "Color.h"

using std::make_pair;

const Color Color::black	= XMFLOAT3(0.0f, 0.0f, 0.0f);
const Color Color::white	= XMFLOAT3(1.0f, 1.0f, 1.0f);
const Color Color::red		= XMFLOAT3(1.0f, 0.0f, 0.0f);
const Color Color::green	= XMFLOAT3(0.0f, 1.0f, 0.0f);
const Color Color::blue		= XMFLOAT3(0.0f, 0.0f, 1.0f);
const Color Color::yellow	= XMFLOAT3(1.0f, 1.0f, 0.0f);
const Color Color::magenta	= XMFLOAT3(1.0f, 0.0f, 1.0f);
const Color Color::cyan		= XMFLOAT3(0.0f, 1.0f, 1.0f);
const Color Color::gray		= XMFLOAT3(0.3f, 0.3f, 0.3f);
const Color Color::orange	= XMFLOAT3(1.0f, 0.5f, 0.0f);
const Color Color::purple	= XMFLOAT3(0.31f, 0.149f, 0.513f);

std::vector<Color> Color::colorPalette;

Color::Color() 
	: 
	value(white.Value())
{}


const std::vector<Color> Color::Palette()
{
	if (colorPalette.size() == 0)
	{
		colorPalette.push_back(Color::red);
		colorPalette.push_back(Color::blue);
		colorPalette.push_back(Color::green);
		colorPalette.push_back(Color::yellow);
		colorPalette.push_back(Color::magenta);
		colorPalette.push_back(Color::cyan);
		colorPalette.push_back(Color::white);
		colorPalette.push_back(Color::orange);
		colorPalette.push_back(Color::purple);
	}
	return colorPalette;
}

Color::~Color(){}

Color::Color(const XMFLOAT3 val)
	: 
	value(val)
{}

Color::Color(float r, float g, float b)
	:
	value(XMFLOAT3(r, g, b))
{}

Color & Color::operator=(const Color & rhs)
{
	this->value = rhs.Value();
	return *this;
}

Color & Color::operator=(const XMFLOAT3& flt)
{
	this->value = flt;
	return *this;
}

//Color::ColorMap Color::colorRefTable
//{
//	{ "red", Color::red },
//	{ "blue", Color::blue },
//	{ "green", Color::green },
//	{ "yellow", Color::yellow },
//	{ "magenta", Color::magenta },
//	{ "cyan", Color::cyan },
//	{ "white", Color::white },
//	{ "orange", Color::orange },
//	{ "purple", Color::purple },
//	{ "black", Color::black }
//};

//Color Color::GetColorByName(std::string colorName)
//{
//	return colorRefTable.at(colorName);
//}
//std::string Color::GetNameByColor(Color c)
//{
//	for (ColorMap::iterator it = colorRefTable.begin(); it != colorRefTable.end(); it++)
//	{
//		if (it->second.value == c.value)
//			return it->first;
//	}
//
//	return "White";
//}
