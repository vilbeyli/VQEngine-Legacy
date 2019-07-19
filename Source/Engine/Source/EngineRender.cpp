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


#include "Engine.h"
#include "Scene.h"
#include "SceneResourceView.h"

#include "Renderer/Renderer.h"


using namespace VQEngine;



void Engine::SendLightData() const
{
#if USE_DX12
	//TODO: remove this function
	assert(false);
	return;
#else
	const float shadowDimension = static_cast<float>(mShadowMapPass.mShadowMapDimension_Spot);
	// LIGHTS ( POINT | SPOT | DIRECTIONAL )
	//
	const vec2 directionalShadowMapDimensions = mShadowMapPass.GetDirectionalShadowMapDimensions(mpRenderer);
	mpRenderer->SetConstantStruct("Lights", &mSceneLightData._cb);
	mpRenderer->SetConstant2f("spotShadowMapDimensions", vec2(shadowDimension, shadowDimension));
	mpRenderer->SetConstant1f("directionalShadowMapDimension", directionalShadowMapDimensions.x());
	//mpRenderer->SetConstant2f("directionalShadowMapDimensions", directionalShadowMapDimensions);


	// SHADOW MAPS
	//
	mpRenderer->SetTextureArray("texSpotShadowMaps", mShadowMapPass.mShadowMapTextures_Spot);
	mpRenderer->SetTextureArray("texPointShadowMaps", mShadowMapPass.mShadowMapTextures_Point);
	if (mShadowMapPass.mShadowMapTexture_Directional != -1)
		mpRenderer->SetTextureArray("texDirectionalShadowMaps", mShadowMapPass.mShadowMapTexture_Directional);

#ifdef _DEBUG
	const SceneLightingConstantBuffer::cb& cb = mSceneLightData._cb;	// constant buffer shorthand
	if (cb.pointLightCount > cb.pointLights.size())	OutputDebugString("Warning: light count larger than MAX_LIGHTS\n");
	if (cb.spotLightCount > cb.spotLights.size())	OutputDebugString("Warning: spot count larger than MAX_SPOTS\n");
#endif
#endif
}





//------------------------------------------------------
// RENDER
//------------------------------------------------------
void Engine::PreRender()
{
#if LOAD_ASYNC
	if (mbLoading) return;
#endif
	mpCPUProfiler->BeginEntry("PreRender()");

	mpActiveScene->mSceneView.bIsPBRLightingUsed = IsLightingModelPBR();
	mpActiveScene->mSceneView.bIsDeferredRendering = mEngineConfig.bDeferredOrForward;

	// TODO: #RenderPass or Scene should manage this.
	// mTBNDrawObjects.clear();
	// std::vector<const GameObject*> objects;
	// mpActiveScene->GetSceneObjects(objects);
	// for (const GameObject* obj : objects)
	// {
	// 	if (obj->mRenderSettings.bRenderTBN)
	// 		mTBNDrawObjects.push_back(obj);
	// }

	mpActiveScene->PreRender(mFrameStats, mSceneLightData);
	mFrameStats.rstats = mpRenderer->GetRenderStats();
	mFrameStats.fps = GetFPS();

	mpCPUProfiler->EndEntry();
}

