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


#include "ObjectCullingSystem.h"
#include "GameObject.h"
//#include "Utilities/vectormath.h"

#include "RenderPasses/RenderPasses.h"
#include "Utilities/Log.h"

namespace VQEngine
{
	bool IsSphereInFrustum(const FrustumPlaneset& frustum, const Sphere& sphere)
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
			const float D = XMVector3Dot(N, sphere.c).m128_f32[0] + frustum.abcd[plane].w;
			//const float D = XMVector3Dot(planeNormal, sphere.c).m128_f32[0] + frustum.abcd[plane].w;
			//const float D = XMVector3Dot(planeNormal, sphere.c).m128_f32[0];

#if 0
			if (D < -sphereRadius) // outside
				return false;

			else if (D < sphereRadius) // intersect
			{
				bInside = true;
			}
#else
		//if (fabsf(D) > sphere.r)
		//	return
			if (D < 0.0f)
			{
				//if ( (-D -frustum.abcd[plane].w) > sphere.r)
				if (-D > sphere.r)
					return false;
			}
#endif
		}
		return bInside;
	}

	bool IsBoundingBoxVisibleFromFrustum(const FrustumPlaneset& frustum, const BoundingBox& aabb)
	{
		std::array<vec4, 8> points = aabb.GetCornerPointsV4();

		constexpr float EPSILON = 0.000002f;
		constexpr XMFLOAT4 F4_EPSILON = XMFLOAT4(EPSILON, EPSILON, EPSILON, EPSILON);
		const XMVECTOR V_EPSILON = XMLoadFloat4(&F4_EPSILON);

		for (int i = 0; i < 6; ++i)	// for each plane
		{
			bool bInside = false;
			for (int j = 0; j < 8; ++j)	// for each point
			{
#if 1
				if (XMVector4Dot(points[j], frustum.abcd[i]).m128_f32[0] > EPSILON)
#else
				const XMVECTOR DOT = XMVector4Dot(points[j], frustum.abcd[i]);
				if(XMVector4Greater(DOT, V_EPSILON))
#endif
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



	bool IsBoundingBoxInsideSphere_Approx(const BoundingBox& aabb, const Sphere& sphere)
	{
		// do a rough test here: 
		// derive a bounding sphere from AABB and then test if spheres intersect
		//
		const vec3 diag = (aabb.hi - aabb.low) * 0.5f;  // AABB diagonal vector
		Sphere BS							// bounding sphere of the AABB
		(
			  (aabb.hi + aabb.low) * 0.5f
			, std::sqrtf(XMVector3Dot(diag, diag).m128_f32[0])
		);

		return IsIntersecting(sphere, BS);
	}

	bool IsIntersecting(const Sphere& s1, const Sphere& s2)
	{
		vec3 vDist = s1.c - s2.c;
		vDist = XMVector3Dot(vDist, vDist); // sqDist
		
		const float centerDistance = std::sqrtf(vDist.x());
		const float radiusSum = s1.r + s2.r;

		if (centerDistance > radiusSum) // dist > r1 + r2 := no intersection
			return false;

		return true;
	}

	size_t CullMeshes(
		const FrustumPlaneset& frustumPlanes,
		const GameObject* pObj,
		MeshDrawData& meshDrawData
	)
	{
		size_t numCulled = pObj->GetModelData().mMeshIDs.size();

#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
		const XMMATRIX  matWorld = pObj->GetTransform().WorldTransformationMatrix();
#else
		meshDrawData.matWorld = pObj->GetTransform().WorldTransformationMatrix();
		const XMMATRIX& matWorld = meshDrawData.matWorld;
#endif

		// first test at GameObject granularity
		BoundingBox BB = pObj->GetAABB();
		BB.low = XMVector3Transform(BB.low, matWorld);
		BB.hi  = XMVector3Transform(BB.hi , matWorld); // world space BB
		if (!IsBoundingBoxVisibleFromFrustum(frustumPlanes, BB))
		{
			return numCulled;
		}

		// if GameObject is visible, then test individual meshes.
		const std::vector<MeshID>& objMeshIDs = pObj->GetModelData().mMeshIDs;
		for (MeshID meshIDIndex = 0; meshIDIndex < objMeshIDs.size(); ++meshIDIndex)
		{
			const MeshID meshID = objMeshIDs[meshIDIndex];

			BB = pObj->GetMeshBBs()[meshIDIndex]; // local space BB
			BB.low = XMVector3Transform(BB.low, matWorld);
			BB.hi  = XMVector3Transform(BB.hi , matWorld); // world space BB
			if (IsBoundingBoxVisibleFromFrustum(frustumPlanes, BB))
			{
#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
				meshDrawData.AddMeshTransformation(meshID, matWorld);
#else
				meshDrawData.meshIDs.push_back(meshID);
#endif
				// we initially assumed we culled everything, so we correct 
				// the assumption here by decrementing the number of culled.
				--numCulled;
			}
		}
		return numCulled;
	}

	static std::vector<int> indicesOfVisibleMeshes(5000, -1);

#if 0
	size_t CullMeshes(CullMeshData& data)
	{
		const size_t numWorkItems = data.pLocalSpaceBoundingBoxes->size();
		size_t numCulled = numWorkItems;

		// shorthands
#if SHADOW_PASS_USE_INSTANCED_DRAW_DATA
		const XMMATRIX&  matWorld = data.matWorld;
#else
		meshDrawData.matWorld = data.matWorld;
		const XMMATRIX& matWorld = meshDrawData.matWorld;
#endif

		// container resize if necessary
		if (indicesOfVisibleMeshes.size() < numWorkItems)
		{
			indicesOfVisibleMeshes.resize(numWorkItems, -1);
		}
		int currOutputIndex = 0;

		// transform BB and Cull
		for (int currMeshIDIndex =0; currMeshIDIndex <numWorkItems; ++currMeshIDIndex)
		{
			BoundingBox BB = (*data.pLocalSpaceBoundingBoxes)[currMeshIDIndex];
			BB.low = XMVector4Transform(vec4(BB.low, 1.0f), matWorld);
			BB.hi = XMVector4Transform(vec4(BB.hi, 1.0f), matWorld); // world space BB
			if (IsVisible(data.frustumPlanes, BB))
			{
				indicesOfVisibleMeshes[currOutputIndex++] = currMeshIDIndex;

				// we initially assumed we culled everything, so we correct 
				// the assumption here by decrementing the number of culled.
				--numCulled;
			}
		}
		for (int i = 0; i < currOutputIndex; ++i)
		{
			data.pMeshDrawData->AddMeshTransformation((*data.pMeshIDs)[indicesOfVisibleMeshes[i]], matWorld);
		}

		return numCulled;
	}
#endif

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
				const XMMATRIX worldRotation = pObj->GetTransform().RotationMatrix();
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

			if (IsBoundingBoxVisibleFromFrustum(frustumPlanes, aabb_world))
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

std::array<vec4, 8> BoundingBox::GetCornerPointsV4() const
{
	return std::array<vec4, 8> 
	{
		vec4( this->low.x(), this->low.y(), this->low.z(), 1.0f ),
		vec4( this->hi.x() , this->low.y(), this->low.z(), 1.0f ),
		vec4( this->hi.x() , this->hi.y() , this->low.z(), 1.0f ),
		vec4( this->low.x(), this->hi.y() , this->low.z(), 1.0f ),

		vec4( this->low.x(), this->low.y(), this->hi.z() , 1.0f ),
		vec4( this->hi.x() , this->low.y(), this->hi.z() , 1.0f ),
		vec4( this->hi.x() , this->hi.y() , this->hi.z() , 1.0f ),
		vec4( this->low.x(), this->hi.y() , this->hi.z() , 1.0f )
	};
}

std::array<vec3, 8> BoundingBox::GetCornerPointsV3() const
{
	return std::array<vec3, 8> 
	{
		vec3(this->low.x(), this->low.y(), this->low.z()),
		vec3(this->hi.x() , this->low.y(), this->low.z()),
		vec3(this->hi.x() , this->hi.y() , this->low.z()),
		vec3(this->low.x(), this->hi.y() , this->low.z()),

		vec3(this->low.x(), this->low.y(), this->hi.z() ),
		vec3(this->hi.x() , this->low.y(), this->hi.z() ),
		vec3(this->hi.x() , this->hi.y() , this->hi.z() ),
		vec3(this->low.x(), this->hi.y() , this->hi.z() )
	};
}
