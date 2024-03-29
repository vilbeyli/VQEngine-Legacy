//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2018  - Volkan Ilbeyli
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
#include "Utilities/Log.h"
#include "Application/Input.h"
#include "Engine/Engine.h"

#define CAMERA_DEBUG 1

Camera::Camera()
	:
	MoveSpeed(1000.0f),
	AngularSpeedDeg(20.0f),
	Drag(9.5f),
	mPitch(0.0f),
	mYaw(0.0f),
	mPosition(vec3::ZeroF3),
	mVelocity(vec3::ZeroF3),
	mRT_LinearDepthLUT(-1)
{
	XMStoreFloat4x4(&mMatProj, XMMatrixIdentity());
	XMStoreFloat4x4(&mMatView, XMMatrixIdentity());
}

Camera::~Camera(void)
{}

void Camera::ConfigureCamera(const Settings::Camera& cameraSettings, const Settings::Window& windowSettings, Renderer* pRenderer)
{
	const auto& NEAR_PLANE = cameraSettings.nearPlane;
	const auto& FAR_PLANE = cameraSettings.farPlane;
	const float AspectRatio = static_cast<float>(windowSettings.width) / windowSettings.height;
	const float VerticalFoV = cameraSettings.fovV * DEG2RAD;

	m_settings = cameraSettings;
	m_settings.aspect = AspectRatio;

	SetOthoMatrix(windowSettings.width, windowSettings.height, NEAR_PLANE, FAR_PLANE);
	SetProjectionMatrix(VerticalFoV, AspectRatio, NEAR_PLANE, FAR_PLANE);

	SetPosition(cameraSettings.x, cameraSettings.y, cameraSettings.z);
	mYaw = mPitch = 0;
	Rotate(cameraSettings.yaw * DEG2RAD, cameraSettings.pitch * DEG2RAD, 1.0f);

	// if we haven't initialized the LUT render target
	//if (mRT_LinearDepthLUT == -1)
	//{
	//	mRT_LinearDepthLUT = pRenderer->AddRenderTarget();
	//}

// PANINI TEST
//
//#if 1
//	SetProjectionMatrix(m_settings.fovV * DEG2RAD, m_settings.aspect, m_settings.nearPlane, m_settings.farPlane);
//#else  // test fov
//	static float gFoVx = 90.0f;
//	static float gFoVy = 45.0f;
//
//	float dFoV = 0.0f;
//	//if (ENGINE->INP()->IsScrollDown()) dFoV = -5.0f;
//	//if (ENGINE->INP()->IsScrollUp()  ) dFoV = +5.0f;
//	gFoVx += dFoV;
//	gFoVy += dFoV;
//
//	m_settings.fovH = gFoVx;
//
//	SetProjectionMatrixHFov(gFoVx * DEG2RAD, 1.0f / m_settings.aspect, m_settings.nearPlane, m_settings.farPlane);
//	//SetProjectionMatrix(gFoVy * DEG2RAD, m_settings.aspect, m_settings.nearPlane, m_settings.farPlane);
//#endif
}


void Camera::SetOthoMatrix(int screenWidth, int screenHeight, float screenNear, float screenFar)
{
	XMStoreFloat4x4(&mMatProj, XMMatrixOrthographicLH((float)screenWidth, (float)screenHeight, screenNear, screenFar));
}

void Camera::SetProjectionMatrix(float fovy, float screenAspect, float screenNear, float screenFar)
{
	XMStoreFloat4x4(&mMatProj, XMMatrixPerspectiveFovLH(fovy, screenAspect, screenNear, screenFar));
}

void Camera::SetProjectionMatrixHFov(float fovx, float screenAspectInverse, float screenNear, float screenFar)
{	// horizonital FOV
	const float FarZ = screenFar; float NearZ = screenNear;
	const float r = screenAspectInverse;
	
	const float Width = 1.0f / tanf(fovx*0.5f);
	const float Height = Width / r;
	const float fRange = FarZ / (FarZ - NearZ);

	XMMATRIX M;	
	M.r[0].m128_f32[0] = Width;
	M.r[0].m128_f32[1] = 0.0f;
	M.r[0].m128_f32[2] = 0.0f;
	M.r[0].m128_f32[3] = 0.0f;

	M.r[1].m128_f32[0] = 0.0f;
	M.r[1].m128_f32[1] = Height;
	M.r[1].m128_f32[2] = 0.0f;
	M.r[1].m128_f32[3] = 0.0f;

	M.r[2].m128_f32[0] = 0.0f;
	M.r[2].m128_f32[1] = 0.0f;
	M.r[2].m128_f32[2] = fRange;
	M.r[2].m128_f32[3] = 1.0f;

	M.r[3].m128_f32[0] = 0.0f;
	M.r[3].m128_f32[1] = 0.0f;
	M.r[3].m128_f32[2] = -fRange * NearZ;
	M.r[3].m128_f32[3] = 0.0f;
	XMStoreFloat4x4(&mMatProj, M);
}

