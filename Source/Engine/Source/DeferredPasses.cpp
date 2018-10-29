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

constexpr int DRAW_INSTANCED_COUNT_GBUFFER_PASS = 64;

void DeferredRenderingPasses::Initialize(Renderer * pRenderer)
{
	//const ShaderMacro testMacro = { "TEST_DEFINE", "1" };

	// TODO: reduce the code redundancy
	const char* pFSQ_VS = "FullScreenQuad_vs.hlsl";
	const char* pLight_VS = "MVPTransformationWithUVs_vs.hlsl";
	const ShaderDesc geomShaderDesc = { "GBufferPass", {
		ShaderStageDesc{ "Deferred_Geometry_vs.hlsl", {} },
		ShaderStageDesc{ "Deferred_Geometry_ps.hlsl", {} }
	} };
	const std::vector<ShaderMacro> instancedGeomShaderMacros =
	{
		ShaderMacro{ "INSTANCED", "1" },
		ShaderMacro{ "INSTANCE_COUNT", std::to_string(DRAW_INSTANCED_COUNT_GBUFFER_PASS)}
	};
	const ShaderDesc geomShaderInstancedDesc = { "InstancedGBufferPass",
	{
		ShaderStageDesc{ "Deferred_Geometry_vs.hlsl", instancedGeomShaderMacros },
		ShaderStageDesc{ "Deferred_Geometry_ps.hlsl", instancedGeomShaderMacros }
	} };
	const ShaderDesc ambientShaderDesc = { "Deferred_Ambient",
	{
		ShaderStageDesc{ pFSQ_VS, {} },
		ShaderStageDesc{ "deferred_brdf_ambient_ps.hlsl", {} }
	} };
	const ShaderDesc ambientIBLShaderDesc = { "Deferred_AmbientIBL",
	{
		ShaderStageDesc{ pFSQ_VS, {} },
		ShaderStageDesc{ "deferred_brdf_ambientIBL_ps.hlsl", {} }
	} };
	const ShaderDesc BRDFLightingShaderDesc = { "Deferred_BRDF_Lighting",
	{
		ShaderStageDesc{ pFSQ_VS, {} },
		ShaderStageDesc{ "deferred_brdf_lighting_ps.hlsl", {} }
	} };
	const ShaderDesc phongLighintShaderDesc = { "Deferred_Phong_Lighting",
	{
		ShaderStageDesc{ pFSQ_VS, {} },
		ShaderStageDesc{ "deferred_phong_lighting_ps.hlsl", {} }
	} };
	const ShaderDesc BRDF_PointLightShaderDesc = { "Deferred_BRDF_Point",
	{
		ShaderStageDesc{ pLight_VS, {} },
		ShaderStageDesc{ "deferred_brdf_pointLight_ps.hlsl", {} }
	} };
	const ShaderDesc BRDF_SpotLightShaderDesc = { "Deferred_BRDF_Spot",
	{
		ShaderStageDesc{ pLight_VS, {} },
		ShaderStageDesc{ "deferred_brdf_spotLight_ps.hlsl", {} }
	} };

	InitializeGBuffer(pRenderer);

	_geometryShader = pRenderer->CreateShader(geomShaderDesc);
	_geometryInstancedShader = pRenderer->CreateShader(geomShaderInstancedDesc);
	_ambientShader = pRenderer->CreateShader(ambientShaderDesc);
	_ambientIBLShader = pRenderer->CreateShader(ambientIBLShaderDesc);
	_BRDFLightingShader = pRenderer->CreateShader(BRDFLightingShaderDesc);
	_phongLightingShader = pRenderer->CreateShader(phongLighintShaderDesc);
	_spotLightShader = pRenderer->CreateShader(BRDF_PointLightShaderDesc);
	_pointLightShader = pRenderer->CreateShader(BRDF_SpotLightShaderDesc);

	// deferred geometry is accessed from elsewhere, needs to be globally defined
	assert(EShaders::DEFERRED_GEOMETRY == _geometryShader);	// this assumption may break, make sure it doesn't...
}

