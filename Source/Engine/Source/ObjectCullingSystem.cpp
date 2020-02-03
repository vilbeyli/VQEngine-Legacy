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
#include "Light.h"

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
#if 0
		// Points:  2.36 ms
		// Spots :  1.48 ms
		std::array<vec4, 8> points = aabb.GetCornerPointsV4();
#else
		// Points:  2.20 ms -7.2%
		// Spots :  1.22 ms -21.3%
		//std::array<XMVECTOR, 8> points = aabb.GetCornerPointsV4_0();
		std::array<XMVECTOR, 8> points = aabb.GetCornerPointsV4_1();
#endif

		constexpr float EPSILON = 0.000002f;
		constexpr XMFLOAT4 F4_EPSILON = XMFLOAT4(EPSILON, EPSILON, EPSILON, EPSILON);
		const XMVECTOR V_EPSILON = XMLoadFloat4(&F4_EPSILON);

		const FrustumPlaneset frustum_ = frustum; // local stack copy

		for (int i = 0; i < 6; ++i)	// for each plane
		{
			bool bInside = false;
			//
			// Note:
			//
			// testing points one-by-one in a loop and breaking if Dot>EPSILON
			// is slower due to branch prediction failures, like below:
			//
			/// for (int j = 0; j < 8; ++j)	// for each point
			/// {
			/// 	if (XMVector4Dot(points[j], frustum.abcd[i]).m128_f32[0] > EPSILON)
			/// 	{
			/// 		bInside = true;
			/// 		break;
			/// 	}
			/// }
			//
			// Instead, just test all points at once and don't bother breaking - this is faster.
			//
			bInside = (XMVector4Dot(points[0], frustum.abcd[i]).m128_f32[0] > EPSILON
					|| XMVector4Dot(points[1], frustum.abcd[i]).m128_f32[0] > EPSILON
					|| XMVector4Dot(points[2], frustum.abcd[i]).m128_f32[0] > EPSILON
					|| XMVector4Dot(points[3], frustum.abcd[i]).m128_f32[0] > EPSILON
					
					|| XMVector4Dot(points[4], frustum.abcd[i]).m128_f32[0] > EPSILON
					|| XMVector4Dot(points[5], frustum.abcd[i]).m128_f32[0] > EPSILON
					|| XMVector4Dot(points[6], frustum.abcd[i]).m128_f32[0] > EPSILON
					|| XMVector4Dot(points[7], frustum.abcd[i]).m128_f32[0] > EPSILON
			);

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

	size_t FrustumCullMeshes(
		const FrustumPlaneset& frustumPlanes,
		const GameObject* pObj,
		MeshInstanceTransformationLookup& meshDrawData
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
		const BoundingBox& BB = pObj->GetAABB_World();
		if (!IsBoundingBoxVisibleFromFrustum(frustumPlanes, BB))
		{
			return numCulled;
		}

		// if GameObject is visible, then test individual meshes.
		const std::vector<MeshID>& objMeshIDs = pObj->GetModelData().mMeshIDs;
		for (MeshID meshIDIndex = 0; meshIDIndex < objMeshIDs.size(); ++meshIDIndex)
		{
			const MeshID meshID = objMeshIDs[meshIDIndex];

			const BoundingBox& BBMesh = pObj->GetMeshBBs_World()[meshIDIndex];
			if (IsBoundingBoxVisibleFromFrustum(frustumPlanes, BBMesh))
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


	std::vector<const GameObject*> DistanceCullLight(const Light* l, const std::vector<const GameObject*>& sceneShadowCasterObjects, int& outNumCulledMeshes)
	{
		std::vector<const GameObject*> culledGameObjectList(sceneShadowCasterObjects.size(), nullptr);
		int numObjs = 0;
		for (const GameObject* pObj : sceneShadowCasterObjects)
		{
			if (IsBoundingBoxInsideSphere_Approx(pObj->GetAABB_World(), Sphere(l->GetTransform()._position, l->mRange)))
				culledGameObjectList[numObjs++] = pObj;
			else
				outNumCulledMeshes += pObj->GetModelData().mMeshIDs.size();
		}
		return std::move(culledGameObjectList);
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

} // namespace VQEngine

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
//		return std::array<vec4, 8> 
//00007FF772BA0989  mov         rax,qword ptr [rsp+108h]  
//00007FF772BA0991  mov         qword ptr [rsp+30h],rax  
//00007FF772BA0996  mov         rax,qword ptr [this]  
//00007FF772BA099E  mov         rcx,rax  
//00007FF772BA09A1  call        vec3::z (07FF772A86B45h)  
//00007FF772BA09A6  movss       xmm0,dword ptr [rax]  
//00007FF772BA09AA  movss       dword ptr [rsp+38h],xmm0  
//00007FF772BA09B0  mov         rax,qword ptr [this]  
//00007FF772BA09B8  mov         rcx,rax  
//00007FF772BA09BB  call        vec3::y (07FF772A8C07Ch)  
//00007FF772BA09C0  movss       xmm0,dword ptr [rax]  
//00007FF772BA09C4  movss       dword ptr [rsp+3Ch],xmm0  
//00007FF772BA09CA  mov         rax,qword ptr [this]  
//00007FF772BA09D2  mov         rcx,rax  
//00007FF772BA09D5  call        vec3::x (07FF772A8F51Ah)  
//00007FF772BA09DA  movss       xmm0,dword ptr [rax]  
//00007FF772BA09DE  movss       dword ptr [rsp+40h],xmm0  
//00007FF772BA09E4  movss       xmm0,dword ptr [__real@3f800000 (07FF772D772C0h)]  
//00007FF772BA09EC  movss       dword ptr [rsp+20h],xmm0  
//00007FF772BA09F2  movss       xmm3,dword ptr [rsp+38h]  
//00007FF772BA09F8  movss       xmm2,dword ptr [rsp+3Ch]  
//00007FF772BA09FE  movss       xmm1,dword ptr [rsp+40h]  
//00007FF772BA0A04  mov         rcx,qword ptr [rsp+30h]  
//00007FF772BA0A09  call        vec4::vec4 (07FF772A83ED6h)  
//00007FF772BA0A0E  mov         rax,qword ptr [rsp+108h]  
//00007FF772BA0A16  add         rax,10h  
//00007FF772BA0A1A  mov         qword ptr [rsp+48h],rax  
//00007FF772BA0A1F  mov         rax,qword ptr [this]  
//00007FF772BA0A27  mov         rcx,rax  
//00007FF772BA0A2A  call        vec3::z (07FF772A86B45h)  
//00007FF772BA0A2F  movss       xmm0,dword ptr [rax]  
//00007FF772BA0A33  movss       dword ptr [rsp+50h],xmm0  
//00007FF772BA0A39  mov         rax,qword ptr [this]  
//00007FF772BA0A41  mov         rcx,rax  
//00007FF772BA0A44  call        vec3::y (07FF772A8C07Ch)  
//00007FF772BA0A49  movss       xmm0,dword ptr [rax]  
//00007FF772BA0A4D  movss       dword ptr [rsp+54h],xmm0  
//00007FF772BA0A53  mov         rax,qword ptr [this]  
//00007FF772BA0A5B  add         rax,0Ch  
//00007FF772BA0A5F  mov         rcx,rax  
//00007FF772BA0A62  call        vec3::x (07FF772A8F51Ah)  
//00007FF772BA0A67  movss       xmm0,dword ptr [rax]  
//00007FF772BA0A6B  movss       dword ptr [rsp+58h],xmm0  
//00007FF772BA0A71  movss       xmm0,dword ptr [__real@3f800000 (07FF772D772C0h)]  
//00007FF772BA0A79  movss       dword ptr [rsp+20h],xmm0  
//00007FF772BA0A7F  movss       xmm3,dword ptr [rsp+50h]  
//00007FF772BA0A85  movss       xmm2,dword ptr [rsp+54h]  
//00007FF772BA0A8B  movss       xmm1,dword ptr [rsp+58h]  
//00007FF772BA0A91  mov         rcx,qword ptr [rsp+48h]  
//00007FF772BA0A96  call        vec4::vec4 (07FF772A83ED6h)  
//00007FF772BA0A9B  mov         rax,qword ptr [rsp+108h]  
//00007FF772BA0AA3  add         rax,20h  
//00007FF772BA0AA7  mov         qword ptr [rsp+60h],rax  
//00007FF772BA0AAC  mov         rax,qword ptr [this]  
//00007FF772BA0AB4  mov         rcx,rax  
//00007FF772BA0AB7  call        vec3::z (07FF772A86B45h)  
//00007FF772BA0ABC  movss       xmm0,dword ptr [rax]  
//00007FF772BA0AC0  movss       dword ptr [rsp+68h],xmm0  
//00007FF772BA0AC6  mov         rax,qword ptr [this]  
//00007FF772BA0ACE  add         rax,0Ch  
//00007FF772BA0AD2  mov         rcx,rax  
//00007FF772BA0AD5  call        vec3::y (07FF772A8C07Ch)  
//00007FF772BA0ADA  movss       xmm0,dword ptr [rax]  
//00007FF772BA0ADE  movss       dword ptr [rsp+6Ch],xmm0  
//00007FF772BA0AE4  mov         rax,qword ptr [this]  
//00007FF772BA0AEC  add         rax,0Ch  
//00007FF772BA0AF0  mov         rcx,rax  
//00007FF772BA0AF3  call        vec3::x (07FF772A8F51Ah)  
//00007FF772BA0AF8  movss       xmm0,dword ptr [rax]  
//00007FF772BA0AFC  movss       dword ptr [rsp+70h],xmm0  
//00007FF772BA0B02  movss       xmm0,dword ptr [__real@3f800000 (07FF772D772C0h)]  
//00007FF772BA0B0A  movss       dword ptr [rsp+20h],xmm0  
//00007FF772BA0B10  movss       xmm3,dword ptr [rsp+68h]  
//00007FF772BA0B16  movss       xmm2,dword ptr [rsp+6Ch]  
//00007FF772BA0B1C  movss       xmm1,dword ptr [rsp+70h]  
//00007FF772BA0B22  mov         rcx,qword ptr [rsp+60h]  
//00007FF772BA0B27  call        vec4::vec4 (07FF772A83ED6h)  
//00007FF772BA0B2C  mov         rax,qword ptr [rsp+108h]  
//00007FF772BA0B34  add         rax,30h  
//00007FF772BA0B38  mov         qword ptr [rsp+78h],rax  
//00007FF772BA0B3D  mov         rax,qword ptr [this]  
//00007FF772BA0B45  mov         rcx,rax  
//00007FF772BA0B48  call        vec3::z (07FF772A86B45h)  
//00007FF772BA0B4D  movss       xmm0,dword ptr [rax]  
//00007FF772BA0B51  movss       dword ptr [rsp+80h],xmm0  
//00007FF772BA0B5A  mov         rax,qword ptr [this]  
//00007FF772BA0B62  add         rax,0Ch  
//00007FF772BA0B66  mov         rcx,rax  
//00007FF772BA0B69  call        vec3::y (07FF772A8C07Ch)  
//00007FF772BA0B6E  movss       xmm0,dword ptr [rax]  
//00007FF772BA0B72  movss       dword ptr [rsp+84h],xmm0  
//00007FF772BA0B7B  mov         rax,qword ptr [this]  
//00007FF772BA0B83  mov         rcx,rax  
//00007FF772BA0B86  call        vec3::x (07FF772A8F51Ah)  
//00007FF772BA0B8B  movss       xmm0,dword ptr [rax]  
//00007FF772BA0B8F  movss       dword ptr [rsp+88h],xmm0  
//00007FF772BA0B98  movss       xmm0,dword ptr [__real@3f800000 (07FF772D772C0h)]  
//00007FF772BA0BA0  movss       dword ptr [rsp+20h],xmm0  
//00007FF772BA0BA6  movss       xmm3,dword ptr [rsp+80h]  
//00007FF772BA0BAF  movss       xmm2,dword ptr [rsp+84h]  
//00007FF772BA0BB8  movss       xmm1,dword ptr [rsp+88h]  
//00007FF772BA0BC1  mov         rcx,qword ptr [rsp+78h]  
//00007FF772BA0BC6  call        vec4::vec4 (07FF772A83ED6h)  
//00007FF772BA0BCB  mov         rax,qword ptr [rsp+108h]  
//00007FF772BA0BD3  add         rax,40h  
//00007FF772BA0BD7  mov         qword ptr [rsp+90h],rax  
//00007FF772BA0BDF  mov         rax,qword ptr [this]  
//00007FF772BA0BE7  add         rax,0Ch  
//00007FF772BA0BEB  mov         rcx,rax  
//00007FF772BA0BEE  call        vec3::z (07FF772A86B45h)  
//00007FF772BA0BF3  movss       xmm0,dword ptr [rax]  
//	return std::array<vec4, 8> 
//00007FF772BA0BF7  movss       dword ptr [rsp+98h],xmm0  
//00007FF772BA0C00  mov         rax,qword ptr [this]  
//00007FF772BA0C08  mov         rcx,rax  
//00007FF772BA0C0B  call        vec3::y (07FF772A8C07Ch)  
//00007FF772BA0C10  movss       xmm0,dword ptr [rax]  
//00007FF772BA0C14  movss       dword ptr [rsp+9Ch],xmm0  
//00007FF772BA0C1D  mov         rax,qword ptr [this]  
//00007FF772BA0C25  mov         rcx,rax  
//00007FF772BA0C28  call        vec3::x (07FF772A8F51Ah)  
//00007FF772BA0C2D  movss       xmm0,dword ptr [rax]  
//00007FF772BA0C31  movss       dword ptr [rsp+0A0h],xmm0  
//00007FF772BA0C3A  movss       xmm0,dword ptr [__real@3f800000 (07FF772D772C0h)]  
//00007FF772BA0C42  movss       dword ptr [rsp+20h],xmm0  
//00007FF772BA0C48  movss       xmm3,dword ptr [rsp+98h]  
//00007FF772BA0C51  movss       xmm2,dword ptr [rsp+9Ch]  
//00007FF772BA0C5A  movss       xmm1,dword ptr [rsp+0A0h]  
//00007FF772BA0C63  mov         rcx,qword ptr [rsp+90h]  
//00007FF772BA0C6B  call        vec4::vec4 (07FF772A83ED6h)  
//00007FF772BA0C70  mov         rax,qword ptr [rsp+108h]  
//00007FF772BA0C78  add         rax,50h  
//00007FF772BA0C7C  mov         qword ptr [rsp+0A8h],rax  
//00007FF772BA0C84  mov         rax,qword ptr [this]  
//00007FF772BA0C8C  add         rax,0Ch  
//00007FF772BA0C90  mov         rcx,rax  
//00007FF772BA0C93  call        vec3::z (07FF772A86B45h)  
//00007FF772BA0C98  movss       xmm0,dword ptr [rax]  
//00007FF772BA0C9C  movss       dword ptr [rsp+0B0h],xmm0  
//00007FF772BA0CA5  mov         rax,qword ptr [this]  
//00007FF772BA0CAD  mov         rcx,rax  
//00007FF772BA0CB0  call        vec3::y (07FF772A8C07Ch)  
//00007FF772BA0CB5  movss       xmm0,dword ptr [rax]  
//00007FF772BA0CB9  movss       dword ptr [rsp+0B4h],xmm0  
//00007FF772BA0CC2  mov         rax,qword ptr [this]  
//00007FF772BA0CCA  add         rax,0Ch  
//00007FF772BA0CCE  mov         rcx,rax  
//00007FF772BA0CD1  call        vec3::x (07FF772A8F51Ah)  
//00007FF772BA0CD6  movss       xmm0,dword ptr [rax]  
//00007FF772BA0CDA  movss       dword ptr [rsp+0B8h],xmm0  
//00007FF772BA0CE3  movss       xmm0,dword ptr [__real@3f800000 (07FF772D772C0h)]  
//00007FF772BA0CEB  movss       dword ptr [rsp+20h],xmm0  
//00007FF772BA0CF1  movss       xmm3,dword ptr [rsp+0B0h]  
//00007FF772BA0CFA  movss       xmm2,dword ptr [rsp+0B4h]  
//00007FF772BA0D03  movss       xmm1,dword ptr [rsp+0B8h]  
//00007FF772BA0D0C  mov         rcx,qword ptr [rsp+0A8h]  
//00007FF772BA0D14  call        vec4::vec4 (07FF772A83ED6h)  
//00007FF772BA0D19  mov         rax,qword ptr [rsp+108h]  
//00007FF772BA0D21  add         rax,60h  
//00007FF772BA0D25  mov         qword ptr [rsp+0C0h],rax  
//00007FF772BA0D2D  mov         rax,qword ptr [this]  
//00007FF772BA0D35  add         rax,0Ch  
//00007FF772BA0D39  mov         rcx,rax  
//00007FF772BA0D3C  call        vec3::z (07FF772A86B45h)  
//00007FF772BA0D41  movss       xmm0,dword ptr [rax]  
//00007FF772BA0D45  movss       dword ptr [rsp+0C8h],xmm0  
//00007FF772BA0D4E  mov         rax,qword ptr [this]  
//00007FF772BA0D56  add         rax,0Ch  
//00007FF772BA0D5A  mov         rcx,rax  
//00007FF772BA0D5D  call        vec3::y (07FF772A8C07Ch)  
//00007FF772BA0D62  movss       xmm0,dword ptr [rax]  
//00007FF772BA0D66  movss       dword ptr [rsp+0CCh],xmm0  
//00007FF772BA0D6F  mov         rax,qword ptr [this]  
//00007FF772BA0D77  add         rax,0Ch  
//00007FF772BA0D7B  mov         rcx,rax  
//00007FF772BA0D7E  call        vec3::x (07FF772A8F51Ah)  
//00007FF772BA0D83  movss       xmm0,dword ptr [rax]  
//00007FF772BA0D87  movss       dword ptr [rsp+0D0h],xmm0  
//00007FF772BA0D90  movss       xmm0,dword ptr [__real@3f800000 (07FF772D772C0h)]  
//00007FF772BA0D98  movss       dword ptr [rsp+20h],xmm0  
//00007FF772BA0D9E  movss       xmm3,dword ptr [rsp+0C8h]  
//00007FF772BA0DA7  movss       xmm2,dword ptr [rsp+0CCh]  
//00007FF772BA0DB0  movss       xmm1,dword ptr [rsp+0D0h]  
//00007FF772BA0DB9  mov         rcx,qword ptr [rsp+0C0h]  
//00007FF772BA0DC1  call        vec4::vec4 (07FF772A83ED6h)  
//00007FF772BA0DC6  mov         rax,qword ptr [rsp+108h]  
//00007FF772BA0DCE  add         rax,70h  
//00007FF772BA0DD2  mov         qword ptr [rsp+0D8h],rax  
//00007FF772BA0DDA  mov         rax,qword ptr [this]  
//00007FF772BA0DE2  add         rax,0Ch  
//00007FF772BA0DE6  mov         rcx,rax  
//00007FF772BA0DE9  call        vec3::z (07FF772A86B45h)  
//00007FF772BA0DEE  movss       xmm0,dword ptr [rax]  
//00007FF772BA0DF2  movss       dword ptr [rsp+0E0h],xmm0  
//00007FF772BA0DFB  mov         rax,qword ptr [this]  
//00007FF772BA0E03  add         rax,0Ch  
//00007FF772BA0E07  mov         rcx,rax  
//00007FF772BA0E0A  call        vec3::y (07FF772A8C07Ch)  
//00007FF772BA0E0F  movss       xmm0,dword ptr [rax]  
//00007FF772BA0E13  movss       dword ptr [rsp+0E4h],xmm0  
//00007FF772BA0E1C  mov         rax,qword ptr [this]  
//00007FF772BA0E24  mov         rcx,rax  
//00007FF772BA0E27  call        vec3::x (07FF772A8F51Ah)  
//00007FF772BA0E2C  movss       xmm0,dword ptr [rax]  
//00007FF772BA0E30  movss       dword ptr [rsp+0E8h],xmm0  
//00007FF772BA0E39  movss       xmm0,dword ptr [__real@3f800000 (07FF772D772C0h)]  
//00007FF772BA0E41  movss       dword ptr [rsp+20h],xmm0  
//00007FF772BA0E47  movss       xmm3,dword ptr [rsp+0E0h]  
//00007FF772BA0E50  movss       xmm2,dword ptr [rsp+0E4h]  
//00007FF772BA0E59  movss       xmm1,dword ptr [rsp+0E8h]  
//00007FF772BA0E62  mov         rcx,qword ptr [rsp+0D8h]  
//00007FF772BA0E6A  call        vec4::vec4 (07FF772A83ED6h)  
//00007FF772BA0E6F  mov         rax,qword ptr [rsp+108h]  
//	{
//		vec4( this->low.x(), this->low.y(), this->low.z(), 1.0f ),
//		vec4( this->hi.x() , this->low.y(), this->low.z(), 1.0f ),
//		vec4( this->hi.x() , this->hi.y() , this->low.z(), 1.0f ),
//		vec4( this->low.x(), this->hi.y() , this->low.z(), 1.0f ),
//
//		vec4( this->low.x(), this->low.y(), this->hi.z() , 1.0f ),
//		vec4( this->hi.x() , this->low.y(), this->hi.z() , 1.0f ),
//		vec4( this->hi.x() , this->hi.y() , this->hi.z() , 1.0f ),
//		vec4( this->low.x(), this->hi.y() , this->hi.z() , 1.0f )
//	};
//}
//00007FF772BA0E77  add         rsp,0F0h  
//00007FF772BA0E7E  pop         rdi  
//00007FF772BA0E7F  ret  
}

std::array<XMVECTOR, 8> BoundingBox::GetCornerPointsV4_0() const
{
	const XMFLOAT3& l3 = this->low._v;
	const XMFLOAT3& h3 = this->hi._v;
	
	std::array<XMVECTOR, 8> arr = 
	{
		  XMLoadFloat3(&l3)
		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->low.y(), this->low.z(), 1.0f))
		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->hi.y() , this->low.z(), 1.0f))
		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->hi.y() , this->low.z(), 1.0f))

		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->low.y(), this->hi.z() , 1.0f))
		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->low.y(), this->hi.z() , 1.0f))
		, XMLoadFloat3(&h3)
		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->hi.y() , this->hi.z() , 1.0f))
	};
