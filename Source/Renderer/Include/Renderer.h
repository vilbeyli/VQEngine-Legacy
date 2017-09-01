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

#include "D3DManager.h"
#include "Shader.h"
#include "Texture.h"
#include "Settings.h"

#include "Mesh.h"
#include "GameObject.h"

#include "RenderCommands.h"
#include "WorkerPool.h"

#include <memory>
#include <stack>

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
using SamplerID           = int;
using RasterizerStateID   = int;
using BlendStateID		  = int;
using DepthStencilStateID = int;
using RenderTargetID	  = int;
using DepthStencilID	  = int;

using RasterizerState   = ID3D11RasterizerState;
using DepthStencilState = ID3D11DepthStencilState;

using DepthStencil		= ID3D11DepthStencilView;	// todo struct?

enum EDefaultRasterizerState
{
	CULL_NONE = 0,
	CULL_FRONT,
	CULL_BACK,

	RASTERIZER_STATE_COUNT
};

enum EDefaultBlendState
{
	DISABLED,
	ADDITIVE_COLOR,
	ALPHA_BLEND,

	BLEND_STATE_COUNT
};

struct BlendState
{
	BlendState() : ptr(nullptr) {}
	ID3D11BlendState* ptr; 
};

struct RenderTarget
{
	RenderTarget() : pRenderTargetView(nullptr) {}
	ID3D11Resource*	GetTextureResource() const { return texture._tex2D; }
	//--------------------------------------------
	Texture						texture;
	ID3D11RenderTargetView*		pRenderTargetView;
};

class Renderer
{
	friend class Engine;		// two main components have private access 
	friend class SceneManager;	// to Renderer mainly for the private functions

	friend struct SetTextureCommand;
	friend struct SetSamplerCommand;	// todo: refactor commands - dont use friend for commands

public:
	Renderer();
	~Renderer();
	bool Initialize(HWND hwnd, const Settings::Renderer& settings);
	void Exit();

	// getters
	inline HWND				GetWindow()		const { return m_Direct3D->WindowHandle(); };
	inline float			AspectRatio()	const { return m_Direct3D->AspectRatio();  };
	inline unsigned			WindowHeight()	const { return m_Direct3D->WindowHeight(); };
	inline unsigned			WindowWidth()	const { return m_Direct3D->WindowWidth();  };
	inline RenderTargetID	GetDefaultRenderTarget() const	                { return m_state._mainRenderTarget; }
	inline DepthStencilID	GetDefaultDepthStencil() const	                { return m_state._boundDepthStencil; }
	inline TextureID		GetDefaultRenderTargetTexture() const           { return m_renderTargets[m_state._mainRenderTarget].texture._id; }
	inline TextureID		GetRenderTargetTexture(RenderTargetID RT) const { return m_renderTargets[RT].texture._id; }

	// resource initialization 
	ShaderID				AddShader(const std::string& shaderFileName , const std::vector<InputLayout>& layouts);
	ShaderID				AddShader(const std::string& shaderName, const std::vector<std::string>& shaderFileNames, const std::vector<EShaderType>& shaderTypes, const std::vector<InputLayout>& layouts);
	TextureID				CreateTextureFromFile(const std::string& shdFileName, const std::string& fileRoot = s_textureRoot);
	const Texture&			CreateTexture2D(int widht, int height);
	TextureID				CreateTexture2D(D3D11_TEXTURE2D_DESC& textureDesc, bool initializeSRV);
	TextureID				CreateCubemapTexture(const std::vector<std::string>& textureFiles);
	
