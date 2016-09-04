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



#ifndef UTILS_CPP
#define UTILS_CPP

#include <string>
#include <vector>

//#include <rapidjson/document.h>
//void PrintParsingError(rapidjson::Document* doc, const char* fileName = "FILENAME_NOT_DEFINED");

// STRING PROCESSING
//-----------------------------------------------------------------------------------------------
std::vector<std::string> split(const char* s,			char c = ' ');
std::vector<std::string> split(const std::string& s,	char c = ' ');
std::string	GetFileNameFromPath(const std::string&);

bool isNormalMap(const std::string& fileName);
std::string GetTextureNameFromDirectory(const std::string& dir);

// RANDOM
//------------------------------
float	RandF(float l, float h);
int		RandI(int l, int h);

#endif