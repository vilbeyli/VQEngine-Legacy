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
#include "SceneResources.h"
#include "Engine.h"
#include "GameObject.h"
#include "Light.h"
#include "SceneView.h"

#include "Renderer/Renderer.h"

#if _DEBUG
#include "Utilities/Log.h"
#endif

constexpr int DRAW_INSTANCED_COUNT_DEPTH_PASS = 256;



void ShadowMapPass::InitializeSpotLightShadowMaps(Renderer* pRenderer, const Settings::ShadowMap& shadowMapSettings)
{
	this->mShadowMapDimension_Spot = static_cast<int>(shadowMapSettings.dimension);

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

	// CREATE DEPTH TARGETS: SPOT LIGHTS
	//
	texDesc.height = texDesc.width = mShadowMapDimension_Spot;
	texDesc.arraySize = NUM_SPOT_LIGHT_SHADOW;
	auto DepthTargetIDs = pRenderer->AddDepthTarget(depthDesc);
	this->mDepthTargets_Spot.resize(NUM_SPOT_LIGHT_SHADOW);
	std::copy(RANGE(DepthTargetIDs), this->mDepthTargets_Spot.begin());

	this->mShadowMapTextures_Spot = this->mDepthTargets_Spot.size() > 0
		? pRenderer->GetDepthTargetTexture(this->mDepthTargets_Spot[0])
		: -1;

	this->mShadowMapShader = EShaders::SHADOWMAP_DEPTH;


	ShaderDesc instancedShaderDesc = { "DepthShader",
		ShaderStageDesc{"DepthShader_vs.hlsl", { 
			ShaderMacro{ "INSTANCED"     , "1" }, 
			ShaderMacro{ "INSTANCE_COUNT", std::to_string(DRAW_INSTANCED_COUNT_DEPTH_PASS) } 
		}},
		ShaderStageDesc{"DepthShader_ps.hlsl" , {} }
	};
	this->mShadowMapShaderInstanced = pRenderer->CreateShader(instancedShaderDesc);

	ZeroMemory(&mShadowViewPort_Spot, sizeof(D3D11_VIEWPORT));
	this->mShadowViewPort_Spot.Height = static_cast<float>(mShadowMapDimension_Spot);
	this->mShadowViewPort_Spot.Width = static_cast<float>(mShadowMapDimension_Spot);
	this->mShadowViewPort_Spot.MinDepth = 0.f;
	this->mShadowViewPort_Spot.MaxDepth = 1.f;

}

void ShadowMapPass::InitializeDirectionalLightShadowMap(Renderer * pRenderer, const Settings::ShadowMap & shadowMapSettings)
{
	const int textureDimension = static_cast<int>(shadowMapSettings.dimension);
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
		this->mDepthTarget_Directional = pRenderer->AddDepthTarget(depthDesc)[0];
		this->mShadowMapTexture_Directional = pRenderer->GetDepthTargetTexture(this->mDepthTarget_Directional);
	}
	else // other times - check if dimension changed
	{
		const bool bDimensionChanged = shadowMapSettings.dimension != pRenderer->GetTextureObject(pRenderer->GetDepthTargetTexture(this->mDepthTarget_Directional))._width;
		if (bDimensionChanged)
		{
			pRenderer->RecycleDepthTarget(this->mDepthTarget_Directional, depthDesc);
		}
	}

	mShadowViewPort_Directional.Width = static_cast<FLOAT>(textureDimension);
	mShadowViewPort_Directional.Height = static_cast<FLOAT>(textureDimension);
	mShadowViewPort_Directional.MinDepth = 0.f;
	mShadowViewPort_Directional.MaxDepth = 1.f;
}

