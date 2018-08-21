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
#pragma once

#include "RenderCommands.h"
#include "Texture.h"
#include "Shader.h"
#include "RenderingStructs.h"

#include "Engine/Settings.h"

#include "Utilities/Color.h"
#include "Utilities/utils.h"
#include "Engine/DataStructures.h"
//#include "WorkerPool.h"

#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <mutex>

class BufferObject;
class Camera;
class D3DManager;
namespace DirectX  { class ScratchImage; }

class Renderer
{
	friend class Engine;		
	friend class SceneManager;
	friend class Shader;		// shader load/unload

	friend struct SetTextureCommand;
	friend struct SetSamplerCommand;	// todo: refactor commands - don't use friend for commands

public:
	Renderer();
	~Renderer();

	//----------------------------------------------------------------------------------------------------------------
	// CORE INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	bool					Initialize(HWND hwnd, const Settings::Window& settings);
	void					Exit();
	inline void				ReloadShaders() { Shader::UnloadShaders(this); Shader::LoadShaders(this); }

	//----------------------------------------------------------------------------------------------------------------
	// GETTERS
	//----------------------------------------------------------------------------------------------------------------
	HWND					GetWindow()		const; 
	float					AspectRatio()	const;
	unsigned				WindowHeight()	const;
	unsigned				WindowWidth()	const;
	vec2					GetWindowDimensionsAsFloat2() const;
	inline RenderTargetID	GetDefaultRenderTarget() const	                { return mBackBufferRenderTarget; }
	inline DepthTargetID	GetBoundDepthTarget() const	                    { return mPipelineState.depthTargets; }
	inline TextureID		GetDefaultRenderTargetTexture() const           { return mRenderTargets[mBackBufferRenderTarget].texture._id; }
	inline TextureID		GetRenderTargetTexture(RenderTargetID RT) const { return mRenderTargets[RT].texture._id; }
	inline TextureID		GetDepthTargetTexture(DepthTargetID DT) const   { return mDepthTargets[DT].texture._id; }
	const PipelineState&	GetState() const;
	const RendererStats&	GetRenderStats() const { return mRenderStats; }

	const Shader*			GetShader(ShaderID shader_id) const;
	const Texture&			GetTextureObject(TextureID) const;
	const TextureID			GetTexture(const std::string name) const;
	inline const ShaderID	GetActiveShader() const { return mPipelineState.shader; }
	const Buffer&			GetVertexBuffer(BufferID id) { return mVertexBuffers[id]; }

	//----------------------------------------------------------------------------------------------------------------
	// RESOURCE INITIALIZATION
	//----------------------------------------------------------------------------------------------------------------
	// --- SHADER
	ShaderID				CreateShader(const ShaderDesc& shaderDesc);

	// --- TEXTURE
	//						example params:			"bricks_d.png", "Data/Textures/"
	TextureID				CreateTextureFromFile(	const std::string&	texFileName, const std::string&	fileRoot = sTextureRoot);
	TextureID				CreateTexture2D(const TextureDesc& texDesc);
	TextureID				CreateTexture2D(D3D11_TEXTURE2D_DESC&	textureDesc, bool initializeSRV);	// used by AddRenderTarget() | todo: remove this?
	TextureID				CreateHDRTexture(const std::string& texFileName, const std::string& fileRoot = sHDRTextureRoot);
	TextureID				CreateCubemapFromFaceTextures(const std::vector<std::string>& textureFiles, bool bGenerateMips, unsigned mipLevels = 1);

	// --- BUFFER
	BufferID				CreateBuffer(const BufferDesc& bufferDesc, const void* pData = nullptr);

	// --- PIPELINE STATE OBJECTS
	SamplerID				CreateSamplerState(D3D11_SAMPLER_DESC&	samplerDesc );	// TODO: samplerDesc

	RasterizerStateID		AddRasterizerState(ERasterizerCullMode cullMode, ERasterizerFillMode fillMode, bool bEnableDepthClip, bool bEnableScissors);

