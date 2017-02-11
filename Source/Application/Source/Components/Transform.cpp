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


const XMVECTOR Transform::Right		= XMVectorSet(+1.0f, +0.0f, +0.0f, 0.0f);
const XMVECTOR Transform::Left		= XMVectorSet(-1.0f, +0.0f, +0.0f, 0.0f);
const XMVECTOR Transform::Up		= XMVectorSet(+0.0f, +1.0f, +0.0f, 0.0f);
const XMVECTOR Transform::Down		= XMVectorSet(+0.0f, -1.0f, +0.0f, 0.0f);
const XMVECTOR Transform::Forward	= XMVectorSet(+0.0f, +0.0f, +1.0f, 0.0f);
const XMVECTOR Transform::Backward	= XMVectorSet(+0.0f, +0.0f, -1.0f, 0.0f);

Transform::Transform(const XMFLOAT3 position, const XMFLOAT3 rotation, const XMFLOAT3 scale) 
	: 
	m_position(position),
	m_rotation(rotation), 
	m_scale(scale),
	//mOrientation_(glm::quat()),
	Component(ComponentType::TRANSFORM, "Transform") 
{}

Transform::~Transform() {}

void Transform::Translate(XMVECTOR translation)
{
	XMVECTOR pos = XMLoadFloat3(&m_position);
	pos += translation;
	XMStoreFloat3(&m_position, pos);
}

void Transform::Rotate(XMVECTOR rotation)
{
	XMVECTOR rot = XMLoadFloat3(&m_rotation);
	rot += rotation;
	XMStoreFloat3(&m_rotation, rot);
}

void Transform::Scale(XMVECTOR scl)
{
	XMStoreFloat3(&m_scale, scl);
}

XMMATRIX Transform::WorldTransformationMatrix() const
{
	XMVECTOR scale = GetScale();
	XMVECTOR translation = GetPositionV();
	XMVECTOR zeroVec = XMVectorSet(0, 0, 0, 1);
	XMVECTOR rotation = XMQuaternionRotationRollPitchYawFromVector(GetRotationV());
	return XMMatrixAffineTransformation(scale, zeroVec, rotation, translation);
}

XMMATRIX Transform::RotationMatrix() const
{
	float pitch = m_rotation.x;
	float yaw	= m_rotation.y;
	float roll	= m_rotation.z;
	return  XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
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