//	std::array<XMVECTOR, 8> arr = 
//	{
//		  XMLoadFloat3(&l3)
//00007FF6C60B0EEA  mov         rcx,qword ptr [l3]  
//00007FF6C60B0EEF  call        GameObject::GetMeshBBs (07FF6C5F999B7h)  
//00007FF6C60B0EF4  movaps      xmmword ptr [rsp+0E0h],xmm0  
//00007FF6C60B0EFC  movaps      xmm0,xmmword ptr [rsp+0E0h]  
//00007FF6C60B0F04  movaps      xmmword ptr [arr],xmm0  
//		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->low.y(), this->low.z(), 1.0f))
//00007FF6C60B0F09  mov         rax,qword ptr [this]  
//00007FF6C60B0F11  mov         rcx,rax  
//00007FF6C60B0F14  call        vec3::z (07FF6C5F96B45h)  
//00007FF6C60B0F19  movss       xmm0,dword ptr [rax]  
//00007FF6C60B0F1D  movss       dword ptr [rsp+1C8h],xmm0  
//00007FF6C60B0F26  mov         rax,qword ptr [this]  
//00007FF6C60B0F2E  mov         rcx,rax  
//00007FF6C60B0F31  call        vec3::y (07FF6C5F9C07Ch)  
//00007FF6C60B0F36  movss       xmm0,dword ptr [rax]  
//00007FF6C60B0F3A  movss       dword ptr [rsp+1CCh],xmm0  
//00007FF6C60B0F43  mov         rax,qword ptr [this]  
//00007FF6C60B0F4B  add         rax,0Ch  
//00007FF6C60B0F4F  mov         rcx,rax  
//00007FF6C60B0F52  call        vec3::x (07FF6C5F9F51Ah)  
//00007FF6C60B0F57  movss       xmm0,dword ptr [rax]  
//00007FF6C60B0F5B  movss       dword ptr [rsp+1D0h],xmm0  
//00007FF6C60B0F64  movss       xmm0,dword ptr [__real@3f800000 (07FF6C62872C0h)]  
//00007FF6C60B0F6C  movss       dword ptr [rsp+20h],xmm0  
//00007FF6C60B0F72  movss       xmm3,dword ptr [rsp+1C8h]  
//00007FF6C60B0F7B  movss       xmm2,dword ptr [rsp+1CCh]  
//00007FF6C60B0F84  movss       xmm1,dword ptr [rsp+1D0h]  
//00007FF6C60B0F8D  lea         rcx,[rsp+168h]  
//00007FF6C60B0F95  call        DirectX::XMFLOAT4::XMFLOAT4 (07FF6C5F9A3BCh)  
//00007FF6C60B0F9A  mov         rcx,rax  
//00007FF6C60B0F9D  call        DirectX::XMLoadFloat3 (07FF6C5F94BBFh)  
//00007FF6C60B0FA2  movaps      xmmword ptr [rsp+0F0h],xmm0  
//00007FF6C60B0FAA  movaps      xmm0,xmmword ptr [rsp+0F0h]  
//00007FF6C60B0FB2  movaps      xmmword ptr [rsp+60h],xmm0  
//		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->hi.y() , this->low.z(), 1.0f))
//00007FF6C60B0FB7  mov         rax,qword ptr [this]  
//00007FF6C60B0FBF  mov         rcx,rax  
//00007FF6C60B0FC2  call        vec3::z (07FF6C5F96B45h)  
//00007FF6C60B0FC7  movss       xmm0,dword ptr [rax]  
//00007FF6C60B0FCB  movss       dword ptr [rsp+1D4h],xmm0  
//00007FF6C60B0FD4  mov         rax,qword ptr [this]  
//00007FF6C60B0FDC  add         rax,0Ch  
//00007FF6C60B0FE0  mov         rcx,rax  
//00007FF6C60B0FE3  call        vec3::y (07FF6C5F9C07Ch)  
//00007FF6C60B0FE8  movss       xmm0,dword ptr [rax]  
//00007FF6C60B0FEC  movss       dword ptr [rsp+1D8h],xmm0  
//00007FF6C60B0FF5  mov         rax,qword ptr [this]  
//00007FF6C60B0FFD  add         rax,0Ch  
//00007FF6C60B1001  mov         rcx,rax  
//00007FF6C60B1004  call        vec3::x (07FF6C5F9F51Ah)  
//00007FF6C60B1009  movss       xmm0,dword ptr [rax]  
//00007FF6C60B100D  movss       dword ptr [rsp+1DCh],xmm0  
//00007FF6C60B1016  movss       xmm0,dword ptr [__real@3f800000 (07FF6C62872C0h)]  
//00007FF6C60B101E  movss       dword ptr [rsp+20h],xmm0  
//00007FF6C60B1024  movss       xmm3,dword ptr [rsp+1D4h]  
//00007FF6C60B102D  movss       xmm2,dword ptr [rsp+1D8h]  
//00007FF6C60B1036  movss       xmm1,dword ptr [rsp+1DCh]  
//00007FF6C60B103F  lea         rcx,[rsp+178h]  
//00007FF6C60B1047  call        DirectX::XMFLOAT4::XMFLOAT4 (07FF6C5F9A3BCh)  
//00007FF6C60B104C  mov         rcx,rax  
//00007FF6C60B104F  call        DirectX::XMLoadFloat3 (07FF6C5F94BBFh)  
//00007FF6C60B1054  movaps      xmmword ptr [rsp+100h],xmm0  
//00007FF6C60B105C  movaps      xmm0,xmmword ptr [rsp+100h]  
//00007FF6C60B1064  movaps      xmmword ptr [rsp+70h],xmm0  
//		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->hi.y() , this->low.z(), 1.0f))
//00007FF6C60B1069  mov         rax,qword ptr [this]  
//00007FF6C60B1071  mov         rcx,rax  
//00007FF6C60B1074  call        vec3::z (07FF6C5F96B45h)  
//00007FF6C60B1079  movss       xmm0,dword ptr [rax]  
//00007FF6C60B107D  movss       dword ptr [rsp+1E0h],xmm0  
//00007FF6C60B1086  mov         rax,qword ptr [this]  
//00007FF6C60B108E  add         rax,0Ch  
//00007FF6C60B1092  mov         rcx,rax  
//00007FF6C60B1095  call        vec3::y (07FF6C5F9C07Ch)  
//00007FF6C60B109A  movss       xmm0,dword ptr [rax]  
//00007FF6C60B109E  movss       dword ptr [rsp+1E4h],xmm0  
//00007FF6C60B10A7  mov         rax,qword ptr [this]  
//00007FF6C60B10AF  mov         rcx,rax  
//00007FF6C60B10B2  call        vec3::x (07FF6C5F9F51Ah)  
//00007FF6C60B10B7  movss       xmm0,dword ptr [rax]  
//00007FF6C60B10BB  movss       dword ptr [rsp+1E8h],xmm0  
//00007FF6C60B10C4  movss       xmm0,dword ptr [__real@3f800000 (07FF6C62872C0h)]  
//00007FF6C60B10CC  movss       dword ptr [rsp+20h],xmm0  
//00007FF6C60B10D2  movss       xmm3,dword ptr [rsp+1E0h]  
//00007FF6C60B10DB  movss       xmm2,dword ptr [rsp+1E4h]  
//00007FF6C60B10E4  movss       xmm1,dword ptr [rsp+1E8h]  
//00007FF6C60B10ED  lea         rcx,[rsp+188h]  
//00007FF6C60B10F5  call        DirectX::XMFLOAT4::XMFLOAT4 (07FF6C5F9A3BCh)  
//00007FF6C60B10FA  mov         rcx,rax  
//00007FF6C60B10FD  call        DirectX::XMLoadFloat3 (07FF6C5F94BBFh)  
//00007FF6C60B1102  movaps      xmmword ptr [rsp+110h],xmm0  
//00007FF6C60B110A  movaps      xmm0,xmmword ptr [rsp+110h]  
//00007FF6C60B1112  movaps      xmmword ptr [rsp+80h],xmm0  
//
//		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->low.y(), this->hi.z() , 1.0f))
//00007FF6C60B111A  mov         rax,qword ptr [this]  
//00007FF6C60B1122  add         rax,0Ch  
//00007FF6C60B1126  mov         rcx,rax  
//00007FF6C60B1129  call        vec3::z (07FF6C5F96B45h)  
//00007FF6C60B112E  movss       xmm0,dword ptr [rax]  
//00007FF6C60B1132  movss       dword ptr [rsp+1ECh],xmm0  
//00007FF6C60B113B  mov         rax,qword ptr [this]  
//00007FF6C60B1143  mov         rcx,rax  
//00007FF6C60B1146  call        vec3::y (07FF6C5F9C07Ch)  
//00007FF6C60B114B  movss       xmm0,dword ptr [rax]  
//00007FF6C60B114F  movss       dword ptr [rsp+1F0h],xmm0  
//00007FF6C60B1158  mov         rax,qword ptr [this]  
//
//		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->low.y(), this->hi.z() , 1.0f))
//00007FF6C60B1160  mov         rcx,rax  
//00007FF6C60B1163  call        vec3::x (07FF6C5F9F51Ah)  
//00007FF6C60B1168  movss       xmm0,dword ptr [rax]  
//00007FF6C60B116C  movss       dword ptr [rsp+1F4h],xmm0  
//00007FF6C60B1175  movss       xmm0,dword ptr [__real@3f800000 (07FF6C62872C0h)]  
//00007FF6C60B117D  movss       dword ptr [rsp+20h],xmm0  
//00007FF6C60B1183  movss       xmm3,dword ptr [rsp+1ECh]  
//00007FF6C60B118C  movss       xmm2,dword ptr [rsp+1F0h]  
//00007FF6C60B1195  movss       xmm1,dword ptr [rsp+1F4h]  
//00007FF6C60B119E  lea         rcx,[rsp+198h]  
//00007FF6C60B11A6  call        DirectX::XMFLOAT4::XMFLOAT4 (07FF6C5F9A3BCh)  
//00007FF6C60B11AB  mov         rcx,rax  
//00007FF6C60B11AE  call        DirectX::XMLoadFloat3 (07FF6C5F94BBFh)  
//00007FF6C60B11B3  movaps      xmmword ptr [rsp+120h],xmm0  
//00007FF6C60B11BB  movaps      xmm0,xmmword ptr [rsp+120h]  
//00007FF6C60B11C3  movaps      xmmword ptr [rsp+90h],xmm0  
//		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->low.y(), this->hi.z() , 1.0f))
//00007FF6C60B11CB  mov         rax,qword ptr [this]  
//00007FF6C60B11D3  add         rax,0Ch  
//00007FF6C60B11D7  mov         rcx,rax   
//00007FF6C60B11DA  call        vec3::z (07FF6C5F96B45h)  
//00007FF6C60B11DF  movss       xmm0,dword ptr [rax]  
//00007FF6C60B11E3  movss       dword ptr [rsp+1F8h],xmm0  
//00007FF6C60B11EC  mov         rax,qword ptr [this]  
//00007FF6C60B11F4  mov         rcx,rax  
//00007FF6C60B11F7  call        vec3::y (07FF6C5F9C07Ch)  
//00007FF6C60B11FC  movss       xmm0,dword ptr [rax]  
//00007FF6C60B1200  movss       dword ptr [rsp+1FCh],xmm0  
//00007FF6C60B1209  mov         rax,qword ptr [this]  
//00007FF6C60B1211  add         rax,0Ch  
//00007FF6C60B1215  mov         rcx,rax  
//00007FF6C60B1218  call        vec3::x (07FF6C5F9F51Ah)  
//00007FF6C60B121D  movss       xmm0,dword ptr [rax]  
//00007FF6C60B1221  movss       dword ptr [rsp+200h],xmm0  
//00007FF6C60B122A  movss       xmm0,dword ptr [__real@3f800000 (07FF6C62872C0h)]  
//00007FF6C60B1232  movss       dword ptr [rsp+20h],xmm0  
//00007FF6C60B1238  movss       xmm3,dword ptr [rsp+1F8h]  
//00007FF6C60B1241  movss       xmm2,dword ptr [rsp+1FCh]  
//00007FF6C60B124A  movss       xmm1,dword ptr [rsp+200h]  
//00007FF6C60B1253  lea         rcx,[rsp+1A8h]  
//00007FF6C60B125B  call        DirectX::XMFLOAT4::XMFLOAT4 (07FF6C5F9A3BCh)  
//00007FF6C60B1260  mov         rcx,rax  
//00007FF6C60B1263  call        DirectX::XMLoadFloat3 (07FF6C5F94BBFh)  
//00007FF6C60B1268  movaps      xmmword ptr [rsp+130h],xmm0  
//00007FF6C60B1270  movaps      xmm0,xmmword ptr [rsp+130h]  
//00007FF6C60B1278  movaps      xmmword ptr [rsp+0A0h],xmm0  
//		, XMLoadFloat3(&h3)
//00007FF6C60B1280  mov         rcx,qword ptr [h3]  
//00007FF6C60B1285  call        GameObject::GetMeshBBs (07FF6C5F999B7h)  
//00007FF6C60B128A  movaps      xmmword ptr [rsp+140h],xmm0  
//00007FF6C60B1292  movaps      xmm0,xmmword ptr [rsp+140h]  
//00007FF6C60B129A  movaps      xmmword ptr [rsp+0B0h],xmm0  
//		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->hi.y() , this->hi.z() , 1.0f))
//00007FF6C60B12A2  mov         rax,qword ptr [this]  
//00007FF6C60B12AA  add         rax,0Ch  
//00007FF6C60B12AE  mov         rcx,rax  
//00007FF6C60B12B1  call        vec3::z (07FF6C5F96B45h)  
//00007FF6C60B12B6  movss       xmm0,dword ptr [rax]  
//00007FF6C60B12BA  movss       dword ptr [rsp+204h],xmm0  
//00007FF6C60B12C3  mov         rax,qword ptr [this]  
//00007FF6C60B12CB  add         rax,0Ch  
//00007FF6C60B12CF  mov         rcx,rax  
//00007FF6C60B12D2  call        vec3::y (07FF6C5F9C07Ch)  
//00007FF6C60B12D7  movss       xmm0,dword ptr [rax]  
//00007FF6C60B12DB  movss       dword ptr [rsp+208h],xmm0  
//00007FF6C60B12E4  mov         rax,qword ptr [this]  
//00007FF6C60B12EC  mov         rcx,rax  
//00007FF6C60B12EF  call        vec3::x (07FF6C5F9F51Ah)  
//00007FF6C60B12F4  movss       xmm0,dword ptr [rax]  
//00007FF6C60B12F8  movss       dword ptr [rsp+20Ch],xmm0  
//00007FF6C60B1301  movss       xmm0,dword ptr [__real@3f800000 (07FF6C62872C0h)]  
//00007FF6C60B1309  movss       dword ptr [rsp+20h],xmm0  
//00007FF6C60B130F  movss       xmm3,dword ptr [rsp+204h]  
//00007FF6C60B1318  movss       xmm2,dword ptr [rsp+208h]  
//00007FF6C60B1321  movss       xmm1,dword ptr [rsp+20Ch]  
//00007FF6C60B132A  lea         rcx,[rsp+1B8h]  
//00007FF6C60B1332  call        DirectX::XMFLOAT4::XMFLOAT4 (07FF6C5F9A3BCh)  
//00007FF6C60B1337  mov         rcx,rax  
//00007FF6C60B133A  call        DirectX::XMLoadFloat3 (07FF6C5F94BBFh)  
//00007FF6C60B133F  movaps      xmmword ptr [rsp+150h],xmm0  
//00007FF6C60B1347  movaps      xmm0,xmmword ptr [rsp+150h]  
//00007FF6C60B134F  movaps      xmmword ptr [rsp+0C0h],xmm0  
//	};



	arr[0].m128_f32[3] = 1.0f;
	arr[6].m128_f32[3] = 1.0f;

