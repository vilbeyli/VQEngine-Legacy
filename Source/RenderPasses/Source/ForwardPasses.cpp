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
#include "Engine/Scene.h"

#include "Renderer/Renderer.h"

constexpr int DRAW_INSTANCED_COUNT_ZPREPASS = 64;

void ZPrePass::Initialize(Renderer* pRenderer)
{
	const std::vector<ShaderMacro> instancedGeomShaderMacros =
	{
		ShaderMacro{ "INSTANCED", "1" },
		ShaderMacro{ "INSTANCE_COUNT", std::to_string(DRAW_INSTANCED_COUNT_ZPREPASS)}
	};
	const ShaderDesc shaders[2] =
	{
		ShaderDesc {"ZPrePass", {
			ShaderStageDesc{"Deferred_Geometry_vs.hlsl"            , {} },
			ShaderStageDesc{"ViewSpaceNormalsAndPositions_ps.hlsl" , {} }
		}},
		ShaderDesc {"ZPrePass-Instanced", {
			ShaderStageDesc{"Deferred_Geometry_vs.hlsl"            , {instancedGeomShaderMacros} },
			ShaderStageDesc{"ViewSpaceNormalsAndPositions_ps.hlsl" , {} }
		}},
	};

	objShader = pRenderer->CreateShader(shaders[0]);
	objShaderInstanced = pRenderer->CreateShader(shaders[1]);
}

