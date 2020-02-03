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

#include "RenderPasses.h"
#include "Engine/SceneResourceView.h"
#include "Engine/Engine.h"
#include "Engine/GameObject.h"
#include "Engine/Light.h"
#include "Engine/SceneView.h"

#include "Renderer/Renderer.h"
#include "Renderer/GeometryGenerator.h"

#if _DEBUG
#include "Utilities/Log.h"
#endif

#define FORCE_NO_CULL_SPOTLIGHTS 1
#define FORCE_NO_CULL_POINTLIGHTS 1

#if !SHADOW_PASS_USE_INSTANCED_DRAW_DATA
MeshDrawData::MeshDrawData(const GameObject* pObj_)
	: meshIDs(pObj_->GetModelData().mMeshIDs)
	, matWorld(pObj_->GetTransform().WorldTransformationMatrix())

#ifdef _DEBUG
	, pObj(pObj_)
#endif
{}


MeshDrawData::MeshDrawData()
	: meshIDs()
	, matWorld(XMMatrixIdentity())
#ifdef _DEBUG
	, pObj(nullptr)
#endif
{}
#endif



vec2 ShadowMapPass::GetDirectionalShadowMapDimensions(Renderer* pRenderer) const
{
	if (this->mDepthTarget_Directional == -1)
		return vec2(0, 0);

	const float dim = static_cast<float>(pRenderer->GetTextureObject(pRenderer->GetDepthTargetTexture(this->mDepthTarget_Directional))._width);
	return vec2(dim);
}



void ShadowMapPass::Initialize(Renderer* pRenderer, const Settings::ShadowMap& shadowMapSettings)
{
	this->mpRenderer = pRenderer;
	this->mShadowMapShader = EShaders::SHADOWMAP_DEPTH;
	this->mShadowMapShaderInstanced = EShaders::SHADOWMAP_DEPTH_INSTANCED;

	const ShaderDesc cubemapDepthShaderDesc = { "ShadowCubeMapShader",
		ShaderStageDesc{"ShadowCubeMapShader_vs.hlsl", {
#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
			ShaderMacro{ "INSTANCED"     , "1" },
			ShaderMacro{ "INSTANCE_COUNT", std::to_string(MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS) }
#else
			{}
#endif
		}}
		, ShaderStageDesc{"ShadowCubeMapShader_ps.hlsl" , {} }
	};
	this->mShadowCubeMapShader = pRenderer->CreateShader(cubemapDepthShaderDesc);

	InitializeSpotLightShadowMaps(shadowMapSettings);
	InitializePointLightShadowMaps(shadowMapSettings);

	//Log::Info("")
}


