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

#include "utils.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>

#include <atlbase.h>
#include <atlconv.h>

#ifdef _DEBUG
#include <cassert>
#endif _DEBUG

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
	return split(str.c_str(), c);
}

float RandF(float l, float h)
{
	float n = (float)rand() / RAND_MAX;
	return l + n*(h - l);
}

// [)
int RandI(int l, int h) 
{
	int offset = rand() % (h - l);
	return l + offset;
}
size_t RandU(size_t l, size_t h)
{
#ifdef _DEBUG
	assert(l <= h);
#endif
	int offset = rand() % (h - l);
	return l + static_cast<size_t>(offset);
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

//---------------------------------------------------------------------------------


const XMVECTOR vec3::Zero		= XMVectorZero();
const XMVECTOR vec3::Up			= XMVectorSet(+0.0f, +1.0f, +0.0f, +0.0f);
const XMVECTOR vec3::Down		= XMVectorSet(+0.0f, -1.0f, +0.0f, +0.0f);
const XMVECTOR vec3::Left		= XMVectorSet(-1.0f, +0.0f, +0.0f, +0.0f);
const XMVECTOR vec3::Right		= XMVectorSet(+1.0f, +0.0f, +0.0f, +0.0f);
const XMVECTOR vec3::Forward	= XMVectorSet(+0.0f, +0.0f, +1.0f, +0.0f);
const XMVECTOR vec3::Back		= XMVectorSet(+0.0f, +0.0f, -1.0f, +0.0f);

const vec3 vec3::ZeroF3			= vec3();
const vec3 vec3::UpF3			= vec3(+0.0f, +1.0f, +0.0f);
const vec3 vec3::DownF3			= vec3(+0.0f, -1.0f, +0.0f);
const vec3 vec3::LeftF3			= vec3(-1.0f, +0.0f, +0.0f);
const vec3 vec3::RightF3		= vec3(+1.0f, +0.0f, +0.0f);
const vec3 vec3::ForwardF3		= vec3(+0.0f, +0.0f, +1.0f);
const vec3 vec3::BackF3			= vec3(+0.0f, +0.0f, -1.0f);

const XMVECTOR vec2::Zero		= XMVectorZero();
const XMVECTOR vec2::Up			= XMVectorSet(+0.0f, +1.0f, +0.0f, +0.0f);
const XMVECTOR vec2::Down		= XMVectorSet(+0.0f, -1.0f, +0.0f, +0.0f);
const XMVECTOR vec2::Left		= XMVectorSet(-1.0f, +0.0f, +0.0f, +0.0f);
const XMVECTOR vec2::Right		= XMVectorSet(+1.0f, +0.0f, +0.0f, +0.0f);

const vec2 vec2::ZeroF2			= vec2();
const vec2 vec2::UpF2			= vec2(+0.0f, +1.0f);
const vec2 vec2::DownF2			= vec2(+0.0f, -1.0f);
const vec2 vec2::LeftF2			= vec2(-1.0f, +0.0f);
const vec2 vec2::RightF2		= vec2(+1.0f, +0.0f);


vec3::vec3()					: _v(XMFLOAT3(0.0f, 0.0f, 0.0f)){}
vec3::vec3(const vec3& v_in)	: _v(v_in._v) {}
vec3::vec3(const XMFLOAT3& f3)	: _v(f3) {}
vec3::vec3(const XMVECTOR& v_in){ XMStoreFloat3(&_v, v_in); }
vec3::vec3(float x, float y, float z) : _v(x, y, z) {}
vec3::vec3(float x) : _v(x, x, x) {}

vec2::vec2()					: _v(XMFLOAT2(0.0f, 0.0f)) {}
vec2::vec2(const vec3& v3)		: _v(v3.x(), v3.y()){}
vec2::vec2(const vec2& v_in)	: _v(v_in._v) {}
vec2::vec2(float x, float y)	: _v(x, y) {}
vec2::vec2(float f)				: _v(f, f) {}
vec2::vec2(const XMFLOAT2& f2)	: _v(f2) {}
vec2::vec2(const XMFLOAT3& f3)	: _v(f3.x, f3.y) {}
vec2::vec2(const XMVECTOR& v_in) { XMStoreFloat2(&_v, v_in); }

vec3::operator XMVECTOR() const	{	return XMLoadFloat3(&_v);	}
vec3::operator XMFLOAT3() const	{	return _v;					}
vec2::operator XMVECTOR() const {	return XMLoadFloat2(&_v); }
vec2::operator XMFLOAT2() const {	return _v; }

float& vec3::x() { return _v.x; }
float& vec3::y() { return _v.y; }
float& vec3::z() { return _v.z; }
float& vec3::x() const { return const_cast<float&>(_v.x); }
float& vec3::y() const { return const_cast<float&>(_v.y); }
float& vec3::z() const { return const_cast<float&>(_v.z); }

float& vec2::x() { return _v.x; }
float& vec2::y() { return _v.y; }
float& vec2::x() const { return const_cast<float&>(_v.x); }
float& vec2::y() const { return const_cast<float&>(_v.y); }

void vec3::normalize()
{
	XMVECTOR v = XMLoadFloat3(&_v);
	_v = vec3(XMVector3Normalize(v));
}

const vec3 vec3::normalized() const
{
	XMVECTOR v = XMLoadFloat3(&_v);
	return vec3(XMVector3Normalize(v));
}

void vec2::normalize()
{
	XMVECTOR v = XMLoadFloat2(&_v);
	_v = vec2(XMVector2Normalize(v));
}

const vec2 vec2::normalized() const
{
	XMVECTOR v = XMLoadFloat2(&_v);
	return vec2(XMVector2Normalize(v));
}
