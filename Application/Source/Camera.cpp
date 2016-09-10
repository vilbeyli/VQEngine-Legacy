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

#include "Camera.h"
#include "Input.h"
#include <windows.h>
#include <string>
#include "SystemDefs.h"

#define CAM_ANGULAR_SPEED_DEG 60.0f
#define CAM_MOVE_SPEED 20.0f
#define DEG2RAD XM_PI / 180.0f

Camera::Camera(Input const* inp)
	:
	m_input(inp)
{}

Camera::~Camera(void)
{}

//void * Camera::operator new(size_t sz)
//{
//	return _mm_malloc(sz, 16);
//}
//
//void Camera::operator delete(void * p)
//{
//	_mm_free(p);
//}

void Camera::SetOthoMatrix(int screenWidth, int screenHeight, float screenNear, float screenFar)
{
	XMStoreFloat4x4(&m_orthoMatrix, XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenFar));
}

void Camera::SetProjectionMatrix(float fov, float screenAspect, float screenNear, float screenFar)
{
	XMStoreFloat4x4(&m_projectionMatrix, XMMatrixPerspectiveFovLH(fov, screenAspect, screenNear, screenFar));
}

// updates View Matrix
void Camera::Update(float dt)
{
	Rotate(dt);
	Move(dt);

	XMVECTOR up		= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR pos	= m_transform.GetPositionV();

	//transform the lookat and up vector by rotation matrix
	lookAt	= XMVector3TransformCoord(lookAt, m_transform.RotationMatrix());
	up		= XMVector3TransformCoord(up,	  m_transform.RotationMatrix());

	//translate the lookat
	lookAt = pos + lookAt;

	//create view matrix
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixLookAtLH(pos, lookAt, up));
}


XMFLOAT3 Camera::GetPositionF() const
{
	return m_transform.GetPositionF3();
}

XMMATRIX Camera::GetViewMatrix() const
{
	return XMLoadFloat4x4(&m_viewMatrix);
}

XMMATRIX Camera::GetProjectionMatrix() const
{
	return  XMLoadFloat4x4(&m_projectionMatrix);
}

XMMATRIX Camera::GetOrthoMatrix() const
{
	return  XMLoadFloat4x4(&m_orthoMatrix);
}

void Camera::SetPosition(float x, float y, float z)
{
	m_transform.SetPosition(XMFLOAT3(x, y, z));
}


void Camera::Rotate(float yaw, float pitch, const float dt)
{
	float delta = CAM_ANGULAR_SPEED_DEG * DEG2RAD * dt;
	float yaw_		= yaw   * delta;
	float pitch_	= pitch * delta;
	XMVECTOR rot = XMVectorSet(pitch_, yaw_, 0, 0);
	m_transform.Rotate(rot);
}

// internal update functions
void Camera::Rotate(const float dt)
{
	const long* dxdy = m_input->GetDelta();
	float dy = static_cast<float>(dxdy[1]);
	float dx = static_cast<float>(dxdy[0]);
	Rotate(dx, dy, dt);
}

void Camera::Move(const float dt)
{
	float pitch = m_transform.GetRotationV().m128_f32[0];
	float yaw	= m_transform.GetRotationV().m128_f32[1];
	float roll	= m_transform.GetRotationV().m128_f32[2];
	XMMATRIX MRotation = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	XMVECTOR translation = XMVectorSet(0,0,0,0);
	if (m_input->IsKeyDown(0x41))		translation += XMVector3TransformCoord(Transform::Left,		MRotation);
	if (m_input->IsKeyDown(0x44))		translation += XMVector3TransformCoord(Transform::Right,	MRotation);
	if (m_input->IsKeyDown(0x57))		translation += XMVector3TransformCoord(Transform::Forward,	MRotation);
	if (m_input->IsKeyDown(0x53))		translation += XMVector3TransformCoord(Transform::Backward,	MRotation);
	if (m_input->IsKeyDown(VK_SPACE))	translation += XMVector3TransformCoord(Transform::Up,		MRotation);
	if (m_input->IsKeyDown(VK_CONTROL))	translation += XMVector3TransformCoord(Transform::Down,		MRotation);
	if (m_input->IsKeyDown(VK_SHIFT))	translation *= 5.0f;
	m_transform.Translate(translation * CAM_MOVE_SPEED * dt);
}

//void Camera::SetRotation(float x, float y, float z)
//{
//	m_rotation = XMFLOAT3(x * (float)XM_PI/180.0f, y * (float)XM_PI/180.0f, z * (float)XM_PI/180.0f);
//}
