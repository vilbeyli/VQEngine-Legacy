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

	friend struct SetTextureCommand;
	friend struct SetSamplerCommand;	// todo: refactor commands - don't use friend for commands

public:
	Renderer();
	~Renderer();

	//----------------------------------------------------------------------------------------------------------------
	// CORE INTERFACE
	//----------------------------------------------------------------------------------------------------------------
	bool					Initialize(HWND hwnd, const Settings::Window& settings, const Settings::Rendering& rendererSettings);
	void					Exit();
	void					ReloadShaders();

	//----------------------------------------------------------------------------------------------------------------
	// GETTERS
	//----------------------------------------------------------------------------------------------------------------
	HWND					GetWindow()		const;
	float					AspectRatio()	const;
	unsigned				WindowHeight()	const;
	unsigned				WindowWidth()	const;
	unsigned				FrameRenderTargetHeight() const;
	unsigned				FrameRenderTargetWidth() const;
	vec2					FrameRenderTargetDimensionsAsFloat2() const;
	vec2					GetWindowDimensionsAsFloat2() const;
	inline RenderTargetID	GetBackBufferRenderTarget() const { return mBackBufferRenderTarget; }
	inline DepthTargetID	GetBoundDepthTarget() const { return mPipelineState.depthTargets; }
	inline TextureID		GetDefaultRenderTargetTexture() const { return mRenderTargets[mBackBufferRenderTarget].texture._id; }
	inline TextureID		GetRenderTargetTexture(RenderTargetID RT) const { return mRenderTargets[RT].texture._id; }
	inline TextureID		GetDepthTargetTexture(DepthTargetID DT) const { return mDepthTargets[DT].texture._id; }
	const PipelineState&	GetPipelineState() const;
	inline const RendererStats&	GetRenderStats() const { return mRenderStats; }
	const BufferDesc		GetBufferDesc(EBufferType bufferType, BufferID bufferID) const;

	const Shader*			GetShader(ShaderID shader_id) const;
	const Texture&			GetTextureObject(TextureID) const;
	const TextureID			GetTexture(const std::string name) const;
	inline const ShaderID	GetActiveShader() const { return mPipelineState.shader; }
	inline const Buffer&	GetVertexBuffer(BufferID id) { return mVertexBuffers[id]; }
	ShaderDesc				GetShaderDesc(ShaderID shaderID) const;
	EImageFormat			GetTextureImageFormat(TextureID) const;

	//----------------------------------------------------------------------------------------------------------------
	// RESOURCE INITIALIZATION
	//----------------------------------------------------------------------------------------------------------------
	// --- SHADER
	ShaderID				CreateShader(const ShaderDesc& shaderDesc);
	ShaderID				ReloadShader(const ShaderDesc& shaderDesc, const ShaderID shaderID);

	// --- TEXTURE
	//						example params:			"bricks_d.png", "Data/Textures/"
	TextureID				CreateTextureFromFile(const std::string& texFileName, const std::string& fileRoot = sTextureRoot, bool bGenerateMips = false);
	TextureID				CreateTexture2D(const TextureDesc& texDesc);
	TextureID				CreateTexture2D(D3D11_TEXTURE2D_DESC&	textureDesc, bool initializeSRV);	// used by AddRenderTarget() | todo: remove this?
	TextureID				CreateHDRTexture(const std::string& texFileName, const std::string& fileRoot = sHDRTextureRoot);
	TextureID				CreateCubemapFromFaceTextures(const std::vector<std::string>& textureFiles, bool bGenerateMips, unsigned mipLevels = 1);

	// --- SAMPLER
	//
	SamplerID				CreateSamplerState(D3D11_SAMPLER_DESC&	samplerDesc);	// TODO: samplerDesc

	// --- BUFFER
	BufferID				CreateBuffer(const BufferDesc& bufferDesc, const void* pData = nullptr, const char* pBufferName = nullptr);

	// --- PIPELINE STATES
	RasterizerStateID		AddRasterizerState(ERasterizerCullMode cullMode, ERasterizerFillMode fillMode, bool bEnableDepthClip, bool bEnableScissors);
	DepthStencilStateID		AddDepthStencilState(bool bEnableDepth, bool bEnableStencil);	// TODO: depthStencilStateDesc
	DepthStencilStateID		AddDepthStencilState(const D3D11_DEPTH_STENCIL_DESC& dsDesc);   //
	BlendStateID			AddBlendState( /*TODO params*/);

	// --- RENDER / DEPTH TARGETS
	RenderTargetID				AddRenderTarget(const RenderTargetDesc& renderTargetDesc);
	std::vector<DepthTargetID>	AddDepthTarget(const DepthTargetDesc& depthTargetDesc);

	bool						RecycleDepthTarget(DepthTargetID depthTargetID, const DepthTargetDesc& newDepthTargetDesc);

	// uses the given texture object, doesn't create a new texture for the render target
	//
	RenderTargetID				AddRenderTarget(const Texture& textureObj, D3D11_RENDER_TARGET_VIEW_DESC& RTVDesc);


	//----------------------------------------------------------------------------------------------------------------
	// PIPELINE STATE MANAGEMENT
	//----------------------------------------------------------------------------------------------------------------
	void					SetViewport(const unsigned width, const unsigned height);
	void					SetViewport(const vec2 widhtHeight) { SetViewport(static_cast<unsigned>(widhtHeight.x()), static_cast<unsigned>(widhtHeight.y())); }
	void					SetViewport(const D3D11_VIEWPORT& viewport);
	void					SetShader(ShaderID shaderID, bool bUnbindRenderTargets = false, bool bUnbindTextures = true);
	void					SetVertexBuffer(BufferID bufferID);
	void					SetIndexBuffer(BufferID bufferID);
	void					SetUABuffer(BufferID bufferID);
	void					SetTexture(const char* texName, TextureID tex);
	void					SetRWTexture(const char* texName, TextureID tex);
	inline void				SetTextureArray(const char* texName, TextureID texArray) { SetTexture(texName, texArray); }
	inline void				SetTextureFromArraySlice(const char* texName, TextureID texArray, unsigned slice) { SetTexture_(texName, texArray, slice); }
	
	template <typename... Args>
	void					SetTextureArray(const char* texName, Args const&... TextureIDs);

	void					SetTextureArray(const char* texName, const std::array<TextureID, TEXTURE_ARRAY_SIZE>& TextureIDs, unsigned numTextures);

	
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
	
	void					Dispatch(int x, int y, int z);

	void					DrawQuadOnScreen(const DrawQuadOnScreenCommand& cmd); // BottomLeft<x,y> = (0,0)
	
	void					DrawLine();
	void					DrawLine(const vec3& pos1, const vec3& pos2, const vec3& color = LinearColor().Value());



