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


#include "GameObject.h"
//#include "Utilities/vectormath.h"

#include "RenderPasses/RenderPasses.h"
#include "Utilities/Log.h"

namespace VQEngine
{
	bool IsSphereInFrustum(const FrustumPlaneset& frustum, const vec3& sphereCenter, const float sphereRadius)
	{
		bool bInside = true;
		for (int plane = 0; plane < 6; ++plane)
		{
			vec3 N = vec3(
				frustum.abcd[plane].x
				, frustum.abcd[plane].y
				, frustum.abcd[plane].z
			);
			const vec3 planeNormal = N.normalized();
			const float D = XMVector3Dot(N, sphereCenter).m128_f32[0] + frustum.abcd[plane].w;
			//const float D = XMVector3Dot(planeNormal, sphereCenter).m128_f32[0] + frustum.abcd[plane].w;
			//const float D = XMVector3Dot(planeNormal, sphereCenter).m128_f32[0];

#if 0
			if (D < -sphereRadius) // outside
				return false;

			else if (D < sphereRadius) // intersect
			{
				bInside = true;
			}
#else
		//if (fabsf(D) > sphereRadius)
		//	return
			if (D < 0.0f)
			{
				//if ( (-D -frustum.abcd[plane].w) > sphereRadius)
				if (-D > sphereRadius)
					return false;
			}
#endif
		}
		return bInside;
	}


	bool IsVisible(const FrustumPlaneset& frustum, const BoundingBox& aabb)
	{
		const vec4 points[] =
		{
		{ aabb.low.x(), aabb.low.y(), aabb.low.z(), 1.0f },
		{ aabb.hi.x() , aabb.low.y(), aabb.low.z(), 1.0f },
		{ aabb.hi.x() , aabb.hi.y() , aabb.low.z(), 1.0f },
		{ aabb.low.x(), aabb.hi.y() , aabb.low.z(), 1.0f },

		{ aabb.low.x(), aabb.low.y(), aabb.hi.z() , 1.0f},
		{ aabb.hi.x() , aabb.low.y(), aabb.hi.z() , 1.0f},
		{ aabb.hi.x() , aabb.hi.y() , aabb.hi.z() , 1.0f},
		{ aabb.low.x(), aabb.hi.y() , aabb.hi.z() , 1.0f},
		};

		for (int i = 0; i < 6; ++i)	// for each plane
		{
			bool bInside = false;
			for (int j = 0; j < 8; ++j)	// for each point
			{
				if (XMVector4Dot(points[j], frustum.abcd[i]).m128_f32[0] > 0.000002f)
				{
					bInside = true;
					break;
				}
			}
			if (!bInside)
			{
				return false;
			}
		}
		return true;
	}



	size_t CullMeshes(
		const FrustumPlaneset& frustumPlanes,
		const GameObject* pObj,
		MeshDrawData& meshDrawData
	)
	{
		size_t numCulled = 0;

#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
		const XMMATRIX  matWorld = pObj->GetTransform().WorldTransformationMatrix();
#else
		meshDrawData.matWorld = pObj->GetTransform().WorldTransformationMatrix();
		const XMMATRIX& matWorld = meshDrawData.matWorld;
#endif

		const std::vector<MeshID>& objMeshIDs = pObj->GetModelData().mMeshIDs;
		for (MeshID meshIDIndex = 0; meshIDIndex < objMeshIDs.size(); ++meshIDIndex)
		{
			const MeshID meshID = objMeshIDs[meshIDIndex];
			const BoundingBox localBB = pObj->GetMeshBBs()[meshIDIndex];
			const BoundingBox worldBB = BoundingBox
			({
				XMVector4Transform(vec4(localBB.low, 1.0f), matWorld),
				XMVector4Transform(vec4(localBB.hi, 1.0f),  matWorld)
				});

			if (IsVisible(frustumPlanes, worldBB))
			{

#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
				meshDrawData.AddMeshTransformation(meshID, matWorld);
#else
				meshDrawData.meshIDs.push_back(meshID);
#endif
			}
			else
				++numCulled;
		}
		return numCulled;
	}




	size_t CullGameObjects(
		const FrustumPlaneset&                  frustumPlanes
		, const std::vector<const GameObject*>& pObjs
		, std::vector<const GameObject*>&       pCulledObjs
	)
	{
		size_t currIdx = 0;
		std::for_each(RANGE(pObjs), [&](const GameObject* pObj)
		{
			// aabb is static and in world space during load time.
			// this wouldn't work for dynamic objects in this state.
			const BoundingBox aabb_world = [&]()
			{
				const XMMATRIX world = pObj->GetTransform().WorldTransformationMatrix();
				const XMMATRIX worldRotation = pObj->GetTransform().RotationMatrix();
				const BoundingBox& aabb_local = pObj->GetAABB();
#if 1
				// transform low and high points of the bounding box: model->world
				return BoundingBox(
					{
						XMVector4Transform(vec4(aabb_local.low, 1.0f), world),
						XMVector4Transform(vec4(aabb_local.hi , 1.0f), world)
					});
#else
				//----------------------------------------------------------------------------------
				// TODO: there's an error in the code below. 
				// bug repro: turn your back to the nanosuit in sponza scene -> suit won't be culled.
				//----------------------------------------------------------------------------------
				// transform center and extent and construct high and low later
				// we can use XMVector3Transform() for the extent vector to save some instructions
				const vec3 extent = aabb_local.hi - aabb_local.low;
				const vec4 center = vec4((aabb_local.hi + aabb_local.low) * 0.5f, 1.0f);
				const vec3 tfC = XMVector4Transform(center, world);
				const vec3 tfEx = XMVector3Transform(extent, worldRotation) * 0.5f;
				return BoundingBox(
					{
						{tfC - tfEx},	// lo
						{tfC + tfEx}	// hi
					});
#endif
			}();

			//assert(!pObj->GetModelData().mMeshIDs.empty());
			if (pObj->GetModelData().mMeshIDs.empty())
			{
#if _DEBUG
				Log::Warning("CullGameObject(): GameObject with empty mesh list.");
#endif
				return;
			}

			if (IsVisible(frustumPlanes, aabb_world))
			{
				pCulledObjs.push_back(pObj);
				++currIdx;
			}
		});
		return pObjs.size() - currIdx;
	}



#if THREADED_FRUSTUM_CULL
	struct CullMeshWorkerData
	{
		// in
		const Light* pLight;
		const std::vector<const GameObject*>& renderableList;

		// out
		std::array< MeshDrawList, 6> meshListForPoints;
	};
#endif

}