//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018 - Volkan Ilbeyli
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
#include <vector>

using namespace DirectX;

using VecPair = std::pair<XMVECTOR, XMVECTOR>;

// macros
#define DEG2RAD (XM_PI / 180.0f)
#define RAD2DEG (180.0f / XM_PI)
#define PI		XM_PI
#define PI_DIV2 XM_PIDIV2

struct vec3
{
	static vec3 Rand();
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
	inline vec3 operator +(const vec3& v) { return vec3(this->x() + v.x(), this->y() + v.y(), this->z() + v.z()); };
	inline vec3& operator +=(const vec3& v) { *this = *this + v; return *this; };

	inline float& vec3::x() { return _v.x; }
	inline float& vec3::y() { return _v.y; }
	inline float& vec3::z() { return _v.z; }
	inline float& vec3::x() const { return const_cast<float&>(_v.x); }
	inline float& vec3::y() const { return const_cast<float&>(_v.y); }
	inline float& vec3::z() const { return const_cast<float&>(_v.z); }

	void normalize();
	const vec3 normalized() const;
	const std::string print() const;

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
	vec2(int, int);
	vec2(float);
	vec2(const XMFLOAT3& f3);
	vec2(const XMFLOAT2& f2);
	vec2(const XMVECTOR& v_in);

	operator XMVECTOR() const;
	operator XMFLOAT2() const;
	bool operator ==(const vec2&) const;
	inline vec2& operator +=(const vec2& v) { *this = *this + v; return *this; };

	inline float& vec2::x() { return _v.x; }
	inline float& vec2::y() { return _v.y; }
	inline float& vec2::x() const { return const_cast<float&>(_v.x); }
	inline float& vec2::y() const { return const_cast<float&>(_v.y); }

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

	vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
	vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
	vec4(const vec3& v3) : x(v3.x()), y(v3.y()), z(v3.z()), w(0.0f) {}
	vec4(const vec3& v3, float _w) : x(v3.x()), y(v3.y()), z(v3.z()), w(_w) {}

	operator XMVECTOR() const;
};


/// 3D
//===============================================================================================
struct Point
{
	Point() : pos(0, 0, 0) {}
	Point(float x, float y, float z) : pos(x, y, z) {}
	Point(const XMFLOAT3& f3) : pos(f3) {}
	Point(const XMVECTOR& vec) { XMStoreFloat3(&pos, vec); }
	Point(const Point& r) : pos(r.pos.x, r.pos.y, r.pos.z) {}
	//-------------------------------------------------------------------
	operator XMFLOAT3() const { return pos; }
	operator XMVECTOR() const { return XMLoadFloat3(&pos); }
	Point					operator*(float f)	const { return Point(pos.x*f, pos.y*f, pos.z*f); }
	Point					operator-(const Point& p) { return Point(pos.x - p.pos.x, pos.y - p.pos.y, pos.z - p.pos.z); }
	inline static float		Distance(const Point& p1, const Point& p2) { XMVECTOR AB = XMLoadFloat3(&p2.pos) - XMLoadFloat3(&p1.pos); return sqrtf(XMVector3Dot(AB, AB).m128_f32[0]); }
	//-------------------------------------------------------------------
	XMFLOAT3 pos;
};

class Quaternion
{
public:
	static Quaternion Identity();
	static Quaternion FromAxisAngle(const XMVECTOR& axis, const float angle);
	static Quaternion Lerp(const Quaternion& from, const Quaternion& to, float t);
	static Quaternion Slerp(const Quaternion& from, const Quaternion& to, float t);
	static vec3 ToEulerRad(const Quaternion& Q);
	static vec3 ToEulerDeg(const Quaternion& Q);
	//-----------------------------------------------
	//Quaternion(float roll, float pitch, float yaw);
	//Quaternion(const vec3& rollPitchYaw);
	Quaternion(const XMMATRIX& rotMatrix);
	Quaternion(float s, const XMVECTOR& v);
	//-----------------------------------------------
	Quaternion  operator+(const Quaternion& q) const;
	Quaternion  operator*(const Quaternion& q) const;
	Quaternion  operator*(float c) const;
	bool		operator==(const Quaternion& q) const;
	float		Dot(const Quaternion& q) const;
	float		Len() const;
	Quaternion  Inverse() const;
	Quaternion  Conjugate() const;
	XMMATRIX	Matrix() const;
	Quaternion&	Normalize();