// ====================================================================================
// RENDER FUNCTIONS
// ====================================================================================
void Engine::Render()
{
	mpCPUProfiler->BeginEntry("Render()");

	mpGPUProfiler->BeginProfile(mFrameCount);
	mpGPUProfiler->BeginEntry("GPU"); // where does this close?

#if USE_DX12
	// ===================================================================================
	// DIRECTX 12 RENDERER                                                               =
	// ===================================================================================



#else
	// ===================================================================================
	// DIRECTX 11 RENDERER                                                               =
	// ===================================================================================

	const XMMATRIX& viewProj = mpActiveScene->mSceneView.viewProj;
	const bool bSceneSSAO = mpActiveScene->mSceneView.sceneRenderSettings.ssao.bEnabled;
	const bool bAAResolve = sEngineSettings.rendering.antiAliasing.IsAAEnabled();

	// SHADOW MAPS
	//------------------------------------------------------------------------
	mpCPUProfiler->BeginEntry("Shadow Pass");
	mpGPUProfiler->BeginEntry("Shadow Pass");
	mpRenderer->BeginEvent("Shadow Pass");

	mpRenderer->UnbindRenderTargets();	// unbind the back render target | every pass should have their own render targets
	mShadowMapPass.RenderShadowMaps(mpRenderer, mpActiveScene->mShadowView, mpGPUProfiler);

	mpRenderer->EndEvent();
	mpCPUProfiler->EndEntry();
	mpGPUProfiler->EndEntry();

	// LIGHTING PASS
	//------------------------------------------------------------------------
	mpRenderer->ResetPipelineState();

	//==========================================================================
	// DEFERRED RENDERER
	//==========================================================================
	if (mEngineConfig.bDeferredOrForward)
	{
		const GBuffer& gBuffer = mDeferredRenderingPasses._GBuffer;
		const TextureID texNormal = mpRenderer->GetRenderTargetTexture(gBuffer.mRTNormals);
		const TextureID texDiffuseRoughness = mpRenderer->GetRenderTargetTexture(gBuffer.mRTDiffuseRoughness);
		const TextureID texSpecularMetallic = mpRenderer->GetRenderTargetTexture(gBuffer.mRTSpecularMetallic);
		const TextureID texDepthTexture = mpRenderer->mDefaultDepthBufferTexture;
		const TextureID tSSAO = mEngineConfig.bSSAO && bSceneSSAO
			? mAOPass.GetBlurredAOTexture(mpRenderer)
			: mAOPass.whiteTexture4x4;
		const DeferredRenderingPasses::RenderParams deferredLightingParams =
		{
			mpRenderer
			, mpActiveScene->mSceneView
			, mSceneLightData
			, tSSAO
			, sEngineSettings.rendering.bUseBRDFLighting
		};

		// GEOMETRY - DEPTH PASS
		mpGPUProfiler->BeginEntry("Geometry Pass");
		mpCPUProfiler->BeginEntry("Geometry Pass");
		mpRenderer->BeginEvent("Geometry Pass");
		mDeferredRenderingPasses.RenderGBuffer(mpRenderer, mpActiveScene, mpActiveScene->mSceneView);
		mpRenderer->EndEvent();
		mpCPUProfiler->EndEntry();
		mpGPUProfiler->EndEntry();

		// AMBIENT OCCLUSION  PASS
		mpCPUProfiler->BeginEntry("AO Pass");
		if (mEngineConfig.bSSAO && bSceneSSAO)
		{
			mAOPass.RenderAmbientOcclusion(mpRenderer, texNormal, mpActiveScene->mSceneView);
		}
		mpCPUProfiler->EndEntry(); // AO Pass

		// DEFERRED LIGHTING PASS
		mpCPUProfiler->BeginEntry("Lighting Pass");
		mpGPUProfiler->BeginEntry("Lighting Pass");
		mpRenderer->BeginEvent("Lighting Pass");
		mDeferredRenderingPasses.RenderLightingPass(deferredLightingParams);
		mpRenderer->EndEvent();
		mpCPUProfiler->EndEntry();
		mpGPUProfiler->EndEntry();

		mpCPUProfiler->BeginEntry("Skybox & Lights");

		// LIGHT SOURCES
		mpRenderer->BindDepthTarget(mWorldDepthTarget);

		// SKYBOX
		//
		if (mpActiveScene->HasSkybox())
		{
			mpGPUProfiler->BeginEntry("Skybox Pass");
			mpActiveScene->RenderSkybox(mpActiveScene->mSceneView.viewProj);
			mpGPUProfiler->EndEntry();
		}

		mpCPUProfiler->EndEntry();
	}

	//==========================================================================
	// FORWARD RENDERER
	//==========================================================================
	else
	{
		const bool bZPrePass = mEngineConfig.bSSAO && bSceneSSAO;
		const TextureID tSSAO = bZPrePass
			? mAOPass.GetBlurredAOTexture(mpRenderer)
			: mAOPass.whiteTexture4x4;
		const TextureID texIrradianceMap = mpActiveScene->mSceneView.environmentMap.irradianceMap;
		const SamplerID smpEnvMap = mpActiveScene->mSceneView.environmentMap.envMapSampler < 0
			? EDefaultSamplerState::POINT_SAMPLER
			: mpActiveScene->mSceneView.environmentMap.envMapSampler;
		const TextureID prefilteredEnvMap = mpActiveScene->mSceneView.environmentMap.prefilteredEnvironmentMap;
		const TextureID tBRDFLUT = EnvironmentMap::sBRDFIntegrationLUTTexture;

		const RenderTargetID lightingRenderTarget = bAAResolve
			? mDeferredRenderingPasses._shadeTarget // resource reusage
			: mPostProcessPass._worldRenderTarget;

		// AMBIENT OCCLUSION - Z-PREPASS
		if (bZPrePass)
		{
			mpRenderer->SetViewport(mpRenderer->FrameRenderTargetWidth(), mpRenderer->FrameRenderTargetHeight());
			const RenderTargetID normals = mDeferredRenderingPasses._GBuffer.mRTNormals; // resource reusage
			const TextureID texNormal = mpRenderer->GetRenderTargetTexture(normals);
			const ZPrePass::RenderParams zPrePassParams =
			{
				mpRenderer,
				mpActiveScene,
				mpActiveScene->mSceneView,
				normals
			};

			mpGPUProfiler->BeginEntry("Z-PrePass");
			mZPrePass.RenderDepth(zPrePassParams);
			mpGPUProfiler->EndEntry();

			mpRenderer->BeginEvent("Ambient Occlusion Pass");
			{
				mAOPass.RenderAmbientOcclusion(mpRenderer, texNormal, mpActiveScene->mSceneView);
			}
			mpRenderer->EndEvent(); // Ambient Occlusion Pass
		}

		const bool bDoClearColor = true;
		const bool bDoClearDepth = !bZPrePass;
		const bool bDoClearStencil = false;
		ClearCommand clearCmd(
			bDoClearColor, bDoClearDepth, bDoClearStencil,
			{ 0, 0, 0, 0 }, 1, 0
		);

		mpRenderer->BindRenderTarget(lightingRenderTarget);
		mpRenderer->BindDepthTarget(mWorldDepthTarget);
		mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_TEST_ONLY);
		mpRenderer->BeginRender(clearCmd);

		// SKYBOX
		// if we're not rendering the skybox, call apply() to unbind
		// shadow light depth target so we can bind it in the lighting pass
		// otherwise, skybox render pass will take care of it
		if (mpActiveScene->HasSkybox())
		{
			mpGPUProfiler->BeginEntry("Skybox Pass");
			mpActiveScene->RenderSkybox(mpActiveScene->mSceneView.viewProj);
			mpGPUProfiler->EndEntry();
		}
		else
		{
			// todo: this might be costly, profile this
			mpRenderer->SetShader(mSelectedShader);	// set shader so apply won't complain 
			mpRenderer->Apply();					// apply to bind depth stencil
		}

		// LIGHTING
		const ForwardLightingPass::RenderParams forwardLightingParams =
		{
			  mpRenderer
			, mpActiveScene
			, mpActiveScene->mSceneView
			, mSceneLightData
			, tSSAO
			, lightingRenderTarget
			, mAOPass.blackTexture4x4
			, bZPrePass
		};

		mpGPUProfiler->BeginEntry("Lighting<Forward> Pass");
		mForwardLightingPass.RenderLightingPass(forwardLightingParams);
		mpGPUProfiler->EndEntry();
	}

	mpActiveScene->RenderLights(); // when skymaps are disabled, there's an error here that needs fixing.

	RenderDebug(viewProj);

	// AA RESOLVE
	//------------------------------------------------------------------------
	if (bAAResolve)
	{
		// currently only SSAA is supported
		mpRenderer->BeginEvent("Resolve AA");
		mpGPUProfiler->BeginEntry("Resolve AA");
		mpCPUProfiler->BeginEntry("Resolve AA");

		mAAResolvePass.SetInputTexture(mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._shadeTarget));
		mAAResolvePass.Render(mpRenderer);

		mpCPUProfiler->EndEntry();	// Resolve AA
		mpGPUProfiler->EndEntry();	// Resolve AA
		mpRenderer->EndEvent(); // Resolve AA
	}

	// Determine the output texture for Lighting Pass
	// which is also the input texture for post processing
	//
	const TextureID postProcessInputTextureID = mpRenderer->GetRenderTargetTexture(bAAResolve
		? mAAResolvePass.mResolveTarget     // SSAA (shares the same render target for deferred and forward)
		: (mEngineConfig.bDeferredOrForward
			? mDeferredRenderingPasses._shadeTarget // deferred / no SSAA
			: mPostProcessPass._worldRenderTarget)  // forward  / no SSAA
	);


	mpRenderer->SetBlendState(EDefaultBlendState::DISABLED);

	// POST PROCESS PASS | DEBUG PASS | UI PASS
	//------------------------------------------------------------------------
	mpCPUProfiler->BeginEntry("Post Process");
	mpGPUProfiler->BeginEntry("Post Process");