void ShadowMapPass::InitializeSpotLightShadowMaps(const Settings::ShadowMap& shadowMapSettings)
{
	assert(mpRenderer);
	this->mShadowMapDimension_Spot = static_cast<int>(shadowMapSettings.spotShadowMapDimensions);

#if _DEBUG
	//pRenderer->m_Direct3D->ReportLiveObjects("--------SHADOW_PASS_INIT");
#endif

	// check feature support & error handle:
	// https://msdn.microsoft.com/en-us/library/windows/apps/dn263150
	const bool bDepthOnly = true;
	const EImageFormat format = bDepthOnly ? R32 : R24G8;

	DepthTargetDesc depthDesc;
	depthDesc.format = bDepthOnly ? D32F : D24UNORM_S8U;

	TextureDesc& texDesc = depthDesc.textureDesc;
	texDesc.format = format;
	texDesc.usage = static_cast<ETextureUsage>(DEPTH_TARGET | RESOURCE);
	texDesc.height = texDesc.width = mShadowMapDimension_Spot;
	texDesc.arraySize = NUM_SPOT_LIGHT_SHADOW;

	// CREATE DEPTH TARGETS: SPOT LIGHTS
	//
	auto DepthTargetIDs = mpRenderer->AddDepthTarget(depthDesc);
	this->mDepthTargets_Spot.resize(NUM_SPOT_LIGHT_SHADOW);
	std::copy(RANGE(DepthTargetIDs), this->mDepthTargets_Spot.begin());

	this->mShadowMapTextures_Spot = this->mDepthTargets_Spot.size() > 0
		? mpRenderer->GetDepthTargetTexture(this->mDepthTargets_Spot[0])
		: -1;


}
void ShadowMapPass::InitializePointLightShadowMaps(const Settings::ShadowMap& shadowMapSettings)
{
	assert(mpRenderer);
	this->mShadowMapDimension_Point = static_cast<int>(shadowMapSettings.pointShadowMapDimensions);

#if _DEBUG
	//pRenderer->m_Direct3D->ReportLiveObjects("--------SHADOW_PASS_INIT");
#endif

	// check feature support & error handle:
	// https://msdn.microsoft.com/en-us/library/windows/apps/dn263150
	const bool bDepthOnly = true;
	const EImageFormat format = bDepthOnly ? R32 : R24G8;


	// CREATE DEPTH TARGETS: POINT LIGHTS
	//
	DepthTargetDesc depthDesc;
	depthDesc.format = bDepthOnly ? D32F : D24UNORM_S8U;

	TextureDesc& texDesc = depthDesc.textureDesc;
	texDesc.format = format;
	texDesc.usage = static_cast<ETextureUsage>(DEPTH_TARGET | RESOURCE);
	texDesc.bIsCubeMap = true;
	texDesc.height = texDesc.width = this->mShadowMapDimension_Point;
	texDesc.arraySize = NUM_POINT_LIGHT_SHADOW;
	texDesc.texFileName = "Point Light Shadow Maps";

	auto DepthTargetIDs = mpRenderer->AddDepthTarget(depthDesc);
	this->mDepthTargets_Point.resize(NUM_POINT_LIGHT_SHADOW * 6 /*each face = one depth target*/);
	std::copy(RANGE(DepthTargetIDs), this->mDepthTargets_Point.begin());

	this->mShadowMapTextures_Point = this->mDepthTargets_Point.size() > 0
		? mpRenderer->GetDepthTargetTexture(this->mDepthTargets_Point[0])
		: -1;

}
void ShadowMapPass::InitializeDirectionalLightShadowMap(const Settings::ShadowMap & shadowMapSettings)
{
	assert(mpRenderer);
	const int textureDimension = static_cast<int>(shadowMapSettings.directionalShadowMapDimensions);
	const EImageFormat format = R32;

	DepthTargetDesc depthDesc;
	depthDesc.format = D32F;
	TextureDesc& texDesc = depthDesc.textureDesc;
	texDesc.format = format;
	texDesc.usage = static_cast<ETextureUsage>(DEPTH_TARGET | RESOURCE);
	texDesc.height = texDesc.width = textureDimension;
	texDesc.arraySize = 1;

	// first time - add target
	if (this->mDepthTarget_Directional == -1)
	{
		this->mDepthTarget_Directional = mpRenderer->AddDepthTarget(depthDesc)[0];
		this->mShadowMapTexture_Directional = mpRenderer->GetDepthTargetTexture(this->mDepthTarget_Directional);
	}
	else // other times - check if dimension changed
	{
		const bool bDimensionChanged = shadowMapSettings.spotShadowMapDimensions != mpRenderer->GetTextureObject(mpRenderer->GetDepthTargetTexture(this->mDepthTarget_Directional))._width;
		if (bDimensionChanged)
		{
			mpRenderer->RecycleDepthTarget(this->mDepthTarget_Directional, depthDesc);
		}
	}
}


#define INSTANCED_DRAW 1