void DeferredRenderingPasses::InitializeGBuffer(Renderer* pRenderer)
{
	EImageFormat imageFormats[2] = { RGBA32F, RGBA16F };
	RenderTargetDesc rtDesc[2] = { {}, {} };
	for (int i = 0; i < 2; ++i)
	{
		rtDesc[i].textureDesc.width = pRenderer->WindowWidth();
		rtDesc[i].textureDesc.height = pRenderer->WindowHeight();
		rtDesc[i].textureDesc.mipCount = 1;
		rtDesc[i].textureDesc.arraySize = 1;
		rtDesc[i].textureDesc.usage = ETextureUsage::RENDER_TARGET_RW;
		rtDesc[i].textureDesc.format = imageFormats[i];
		rtDesc[i].format = imageFormats[i];
	}

	constexpr size_t Float3TypeIndex = 0;		constexpr size_t Float4TypeIndex = 1;
	this->_GBuffer._normalRT = pRenderer->AddRenderTarget(rtDesc[Float3TypeIndex]);
	this->_GBuffer._diffuseRoughnessRT = pRenderer->AddRenderTarget(rtDesc[Float4TypeIndex]);
	this->_GBuffer._specularMetallicRT = pRenderer->AddRenderTarget(rtDesc[Float4TypeIndex]);
	this->_GBuffer.bInitialized = true;

	// http://download.nvidia.com/developer/presentations/2004/6800_Leagues/6800_Leagues_Deferred_Shading.pdf
	// Option: trade storage for computation
	//  - Store pos.z     and compute xy from z + window.xy		(implemented)
	//	- Store normal.xy and compute z = sqrt(1 - x^2 - y^2)
	//
	{	// Geometry depth stencil state descriptor
		D3D11_DEPTH_STENCILOP_DESC dsOpDesc = {};
		dsOpDesc.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		dsOpDesc.StencilDepthFailOp = D3D11_STENCIL_OP_INCR_SAT;
		dsOpDesc.StencilPassOp = D3D11_STENCIL_OP_INCR_SAT;
		dsOpDesc.StencilFunc = D3D11_COMPARISON_ALWAYS;

		D3D11_DEPTH_STENCIL_DESC desc = {};
		desc.FrontFace = dsOpDesc;
		desc.BackFace = dsOpDesc;

		desc.DepthEnable = true;
		desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

		desc.StencilEnable = true;
		desc.StencilReadMask = 0xFF;
		desc.StencilWriteMask = 0xFF;

		_geometryStencilState = pRenderer->AddDepthStencilState(desc);
	}

}

void DeferredRenderingPasses::ClearGBuffer(Renderer* pRenderer)
{
	const bool bDoClearColor = true;
	const bool bDoClearDepth = false;
	const bool bDoClearStencil = false;
	ClearCommand clearCmd(
		bDoClearColor, bDoClearDepth, bDoClearStencil,
		{ 0, 0, 0, 0 }, 0, 0
	);
	pRenderer->BindRenderTargets(_GBuffer._diffuseRoughnessRT, _GBuffer._specularMetallicRT, _GBuffer._normalRT);
	pRenderer->BeginRender(clearCmd);
}