#if FULLSCREEN_DEBUG_TEXTURE
	const TextureID texDebug = mAOPass.GetBlurredAOTexture(mpRenderer);
	mPostProcessPass.Render(mpRenderer, mEngineConfig.bBloom, mbOutputDebugTexture
		? texDebug
		: postProcessInputTextureID
	);
#else
	mPostProcessPass.Render(mpRenderer, mEngineConfig.bBloom, postProcessInputTextureID);
#endif
	mpCPUProfiler->EndEntry();
	mpGPUProfiler->EndEntry();

	//------------------------------------------------------------------------
	RenderUI();
	//------------------------------------------------------------------------
#endif
	mpCPUProfiler->EndEntry();	// Render() call
}

void Engine::RenderDebug(const XMMATRIX& viewProj)
{
#if USE_DX12
	// TODO-DX12:
#else

	mpGPUProfiler->BeginEntry("Debug Pass");
	mpCPUProfiler->BeginEntry("Debug Pass");
	if (mEngineConfig.bBoundingBoxes)	// BOUNDING BOXES
	{
		mpActiveScene->RenderDebug(viewProj);
	}

	if (mEngineConfig.bRenderTargets)	// RENDER TARGETS
	{
		mpCPUProfiler->BeginEntry("Debug Textures");
		const int screenWidth = sEngineSettings.window.width;
		const int screenHeight = sEngineSettings.window.height;
		const float aspectRatio = static_cast<float>(screenWidth) / screenHeight;

		// debug texture strip draw settings
		const int bottomPaddingPx = 0;	 // offset from bottom of the screen
		const int heightPx = 128; // height for every texture
		const int paddingPx = 0;  // padding between debug textures
		const vec2 fullscreenTextureScaledDownSize((float)heightPx * aspectRatio, (float)heightPx);
		const vec2 squareTextureScaledDownSize((float)heightPx, (float)heightPx);

		// Textures to draw
		const TextureID white4x4 = mAOPass.whiteTexture4x4;
		//const TextureID tShadowMap		 = mpRenderer->GetDepthTargetTexture(mShadowMapPass._spotShadowDepthTargets);
		const TextureID tBlurredBloom = mPostProcessPass._bloomPass.GetBloomTexture(mpRenderer);
		const TextureID tDiffuseRoughness = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer.mRTDiffuseRoughness);
		//const TextureID tSceneDepth		 = m_pRenderer->m_state._depthBufferTexture._id;
		const TextureID tSceneDepth = mpRenderer->GetDepthTargetTexture(0);
		const TextureID tNormals = mpRenderer->GetRenderTargetTexture(mDeferredRenderingPasses._GBuffer.mRTNormals);
		const TextureID tAO = mEngineConfig.bSSAO ? mAOPass.GetBlurredAOTexture(mpRenderer) : mAOPass.whiteTexture4x4;
		const TextureID tBRDF = EnvironmentMap::sBRDFIntegrationLUTTexture;
		TextureID preFilteredEnvMap = mpActiveScene->GetEnvironmentMap().prefilteredEnvironmentMap;
		preFilteredEnvMap = preFilteredEnvMap < 0 ? white4x4 : preFilteredEnvMap;
		TextureID tDirectionalShadowMap = (mShadowMapPass.mDepthTarget_Directional == -1 || mpActiveScene->mDirectionalLight.mbEnabled == 0)
			? white4x4
			: mpRenderer->GetDepthTargetTexture(mShadowMapPass.mDepthTarget_Directional);

		const std::vector<DrawQuadOnScreenCommand> quadCmds = [&]()
		{
			// first row -----------------------------------------
			vec2 screenPosition(0.0f, static_cast<float>(bottomPaddingPx + heightPx * 0));
			std::vector<DrawQuadOnScreenCommand> c
			{	//		Pixel Dimensions	 Screen Position (offset below)	  Texture			DepthTexture?
				{ fullscreenTextureScaledDownSize,	screenPosition,			tSceneDepth			, true },
			{ fullscreenTextureScaledDownSize,	screenPosition,			tDiffuseRoughness	, false },
			{ fullscreenTextureScaledDownSize,	screenPosition,			tNormals			, false },
			//{ squareTextureScaledDownSize    ,	screenPosition,			tShadowMap			, true},
			{ fullscreenTextureScaledDownSize,	screenPosition,			tBlurredBloom		, false },
			{ fullscreenTextureScaledDownSize,	screenPosition,			tAO					, false, 1 },
			//{ squareTextureScaledDownSize,		screenPosition,			tBRDF				, false },
			};
			for (size_t i = 1; i < c.size(); i++)	// offset textures accordingly (using previous' x-dimension)
				c[i].bottomLeftCornerScreenCoordinates.x() = c[i - 1].bottomLeftCornerScreenCoordinates.x() + c[i - 1].dimensionsInPixels.x() + paddingPx;

			// second row -----------------------------------------
			screenPosition = vec2(0.0f, static_cast<float>(bottomPaddingPx + heightPx * 1));
			const size_t shadowMapCount = mSceneLightData._cb.pointLightCount_shadow;
			const size_t row_offset = c.size();
			size_t currShadowMap = 0;

			// the directional light
			{
				c.push_back({ squareTextureScaledDownSize, screenPosition, tDirectionalShadowMap, false });

				if (currShadowMap > 0)
				{
					c[row_offset].bottomLeftCornerScreenCoordinates.x()
						= c[row_offset - 1].bottomLeftCornerScreenCoordinates.x()
						+ c[row_offset - 1].dimensionsInPixels.x() + paddingPx;
				}
				++currShadowMap;
			}

			// spot lights
			// TODO: spot light depth texture is stored as a texture array and the
			//       current debug shader doesn't support texture array -> extend
			//       the debug shader in v0.5.0
			//
			//for (size_t i = 0; i < mSceneLightData._cb.spotLightCount_shadow; ++i)
			//{
			//	TextureID tex = mpRenderer->GetDepthTargetTexture(mShadowMapPass.mDepthTargets_Spot[i]);
			//	c.push_back({
			//		squareTextureScaledDownSize, screenPosition, tex, true
			//		});
			//
			//	if (currShadowMap > 0)
			//	{
			//		c[row_offset + i].bottomLeftCornerScreenCoordinates.x()
			//			= c[row_offset + i - 1].bottomLeftCornerScreenCoordinates.x()
			//			+ c[row_offset + i - 1].dimensionsInPixels.x() + paddingPx;
			//	}
			//	++currShadowMap;
			//}

			return c;
		}();


		mpRenderer->BeginEvent("Debug Textures");
		mpRenderer->SetShader(EShaders::DEBUG);
		for (const DrawQuadOnScreenCommand& cmd : quadCmds)
		{
			mpRenderer->DrawQuadOnScreen(cmd);
		}
		mpRenderer->EndEvent();
		mpCPUProfiler->EndEntry();
	}

	// Render TBN vectors (INACTIVE)
	//
