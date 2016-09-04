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
#pragma once

#include <windows.h>
#include "GeometryGenerator.h"
#include "Shader.h"
//#include <string>
//#include <vector>

#include "Mesh.h"

// forward declarations
class D3DManager;
struct ID3D11Device;
struct ID3D11DeviceContext;

class BufferObject;
class Camera;
class Shader;


// typedefs
typedef int ShaderID;
typedef int BufferID;
typedef int RasterizerStateID;
typedef ID3D11RasterizerState RasterizerState;

enum RASTERIZER_STATE
{
	CULL_NONE = 0,
	CULL_FRONT,
	CULL_BACK,

	RS_COUNT
};

class Renderer
{
public:
	Renderer();
	~Renderer();

	bool Init(int width, int height, HWND hwnd);
	void Exit();
	HWND GetWindow() const;
	float AspectRatio() const;
	unsigned WindowHeight() const;
	unsigned WindowWidth() const;

	void EnableAlphaBlending(bool enable);
	void EnableZBuffer(bool enable);

	// shader interface
	ShaderID	AddShader(const std::string& shdFileName, const std::string& fileRoot, const std::vector<InputLayout>& layouts);

	// state management
	void SetShader(ShaderID);
	void SetCamera(Camera* m_camera);
	void SetConstant4x4f(const char* cName, const XMMATRIX& matrix);
	void SetConstant3f(const char* cName, const XMFLOAT3& float3);
	void SetRasterizerState(int stateID);

	void Begin(const float clearColor[4]);
	void Reset();
	void Apply();
	void DrawIndexed();
	void End();
	void PollShaderFiles();

	void SetViewport(const unsigned width, const unsigned height);
	void SetBufferObj(int BufferID);
private:

	void GeneratePrimitives();
	void PollThread();
	void OnShaderChange(LPTSTR dir);

	void InitRasterizerStates();

public:
	static const bool FULL_SCREEN  = false;
	static const bool VSYNC = false;	// insane input lag; turned off

private:
	D3DManager*						m_Direct3D;
	HWND							m_hWnd;
	ID3D11Device*					m_device;
	ID3D11DeviceContext*			m_deviceContext;

	GeometryGenerator				m_geom;

	// render data
	Camera*							m_mainCamera;
	D3D11_VIEWPORT					m_viewPort;

	std::vector<BufferObject*>		m_bufferObjects;
	std::vector<Shader*>			m_shaders;

	// state variables
	ShaderID						m_activeShader;
	BufferID						m_activeBuffer;
	
	std::vector<RasterizerState*>	m_rasterizerStates;
	
	// performance counters
	unsigned long long				m_frameCount;
};


 