void ShadowMapPass::RenderShadowMaps(Renderer* pRenderer, const ShadowView& shadowView, GPUProfiler* pGPUProfiler) const
{
	//-----------------------------------------------------------------------------------------------
#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA

#else
	auto RenderDepthMeshes = [&](const MeshDrawData& drawData, const XMMATRIX& viewProj, bool bIsCubemap = false)
	{
		const XMMATRIX& matWorld = drawData.matWorld;
		if (bIsCubemap)
		{
			const PerObjectMatricesCubemap objMats = PerObjectMatricesCubemap({ matWorld, matWorld * viewProj });
			pRenderer->SetConstantStruct("ObjMats", &objMats);
		}
		else
		{
			const PerObjectMatrices objMats = PerObjectMatrices({ drawData.matWorld * viewProj });
			pRenderer->SetConstantStruct("ObjMats", &objMats);
		}
		std::for_each(drawData.meshIDs.begin(), drawData.meshIDs.end(), [&](MeshID id)
		{
			const RasterizerStateID rasterizerState = GeometryGenerator::Is2DGeometry(id) ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBufferIDsOfMesh(ENGINE->mpActiveScene, id);

			pRenderer->SetRasterizerState(rasterizerState);
			pRenderer->SetVertexBuffer(IABuffer.first);
			pRenderer->SetIndexBuffer(IABuffer.second);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		});
	};
#endif
	//-----------------------------------------------------------------------------------------------
	D3D11_VIEWPORT viewPort = {};
	viewPort.MinDepth = 0.f;
	viewPort.MaxDepth = 1.f;

	const bool bNoShadowingLights = shadowView.spots.empty() && shadowView.points.empty() && shadowView.pDirectional == nullptr;
	if (bNoShadowingLights) return;

	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_WRITE);
	pRenderer->SetShader(mShadowMapShaderInstanced);

	// CLEAR SHADOW MAPS
	//
	pGPUProfiler->BeginEntry("Clear");
	for (size_t i = 0; i < shadowView.spots.size(); i++)
	{
		pRenderer->BindDepthTarget(mDepthTargets_Spot[i]);	// only depth stencil buffer
		pRenderer->BeginRender(ClearCommand::Depth(1.0f));
	}
	for (size_t i = 0; i < shadowView.points.size(); i++)
	{
		for (int face = 0; face < 6; ++face)
		{
			const size_t depthTargetIndex = i * 6 + face;
			pRenderer->BindDepthTarget(mDepthTargets_Point[depthTargetIndex]);	// only depth stencil buffer
			pRenderer->BeginRender(ClearCommand::Depth(1.0f));
		}
	}
	pGPUProfiler->EndEntry(); // Clear


	//-----------------------------------------------------------------------------------------------
	// SPOT LIGHT SHADOW MAPS
	//-----------------------------------------------------------------------------------------------
	viewPort.Height = static_cast<float>(mShadowMapDimension_Spot);
	viewPort.Width = static_cast<float>(mShadowMapDimension_Spot);
	pGPUProfiler->BeginEntry("Spots");
	pRenderer->SetViewport(viewPort);
	for (size_t i = 0; i < shadowView.spots.size(); i++)
	{
		const Light* l = shadowView.spots[i];
		const XMMATRIX viewProj = l->GetLightSpaceMatrix();
#if _DEBUG
		if (shadowView.shadowMapMeshDrawListLookup.find(shadowView.spots[i]) == shadowView.shadowMapMeshDrawListLookup.end())
		{
			Log::Error("Spot light not found in shadowmap render list lookup");
			continue;
		}
#endif


		if (mDepthTargets_Spot[i] == -1)
		{
#if _DEBUG
			Log::Error("Invalid depth target =-1 !");
#endif
			continue;
		}

		pRenderer->BeginEvent("Spot[" + std::to_string(i) + "]: DrawSceneZ()");
		pRenderer->BindDepthTarget(mDepthTargets_Spot[i]);	// only depth stencil buffer
		//pRenderer->Apply();

#if 0
		for (const GameObject* pObj : shadowView.shadowMapRenderListLookUp.at(shadowView.spots[i]))
		{
			const ModelData& model = pObj->GetModelData();
			const XMMATRIX matWorld = pObj->GetTransform().WorldTransformationMatrix();
			const DepthOnlyPass_PerObjectMatrices objMats = DepthOnlyPass_PerObjectMatrices({ matWorld * viewProj });
			pRenderer->SetConstantStruct("ObjMats", &objMats);
			
			std::for_each(model.mMeshIDs.begin(), model.mMeshIDs.end(), [&](MeshID id)
			{
#if FORCE_NO_CULL_SPOTLIGHTS
				const RasterizerStateID rasterizerState = EDefaultRasterizerState::CULL_BACK;
#else
				const RasterizerStateID rasterizerState = GeometryGenerator::Is2DGeometry(static_cast<EGeometry>(id))
					? EDefaultRasterizerState::CULL_NONE
					: EDefaultRasterizerState::CULL_FRONT;
#endif
				const auto IABuffer = SceneResourceView::GetVertexAndIndexBufferIDsOfMesh(ENGINE->mpActiveScene, id, pObj);

				pRenderer->SetRasterizerState(rasterizerState);
				pRenderer->SetVertexBuffer(IABuffer.first);
				pRenderer->SetIndexBuffer(IABuffer.second);
				pRenderer->Apply();
				pRenderer->DrawIndexed();
			});
		}
#else
		DepthOnlyPass_InstancedObjectCBuffer cbuffer;
		for (const std::pair<MeshID, std::vector<XMMATRIX>>& f : shadowView.shadowMapMeshDrawListLookup.at(l).meshTransformListLookup)
		{
			const int meshInstanceCount = static_cast<int>(f.second.size());
			const MeshID& meshID        = f.first;
			assert(meshInstanceCount > 0); // make sure no empty meshID transformation list

#if FORCE_NO_CULL_SPOTLIGHTS
			const RasterizerStateID rasterizerState =  EDefaultRasterizerState::CULL_BACK;
#else
			const RasterizerStateID rasterizerState = GeometryGenerator::Is2DGeometry(static_cast<EGeometry>(meshID))
				? EDefaultRasterizerState::CULL_NONE
				: EDefaultRasterizerState::CULL_FRONT;
#endif

			const auto IABuffer = SceneResourceView::GetVertexAndIndexBufferIDsOfMesh(ENGINE->mpActiveScene
				, meshID
#if INCLUDE_OBJECT_POINTER_TO_DRAW_DATA
				, meshInstanceData.back().pObj
#endif
			);

			pRenderer->SetVertexBuffer(IABuffer.first);
			pRenderer->SetIndexBuffer(IABuffer.second);
			pRenderer->SetRasterizerState(rasterizerState);

			int batchCount = 0;
			do
			{
				int instanceID = 0;
				for (; instanceID < MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS; ++instanceID)
				{
					const int renderListIndex = MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS * batchCount + instanceID;
					if (renderListIndex == meshInstanceCount)
						break;

					cbuffer.objMatrices[instanceID] = DepthOnlyPass_PerObjectMatrices
					{
#if INCLUDE_OBJECT_POINTER_TO_DRAW_DATA
							(*pMatrices[meshInstanceData[instanceID].martixID]) * viewProj
#else
							f.second[renderListIndex] * viewProj
#endif
					};
				}

				pRenderer->SetConstantStruct("ObjMats", &cbuffer);
				pRenderer->Apply();
				pRenderer->DrawIndexedInstanced(instanceID);
			} while (batchCount++ < meshInstanceCount / MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS);

		}
#endif
		pRenderer->EndEvent();
	}
	pGPUProfiler->EndEntry();	// spots


	//-----------------------------------------------------------------------------------------------
	// DIRECTIONAL SHADOW MAP
	//-----------------------------------------------------------------------------------------------
	pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_FRONT);
	if (shadowView.pDirectional != nullptr)
	{
		const XMMATRIX viewProj = shadowView.pDirectional->GetLightSpaceMatrix();

		pGPUProfiler->BeginEntry("Directional");
		pRenderer->BeginEvent("Directional: DrawSceneZ()");

		// RENDER NON-INSTANCED SCENE OBJECTS
		//
		const int shadowMapDimension = pRenderer->GetTextureObject(pRenderer->GetDepthTargetTexture(this->mDepthTarget_Directional))._width;
		viewPort.Height = static_cast<float>(shadowMapDimension);
		viewPort.Width = static_cast<float>(shadowMapDimension);
		pRenderer->SetViewport(viewPort);
		pRenderer->BindDepthTarget(mDepthTarget_Directional);
		pRenderer->Apply();
		pRenderer->BeginRender(ClearCommand::Depth(1.0f));
		for (const GameObject* pObj : shadowView.casters)
		{
			;// RenderDepth(pObj, viewProj);
		}


		// RENDER INSTANCED SCENE OBJECTS
		//
		pRenderer->SetShader(mShadowMapShaderInstanced);
		pRenderer->BindDepthTarget(mDepthTarget_Directional);

		DepthOnlyPass_InstancedObjectCBuffer cbuffer;
		for (const RenderListLookupEntry& MeshID_RenderList : shadowView.RenderListsPerMeshType)
		{
			const MeshID& mesh = MeshID_RenderList.first;
			const std::vector<const GameObject*>& renderList = MeshID_RenderList.second;

			const RasterizerStateID rasterizerState = EDefaultRasterizerState::CULL_NONE;// Is2DGeometry(mesh) ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBufferIDsOfMesh(ENGINE->mpActiveScene, mesh, renderList.back());

			pRenderer->SetRasterizerState(rasterizerState);
			pRenderer->SetVertexBuffer(IABuffer.first);
			pRenderer->SetIndexBuffer(IABuffer.second);

			int batchCount = 0;
			do
			{
				int instanceID = 0;
				for (; instanceID < MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS; ++instanceID)
				{
					const int renderListIndex = MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS * batchCount + instanceID;
					if (renderListIndex == renderList.size())
						break;

					cbuffer.objMatrices[instanceID] =
					{
						renderList[renderListIndex]->GetTransform().WorldTransformationMatrix() * viewProj
					};
				}

				pRenderer->SetConstantStruct("ObjMats", &cbuffer);
				pRenderer->Apply();
				pRenderer->DrawIndexedInstanced(instanceID);
			} while (batchCount++ < renderList.size() / MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS);
		}

		pRenderer->EndEvent();
		pGPUProfiler->EndEntry();
	}


	//-----------------------------------------------------------------------------------------------
	// POINT LIGHT SHADOW MAPS
	//-----------------------------------------------------------------------------------------------
	viewPort.Height = static_cast<float>(mShadowMapDimension_Point);
	viewPort.Width = static_cast<float>(mShadowMapDimension_Point);
	pGPUProfiler->BeginEntry("Points");
	pRenderer->SetShader(this->mShadowCubeMapShader);
	pRenderer->SetViewport(viewPort);
	pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_NONE);


	for (size_t i = 0; i < shadowView.points.size(); i++)
	{
		struct PointLightCBuffer
		{
			vec4 lightPosition_farPlane;
		} _cbLight;

#if _DEBUG
		if (shadowView.shadowCubeMapMeshDrawListLookup.find(shadowView.points[i]) == shadowView.shadowCubeMapMeshDrawListLookup.end())
		{
			Log::Error("Point light not found in shadowmap render list lookup");
			continue;;
		}
#endif

		pRenderer->BeginEvent("Point[" + std::to_string(i) + "]: DrawSceneZ()");
		
		_cbLight.lightPosition_farPlane = vec4(shadowView.points[i]->mTransform._position, shadowView.points[i]->mRange);

#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
		DepthOnlyPass_InstancedObjectCubemapCBuffer cbuffer;
#endif		

		// render objects for each face
		for (int face = 0; face < 6; ++face)
		{
			const XMMATRIX viewProj =
				shadowView.points[i]->GetViewMatrix(static_cast<Texture::CubemapUtility::ECubeMapLookDirections>(face))
				* shadowView.points[i]->GetProjectionMatrix();

			const size_t depthTargetIndex = i * 6 + face;
			pRenderer->BindDepthTarget(mDepthTargets_Point[depthTargetIndex]);	// only depth stencil buffer
			pRenderer->SetConstantStruct("cbLight", &_cbLight);
			pRenderer->Apply();

#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
#if INCLUDE_OBJECT_POINTER_TO_DRAW_DATA
			const std::vector<const XMMATRIX*>& pMatrices = shadowView.shadowCubeMapMeshDrawListLookup.at(shadowView.points[i])[face].mpTransformationMatrices;
			for (const std::pair<const MeshID, std::vector<MeshDrawData::ObjectDrawLookupData>>& f : shadowView.shadowCubeMapMeshDrawListLookup.at(shadowView.points[i])[face].mMeshDrawDataLookup)
#else
			for (const std::pair<MeshID, std::vector<XMMATRIX>>& f : shadowView.shadowCubeMapMeshDrawListLookup.at(shadowView.points[i])[face].meshTransformListLookup)
#endif
			{
#if INCLUDE_OBJECT_POINTER_TO_DRAW_DATA
				const std::vector< MeshDrawData::ObjectDrawLookupData>& meshInstanceData = f.second;
				const int meshInstanceCount = static_cast<int>(meshInstanceData.size());
#else
				const int meshInstanceCount = static_cast<int>(f.second.size());
#endif

				const MeshID& meshID = f.first;
				assert(meshInstanceCount > 0); // make sure no empty meshID transformation list

#if FORCE_NO_CULL_POINTLIGHTS
				const RasterizerStateID rasterizerState = EDefaultRasterizerState::CULL_BACK;
#else
				const RasterizerStateID rasterizerState = GeometryGenerator::Is2DGeometry(static_cast<EGeometry>(meshID))
					? EDefaultRasterizerState::CULL_NONE 
					: EDefaultRasterizerState::CULL_FRONT;

#endif
				const auto IABuffer = SceneResourceView::GetVertexAndIndexBufferIDsOfMesh(ENGINE->mpActiveScene
					, meshID
#if INCLUDE_OBJECT_POINTER_TO_DRAW_DATA
					, meshInstanceData.back().pObj
#endif
				);
				pRenderer->SetVertexBuffer(IABuffer.first);
				pRenderer->SetIndexBuffer(IABuffer.second);
				pRenderer->SetRasterizerState(rasterizerState);

				int batchCount = 0;
				do
				{
					int instanceID = 0;
					for (; instanceID < MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS; ++instanceID)
					{
						const int renderListIndex = MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS * batchCount + instanceID;
						if (renderListIndex == meshInstanceCount)
							break;
						
						cbuffer.objMatrices[instanceID] = DepthOnlyPass_PerObjectMatricesCubemap
						{
#if INCLUDE_OBJECT_POINTER_TO_DRAW_DATA
							(*pMatrices[meshInstanceData[instanceID].martixID]),
							(*pMatrices[meshInstanceData[instanceID].martixID]) * viewProj
#else
							f.second[renderListIndex],
							f.second[renderListIndex] * viewProj
#endif
						};
					}

					pRenderer->SetConstantStruct("ObjMats", &cbuffer);
					pRenderer->Apply();
					pRenderer->DrawIndexedInstanced(instanceID);
				} while (batchCount++ < meshInstanceCount / MAX_DRAW_INSTANCED_COUNT__DEPTH_PASS);
			}
			
#else
			for (const MeshDrawData& drawData : shadowView.shadowCubeMapMeshDrawListLookup.at(shadowView.points[i])[face])
			{
				RenderDepthMeshes(drawData, viewProj, true);
			}
#endif

		}
		pRenderer->EndEvent();
	}
	pGPUProfiler->EndEntry();	// Points

}