#if 0
	const bool bIsShaderTBN = !mTBNDrawObjects.empty();
	if (bIsShaderTBN)
	{
		constexpr bool bSendMaterial = false;

		mpRenderer->BeginEvent("Draw TBN Vectors");
		if (mEngineConfig.bForwardOrDeferred)
			mpRenderer->BindDepthTarget(mWorldDepthTarget);

		mpRenderer->SetShader(EShaders::TBN);

		for (const GameObject* obj : mTBNDrawObjects)
			obj->RenderOpaque(mpRenderer, mSceneView, bSendMaterial, mpActiveScene->mMaterials);


		if (mEngineConfig.bForwardOrDeferred)
			mpRenderer->UnbindDepthTarget();

		mpRenderer->SetShader(mSelectedShader);
		mpRenderer->EndEvent();
	}
#endif
	mpCPUProfiler->EndEntry();	// Debug
	mpGPUProfiler->EndEntry();

#endif // USE_DX12
}


void Engine::RenderUI() const
{
#if USE_DX12
	// TODO-DX12:
#else
	mpRenderer->BeginEvent("UI");
	mpGPUProfiler->BeginEntry("UI");
	mpCPUProfiler->BeginEntry("UI");
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_NONE);
	if (mEngineConfig.mbShowProfiler) { mUI.RenderPerfStats(mFrameStats); }
	if (mEngineConfig.mbShowControls) { mUI.RenderEngineControls(); }
	if (!mbLoading)
	{
		if (mpActiveScene)
		{
			mpActiveScene->RenderUI();
		}
#if DEBUG
		else
		{
			Log::Warning("Engine::mpActiveScene = nullptr when !mbLoading ");
		}
#endif
	}

	mpCPUProfiler->EndEntry();	// UI
	mpGPUProfiler->EndEntry();
	mpRenderer->EndEvent(); // UI