//	arr[0].m128_f32[3] = 1.0f;
//00007FF6C60B1357  xor         edx,edx  
//00007FF6C60B1359  lea         rcx,[arr]  
//00007FF6C60B135E  call        std::array<__m128,8>::operator[] (07FF6C5F969C9h)  
//00007FF6C60B1363  mov         ecx,4  
//00007FF6C60B1368  imul        rcx,rcx,3  
//00007FF6C60B136C  movss       xmm0,dword ptr [__real@3f800000 (07FF6C62872C0h)]  
//00007FF6C60B1374  movss       dword ptr [rax+rcx],xmm0  
//	arr[6].m128_f32[3] = 1.0f;
//00007FF6C60B1379  mov         edx,6  
//00007FF6C60B137E  lea         rcx,[arr]  
//00007FF6C60B1383  call        std::array<__m128,8>::operator[] (07FF6C5F969C9h)  
//00007FF6C60B1388  mov         ecx,4  
//00007FF6C60B138D  imul        rcx,rcx,3  
//00007FF6C60B1391  movss       xmm0,dword ptr [__real@3f800000 (07FF6C62872C0h)]  
//00007FF6C60B1399  movss       dword ptr [rax+rcx],xmm0  


	return arr;
}


std::array<XMVECTOR, 8> BoundingBox::GetCornerPointsV4_1() const
{
	const XMFLOAT3& l3 = this->low._v;
	const XMFLOAT3& h3 = this->hi._v;

	std::array<XMVECTOR, 8> arr =
	{
		  XMLoadFloat3(&l3)
		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->low.y(), this->low.z(), 1.0f))
		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->hi.y() , this->low.z(), 1.0f))
		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->hi.y() , this->low.z(), 1.0f))

		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->low.y(), this->hi.z() , 1.0f))
		, XMLoadFloat4(&XMFLOAT4(this->hi.x() , this->low.y(), this->hi.z() , 1.0f))
		, XMLoadFloat3(&h3)
		, XMLoadFloat4(&XMFLOAT4(this->low.x(), this->hi.y() , this->hi.z() , 1.0f))
	};

	arr[0] = XMVectorSetW(arr[0], 1.0f);
	arr[6] = XMVectorSetW(arr[6], 1.0f);
	