// updates View Matrix
void Camera::Update(float dt)
{
	Rotate(dt);
	Move(dt);

	XMVECTOR up		= XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR pos	= mPosition;	
	XMMATRIX MRot	= RotMatrix();

	//transform the lookat and up vector by rotation matrix
	lookAt	= XMVector3TransformCoord(lookAt, MRot);
	up		= XMVector3TransformCoord(up,	  MRot);

	//translate the lookat
	lookAt = pos + lookAt;

	//create view matrix
	XMStoreFloat4x4(&mMatView, XMMatrixLookAtLH(pos, lookAt, up));

	// move based on velocity
	XMVECTOR P = mPosition;
	XMVECTOR V = mVelocity;
	P += V * dt;
	mPosition = P;

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



#if CAMERA_DEBUG
	if(ENGINE->INP()->IsKeyTriggered("T"))
	{
		Log::Info("Camera: ");
		Log::Info("Position | (Yaw,Pitch) :\t %.2f  %.2f  %.2f \t %.2f %.2f", mPosition.x(), mPosition.y(), mPosition.z(), mYaw * RAD2DEG, mPitch * RAD2DEG);
		
		//Log::Info("Rotation:\t (%.2f, %.2f, %.2f, %.2f)", );
	}
#endif

}


vec3 Camera::GetPositionF() const
{
	return mPosition;
}

XMMATRIX Camera::GetViewMatrix() const
{
	return XMLoadFloat4x4(&mMatView);
}

XMMATRIX Camera::GetViewInverseMatrix() const
{
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR lookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	XMVECTOR pos = mPosition;
	XMMATRIX MRot = RotMatrix();

	XMVECTOR dir	= XMVector3Normalize(lookAt - pos);
	XMVECTOR wing	= XMVector3Cross(up, dir);
	XMMATRIX R = XMMatrixIdentity(); 
	R.r[0] = wing;
	R.r[1] = up;
	R.r[2] = dir;
	R.r[0].m128_f32[3] = R.r[1].m128_f32[3] = R.r[2].m128_f32[3] = 0;
	//R = XMMatrixTranspose(R);
	
	XMMATRIX T = XMMatrixIdentity();
	T.r[3] = pos;
	T.r[3].m128_f32[3] = 1.0;
	
	XMMATRIX viewInverse = R * T;	// this is for ViewMatrix
	//	orienting our model using this, we want the inverse of view mat
	// XMMATRIX rotMatrix = XMMatrixTranspose(R) * T.inverse();

	XMMATRIX view = XMLoadFloat4x4(&mMatView);
	XMVECTOR det = XMMatrixDeterminant(view);
	XMMATRIX test = XMMatrixInverse(&det, view);

	return test;
}

XMMATRIX Camera::GetProjectionMatrix() const
{
	return  XMLoadFloat4x4(&mMatProj);
}

FrustumPlaneset Camera::GetViewFrustumPlanes() const
{
	return FrustumPlaneset::ExtractFromMatrix(GetViewMatrix() * GetProjectionMatrix());
}

XMMATRIX Camera::RotMatrix() const
{
	return XMMatrixRotationRollPitchYaw(mPitch, mYaw, 0.0f);
}

void Camera::SetPosition(float x, float y, float z)
{
	mPosition = vec3(x, y, z);
}

void Camera::Rotate(float yaw, float pitch, const float dt)
{
	mYaw   += yaw   * dt;
	mPitch += pitch * dt;
	
	if (mPitch > +90.0f * DEG2RAD) mPitch = +90.0f * DEG2RAD;
	if (mPitch < -90.0f * DEG2RAD) mPitch = -90.0f * DEG2RAD;
}

void Camera::Reset()
{
	const Settings::Camera & cameraSettings = m_settings;
	SetPosition(cameraSettings.x, cameraSettings.y, cameraSettings.z);
	mYaw = mPitch = 0;
	Rotate(cameraSettings.yaw * DEG2RAD, cameraSettings.pitch * DEG2RAD, 1.0f);
}

// internal update functions
void Camera::Rotate(const float dt)
{
	auto m_input = ENGINE->INP();
	const long* dxdy = m_input->GetDelta();
	float dy = static_cast<float>(dxdy[1]);
	float dx = static_cast<float>(dxdy[0]);

	const float delta = AngularSpeedDeg * DEG2RAD * dt;
	Rotate(dx, dy, delta);
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
	if (m_input->IsKeyDown(VK_SHIFT))	translation *= 2.0f;
	translation *= 4.0f;

	XMVECTOR V = mVelocity;
	V += (translation * MoveSpeed - V * Drag) * dt;
	mVelocity = V;
}