//struct InstancedGbufferObjectMatrices { ObjectMatrices objMatrices[DRAW_INSTANCED_COUNT_GBUFFER_PASS]; };
void DeferredRenderingPasses::RenderGBuffer(Renderer* pRenderer, const Scene* pScene, const SceneView& sceneView) const
{
	//--------------------------------------------------------------------------------------------------------------------
	struct InstancedGbufferObjectMaterials { SurfaceMaterial objMaterials[DRAW_INSTANCED_COUNT_GBUFFER_PASS]; };
	auto Is2DGeometry = [](MeshID mesh)
	{
		return mesh == EGeometry::TRIANGLE || mesh == EGeometry::QUAD || mesh == EGeometry::GRID;
	};
	auto RenderObject = [&](const GameObject* pObj)
	{
		const Transform& tf = pObj->GetTransform();
		const ModelData& model = pObj->GetModelData();

		const XMMATRIX world = tf.WorldTransformationMatrix();
		const ObjectMatrices mats =
		{
			world * sceneView.view,
			tf.NormalMatrix(world) * sceneView.view,
			world * sceneView.viewProj,
		};

		pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);

		SurfaceMaterial material;
		for (MeshID id : model.mMeshIDs)
		{
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(pScene, id);

			// SET MATERIAL CONSTANT BUFFER & TEXTURES
			//
			const bool bMeshHasMaterial = model.mMaterialLookupPerMesh.find(id) != model.mMaterialLookupPerMesh.end();
			if (bMeshHasMaterial)
			{
				const MaterialID materialID = model.mMaterialLookupPerMesh.at(id);
				const Material* pMat = SceneResourceView::GetMaterial(pScene, materialID);

				// #TODO: uncomment below when transparency is implemented.
				//if (pMat->IsTransparent())	// avoidable branching - perhaps keeping opaque and transparent meshes on separate vectors is better.
				//	return;

				material = pMat->GetShaderFriendlyStruct();
				pRenderer->SetConstantStruct("surfaceMaterial", &material);
				pRenderer->SetConstantStruct("ObjMatrices", &mats);
				if (pMat->diffuseMap >= 0)	pRenderer->SetTexture("texDiffuseMap", pMat->diffuseMap);
				if (pMat->normalMap >= 0)	pRenderer->SetTexture("texNormalMap", pMat->normalMap);
				if (pMat->specularMap >= 0)	pRenderer->SetTexture("texSpecularMap", pMat->specularMap);
				if (pMat->mask >= 0)		pRenderer->SetTexture("texAlphaMask", pMat->mask);
				pRenderer->SetConstant1f("BRDFOrPhong", 1.0f);	// assume brdf for now

			}
			else
			{
				assert(false);// mMaterials.GetDefaultMaterial(GGX_BRDF)->SetMaterialConstants(pRenderer, EShaders::DEFERRED_GEOMETRY, sceneView.bIsDeferredRendering);
			}

			pRenderer->SetVertexBuffer(IABuffer.first);
			pRenderer->SetIndexBuffer(IABuffer.second);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		};
	};
	//--------------------------------------------------------------------------------------------------------------------

	const bool bDoClearColor = true;
	const bool bDoClearDepth = true;
	const bool bDoClearStencil = true;
	ClearCommand clearCmd(
		bDoClearColor, bDoClearDepth, bDoClearStencil,
		{ 0, 0, 0, 0 }, 1, 0
	);

	pRenderer->SetShader(_geometryShader);
	pRenderer->BindRenderTargets(_GBuffer._diffuseRoughnessRT, _GBuffer._specularMetallicRT, _GBuffer._normalRT);
	pRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());
	pRenderer->SetDepthStencilState(_geometryStencilState);
	pRenderer->SetSamplerState("sNormalSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);
	pRenderer->BeginRender(clearCmd);
	pRenderer->Apply();


	// RENDER NON-INSTANCED SCENE OBJECTS
	//
	int numObj = 0;
	for (const auto* obj : sceneView.culledOpaqueList)
	{
		RenderObject(obj);
		++numObj;
	}



	// RENDER INSTANCED SCENE OBJECTS
	//
	pRenderer->SetShader(_geometryInstancedShader);
	pRenderer->BindRenderTargets(_GBuffer._diffuseRoughnessRT, _GBuffer._specularMetallicRT, _GBuffer._normalRT);
	pRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());
	pRenderer->Apply();

	InstancedObjectMatrices<DRAW_INSTANCED_COUNT_GBUFFER_PASS> cbufferMatrices;
	InstancedGbufferObjectMaterials cbufferMaterials;

	for (const RenderListLookupEntry& MeshID_RenderList : sceneView.culluedOpaqueInstancedRenderListLookup)
	{
		const MeshID& meshID = MeshID_RenderList.first;
		const RenderList& renderList = MeshID_RenderList.second;


		const RasterizerStateID rasterizerState = Is2DGeometry(meshID) ? EDefaultRasterizerState::CULL_NONE : EDefaultRasterizerState::CULL_BACK;
		const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(pScene, meshID);

		pRenderer->SetRasterizerState(rasterizerState);
		pRenderer->SetVertexBuffer(IABuffer.first);
		pRenderer->SetIndexBuffer(IABuffer.second);

		int batchCount = 0;
		do
		{
			int instanceID = 0;
			for (; instanceID < DRAW_INSTANCED_COUNT_GBUFFER_PASS; ++instanceID)
			{
				const int renderListIndex = DRAW_INSTANCED_COUNT_GBUFFER_PASS * batchCount + instanceID;
				if (renderListIndex == renderList.size())
					break;

				const GameObject* pObj = renderList[renderListIndex];
				const Transform& tf = pObj->GetTransform();
				const ModelData& model = pObj->GetModelData();

				const XMMATRIX world = tf.WorldTransformationMatrix();
				cbufferMatrices.objMatrices[instanceID] =
				{
					world * sceneView.view,
					tf.NormalMatrix(world) * sceneView.view,
					world * sceneView.viewProj,
				};

				const bool bMeshHasMaterial = model.mMaterialLookupPerMesh.find(meshID) != model.mMaterialLookupPerMesh.end();
				if (bMeshHasMaterial)
				{
					const MaterialID materialID = model.mMaterialLookupPerMesh.at(meshID);
					const Material* pMat = SceneResourceView::GetMaterial(pScene, materialID);
					cbufferMaterials.objMaterials[instanceID] = pMat->GetShaderFriendlyStruct();
				}
			}

			pRenderer->SetConstantStruct("ObjMatrices", &cbufferMatrices);
			pRenderer->SetConstantStruct("surfaceMaterial", &cbufferMaterials);
			pRenderer->SetConstant1f("BRDFOrPhong", 1.0f);	// assume brdf for now
			pRenderer->Apply();
			pRenderer->DrawIndexedInstanced(instanceID);
		} while (batchCount++ < renderList.size() / DRAW_INSTANCED_COUNT_GBUFFER_PASS);
	}
}

