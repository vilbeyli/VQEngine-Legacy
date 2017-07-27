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

#include "GeometryGenerator.h"
#include "D3DManager.h"
#include "Shader.h"
#include "Texture.h"
#include "Settings.h"

#include "Mesh.h"
#include "GameObject.h"

#include "RenderCommands.h"

#include <memory>

// forward declarations
class D3DManager;
struct ID3D11Device;
struct ID3D11DeviceContext;
class BufferObject;
class Camera;
class Shader;
struct Light;

namespace DirectX { class ScratchImage; }

using Viewport = D3D11_VIEWPORT;

// typedefs
using ShaderID            = int;
using BufferID            = int;
using TextureID           = int;
using RasterizerStateID   = int;
using DepthStencilStateID = int;
using RenderTargetID	  = int;
using DepthStencilID	  = int;

using RasterizerState   = ID3D11RasterizerState;
using DepthStencilState = ID3D11DepthStencilState;
using DepthStencil		= ID3D11DepthStencilView;

enum class DEFAULT_RS_STATE
{
	CULL_NONE = 0,
	CULL_FRONT,
	CULL_BACK,

	RS_COUNT
};

struct RenderTarget
{
	RenderTarget() : _renderTargetView(nullptr) {}
	Texture						_texture;
	ID3D11RenderTargetView*		_renderTargetView;
};

class Renderer
{
	friend class Engine;
	friend struct TextureSetCommand;

public:
	Renderer();
	~Renderer();

	bool Initialize(HWND hwnd, const Settings::Window& settings);
	void		Exit();
	inline HWND		GetWindow()		const { return m_hWnd; };
	inline float	AspectRatio()	const { return m_Direct3D->AspectRatio(); };
	inline unsigned	WindowHeight()	const { return m_Direct3D->WindowHeight(); };
	inline unsigned	WindowWidth()	const { return m_Direct3D->WindowWidth(); };
	inline RenderTargetID GetDefaultRenderTarget() const		{ return m_state._mainRenderTarget; }
	inline TextureID	  GetDefaultRenderTargetTexture() const { return m_renderTargets[m_state._mainRenderTarget]._texture._id; }

	// resource interface
	ShaderID			AddShader(const std::string& shdFileName, const std::string& fileRoot, const std::vector<InputLayout>& layouts, bool geoShader = false);
	RasterizerStateID	AddRSState(RS_CULL_MODE cullMode, RS_FILL_MODE fillMode, bool enableDepthClip);
	
	// todo: return textureID to outside, use Texture& private
	const Texture&		CreateTextureFromFile(const std::string& shdFileName, const std::string& fileRoot = s_textureRoot);
	const Texture&		CreateTexture2D(int widht, int height);
	TextureID			CreateTexture2D(D3D11_TEXTURE2D_DESC& textureDesc);
	TextureID			CreateCubemapTexture(const std::vector<std::string>& textureFiles);
	
	RenderTargetID		AddRenderTarget(D3D11_TEXTURE2D_DESC& RTTextureDesc, D3D11_RENDER_TARGET_VIEW_DESC& RTVDesc);
	DepthStencilID		AddDepthStencil(const D3D11_DEPTH_STENCIL_VIEW_DESC& dsvDesc, ID3D11Texture2D*& surface);
	DepthStencilStateID AddDepthStencilState();	// todo params
	DepthStencilStateID AddDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& dsDesc);

	const Shader*		GetShader(ShaderID shader_id) const;
	const Texture&		GetTexture(TextureID) const;

	// state management
	void SetViewport(const unsigned width, const unsigned height);
	void SetViewport(const D3D11_VIEWPORT& viewport);
	void SetCamera(Camera* m_camera);
	void SetShader(ShaderID);
	void SetBufferObj(int BufferID);
	void SetTexture(const char* texName, TextureID tex);
	void SetRasterizerState(RasterizerStateID rsStateID);
	void SetDepthStencilState(DepthStencilStateID depthStencilStateID);

	void BindRenderTarget(RenderTargetID rtvID);
	void BindDepthStencil(DepthStencilID dsvID);
	void UnbindRenderTarget();
	void UnbindDepthStencil();

	void SetConstant4x4f(const char* cName, const XMMATRIX& matrix);
	inline void SetConstant3f(const char* cName, const vec3& float3) { SetConstant(cName, static_cast<const void*>(&float3.x())); }
	inline void SetConstant1f(const char* cName, const float& data)  { SetConstant(cName, static_cast<const void*>(&data)); }
	inline void SetConstant1i(const char* cName, const int& data)    { SetConstant(cName, static_cast<const void*>(&data)); }
	inline void SetConstantStruct(const char * cName, void* data)    { SetConstant(cName, data); }

	// draw functions
	void DrawIndexed(TOPOLOGY topology = TOPOLOGY::TRIANGLE_LIST);
	void Draw(TOPOLOGY topology = TOPOLOGY::POINT_LIST);
	void DrawLine();
	void DrawLine(const vec3& pos1, const vec3& pos2, const vec3& color = Color().Value());

	void Begin(const float clearColor[4], const float depthValue);
	void End();
	void Reset();

	void Apply();
	
private:
	// multithread shader hostwap
	void PollShaderFiles();
	void PollThread();
	void OnShaderChange(LPTSTR dir);

	// init / load
	void GeneratePrimitives();
	void LoadShaders();
	void InitializeDefaultRenderTarget();
	void InitializeDefaultDepthBuffer();
	void InitializeDefaultRasterizerStates();

	void SetConstant(const char* cName, const void* data);
	//=======================================================================================================================================================
public:
	static const char* s_shaderRoot;
	static const char* s_textureRoot;

	ID3D11Device*					m_device;
	ID3D11DeviceContext*			m_deviceContext;

private:
	D3DManager*						m_Direct3D;
	HWND							m_hWnd;

	GeometryGenerator				m_geom;			// maybe static functions? yes... definitely static functions. todo:

	// render data
	Camera*							m_mainCamera;
	Viewport						m_viewPort;

	std::vector<BufferObject*>		m_bufferObjects;
	std::vector<Shader*>			m_shaders;
	std::vector<Texture>			m_textures;
	std::queue<TextureSetCommand>	m_texSetCommands;

	std::vector<RasterizerState*>	m_rasterizerStates;
	std::vector<DepthStencilState*> m_depthStencilStates;

	std::vector<RenderTarget>		m_renderTargets;
	std::vector<DepthStencil*>		m_depthStencils;	// not ptrs?

	// state objects
	struct StateObjects
	{
		ShaderID			_activeShader;
		BufferID			_activeBuffer;
		RasterizerStateID	_activeRSState;
		DepthStencilStateID	_activeDepthStencilState;
		RenderTargetID		_boundRenderTarget;
		DepthStencilID		_boundDepthStencil;
		RenderTargetID		_mainRenderTarget;
		Texture				_depthBufferTexture;
	} m_state;
	
	// performance counters
	unsigned long long				m_frameCount;

	//std::vector<Point>				m_debugLines;
};
 
