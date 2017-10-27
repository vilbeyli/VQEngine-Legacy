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
#include "Transform.h"

Transform::Transform(const vec3& position, const Quaternion& rotation, const vec3& scale)
	:
	_position(position),
	_rotation(rotation),
	_originalPosition(position),
	_originalRotation(rotation),
	_scale(scale),
	Component(ComponentType::TRANSFORM, "Transform")
{}

Transform::~Transform() {}

Transform & Transform::operator=(const Transform & t)
{
	this->_position = t._position;
	this->_rotation = t._rotation;
	this->_scale    = t._scale;
	return *this;
}

void Transform::Translate(const vec3& translation)
{
	_position = _position + translation;
}

void Transform::Translate(float x, float y, float z)
{
	_position = _position + vec3(x, y, z);
}

void Transform::Scale(const vec3& scl)
{
	_scale = scl;
}

void Transform::RotateAroundPointAndAxis(const vec3& axis, float angle, const vec3& point)
{ 
	vec3 R(_position - point);
	const Quaternion rot = Quaternion::FromAxisAngle(axis, angle);
	R = rot.TransformVector(R);
	_position = point + R;
}

XMMATRIX Transform::WorldTransformationMatrix() const
{
	XMVECTOR scale = _scale;
	XMVECTOR translation = _position;

	//Quaternion Q = Quaternion(GetRotationF3());
	Quaternion Q = _rotation;
	XMVECTOR rotation = XMVectorSet(Q.V.x(), Q.V.y(), Q.V.z(), Q.S);
	//XMVECTOR rotOrigin = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR rotOrigin = XMVectorZero();
	return XMMatrixAffineTransformation(scale, rotOrigin, rotation, translation);
}

DirectX::XMMATRIX Transform::WorldTransformationMatrix_NoScale() const
{
	XMVECTOR scale = XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	XMVECTOR translation = _position;
	Quaternion Q = _rotation;
	XMVECTOR rotation = XMVectorSet(Q.V.x(), Q.V.y(), Q.V.z(), Q.S);
	XMVECTOR rotOrigin = XMVectorZero();
	return XMMatrixAffineTransformation(scale, rotOrigin, rotation, translation);
}

XMMATRIX Transform::RotationMatrix() const
{
	return _rotation.Matrix();
}

// builds normal matrix from world matrix, ignoring translation
// and using inverse-transpose of rotation/scale matrix
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