private:
	void					SetConstant(const char* cName, const void* data);
	void					SetTexture_(const char* texName, TextureID tex, unsigned slice = 0 /* only for texture arrays */ );

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

	Settings::Rendering::AntiAliasing mAntiAliasing;

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

	// RENDERING RESOURCES
	//
	std::vector<Shader*>			mShaders;
	std::vector<Texture>			mTextures;
	std::vector<Sampler>			mSamplers;
	std::vector<Buffer>				mVertexBuffers;
	std::vector<Buffer>				mIndexBuffers;
	std::vector<Buffer>				mUABuffers;

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



// template helper empty function
//
template<typename... Args> inline void pass(Args&&...) {}
// note: we can use the expand operator (...) in a function parameter scope
//       to access the expanded parameters one by one in an enclosed function call.

template <typename... Args>
void Renderer::SetTextureArray(const char* texName, Args const&... TextureIDs)
{
	std::array<TextureID, TEXTURE_ARRAY_SIZE> texIDs = {};
	texIDs.fill(-1);

	unsigned numTextures = 0;
	auto fnAddTextureID = [&](TextureID texID) -> TextureID {texIDs[numTextures++] = texID; return texID; };
	pass ( fnAddTextureID(TextureIDs)... ); // unpack args

	SetTextureArray(texName, texIDs, numTextures);
}