//	arr[0] = XMVectorSetW(arr[0], 1.0f);
//00007FF657D218E7  xor         edx,edx  
//00007FF657D218E9  lea         rcx,[arr]  
//00007FF657D218EE  call        std::array<__m128,8>::operator[] (07FF657C069D3h)  
//00007FF657D218F3  movss       xmm1,dword ptr [__real@3f800000 (07FF657EF82C0h)]  
//00007FF657D218FB  movups      xmm0,xmmword ptr [rax]  
//00007FF657D218FE  call        DirectX::XMVectorSetW (07FF657C073F6h)  
							//				    // Swap w and x
							//    XMVECTOR vResult = XM_PERMUTE_PS(V,_MM_SHUFFLE(0,2,1,3));
							//00007FF78F1F438F  movaps      xmm0,xmmword ptr [rsp+10h]  
							//00007FF78F1F4394  shufps      xmm0,xmmword ptr [rsp+10h],27h  
							//00007FF78F1F439A  movaps      xmmword ptr [rsp+30h],xmm0  
							//00007FF78F1F439F  movaps      xmm0,xmmword ptr [rsp+30h]  
							//00007FF78F1F43A4  movaps      xmmword ptr [vResult],xmm0  
							//    // Convert input to vector
							//    XMVECTOR vTemp = _mm_set_ss(w);
							//00007FF78F1F43A9  movss       xmm0,dword ptr [w]  
							//00007FF78F1F43B2  xorps       xmm1,xmm1  
							//00007FF78F1F43B5  movss       xmm1,xmm0  
							//00007FF78F1F43B9  movaps      xmm0,xmm1  
							//00007FF78F1F43BC  movaps      xmmword ptr [rsp+50h],xmm0  
							//00007FF78F1F43C1  movaps      xmm0,xmmword ptr [rsp+50h]  
							//00007FF78F1F43C6  movaps      xmmword ptr [vTemp],xmm0  
							//    // Replace the x component
							//    vResult = _mm_move_ss(vResult,vTemp);
							//00007FF78F1F43CB  movaps      xmm0,xmmword ptr [vResult]  
							//00007FF78F1F43D0  movaps      xmm1,xmmword ptr [vTemp]  
							//00007FF78F1F43D5  movss       xmm0,xmm1  
							//00007FF78F1F43D9  movaps      xmmword ptr [rsp+60h],xmm0  
							//00007FF78F1F43DE  movaps      xmm0,xmmword ptr [rsp+60h]  
							//00007FF78F1F43E3  movaps      xmmword ptr [vResult],xmm0  
							//    // Swap w and x again
							//    vResult = XM_PERMUTE_PS(vResult,_MM_SHUFFLE(0,2,1,3));
							//00007FF78F1F43E8  movaps      xmm0,xmmword ptr [vResult]  
							//00007FF78F1F43ED  shufps      xmm0,xmmword ptr [vResult],27h  
							//00007FF78F1F43F3  movaps      xmmword ptr [rsp+70h],xmm0  
							//00007FF78F1F43F8  movaps      xmm0,xmmword ptr [rsp+70h]  
							//00007FF78F1F43FD  movaps      xmmword ptr [vResult],xmm0  
							//    return vResult;
							//00007FF78F1F4402  movaps      xmm0,xmmword ptr [vResult]  
