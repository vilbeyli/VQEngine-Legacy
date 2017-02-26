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

// Component header files
//#include "PhysicsComponent.h"
#include "Components/Transform.h"

const XMVECTOR Transform::Right = XMVectorSet(+1.0f, +0.0f, +0.0f, 0.0f);
const XMVECTOR Transform::Left = XMVectorSet(-1.0f, +0.0f, +0.0f, 0.0f);
const XMVECTOR Transform::Up = XMVectorSet(+0.0f, +1.0f, +0.0f, 0.0f);
const XMVECTOR Transform::Down = XMVectorSet(+0.0f, -1.0f, +0.0f, 0.0f);
const XMVECTOR Transform::Forward = XMVectorSet(+0.0f, +0.0f, +1.0f, 0.0f);
const XMVECTOR Transform::Backward = XMVectorSet(+0.0f, +0.0f, -1.0f, 0.0f);

Transform::Transform(const XMFLOAT3 position, const XMFLOAT3 rotation, const XMFLOAT3 scale)
	:
	m_position(position),
	m_rotation(rotation),
	m_scale(scale),
	Component(ComponentType::TRANSFORM, "Transform")
{}

Transform::~Transform() {}

void Transform::Translate(XMVECTOR translation)
{
	XMVECTOR pos = XMLoadFloat3(&m_position);
	pos += translation;
	XMStoreFloat3(&m_position, pos);
}

void Transform::RotateEulerRad(const XMVECTOR& rotation)
{
	XMFLOAT3 rotF3;
	XMStoreFloat3(&rotF3, rotation);
	RotateEulerRad(rotF3);
}

void Transform::RotateEulerRad(const XMFLOAT3& rotation)
{

#if defined(USE_QUATERNIONS)
	Quaternion rot = Quaternion(rotation);	// todo;:
	m_rotation = m_rotation * rot;
#else
	// transform uses euler angles
	m_rotation.x += rotation.x;
	m_rotation.y += rotation.y;
	m_rotation.z += rotation.z;
#endif
}

#if defined(USE_QUATERNIONS)
void Transform::RotateQuat(const Quaternion & q)
{
	m_rotation = m_rotation * q;
}
#endif

void Transform::Scale(XMVECTOR scl)
{
	XMStoreFloat3(&m_scale, scl);
}

XMMATRIX Transform::WorldTransformationMatrix() const
{
	XMVECTOR scale = GetScale();
	XMVECTOR translation = GetPositionV();

	//Quaternion Q = Quaternion(GetRotationF3());
	Quaternion Q = m_rotation;
	XMVECTOR rotation = XMVectorSet(Q.V.x, Q.V.y, Q.V.z, Q.S);
	//XMVECTOR rotOrigin = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR rotOrigin = XMVectorZero();
	return XMMatrixAffineTransformation(scale, rotOrigin, rotation, translation);
}

DirectX::XMMATRIX Transform::WorldTransformationMatrix_NoScale() const
{
	XMVECTOR scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMVECTOR translation = GetPositionV();

	//Quaternion Q = Quaternion(GetRotationF3());
	Quaternion Q = m_rotation;
	XMVECTOR rotation = XMVectorSet(Q.V.x, Q.V.y, Q.V.z, Q.S);
	//XMVECTOR rotOrigin = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR rotOrigin = XMVectorZero();
	return XMMatrixAffineTransformation(scale, rotOrigin, rotation, translation);
}


XMMATRIX Transform::RotationMatrix() const
{
#if defined(USE_QUATERNIONS)
	return m_rotation.Matrix();
#else
	float pitch = m_rotation.x;
	float yaw = m_rotation.y;
	float roll = m_rotation.z;
	return  XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
#endif
}

// transforms a vector from local to global space
XMFLOAT3 Transform::TransfromVector(XMFLOAT3 v)
{
	//XMMATRIX rotateX = glm::rotate(mRotation_.x*3.1415f / 180.0f, XMFLOAT3(1, 0, 0));
	//XMMATRIX rotateY = glm::rotate(mRotation_.y*3.1415f / 180.0f, XMFLOAT3(0, 1, 0));
	//XMMATRIX rotateZ = glm::rotate(mRotation_.z*3.1415f / 180.0f, XMFLOAT3(0, 0, 1));
	//XMMATRIX mRot = rotateX * rotateY * rotateZ;

	//return XMFLOAT3(mRot * glm::vec4(v, 0.0));
	// TODO: Implement
	return XMFLOAT3();
}

DirectX::XMMATRIX Transform::NormalMatrix(const XMMATRIX& world)
{
	XMMATRIX nrm = world;
	nrm.r[3].m128_f32[0] = nrm.r[3].m128_f32[1] = nrm.r[3].m128_f32[2] = 0;
	nrm.r[3].m128_f32[3] = 1;
	XMVECTOR Det = XMMatrixDeterminant(nrm);
	nrm = XMMatrixInverse(&Det, nrm);
	nrm = XMMatrixTranspose(nrm);
	return nrm;
}