	DepthStencilStateID		AddDepthStencilState(bool bEnableDepth, bool bEnableStencil);	// todo params
	DepthStencilStateID		AddDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& dsDesc);

	BlendStateID			AddBlendState( /*todo params*/);

	RenderTargetID			AddRenderTarget(const RenderTargetDesc& renderTargetDesc);
	
	// uses the given texture object, doesn't create a new texture for the render target
	RenderTargetID			AddRenderTarget(const Texture& textureObj, D3D11_RENDER_TARGET_VIEW_DESC& RTVDesc);
	
	std::vector<DepthTargetID>	AddDepthTarget(const DepthTargetDesc& depthTargetDesc);
	
	//----------------------------------------------------------------------------------------------------------------
	// PIPELINE STATE MANAGEMENT
	//----------------------------------------------------------------------------------------------------------------
	void					SetViewport(const unsigned width, const unsigned height);
	void					SetViewport(const D3D11_VIEWPORT& viewport);
	void					SetShader(ShaderID);
	void					SetVertexBuffer(BufferID bufferID);
	void					SetIndexBuffer(BufferID bufferID);
	void					SetTexture(const char* texName, TextureID tex);
	//void					SetTextureArray(const char* texName, const std::vector<TextureID>& tex); // do we allow multiple texture id -> tex2dArr srv ?
	void inline				SetTextureArray(const char* texName, TextureID texArray) { SetTexture(texName, texArray); }
	void					SetSamplerState(const char* samplerName, SamplerID sampler);
	void					SetRasterizerState(RasterizerStateID rsStateID);
	void					SetBlendState(BlendStateID blendStateID);
	void					SetDepthStencilState(DepthStencilStateID depthStencilStateID);
	void					SetScissorsRect(int left, int right, int top, int bottom);

	template <typename... Args> 	
	inline void				BindRenderTargets(Args const&... renderTargetIDs) { mPipelineState.renderTargets = { renderTargetIDs... }; }
	void					BindRenderTarget(RenderTargetID rtvID);
	void					BindDepthTarget(DepthTargetID dsvID);

	void					UnbindRenderTargets();
	void					UnbindDepthTarget();

	void					SetConstant4x4f(const char* cName, const XMMATRIX& matrix);
	inline void				SetConstant3f(const char* cName, const vec3& float3)	{ SetConstant(cName, static_cast<const void*>(&float3.x())); }
	inline void				SetConstant2f(const char* cName, const vec2& float2)	{ SetConstant(cName, static_cast<const void*>(&float2.x())); }
	inline void				SetConstant1f(const char* cName, const float& data)		{ SetConstant(cName, static_cast<const void*>(&data)); }
	inline void				SetConstant1i(const char* cName, const int& data)		{ SetConstant(cName, static_cast<const void*>(&data)); }
	inline void				SetConstantStruct(const char * cName, const void* data) { SetConstant(cName, data); }

	void					BeginRender(const ClearCommand& clearCmd);	// clears the bound render targets

	void					BeginFrame();	// resets render stats
	void					EndFrame();		// presents the swapchain and clears shaders
	void					ResetPipelineState();

	void					UpdateBuffer(BufferID buffer, const void* pData);
	void					Apply();

	void					BeginEvent(const std::string& marker);
	void					EndEvent();

	//----------------------------------------------------------------------------------------------------------------
	// DRAW FUNCTIONS
	//----------------------------------------------------------------------------------------------------------------
	void					DrawIndexed(EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLE_LIST);
	void					DrawIndexedInstanced(int instanceCount, EPrimitiveTopology topology = EPrimitiveTopology::TRIANGLE_LIST);
	void					Draw(int vertCount, EPrimitiveTopology topology = EPrimitiveTopology::POINT_LIST);
	
	void					DrawQuadOnScreen(const DrawQuadOnScreenCommand& cmd); // BottomLeft<x,y> = (0,0)
	
	void					DrawLine();
	void					DrawLine(const vec3& pos1, const vec3& pos2, const vec3& color = LinearColor().Value());



private:
	void					SetConstant(const char* cName, const void* data);

public:
	//----------------------------------------------------------------------------------------------------------------
	// WORKSPACE DIRECTORIES (STATIC)
	//----------------------------------------------------------------------------------------------------------------
	static const char*				sShaderRoot;
	static const char*				sTextureRoot;
	static const char*				sHDRTextureRoot;

	bool SaveTextureToDisk(TextureID texID, const std::string& filePath, bool bConverToSRGB) const;

	//----------------------------------------------------------------------------------------------------------------
	// DATA
	//----------------------------------------------------------------------------------------------------------------
	ID3D11Device*					m_device;
	ID3D11DeviceContext*			m_deviceContext;
	D3DManager*						m_Direct3D;

	//std

	static bool						sEnableBlend; //temp


private:	
	// PIPELINE STATE
	//
	PipelineState					mPipelineState;
	PipelineState					mPrevPipelineState;
	
	std::vector<RasterizerState*>	mRasterizerStates;
	std::vector<DepthStencilState*> mDepthStencilStates;
	std::vector<BlendState>			mBlendStates;

	RenderTargetID					mBackBufferRenderTarget;	// todo: remove or rename
	TextureID						mDefaultDepthBufferTexture;	// todo: remove or rename

	// DATA
	//
	std::vector<Shader*>			mShaders;
	std::vector<Texture>			mTextures;
	std::vector<Sampler>			mSamplers;
	std::vector<Buffer>				mVertexBuffers;
	std::vector<Buffer>				mIndexBuffers;

	std::vector<RenderTarget>		mRenderTargets;
	std::vector<DepthTarget>		mDepthTargets;


	// TODO: refactor command processing
	std::queue<SetTextureCommand>	mSetTextureCmds;
	std::queue<SetSamplerCommand>	mSetSamplerCmds;

	
	// PERFORMANCE COUNTERS
	//
	RendererStats					mRenderStats;

	// WINDOW SETTINGS
	//
	Settings::Window				mWindowSettings;

	//std::vector<Point>			m_debugLines;

	// MULTI-THREADING
	//
	std::mutex						mTexturesMutex;
	//Worker						m_ShaderHotswapPollWatcher;
};
 