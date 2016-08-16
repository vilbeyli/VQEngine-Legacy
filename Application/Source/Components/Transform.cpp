// Component header files
//#include "PhysicsComponent.h"
#include "Transform.h"


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

XMMATRIX Transform::ModelTransformationMatrix() const
{
	// TODO: Implement
	return XMMATRIX();
}

XMMATRIX Transform::RotationMatrix() const
{
	//XMMATRIX rotateX = glm::rotate(mRotation_.x*3.1415f / 180.0f, XMFLOAT3(1, 0, 0));
	//XMMATRIX rotateY = glm::rotate(mRotation_.y*3.1415f / 180.0f, XMFLOAT3(0, 1, 0));
	//XMMATRIX rotateZ = glm::rotate(mRotation_.z*3.1415f / 180.0f, XMFLOAT3(0, 0, 1));
	//return rotateX * rotateY * rotateZ;
	// TODO: Implement
	return XMMATRIX();
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
