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

Camera::Camera(void)
{
	m_position = XMFLOAT3(0, 0, 0);
	m_rotation = XMFLOAT3(0, 0, 0);
}

Camera::~Camera(void)
{
}

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
void Camera::Update()
{
	XMVECTOR up		= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR pos	= XMLoadFloat3(&m_position);

	XMMATRIX rotationMatrix = XMMatrixRotationRollPitchYaw(m_rotation.x, m_rotation.y, m_rotation.z);

	//transform the lookat and up vector by rotation matrix
	lookAt	= XMVector3TransformCoord(lookAt, rotationMatrix);
	up		= XMVector3TransformCoord(up, rotationMatrix);

	//translate the lookat
	lookAt = pos + lookAt;

	//create view matrix
	XMStoreFloat4x4(&m_viewMatrix, XMMatrixLookAtLH(pos, lookAt, up));
}

XMVECTOR Camera::GetPosition()
{
	return XMLoadFloat3(&m_position);
}

XMVECTOR Camera::GetRotation()
{
	return XMLoadFloat3(&m_rotation);
}

XMMATRIX Camera::GetViewMatrix()
{
	return XMLoadFloat4x4(&m_viewMatrix);
}

XMMATRIX Camera::GetProjectionMatrix()
{
	return  XMLoadFloat4x4(&m_projectionMatrix);
}

XMMATRIX Camera::GetOrthoMatrix()
{
	return  XMLoadFloat4x4(&m_orthoMatrix);
}

void Camera::SetPosition(float x, float y, float z)
{
	m_position = XMFLOAT3(x, y, z);
}

void Camera::SetRotation(float x, float y, float z)
{
	m_rotation = XMFLOAT3(x * (float)XM_PI/180.0f, y * (float)XM_PI/180.0f, z * (float)XM_PI/180.0f);
}