void DeferredRenderingPasses::RenderLightingPass(const RenderParams& args) const
{
	ClearCommand cmd = ClearCommand::Color({ 0, 0, 0, 0 });

	const bool bAmbientOcclusionOn = args.tSSAO == -1;
	Renderer* pRenderer = args.pRenderer;

	const vec2 screenSize = pRenderer->GetWindowDimensionsAsFloat2();
	const TextureID texNormal = pRenderer->GetRenderTargetTexture(_GBuffer._normalRT);
	const TextureID texDiffuseRoughness = pRenderer->GetRenderTargetTexture(_GBuffer._diffuseRoughnessRT);
	const TextureID texSpecularMetallic = pRenderer->GetRenderTargetTexture(_GBuffer._specularMetallicRT);
	const ShaderID lightingShader = args.bUseBRDFLighting ? _BRDFLightingShader : _phongLightingShader;
	const TextureID texIrradianceMap = args.sceneView.environmentMap.irradianceMap;
	const SamplerID smpEnvMap = args.sceneView.environmentMap.envMapSampler;
	const TextureID texSpecularMap = args.sceneView.environmentMap.prefilteredEnvironmentMap;
	const TextureID tBRDFLUT = EnvironmentMap::sBRDFIntegrationLUTTexture;
	const TextureID depthTexture = pRenderer->GetDepthTargetTexture(ENGINE->GetWorldDepthTarget());

	const auto IABuffersQuad = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::FULLSCREENQUAD);
	constexpr bool bUnbindRenderTargets = false; // we're switching between lighting shaders w/ same render targets

	pRenderer->UnbindDepthTarget();
	pRenderer->BindRenderTarget(args.target);
	pRenderer->BeginRender(cmd);
	pRenderer->Apply();

	// AMBIENT LIGHTING
	//-----------------------------------------------------------------------------------------
	const bool bSkylight = args.sceneView.bIsIBLEnabled && texIrradianceMap != -1;
	if (bSkylight)
	{
		pRenderer->BeginEvent("Environment Map Lighting Pass");
#if USE_COMPUTE_PASS_UNIT_TEST || USE_COMPUTE_SSAO
		pRenderer->SetShader(_ambientIBLShader, bUnbindRenderTargets, true);
#else
		pRenderer->SetShader(_ambientIBLShader, bUnbindRenderTargets, false);
#endif
		pRenderer->SetTexture("tDiffuseRoughnessMap", texDiffuseRoughness);
		pRenderer->SetTexture("tSpecularMetalnessMap", texSpecularMetallic);
		pRenderer->SetTexture("tNormalMap", texNormal);
		pRenderer->SetTexture("tDepthMap", depthTexture);
		pRenderer->SetTexture("tAmbientOcclusion", args.tSSAO);
		pRenderer->SetTexture("tIrradianceMap", texIrradianceMap);
		pRenderer->SetTexture("tPreFilteredEnvironmentMap", texSpecularMap);
		pRenderer->SetTexture("tBRDFIntegrationLUT", tBRDFLUT);
		pRenderer->SetSamplerState("sEnvMapSampler", smpEnvMap);
		pRenderer->SetSamplerState("sWrapSampler", EDefaultSamplerState::WRAP_SAMPLER);
		pRenderer->SetConstant4x4f("matViewInverse", args.sceneView.viewInverse);
		pRenderer->SetConstant4x4f("matProjInverse", args.sceneView.projInverse);
	}
	else
	{
		pRenderer->BeginEvent("Ambient Pass");
		pRenderer->SetShader(_ambientShader, bUnbindRenderTargets);
		pRenderer->SetTexture("tDiffuseRoughnessMap", texDiffuseRoughness);
		pRenderer->SetTexture("tAmbientOcclusion", args.tSSAO);
	}
	pRenderer->SetSamplerState("sNearestSampler", EDefaultSamplerState::POINT_SAMPLER);
	pRenderer->SetConstant1f("ambientFactor", args.sceneView.sceneRenderSettings.ssao.ambientFactor);
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
	pRenderer->EndEvent();


	// DIFFUSE & SPECULAR LIGHTING
	//-----------------------------------------------------------------------------------------
	pRenderer->BeginEvent("Lighting Pass");
	pRenderer->SetBlendState(EDefaultBlendState::ADDITIVE_COLOR);

	// draw fullscreen quad for lighting for now. Will add light volumes
	// as the scene gets more complex or depending on performance needs.