#define INSTANCED_DRAW 1
void ShadowMapPass::RenderShadowMaps(Renderer* pRenderer, const ShadowView& shadowView, GPUProfiler* pGPUProfiler) const
{
	//-----------------------------------------------------------------------------------------------
	struct PerObjectMatrices { XMMATRIX wvp; };
	struct InstancedObjectCBuffer { PerObjectMatrices objMatrices[DRAW_INSTANCED_COUNT_DEPTH_PASS]; };
	auto Is2DGeometry = [](MeshID mesh)
	{
		return mesh == EGeometry::TRIANGLE || mesh == EGeometry::QUAD || mesh == EGeometry::GRID;
	};
	auto RenderDepth = [&](const GameObject* pObj, const XMMATRIX& viewProj)
	{
		const ModelData& model = pObj->GetModelData();
		const PerObjectMatrices objMats = PerObjectMatrices({ pObj->GetTransform().WorldTransformationMatrix() * viewProj });

		pRenderer->SetConstantStruct("ObjMats", &objMats);
		std::for_each(model.mMeshIDs.begin(), model.mMeshIDs.end(), [&](MeshID id)
		{
			const RasterizerStateID rasterizerState = Is2DGeometry(id) ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(ENGINE->mpActiveScene, id);

			pRenderer->SetRasterizerState(rasterizerState);
			pRenderer->SetVertexBuffer(IABuffer.first);
			pRenderer->SetIndexBuffer(IABuffer.second);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		});
	};
	//-----------------------------------------------------------------------------------------------


	const bool bNoShadowingLights = shadowView.spots.empty() && shadowView.points.empty() && shadowView.pDirectional == nullptr;
	if (bNoShadowingLights) return;

	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_WRITE);
	pRenderer->SetShader(mShadowMapShader);					// shader for rendering z buffer

	// SPOT LIGHT SHADOW MAPS
	//
	pGPUProfiler->BeginEntry("Spots");
	pRenderer->SetViewport(mShadowViewPort_Spot);
	for (size_t i = 0; i < shadowView.spots.size(); i++)
	{
		const XMMATRIX viewProj = shadowView.spots[i]->GetLightSpaceMatrix();
		pRenderer->BeginEvent("Spot[" + std::to_string(i) + "]: DrawSceneZ()");
#if _DEBUG
		if (shadowView.shadowMapRenderListLookUp.find(shadowView.spots[i]) == shadowView.shadowMapRenderListLookUp.end())
		{
			Log::Error("Spot light not found in shadowmap render list lookup");
			continue;;
		}
#endif
		pRenderer->BindDepthTarget(mDepthTargets_Spot[i]);	// only depth stencil buffer
		pRenderer->BeginRender(ClearCommand::Depth(1.0f));
		pRenderer->Apply();

		for (const GameObject* pObj : shadowView.shadowMapRenderListLookUp.at(shadowView.spots[i]))
		{
			RenderDepth(pObj, viewProj);
		}
		pRenderer->EndEvent();
	}
	pGPUProfiler->EndEntry();	// spots


	// DIRECTIONAL SHADOW MAP
	//
	if (shadowView.pDirectional != nullptr)
	{
		const XMMATRIX viewProj = shadowView.pDirectional->GetLightSpaceMatrix();

		pGPUProfiler->BeginEntry("Directional");
		pRenderer->BeginEvent("Directional: DrawSceneZ()");

		// RENDER NON-INSTANCED SCENE OBJECTS
		//
		pRenderer->SetViewport(mShadowViewPort_Directional);
		pRenderer->BindDepthTarget(mDepthTarget_Directional);
		pRenderer->Apply();
		pRenderer->BeginRender(ClearCommand::Depth(1.0f));
		for (const GameObject* pObj : shadowView.casters)
		{
			RenderDepth(pObj, viewProj);
		}


		// RENDER INSTANCED SCENE OBJECTS
		//
		pRenderer->SetShader(mShadowMapShaderInstanced);
		pRenderer->BindDepthTarget(mDepthTarget_Directional);

		InstancedObjectCBuffer cbuffer;
		for (const RenderListLookupEntry& MeshID_RenderList : shadowView.RenderListsPerMeshType)
		{
			const MeshID& mesh = MeshID_RenderList.first;
			const std::vector<const GameObject*>& renderList = MeshID_RenderList.second;

			const RasterizerStateID rasterizerState = EDefaultRasterizerState::CULL_NONE;// Is2DGeometry(mesh) ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_FRONT;
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(ENGINE->mpActiveScene, mesh);

			pRenderer->SetRasterizerState(rasterizerState);
			pRenderer->SetVertexBuffer(IABuffer.first);
			pRenderer->SetIndexBuffer(IABuffer.second);

			int batchCount = 0;
			do
			{
				int instanceID = 0;
				for (; instanceID < DRAW_INSTANCED_COUNT_DEPTH_PASS; ++instanceID)
				{
					const int renderListIndex = DRAW_INSTANCED_COUNT_DEPTH_PASS * batchCount + instanceID;
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
			} while (batchCount++ < renderList.size() / DRAW_INSTANCED_COUNT_DEPTH_PASS);
		}

		pRenderer->EndEvent();
		pGPUProfiler->EndEntry();
	}
}

