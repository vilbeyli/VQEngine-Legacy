// ---------------------------------------------------------------------------
// Project Name		:	DX11Renderer
// File Name		:	utils.h
// Author			:	Volkan Ilbeyli
// Creation Date	:	2015/11/16
// ---------------------------------------------------------------------------

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