//00007FF657D21903  movaps      xmmword ptr [rsp+160h],xmm0  
//00007FF657D2190B  xor         edx,edx  
//00007FF657D2190D  lea         rcx,[arr]  
//00007FF657D21912  call        std::array<__m128,8>::operator[] (07FF657C069D3h)  
//00007FF657D21917  movaps      xmm0,xmmword ptr [rsp+160h]  
//00007FF657D2191F  movups      xmmword ptr [rax],xmm0  
//	arr[6] = XMVectorSetW(arr[6], 1.0f);
//00007FF657D21922  mov         edx,6  
//00007FF657D21927  lea         rcx,[arr]  
//00007FF657D2192C  call        std::array<__m128,8>::operator[] (07FF657C069D3h)  
//00007FF657D21931  movss       xmm1,dword ptr [__real@3f800000 (07FF657EF82C0h)]  
//00007FF657D21939  movups      xmm0,xmmword ptr [rax]  
//00007FF657D2193C  call        DirectX::XMVectorSetW (07FF657C073F6h)  
//00007FF657D21941  movaps      xmmword ptr [rsp+170h],xmm0  
//00007FF657D21949  mov         edx,6  
//00007FF657D2194E  lea         rcx,[arr]  
//00007FF657D21953  call        std::array<__m128,8>::operator[] (07FF657C069D3h)  
//00007FF657D21958  movaps      xmm0,xmmword ptr [rsp+170h]  
//00007FF657D21960  movups      xmmword ptr [rax],xmm0  

	return arr;
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