void ZPrePass::RenderDepth(const RenderParams& args) const
{
	//--------------------------------------------------------------------------------------------------------------------
	auto RenderObject = [&](const GameObject* pObj)
	{
		const Transform& tf = pObj->GetTransform();
		const ModelData& model = pObj->GetModelData();

		const XMMATRIX world = tf.WorldTransformationMatrix();
		const ObjectMatrices mats =
		{
			world * args.sceneView.view,
			tf.NormalMatrix(world) * args.sceneView.view,
			world * args.sceneView.viewProj,
		};

		args.pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);

		SurfaceMaterial material;
		args.pRenderer->SetConstant1i("textureConfig", 0);
		for (MeshID id : model.mMeshIDs)
		{
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(args.pScene, id);

			// SET MATERIAL CONSTANT BUFFER & TEXTURES
			//
			const bool bMeshHasMaterial = model.mMaterialLookupPerMesh.find(id) != model.mMaterialLookupPerMesh.end();
			if (bMeshHasMaterial)
			{
				const MaterialID materialID = model.mMaterialLookupPerMesh.at(id);
				const Material* pMat = SceneResourceView::GetMaterial(args.pScene, materialID);

				// #TODO: uncomment below when transparency is implemented.
				//if (pMat->IsTransparent())	// avoidable branching - perhaps keeping opaque and transparent meshes on separate vectors is better.
				//	return;

				if (pMat->normalMap >= 0)	args.pRenderer->SetTexture("texNormalMap", pMat->normalMap);
				if (pMat->mask >= 0)		args.pRenderer->SetTexture("texAlphaMask", pMat->mask);
				args.pRenderer->SetConstant1i("textureConfig", pMat->GetTextureConfig());
				args.pRenderer->SetConstant2f("uvScale", pMat->tiling);
				args.pRenderer->SetConstantStruct("ObjMatrices", &mats);
			}
#if _DEBUG
			else
			{
				assert(false);// mMaterials.GetDefaultMaterial(GGX_BRDF)->SetMaterialConstants(pRenderer, EShaders::DEFERRED_GEOMETRY, sceneView.bIsDeferredRendering);
			}
#endif

			args.pRenderer->SetVertexBuffer(IABuffer.first);
			args.pRenderer->SetIndexBuffer(IABuffer.second);
			args.pRenderer->Apply();
			args.pRenderer->DrawIndexed();
		};
	};
	//--------------------------------------------------------------------------------------------------------------------

	const RenderTargetID normals = args.normalRT;
	const TextureID texNormal = args.pRenderer->GetRenderTargetTexture(normals);

	const bool bDoClearColor = true;
	const bool bDoClearDepth = true;
	const bool bDoClearStencil = false;
	ClearCommand clearCmd(
		bDoClearColor, bDoClearDepth, bDoClearStencil,
		{ 0, 0, 0, 0 }, 1, 0
	);

	args.pRenderer->BeginEvent("Z-PrePass");
	args.pRenderer->SetShader(objShader);
	args.pRenderer->SetSamplerState("sNormalSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);
	args.pRenderer->BindRenderTarget(normals);
	args.pRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());
	args.pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_WRITE);
	args.pRenderer->BeginRender(clearCmd);


	// RENDER NON-INSTANCED SCENE OBJECTS
	//
	int numObj = 0;
	for (const GameObject* pObj : args.sceneView.culledOpaqueList)
	{
		RenderObject(pObj);
		++numObj;
	}


	// RENDER INSTANCED SCENE OBJECTS
	//
	args.pRenderer->SetShader(objShaderInstanced);
	args.pRenderer->BindRenderTarget(normals);
	args.pRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());

	InstancedObjectMatrices<DRAW_INSTANCED_COUNT_ZPREPASS> cbufferMatrices;
	for (const RenderListLookupEntry& MeshID_RenderList : args.sceneView.culluedOpaqueInstancedRenderListLookup)
	{
		const MeshID& meshID = MeshID_RenderList.first;
		const RenderList& renderList = MeshID_RenderList.second;
		const RasterizerStateID rasterizerState = EDefaultRasterizerState::CULL_BACK;
		const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(args.pScene, meshID);

		args.pRenderer->SetRasterizerState(rasterizerState);
		args.pRenderer->SetVertexBuffer(IABuffer.first);
		args.pRenderer->SetIndexBuffer(IABuffer.second);

		int batchCount = 0;
		do
		{
			int instanceID = 0;
			for (; instanceID < DRAW_INSTANCED_COUNT_ZPREPASS; ++instanceID)
			{
				const int renderListIndex = DRAW_INSTANCED_COUNT_ZPREPASS * batchCount + instanceID;
				if (renderListIndex == renderList.size())
					break;

				const GameObject* pObj = renderList[renderListIndex];
				const Transform& tf = pObj->GetTransform();
				const ModelData& model = pObj->GetModelData();

				const XMMATRIX world = tf.WorldTransformationMatrix();
				cbufferMatrices.objMatrices[instanceID] =
				{
					world * args.sceneView.view,
					tf.NormalMatrix(world) * args.sceneView.view,
					world * args.sceneView.viewProj,
				};

				const bool bMeshHasMaterial = model.mMaterialLookupPerMesh.find(meshID) != model.mMaterialLookupPerMesh.end();
				if (bMeshHasMaterial)
				{
					const MaterialID materialID = model.mMaterialLookupPerMesh.at(meshID);
					const Material* pMat = SceneResourceView::GetMaterial(args.pScene, materialID);
					//cbufferMaterials.objMaterials[instanceID] = pMat->GetShaderFriendlyStruct();
				}
			}

			args.pRenderer->SetConstantStruct("ObjMatrices", &cbufferMatrices);
			args.pRenderer->Apply();
			args.pRenderer->DrawIndexedInstanced(instanceID);
		} while (batchCount++ < renderList.size() / DRAW_INSTANCED_COUNT_ZPREPASS);
	}

	args.pRenderer->EndEvent(); // Z-PrePass
}


void ForwardLightingPass::Initialize(Renderer* pRenderer)
{
	fwdBRDF = EShaders::FORWARD_BRDF;

	const std::vector<ShaderMacro> instancedShaderMacros =
	{
		ShaderMacro{ "INSTANCED", "1" },
		ShaderMacro{ "INSTANCE_COUNT", std::to_string(DRAW_INSTANCED_COUNT_ZPREPASS)}
	};
	const ShaderDesc instancedBRDFDesc = { "Forward_BRDF-Instanced", {
			ShaderStageDesc{"Forward_BRDF_vs.hlsl", { instancedShaderMacros } },
			ShaderStageDesc{"Forward_BRDF_ps.hlsl", { instancedShaderMacros } }
		}
	};
	fwdBRDFInstanced = pRenderer->CreateShader(instancedBRDFDesc);
}

