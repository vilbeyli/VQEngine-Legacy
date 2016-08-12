#ifndef _CAMERA_H
#define _CAMERA_H

#include <directxmath.h>

using namespace DirectX;

//__declspec(align(16)) class Camera
class Camera
{
public:
	Camera(void);
	~Camera(void);

	// new/delete overrides for 16-bit alignment 
	// to be used by XMVECTORs to utilize SSE instructions
	//void* operator new(size_t sz);
	//void  operator delete(void* p);

	void SetOthoMatrix(int screenWidth, int screenHeight, float screenNear, float screenFar);
	void SetProjectionMatrix(float fov, float screenAspect, float screenNear, float screenFar);

	void Update();

	XMVECTOR GetPosition();
	XMVECTOR GetRotation();
	XMFLOAT4X4 GetViewMatrix();
	XMFLOAT4X4 GetProjectionMatrix();
	XMFLOAT4X4 GetOrthoMatrix();

	void SetPosition(float x, float y, float z);
	void SetRotation(float x, float y, float z);

private:
	XMFLOAT3	m_position;
	XMFLOAT3	m_rotation;
	XMFLOAT4X4	m_viewMatrix;
	XMFLOAT4X4	m_projectionMatrix;
	XMFLOAT4X4	m_orthoMatrix;
};

#endif