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
#include "utils.h"

using namespace DirectX;

class Quaternion
{
public:
	static Quaternion Identity();
	static Quaternion FromAxisAngle(const XMVECTOR& axis, const float angle);
	static Quaternion Lerp (const Quaternion& from, const Quaternion& to, float t);
	static Quaternion Slerp(const Quaternion& from, const Quaternion& to, float t);
	static vec3 ToEulerRad(const Quaternion& Q);
	static vec3 ToEulerDeg(const Quaternion& Q);
	//-----------------------------------------------
	Quaternion(float roll, float pitch, float yaw);
	Quaternion(const vec3& rollPitchYaw);
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

