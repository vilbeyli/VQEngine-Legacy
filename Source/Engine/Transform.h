//	VQEngine | DirectX11 Renderer
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
#include "Utilities/vectormath.h"

struct Transform
{
public:

	//----------------------------------------------------------------------------------------------------------------
	// CONSTRUCTOR / DESTRUCTOR
	//----------------------------------------------------------------------------------------------------------------
	Transform(  const vec3&			position = vec3(0.0f, 0.0f, 0.0f),
				const Quaternion&	rotation = Quaternion::Identity(),
				const vec3&			scale    = vec3(1.0f, 1.0f, 1.0f));
	~Transform();
	Transform& operator=(const Transform&);

	//----------------------------------------------------------------------------------------------------------------
	// GETTERS & SETTERS
	//----------------------------------------------------------------------------------------------------------------
	inline void SetXRotationDeg(float xDeg)            { _rotation = Quaternion::FromAxisAngle(vec3::Right  , xDeg * DEG2RAD); }
	inline void SetYRotationDeg(float yDeg)            { _rotation = Quaternion::FromAxisAngle(vec3::Up     , yDeg * DEG2RAD); }
	inline void SetZRotationDeg(float zDeg)            { _rotation = Quaternion::FromAxisAngle(vec3::Forward, zDeg * DEG2RAD); }
	inline void SetScale(float x, float y, float z)    { _scale	= vec3(x, y, z); }
	inline void SetUniformScale(float s)		       { _scale	= vec3(s, s, s); }
	inline void SetPosition(float x, float y, float z) { _position = vec3(x, y, z); }
	inline void SetPosition(const vec3& pos)		   { _position = pos; }

	//----------------------------------------------------------------------------------------------------------------
	// TRANSFORMATIONS
	//----------------------------------------------------------------------------------------------------------------
	void Translate(const vec3& translation);
	void Translate(float x, float y, float z);
	void Scale(const vec3& scl);
	
	void RotateAroundPointAndAxis(const vec3& axis, float angle, const vec3& point);
	inline void RotateAroundAxisRadians(const vec3& axis, float angle) { RotateInWorldSpace(Quaternion::FromAxisAngle(axis, angle)); }
	inline void RotateAroundAxisDegrees(const vec3& axis, float angle) { RotateInWorldSpace(Quaternion::FromAxisAngle(axis, angle * DEG2RAD)); }

	inline void RotateAroundLocalXAxisDegrees(float angle)	{ RotateInLocalSpace(Quaternion::FromAxisAngle(vec3::XAxis, std::forward<float>(angle * DEG2RAD))); }
	inline void RotateAroundLocalYAxisDegrees(float angle)	{ RotateInLocalSpace(Quaternion::FromAxisAngle(vec3::YAxis, std::forward<float>(angle * DEG2RAD))); }
	inline void RotateAroundLocalZAxisDegrees(float angle)	{ RotateInLocalSpace(Quaternion::FromAxisAngle(vec3::ZAxis, std::forward<float>(angle * DEG2RAD))); }
	inline void RotateAroundGlobalXAxisDegrees(float angle)	{ RotateAroundAxisDegrees(vec3::XAxis, std::forward<float>(angle)); }
	inline void RotateAroundGlobalYAxisDegrees(float angle)	{ RotateAroundAxisDegrees(vec3::YAxis, std::forward<float>(angle)); }
	inline void RotateAroundGlobalZAxisDegrees(float angle)	{ RotateAroundAxisDegrees(vec3::ZAxis, std::forward<float>(angle)); }

	inline void RotateInWorldSpace(const Quaternion& q)	{ _rotation = q * _rotation;	}
	inline void RotateInLocalSpace(const Quaternion& q)	{ _rotation = _rotation * q;	}


	XMMATRIX WorldTransformationMatrix() const;
	XMMATRIX WorldTransformationMatrix_NoScale() const;
	static XMMATRIX NormalMatrix(const XMMATRIX& world);
	XMMATRIX RotationMatrix() const;

	//----------------------------------------------------------------------------------------------------------------
	// DATA
	//----------------------------------------------------------------------------------------------------------------
	vec3				_position;
	Quaternion			_rotation;
	vec3				_scale;
	const vec3			_originalPosition;
	const Quaternion	_originalRotation;
};

