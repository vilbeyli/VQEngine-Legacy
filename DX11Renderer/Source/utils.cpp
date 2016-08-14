#include "utils.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <atlbase.h>
#include <atlconv.h>

using std::vector;
using std::string;
using std::cout;
using std::endl;

vector<string> split(const char* s, char c)
{
	vector<string> result;

	do
	{
		const char* begin = s;
		while (*s != c && *s) s++;			// iterate until delimiter is found
		result.push_back(string(begin, s));
	} while (*s++);

	return result;
}

vector<string> split(const string& str, char c)
{
	return split(str.c_str());
}

float RandF(float l, float h){
	float n = (float)rand() / RAND_MAX;
	return l + n*(h - l);
}

// [)
int RandI(int l, int h){
	int offset = rand() % (h-l);

	return l + offset;
}

std::string GetFileNameFromPath(const std::string& path)
{	// example: path: "Archetypes/player.txt" | return val: "player"
	string no_extension = split(path.c_str(), '.')[0];
	auto tokens = split(no_extension.c_str(), '/');
	string name = tokens[tokens.size()-1];
	return name;
}

//bool isNormalMap(const string& fileName)
//{
//	// nrm_texfile.png
//	string noExt = split(fileName, '.')[0];		// nrm_texfile
//	vector<string> tokens = split(noExt, '_');	// <nrm, texfile>
//	return tokens[0] == "nrm";		// does 'nrm' exist in tokens? normal map : diffuse map;
//}

//std::string GetTextureNameFromDirectory(const std::string& dir)
//{
//	vector<string> tokens = split(dir, '\\');
//	string texName = tokens[tokens.size() - 1];
//	if (texName == dir)
//	{
//		tokens = split(dir, '/');
//		texName = tokens[tokens.size() - 1];
//	}
//	std::transform(texName.begin(), texName.end(), texName.begin(), ::tolower);
//	return texName;
//}