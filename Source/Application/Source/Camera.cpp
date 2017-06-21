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
#include "Engine.h"

#include "utils.h"

Camera::Camera()
	:
	MoveSpeed(1000.0f),
	AngularSpeedDeg(40.0f),
	Drag(15.0f)

	// todo:	1- default init rest
	//			2- read pos/rot/scl from scene file
{}

Camera::~Camera(void)
{}


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
	XMVECTOR pos	= XMLoadFloat3(&m_pos);	
	XMMATRIX MRot	= RotMatrix();

	//transform the lookat and up vector by rotation matrix
	lookAt	= XMVector3TransformCoord(lookAt, MRot);
	up		= XMVector3TransformCoord(up,	  MRot);

	//translate the lookat
	lookAt = pos + lookAt;

	//create view matrix
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixLookAtLH(pos, lookAt, up));

	// move based on velocity
	auto P = XMLoadFloat3(&m_pos);
	auto V = XMLoadFloat3(&m_velocity);
	P += V * dt;
	XMStoreFloat3(&m_pos, P);

	//----------------------------------------------------------------------
	// debug code 
	//----------------------------------------------------------------------
	//{
	//	XMVECTOR dir	= XMVector3Normalize(lookAt - pos);
	//	XMVECTOR wing	= XMVector3Cross(up, dir);
	//	XMMATRIX R = XMMatrixIdentity(); 
	//	R.r[0] = wing;
	//	R.r[1] = up;
	//	R.r[2] = dir;
	//	R.r[0].m128_f32[3] = R.r[1].m128_f32[3] = R.r[2].m128_f32[3] = 0;
	//	R = XMMatrixTranspose(R);
	//	
	//	XMMATRIX T = XMMatrixIdentity();
	//	T.r[3] = -pos;
	//	T.r[3].m128_f32[3] = 1.0;
	//	
	//	XMMATRIX viewMat = T * R;	// this is for ViewMatrix
	//	//	orienting our model using this, we want the inverse of viewmat
	//	// XMMATRIX rotMatrix = XMMatrixTranspose(R) * T.inverse();
	//
	//	int a = 5;
	//}
	//----------------------------------------------------------------------
	// end debug code 
	//----------------------------------------------------------------------
}


XMFLOAT3 Camera::GetPositionF() const
{
	return m_pos;
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

XMMATRIX Camera::RotMatrix() const
{
	return XMMatrixRotationRollPitchYaw(m_pitch, m_yaw, 0.0f);
}

void Camera::SetPosition(float x, float y, float z)
{
	m_pos = XMFLOAT3(x, y, z);
}

void Camera::Rotate(float yaw, float pitch, const float dt)
{
	const float delta  = AngularSpeedDeg * DEG2RAD * dt;
	m_yaw	+= yaw   * delta;
	m_pitch += pitch * delta;
	
	if (m_pitch > +90.0f * DEG2RAD) m_pitch = +90.0f * DEG2RAD;
	if (m_pitch < -90.0f * DEG2RAD) m_pitch = -90.0f * DEG2RAD;
}

// internal update functions
void Camera::Rotate(const float dt)
{
	auto m_input = ENGINE->INP();
	const long* dxdy = m_input->GetDelta();
	float dy = static_cast<float>(dxdy[1]);
	float dx = static_cast<float>(dxdy[0]);
	Rotate(dx, dy, dt);
}

void Camera::Move(const float dt)
{
	auto m_input = ENGINE->INP();
	XMMATRIX MRotation	 = RotMatrix();
	XMVECTOR translation = XMVectorSet(0,0,0,0);
	if (m_input->IsKeyDown('A'))		translation += XMVector3TransformCoord(vec3::Left,		MRotation);
	if (m_input->IsKeyDown('D'))		translation += XMVector3TransformCoord(vec3::Right,		MRotation);
	if (m_input->IsKeyDown('W'))		translation += XMVector3TransformCoord(vec3::Forward,	MRotation);
	if (m_input->IsKeyDown('S'))		translation += XMVector3TransformCoord(vec3::Back,		MRotation);
	if (m_input->IsKeyDown('E'))		translation += XMVector3TransformCoord(vec3::Up,		MRotation);
	if (m_input->IsKeyDown('Q'))		translation += XMVector3TransformCoord(vec3::Down,		MRotation);
	if (m_input->IsKeyDown(VK_SHIFT))	translation *= 5.0f;
	
	// update velocity
	// todo: test vec3 here
	auto V = XMLoadFloat3(&m_velocity);
	V += (translation * MoveSpeed - V * Drag) * dt;
	XMStoreFloat3(&m_velocity, V);
}