void ForwardLightingPass::RenderLightingPass(const RenderParams& args) const
{
	// shorthands
	Renderer* const& pRenderer = args.pRenderer;
	const SceneView& sceneView = args.sceneView;
	//--------------------------------------------------------------------------------------------------------------------
	struct InstancedGbufferObjectMaterials { SurfaceMaterial objMaterials[DRAW_INSTANCED_COUNT_ZPREPASS]; };
	auto Is2DGeometry = [](MeshID mesh)
	{
		return mesh == EGeometry::TRIANGLE || mesh == EGeometry::QUAD || mesh == EGeometry::GRID;
	};
	auto RenderObject = [&](const GameObject* pObj)
	{
		const Transform& tf = pObj->GetTransform();
		const ModelData& model = pObj->GetModelData();

		const XMMATRIX world = tf.WorldTransformationMatrix();
#if 0
		XMVECTOR detWorld = XMMatrixDeterminant(world);
		XMMATRIX worldInverse = XMMatrixInverse(&detWorld, world);
#endif
		const ObjectMatrices mats =
		{
			world * sceneView.viewProj,
			world,
			tf.NormalMatrix(world),
		};

		pRenderer->SetRasterizerState(EDefaultRasterizerState::CULL_BACK);

		SurfaceMaterial material;
		for (MeshID id : model.mMeshIDs)
		{
			const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(args.pScene, id);

			// SET MATERIAL CONSTANT BUFFER & TEXTURES
			//
			const bool bMeshHasMaterial = model.mMaterialLookupPerMesh.find(id) != model.mMaterialLookupPerMesh.end();
			if (bMeshHasMaterial)
			{
				const MaterialID materialID = model.mMaterialLookupPerMesh.at(id);
				const Material* pMat = SceneResourceView::GetMaterial(args.pScene, materialID);

				// #TODO: uncomment below when transparency is implemented.
				//if (pMat->IsTransparent())	// avoidable branching - perhaps keeping opaque and transparent meshes on separate vectors is better.
				//	return;

				material = pMat->GetShaderFriendlyStruct();
				pRenderer->SetConstantStruct("surfaceMaterial", &material);
				pRenderer->SetConstantStruct("ObjMatrices", &mats);

				// #TODO: this is duplicate code, see Deferred.
				pRenderer->SetSamplerState("sAnisoSampler", EDefaultSamplerState::ANISOTROPIC_4_WRAPPED_SAMPLER);
				if (pMat->diffuseMap >= 0)	pRenderer->SetTexture("texDiffuseMap", pMat->diffuseMap);
				if (pMat->normalMap >= 0)	pRenderer->SetTexture("texNormalMap", pMat->normalMap);
				if (pMat->specularMap >= 0)	pRenderer->SetTexture("texSpecularMap", pMat->specularMap);
				if (pMat->mask >= 0)		pRenderer->SetTexture("texAlphaMask", pMat->mask);
				if (pMat->roughnessMap >= 0)	pRenderer->SetTexture("texRoughnessMap", pMat->roughnessMap);
				if (pMat->metallicMap >= 0)		pRenderer->SetTexture("texMetallicMap", pMat->metallicMap);
#if ENABLE_PARALLAX_MAPPING
				if (pMat->heightMap >= 0)		pRenderer->SetTexture("texHeightMap", pMat->heightMap);
#endif
				if (pMat->emissiveMap >= 0)		pRenderer->SetTexture("texEmissiveMap", pMat->emissiveMap);
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

	pRenderer->BeginEvent("Lighting Pass");
	pRenderer->SetShader(/*mSelectedShader*/fwdBRDF);
	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_TEST_ONLY);
	pRenderer->Apply();

	//if (mSelectedShader == EShaders::FORWARD_BRDF || mSelectedShader == EShaders::FORWARD_PHONG)
	const bool bSkylight = args.sceneView.bIsIBLEnabled && args.sceneView.environmentMap.irradianceMap != -1;
	{
		pRenderer->SetTexture("texAmbientOcclusion", args.tSSAO);

		// todo: shader defines -> have a PBR shader with and without environment lighting through preprocessor
		//pRenderer->SetSamplerState("sEnvMapSampler", smpEnvMap);
		
		if (bSkylight)
		{
			pRenderer->SetTexture("tIrradianceMap", args.sceneView.environmentMap.irradianceMap);
			pRenderer->SetTexture("tPreFilteredEnvironmentMap", args.sceneView.environmentMap.prefilteredEnvironmentMap);
			pRenderer->SetTexture("tBRDFIntegrationLUT", EnvironmentMap::sBRDFIntegrationLUTTexture);
			pRenderer->SetSamplerState("sEnvMapSampler", args.sceneView.environmentMap.envMapSampler);
		}

		else
		{
			// even though SetShader() sets null SRVs, somehow when we dont have skylight on
			// the next pass, which is RenderLights(), fails with 'TextureCube bound to Texture Slot 0'
			// debug layer error...
			// hence, set texture here
			//pRenderer->SetTexture("tIrradianceMap", args.tEmptyTex);
			//pRenderer->SetTexture("tPreFilteredEnvironmentMap", args.tEmptyTex);
			//pRenderer->SetTexture("tBRDFIntegrationLUT", args.tEmptyTex);
		}
		//if (mSelectedShader == EShaders::FORWARD_BRDF)
		{
			pRenderer->SetConstant1f("isEnvironmentLightingOn", bSkylight ? 1.0f : 0.0f);
			pRenderer->SetSamplerState("sWrapSampler", EDefaultSamplerState::WRAP_SAMPLER);
			pRenderer->SetSamplerState("sNearestSampler", EDefaultSamplerState::POINT_SAMPLER);
		}
		//else
		//	pRenderer->SetSamplerState("sNormalSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);
		// todo: shader defines -> have a PBR shader with and without environment lighting through preprocessor

		pRenderer->SetConstant1f("ambientFactor", args.sceneView.sceneRenderSettings.ssao.ambientFactor);
		pRenderer->SetConstant3f("cameraPos", args.sceneView.cameraPosition);
		pRenderer->SetConstant2f("screenDimensions", pRenderer->GetWindowDimensionsAsFloat2());
		pRenderer->SetSamplerState("sLinearSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);

		ENGINE->SendLightData();
	}

	// RENDER NON-INSTANCED SCENE OBJECTS
	//
	int numObj = 0;
	for (const auto* obj : args.sceneView.culledOpaqueList)
	{
		RenderObject(obj);
		++numObj;
	}



	// RENDER INSTANCED SCENE OBJECTS
	//
#if 1
	pRenderer->SetShader(fwdBRDFInstanced);
	pRenderer->BindRenderTarget(args.targetRT);
	pRenderer->BindDepthTarget(ENGINE->GetWorldDepthTarget());
	pRenderer->SetDepthStencilState(EDefaultDepthStencilState::DEPTH_STENCIL_WRITE);
	pRenderer->SetTexture("texAmbientOcclusion", args.tSSAO);

	// todo: shader defines -> have a PBR shader with and without environment lighting through preprocessor
	//pRenderer->SetSamplerState("sEnvMapSampler", smpEnvMap);
	if (bSkylight)
	{
		pRenderer->SetTexture("tIrradianceMap", args.sceneView.environmentMap.irradianceMap);
		pRenderer->SetTexture("tPreFilteredEnvironmentMap", args.sceneView.environmentMap.prefilteredEnvironmentMap);
		pRenderer->SetTexture("tBRDFIntegrationLUT", EnvironmentMap::sBRDFIntegrationLUTTexture);
		pRenderer->SetSamplerState("sEnvMapSampler", args.sceneView.environmentMap.envMapSampler);
	}
	else
	{
		// even though SetShader() sets null SRVs, somehow when we dont have skylight on
		// the next pass, which is RenderLights(), fails with 'TextureCube bound to Texture Slot 0'
		// debug layer error...
		// hence, set texture here
		//pRenderer->SetTexture("tIrradianceMap"            , args.tEmptyTex);
		//pRenderer->SetTexture("tPreFilteredEnvironmentMap", args.tEmptyTex);
		//pRenderer->SetTexture("tBRDFIntegrationLUT"       , args.tEmptyTex);
	}
	//if (mSelectedShader == EShaders::FORWARD_BRDF)
	{
		pRenderer->SetConstant1f("isEnvironmentLightingOn", bSkylight ? 1.0f : 0.0f);
		pRenderer->SetSamplerState("sWrapSampler", EDefaultSamplerState::WRAP_SAMPLER);
		pRenderer->SetSamplerState("sNearestSampler", EDefaultSamplerState::POINT_SAMPLER);
	}
	//else
	//	pRenderer->SetSamplerState("sNormalSampler", EDefaultSamplerState::LINEAR_FILTER_SAMPLER_WRAP_UVW);
	// todo: shader defines -> have a PBR shader with and without environment lighting through preprocessor

	pRenderer->SetConstant1f("ambientFactor", args.sceneView.sceneRenderSettings.ssao.ambientFactor);
	pRenderer->SetConstant3f("cameraPos", args.sceneView.cameraPosition);
	pRenderer->SetConstant2f("screenDimensions", pRenderer->GetWindowDimensionsAsFloat2());

	ENGINE->SendLightData();

	InstancedObjectMatrices<DRAW_INSTANCED_COUNT_ZPREPASS> cbufferMatrices;
	InstancedGbufferObjectMaterials cbufferMaterials;

	for (const RenderListLookupEntry& MeshID_RenderList : sceneView.culluedOpaqueInstancedRenderListLookup)
	{
		const MeshID& meshID = MeshID_RenderList.first;
		const RenderList& renderList = MeshID_RenderList.second;
		const RasterizerStateID rasterizerState = EDefaultRasterizerState::CULL_BACK;
		const auto IABuffer = SceneResourceView::GetVertexAndIndexBuffersOfMesh(args.pScene, meshID);

		pRenderer->SetRasterizerState(rasterizerState);
		pRenderer->SetVertexBuffer(IABuffer.first);
		pRenderer->SetIndexBuffer(IABuffer.second);

		int batchCount = 0;
		do
		{
			int instanceID = 0;
			for (; instanceID < DRAW_INSTANCED_COUNT_ZPREPASS; ++instanceID)
			{
				const int renderListIndex = DRAW_INSTANCED_COUNT_ZPREPASS * batchCount + instanceID;
				if (renderListIndex == renderList.size())
					break;

				const GameObject* pObj = renderList[renderListIndex];
				const Transform& tf = pObj->GetTransform();
				const ModelData& model = pObj->GetModelData();
				const XMMATRIX world = tf.WorldTransformationMatrix();
				cbufferMatrices.objMatrices[instanceID] =
				{
					world * sceneView.viewProj,
					world,
					tf.NormalMatrix(world),
				};

				const bool bMeshHasMaterial = model.mMaterialLookupPerMesh.find(meshID) != model.mMaterialLookupPerMesh.end();
				if (bMeshHasMaterial)
				{
					const MaterialID materialID = model.mMaterialLookupPerMesh.at(meshID);
					const Material* pMat = SceneResourceView::GetMaterial(args.pScene, materialID);
					cbufferMaterials.objMaterials[instanceID] = pMat->GetShaderFriendlyStruct();
				}
			}

			pRenderer->SetConstantStruct("ObjMatrices", &cbufferMatrices);
			pRenderer->SetConstantStruct("surfaceMaterial", &cbufferMaterials);
			pRenderer->Apply();
			pRenderer->DrawIndexedInstanced(instanceID);
		} while (batchCount++ < renderList.size() / DRAW_INSTANCED_COUNT_ZPREPASS);
	}
#endif

	pRenderer->EndEvent();	// Lighting Pass
}
