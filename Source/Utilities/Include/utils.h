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
#include <DirectXMath.h>

using namespace DirectX;

// macros
#define DEG2RAD (XM_PI / 180.0f)
#define RAD2DEG (180.0f / XM_PI)
#define PI		XM_PI
#define PI_DIV2 XM_PIDIV2

// typedefs
using VecPair = std::pair<XMVECTOR, XMVECTOR>;
typedef wchar_t WCHAR;    // wc,   16-bit UNICODE character

/// MATH WRAPPERS
//===============================================================================================

struct vec3
{
	static const XMVECTOR Zero;
	static const XMVECTOR Up;
	static const XMVECTOR Down;
	static const XMVECTOR Left;
	static const XMVECTOR Right;
	static const XMVECTOR Forward;
	static const XMVECTOR Back;

	static const vec3 ZeroF3;
	static const vec3 UpF3;
	static const vec3 DownF3;
	static const vec3 LeftF3;
	static const vec3 RightF3;
	static const vec3 ForwardF3;
	static const vec3 BackF3;

	static const vec3 XAxis;
	static const vec3 YAxis;
	static const vec3 ZAxis;

	vec3();
	vec3(const vec3& v_in);
	vec3(float, float, float);
	vec3(float);
	vec3(const XMFLOAT3 & f3);
	vec3(const XMVECTOR& v_in);

	operator XMVECTOR() const;
	operator XMFLOAT3() const;
	bool operator ==(const vec3&) const;
	inline vec3& operator +=(const vec3& v) { *this = *this + v; return *this; };

	float& x();
	float& y();
	float& z();

	float& x() const;
	float& y() const;
	float& z() const;

	void normalize();
	const vec3 normalized() const;
	
	XMFLOAT3 _v;
};

// todo vec.h
struct vec2
{
	static const XMVECTOR Zero;
	static const XMVECTOR Up;
	static const XMVECTOR Down;
	static const XMVECTOR Left;
	static const XMVECTOR Right;

	static const vec2 ZeroF2;
	static const vec2 UpF2;
	static const vec2 DownF2;
	static const vec2 LeftF2;
	static const vec2 RightF2;

	vec2();
	vec2(const vec3& v_in);
	vec2(const vec2& v_in);
	vec2(float, float);
	vec2(float);
	vec2(const XMFLOAT3& f3);
	vec2(const XMFLOAT2& f2);
	vec2(const XMVECTOR& v_in);

	operator XMVECTOR() const;
	operator XMFLOAT2() const;
	bool operator ==(const vec2&) const;
	inline vec2& operator +=(const vec2& v) { *this = *this + v; return *this; };

	float& x();
	float& y();

	float& x() const;
	float& y() const;

	void normalize();
	const vec2 normalized() const;

	XMFLOAT2 _v;
};


struct vec4
{
#if 0
	static const XMVECTOR Zero;
	static const XMVECTOR Up;
	static const XMVECTOR Down;
	static const XMVECTOR Left;
	static const XMVECTOR Right;

	static const vec4 ZeroF2;
	static const vec4 UpF2;
	static const vec4 DownF2;
	static const vec4 LeftF2;
	static const vec4 RightF2;

	vec4();
	vec4(const vec3& v_in);
	vec4(const vec4& v_in);
	vec4(float, float);
	vec4(float);
	vec4(const XMFLOAT3& f3);
	vec4(const XMFLOAT2& f2);
	vec4(const XMVECTOR& v_in);

	operator XMVECTOR() const;
	operator XMFLOAT2() const;
	bool operator ==(const vec4&) const;
	inline vec4& operator +=(const vec4& v) { *this = *this + v; return *this; };

	float& x();
	float& y();

	float& x() const;
	float& y() const;

	void normalize();
	const vec4 normalized() const;

	XMFLOAT2 _v;
#endif
	//union 
	float x, y, z, w;
	vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};


/// STRING PROCESSING
//===============================================================================================
std::vector<std::string> split(const char* s,			char c = ' ');
std::vector<std::string> split(const std::string& s,	char c = ' ');
std::vector<std::string> split(const std::string& s,	const std::vector<char>& delimiters);

template<class... Args> std::vector<std::string> split(const std::string& s, Args&&... args)
{
	const std::vector<char> delimiters = { args... };
	return split(s, delimiters);
}

std::string	GetFileNameFromPath(const std::string&);

struct UnicodeString
{
	std::string  str;
	std::wstring wstr;

	UnicodeString(const std::string& strIn);
	const WCHAR* GetUnicodePtr() const;
};

float inline lerp(float low, float high, float t) { return low + (high - low) * t; }

/// RANDOM
//===============================================================================================
float	RandF(float l, float h);
int		RandI(int l, int h);
size_t	RandU(size_t l, size_t h);

/// 3D
//===============================================================================================
struct Point
{
	Point() : pos(0, 0, 0) {}
	Point(float x, float y, float z)	: pos(x, y, z)					{}
	Point(const XMFLOAT3& f3)			: pos(f3)						{}
	Point(const XMVECTOR& vec)											{ XMStoreFloat3(&pos, vec); }
	Point(const Point& r)				: pos(r.pos.x, r.pos.y, r.pos.z){}
	//-------------------------------------------------------------------
	operator XMFLOAT3() const											{ return pos; }
	operator XMVECTOR() const											{ return XMLoadFloat3(&pos);}
	Point					operator*(float f)	const					{ return Point(pos.x*f, pos.y*f, pos.z*f); }
	Point					operator-(const Point& p)					{ return Point(pos.x - p.pos.x, pos.y - p.pos.y, pos.z - p.pos.z); }
	inline static float		Distance(const Point& p1, const Point& p2)	{ XMVECTOR AB = XMLoadFloat3(&p2.pos) - XMLoadFloat3(&p1.pos); return sqrtf(XMVector3Dot(AB, AB).m128_f32[0]); }
	//-------------------------------------------------------------------
	XMFLOAT3 pos;
};
#endif