	SamplerID				CreateSamplerState(D3D11_SAMPLER_DESC& samplerDesc);
	RasterizerStateID		AddRasterizerState(ERasterizerCullMode cullMode, ERasterizerFillMode fillMode, bool enableDepthClip);
	DepthStencilStateID		AddDepthStencilState();	// todo params
	DepthStencilStateID		AddDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& dsDesc);
	BlendStateID			AddBlendState();

	RenderTargetID			AddRenderTarget(D3D11_TEXTURE2D_DESC& RTTextureDesc, D3D11_RENDER_TARGET_VIEW_DESC& RTVDesc);
	DepthStencilID			AddDepthStencil(const D3D11_DEPTH_STENCIL_VIEW_DESC& dsvDesc, ID3D11Texture2D*& surface);
	

	// pipeline state management
	const Shader*			GetShader(ShaderID shader_id) const;
	const Texture&			GetTextureObject(TextureID) const;
	const TextureID			GetTexture(const std::string name) const;
	inline const ShaderID	GetActiveShader() const { return m_state._activeShader; }

	void SetViewport(const unsigned width, const unsigned height);
	void SetViewport(const D3D11_VIEWPORT& viewport);
	void SetCamera(Camera* m_camera);
	void SetShader(ShaderID);
	void SetBufferObj(int BufferID);
	void SetTexture(const char* texName, TextureID tex);
	void SetSamplerState(const char* texName, SamplerID sampler);
	void SetRasterizerState(RasterizerStateID rsStateID);
	void SetBlendState(BlendStateID blendStateID);
	void SetDepthStencilState(DepthStencilStateID depthStencilStateID);

	void BindRenderTarget(RenderTargetID rtvID);
	template <typename... Args> 	inline void BindRenderTargets(Args const&... renderTargetIDs) { m_state._boundRenderTargets = { renderTargetIDs... }; }

	void BindDepthStencil(DepthStencilID dsvID);

	void UnbindRenderTarget();
	void UnbindDepthStencil();

	void SetConstant4x4f(const char* cName, const XMMATRIX& matrix);
	inline void SetConstant3f(const char* cName, const vec3& float3)	{ SetConstant(cName, static_cast<const void*>(&float3.x())); }
	inline void SetConstant2f(const char* cName, const vec2& float2)	{ SetConstant(cName, static_cast<const void*>(&float2.x())); }
	inline void SetConstant1f(const char* cName, const float& data)		{ SetConstant(cName, static_cast<const void*>(&data)); }
	inline void SetConstant1i(const char* cName, const int& data)		{ SetConstant(cName, static_cast<const void*>(&data)); }
	inline void SetConstantStruct(const char * cName, const void* data) { SetConstant(cName, data); }

	void Begin(const float clearColor[4], const float depthValue);
	void End();
	void Reset();

	void Apply();

	// draw functions
	void DrawIndexed(EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLE_LIST);
	void Draw(EPrimitiveTopology topology = EPrimitiveTopology::POINT_LIST);
	void DrawLine();
	void DrawLine(const vec3& pos1, const vec3& pos2, const vec3& color = Color().Value());
	
private:
	static std::vector<std::string> GetShaderPaths(const std::string& shaderFileName);

	// shader hotswap
	void PollShaderFiles();
	void OnShaderChange(LPTSTR dir);

	// init / load
	void GeneratePrimitives();
	
	void LoadShaders();
	std::stack<std::string> UnloadShaders();
	void ReloadShaders();

	void InitializeDefaultRenderTarget();
	void InitializeDefaultDepthBuffer();
	void InitializeDefaultRasterizerStates();
	void InitializeDefaultBlendStates();

	void SetConstant(const char* cName, const void* data);
	//=======================================================================================================================================================
public:
	static const char*				s_shaderRoot;
	static const char*				s_textureRoot;
	static Settings::Renderer		s_defaultSettings;

	ID3D11Device*					m_device;
	ID3D11DeviceContext*			m_deviceContext;
	D3DManager*						m_Direct3D;

	static bool						sEnableBlend; //temp
private:
	Camera*							m_mainCamera;
	
	std::vector<BufferObject*>		m_bufferObjects;
	std::vector<Shader*>			m_shaders;
	std::vector<Texture>			m_textures;
	std::vector<Sampler>			m_samplers;

	std::queue<SetTextureCommand>	m_setTextureCmds;
	std::queue<SetSamplerCommand>	m_setSamplerCmds;

	std::vector<RasterizerState*>	m_rasterizerStates;
	std::vector<DepthStencilState*> m_depthStencilStates;
	std::vector<BlendState>			m_blendStates;

	std::vector<RenderTarget>		m_renderTargets;
	std::vector<DepthStencil*>		m_depthStencils;

	Viewport						m_viewPort;

	using RenderTargetIDs = std::vector<RenderTargetID>;
	struct StateObjects
	{
		ShaderID			_activeShader;
		BufferID			_activeBuffer;
		RasterizerStateID	_activeRSState;
		DepthStencilStateID	_activeDepthStencilState;
		RenderTargetIDs		_boundRenderTargets;
		DepthStencilID		_boundDepthStencil;
		RenderTargetID		_mainRenderTarget;
		BlendStateID		_activeBlendState;
		Texture				_depthBufferTexture;
	} m_state;
	
	// performance counters
	unsigned long long				m_frameCount;

	//std::vector<Point>				m_debugLines;
	
	//Worker		m_ShaderHotswapPollWatcher;

};
 