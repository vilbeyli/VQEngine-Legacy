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

#ifndef _CAMERA_H
#define _CAMERA_H

#include "Utilities/vectormath.h"
#include "Engine/Settings.h"
#include "Renderer/RenderingEnums.h"

using namespace DirectX;

namespace Settings { struct Camera; struct Window; }

class Input;
class Renderer;

class Camera
{
public:
	Camera();
	~Camera(void);

	void ConfigureCamera(const Settings::Camera& cameraSettings, const Settings::Window& windowSettings, Renderer* pRenderer);

	void SetOthoMatrix(int screenWidth, int screenHeight, float screenNear, float screenFar);
	void SetProjectionMatrix(float fovy, float screenAspect, float screenNear, float screenFar);
	void SetProjectionMatrixHFov(float fovx, float screenAspectInverse, float screenNear, float screenFar);

	void Update(float dt);

	vec3 GetPositionF() const;
	XMMATRIX GetViewMatrix() const;
	XMMATRIX GetViewInverseMatrix() const;
	XMMATRIX GetProjectionMatrix() const;

	// returns World Space frustum plane set 
	//
	FrustumPlaneset GetViewFrustumPlanes() const;
	
	void SetPosition(float x, float y, float z);
	void Rotate(float yaw, float pitch, const float dt);

	void Reset();	// resets camera transform to initial position & orientation
public:
	float Drag;				// 15.0f
	float AngularSpeedDeg;	// 40.0f
	float MoveSpeed;		// 1000.0f

	Settings::Camera m_settings;

private:
	void Move(const float dt);
	void Rotate(const float dt);
	XMMATRIX RotMatrix() const;

private:
	// lod manager stores vec3* mPosition and no need
	// for an interface function to return position pointer.
	friend class LODManager; 

	vec3		mPosition;
	vec3		mVelocity;
	friend class SceneManager;
	float		mYaw, mPitch;
	XMFLOAT4X4	mMatView;
	XMFLOAT4X4	mMatProj;

	RenderTargetID mRT_LinearDepthLUT;
};

#endif