	vec3 TransformVector(const vec3& v) const;

private:	// used by operator()s
	Quaternion(float s, const vec3& v);
	Quaternion();

public:
	// Q = [S, <V>]
	vec3 V;
	float S;
};

//===============================================================================================

struct FrustumPlaneset
{	// plane equations: aX + bY + cZ + d = 0
	vec4 abcd[6];	// r, l, t, b, n, f
	enum EPlaneset
	{
		PL_RIGHT = 0,
		PL_LEFT,
		PL_TOP,
		PL_BOTTOM,
		PL_FAR,
		PL_NEAR
	};

	// src: http://gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdfe
	// gets the frustum planes based on @projectionTransformation. if:
	//
	// - @projectionTransformation is proj			matrix ->  view space plane equations
	// - @projectionTransformation is viewProj		matrix -> world space plane equations
	// - @projectionTransformation is worldViewProj matrix -> model space plane equations
	// 
	inline static FrustumPlaneset ExtractFromMatrix(const XMMATRIX& m)
	{
		FrustumPlaneset viewPlanes;
		viewPlanes.abcd[FrustumPlaneset::PL_RIGHT] = vec4(
			m.r[0].m128_f32[3] - m.r[0].m128_f32[0],
			m.r[1].m128_f32[3] - m.r[1].m128_f32[0],
			m.r[2].m128_f32[3] - m.r[2].m128_f32[0],
			m.r[3].m128_f32[3] - m.r[3].m128_f32[0]
		);
		viewPlanes.abcd[FrustumPlaneset::PL_LEFT] = vec4(
			m.r[0].m128_f32[3] + m.r[0].m128_f32[0],
			m.r[1].m128_f32[3] + m.r[1].m128_f32[0],
			m.r[2].m128_f32[3] + m.r[2].m128_f32[0],
			m.r[3].m128_f32[3] + m.r[3].m128_f32[0]
		);
		viewPlanes.abcd[FrustumPlaneset::PL_TOP] = vec4(
			m.r[0].m128_f32[3] - m.r[0].m128_f32[1],
			m.r[1].m128_f32[3] - m.r[1].m128_f32[1],
			m.r[2].m128_f32[3] - m.r[2].m128_f32[1],
			m.r[3].m128_f32[3] - m.r[3].m128_f32[1]
		);
		viewPlanes.abcd[FrustumPlaneset::PL_BOTTOM] = vec4(
			m.r[0].m128_f32[3] + m.r[0].m128_f32[1],
			m.r[1].m128_f32[3] + m.r[1].m128_f32[1],
			m.r[2].m128_f32[3] + m.r[2].m128_f32[1],
			m.r[3].m128_f32[3] + m.r[3].m128_f32[1]
		);
		viewPlanes.abcd[FrustumPlaneset::PL_FAR] = vec4(
			m.r[0].m128_f32[3] - m.r[0].m128_f32[2],
			m.r[1].m128_f32[3] - m.r[1].m128_f32[2],
			m.r[2].m128_f32[3] - m.r[2].m128_f32[2],
			m.r[3].m128_f32[3] - m.r[3].m128_f32[2]
		);
		viewPlanes.abcd[FrustumPlaneset::PL_NEAR] = vec4(
			m.r[0].m128_f32[2],
			m.r[1].m128_f32[2],
			m.r[2].m128_f32[2],
			m.r[3].m128_f32[2]
		);
		return viewPlanes;
	}
};