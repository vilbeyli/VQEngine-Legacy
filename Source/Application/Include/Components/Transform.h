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
#include "Component.h"
#include <DirectXMath.h>
#include "Quaternion.h"

using namespace DirectX;

#include "utils.h"

#define USE_QUATERNIONS

class Transform : public Component
{
	friend class RigidBody; // hack
public:
	//static Transform* Deserialize(Document & params);
	//virtual void		Serialize(Value& val, Document::AllocatorType& allocator) override;
	static const ComponentType ComponentId = Component::TRANSFORM;

	static const XMVECTOR Right;//= XMVectorSet(+1.0f, +0.0f, +0.0f, 0.0f);
	static const XMVECTOR Left;//= XMVectorSet(-1.0f, +0.0f, +0.0f, 0.0f);
	static const XMVECTOR Up;//= XMVectorSet(+0.0f, +1.0f, +0.0f, 0.0f);
	static const XMVECTOR Down;//= XMVectorSet(+0.0f, -1.0f, +0.0f, 0.0f);
	static const XMVECTOR Forward;//= XMVectorSet(+0.0f, +0.0f, +1.0f, 0.0f);
	static const XMVECTOR Backward;//= XMVectorSet(+0.0f, +0.0f, -1.0f, 0.0f);

	// CONSTRUCTOR / DESTRUCTOR
	//-----------------------------------------------------------------------------------
	Transform(const XMFLOAT3 position = XMFLOAT3(0.0f, 0.0f, 0.0f),
				const XMFLOAT3 rotation = XMFLOAT3(0.0f, 0.0f, 0.0f),
				const XMFLOAT3 scale = XMFLOAT3(1.0f, 1.0f, 1.0f));
	~Transform();

	// GETTERS & SETTERS
	//-----------------------------------------------------------------------------------
	inline XMVECTOR GetPositionV()  const { return XMLoadFloat3(&m_position); }
	inline XMFLOAT3 GetPositionF3() const { return m_position; }
#if !defined(USE_QUATERNIONS)
	inline XMVECTOR GetRotationV()  const { return XMLoadFloat3(&m_rotation); }
	inline XMFLOAT3 GetRotationF3() const { return m_rotation; }
#else
	inline Quaternion GetRotationQ() const { return m_rotation; }
#endif
	inline XMVECTOR GetScale()		const { return XMLoadFloat3(&m_scale); }
	inline XMFLOAT3 GetScaleF3()	const { return m_scale; }

#if !defined(USE_QUATERNIONS)
	inline void SetRotation(XMFLOAT3 rot) { m_rotation = rot; }
	inline void SetRotation(float x, float y, float z) { m_rotation = XMFLOAT3(x, y, z); }
#else
	inline void SetRotationQ(const Quaternion& Q) { m_rotation = Q; }
#endif
	inline void SetRotationDeg(float x, float y, float z) { m_rotation = XMFLOAT3(x * DEG2RAD, y * DEG2RAD, z * DEG2RAD); }
	inline void SetScale(XMFLOAT3 scale) { m_scale = scale; }
	inline void SetScale(float x, float y, float z) { m_scale = XMFLOAT3(x, y, z); }
	inline void SetScaleUniform(float scl) { m_scale = XMFLOAT3(scl, scl, scl); }
	inline void SetPosition(XMFLOAT3 val) { m_position = val; }
	inline void SetPosition(const XMVECTOR& val) { XMStoreFloat3(&m_position, val); }
	inline void SetPosition(float x, float y, float z) { m_position = XMFLOAT3(x, y, z); }
	//inline void SetOrientation(glm::quat orient) { mOrientation_ = orient; }

	// MOVEMENT
	//-----------------------------------------------------------------------------------
	// Translates the position by the provided vector
	void Translate(XMVECTOR translation);

	// Rotates the transform by provided vector (adds x,y,z radian angles)
	void RotateEulerRad(const XMVECTOR& rotation);
	void RotateEulerRad(const XMFLOAT3& rotation);
	void RotateQuat(const Quaternion& q);
	void RotateAroundPointAndAxis(const vec3& axis, float angle, vec3& point);

	void Scale(XMVECTOR scl);

	// Returns the Model Transformation Matrix
	XMMATRIX WorldTransformationMatrix() const;
	XMMATRIX WorldTransformationMatrix_NoScale() const;
	static XMMATRIX NormalMatrix(const XMMATRIX& world);
	XMMATRIX RotationMatrix() const;

	// transforms a vector from local to global space
	XMFLOAT3 TransfromVector(XMFLOAT3 v);

private:
	// TRANSLATION
	XMFLOAT3	m_position;

	// ROTATION
#if !defined(USE_QUATERNIONS)
	XMFLOAT3	m_rotation;	// radians		
#else
	Quaternion	m_rotation;
#endif

	// SCALE
	XMFLOAT3 m_scale;

};