#ifdef USE_LIGHT_VOLUMES
#if 0
	const auto IABuffersSphere = ENGINE->GetGeometryVertexAndIndexBuffers(EGeometry::SPHERE);

	pRenderer->SetConstant3f("CameraWorldPosition", sceneView.pCamera->GetPositionF());
	pRenderer->SetConstant2f("ScreenSize", screenSize);
	pRenderer->SetTexture("texDiffuseRoughnessMap", texDiffuseRoughness);
	pRenderer->SetTexture("texSpecularMetalnessMap", texSpecularMetallic);
	pRenderer->SetTexture("texNormals", texNormal);

	// POINT LIGHTS
	pRenderer->SetShader(SHADERS::DEFERRED_BRDF_POINT);
	pRenderer->SetVertexBuffer(IABuffersSphere.first);
	pRenderer->SetIndexBuffer(IABuffersSphere.second);
	//pRenderer->SetRasterizerState(ERasterizerCullMode::)
	for (const Light& light : lights)
	{
		if (light._type == Light::ELightType::POINT)
		{
			const float& r = light._range;	//bounding sphere radius
			const vec3&  pos = light._transform._position;
			const XMMATRIX world = {
				r, 0, 0, 0,
				0, r, 0, 0,
				0, 0, r, 0,
				pos.x(), pos.y(), pos.z(), 1,
			};

			const XMMATRIX wvp = world * sceneView.viewProj;
			const LightShaderSignature lightData = light.ShaderSignature();
			pRenderer->SetConstant4x4f("worldViewProj", wvp);
			pRenderer->SetConstantStruct("light", &lightData);
			pRenderer->Apply();
			pRenderer->DrawIndexed();
		}
	}
#endif

	// SPOT LIGHTS
#if 0
	pRenderer->SetShader(SHADERS::DEFERRED_BRDF);
	pRenderer->SetConstant3f("cameraPos", m_pCamera->GetPositionF());

	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);

	// for spot lights

	pRenderer->Apply();
	pRenderer->DrawIndexed();
#endif

#else
	pRenderer->SetShader(lightingShader, bUnbindRenderTargets);
	ENGINE->SendLightData();

	pRenderer->SetConstant4x4f("matView", args.sceneView.view);
	pRenderer->SetConstant4x4f("matViewToWorld", args.sceneView.viewInverse);
	pRenderer->SetConstant4x4f("matPorjInverse", args.sceneView.projInverse);
	//pRenderer->SetSamplerState("sNearestSampler", EDefaultSamplerState::POINT_SAMPLER);
	pRenderer->SetSamplerState("sLinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER);
	pRenderer->SetSamplerState("sShadowSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);
	pRenderer->SetTexture("texDiffuseRoughnessMap", texDiffuseRoughness);
	pRenderer->SetTexture("texSpecularMetalnessMap", texSpecularMetallic);
	pRenderer->SetTexture("texNormals", texNormal);
	pRenderer->SetTexture("texDepth", depthTexture);
	pRenderer->SetVertexBuffer(IABuffersQuad.first);
	pRenderer->SetIndexBuffer(IABuffersQuad.second);
	pRenderer->Apply();
	pRenderer->DrawIndexed();
#endif	// light volumes
	pRenderer->EndEvent(); // Lighting Pass
	pRenderer->SetBlendState(EDefaultBlendState::DISABLED);

}