#endif
}


void Engine::RenderLoadingScreen(bool bOneTimeRender) const
{
	const XMMATRIX matTransformation = XMMatrixIdentity();
	const auto IABuffers = SceneResourceView::GetBuiltinMeshVertexAndIndexBufferID(EGeometry::FULLSCREENQUAD);

	if (bOneTimeRender)
	{
		mpRenderer->BeginFrame();
	}
	else
	{
		mpGPUProfiler->BeginEntry("Loading Screen");
	}

#if USE_DX12
	// TODO-DX12:
#else
	mpRenderer->SetShader(EShaders::UNLIT);
	mpRenderer->UnbindDepthTarget();
	mpRenderer->BindRenderTarget(0);
	mpRenderer->SetVertexBuffer(IABuffers.first);
	mpRenderer->SetIndexBuffer(IABuffers.second);
	mpRenderer->SetTexture("texDiffuseMap", mActiveLoadingScreen);
	mpRenderer->SetConstant1f("isDiffuseMap", 1.0f);
	mpRenderer->SetConstant3f("diffuse", vec3(1.0f, 1, 1));
	mpRenderer->SetConstant4x4f("worldViewProj", matTransformation);
	mpRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);
	mpRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_DISABLED);
	mpRenderer->SetViewport(mpRenderer->WindowWidth(), mpRenderer->WindowHeight());
	mpRenderer->Apply();
	mpRenderer->DrawIndexed();
#endif

	if (bOneTimeRender)
	{
		mpRenderer->EndFrame();
#if USE_DX12
		// TODO-DX12:
#else
		mpRenderer->UnbindRenderTargets();
#endif
	}
	else
	{
		mpGPUProfiler->EndEntry();
	}
}
