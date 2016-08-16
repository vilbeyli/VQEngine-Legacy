#pragma once

#include <windows.h>
#include "GeometryGenerator.h"
#include <string>
#include <vector>

#include "Shader.h"

// forward declarations
class D3DManager;
struct ID3D11Device;
struct ID3D11DeviceContext;

class BufferObject;
class Camera;
class Shader;

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Init(int width, int height, HWND hwnd);
	void Exit();
	bool MakeFrame();

	void EnableAlphaBlending(bool enable);
	void EnableZBuffer(bool enable);

	ShaderID AddShader(const std::string& shdFileName, const std::string& fileRoot = "");

	// state management
	void SetShader(ShaderID);
	void Reset();

	void SetCamera(Camera* m_camera);
private:
	bool Render();

	void GeneratePrimitives();

public:
	static const bool FULL_SCREEN  = false;
	static const bool VSYNC = true;
private:
	D3DManager*						m_Direct3D;
	HWND							m_hWnd;
	ID3D11Device*					m_device;
	ID3D11DeviceContext*			m_deviceContext;

	GeometryGenerator				m_geom;

	Camera*							m_mainCamera;
	std::vector<BufferObject*>		m_bufferObjects;
	std::vector<Shader*>			m_shaders;

	// state variables
	ShaderID						m_activeShader;

};


// FUNCTION PROTOTYPES
//----------------------------------------------------------------------


// GLOBALS
//----------------------------------------------------------------------
const float g_farPlane		= 1000.0f;
const float g_nearPlane		= 0.1f;

 