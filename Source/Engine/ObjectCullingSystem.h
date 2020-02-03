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

#include <vector>

#include "Application/HandleTypedefs.h"

#include "Utilities/vectormath.h"

struct FrustumPlaneset;
struct vec3;
struct MeshInstanceTransformationLookup;

class GameObject;


struct BoundingBox
{
	vec3 low = vec3::Zero;
	vec3 hi = vec3::Zero;
	DirectX::XMMATRIX GetWorldTransformationMatrix() const;
	std::array<vec4, 8> GetCornerPointsV4() const;
	std::array<XMVECTOR, 8> GetCornerPointsV4_0() const;
	std::array<XMVECTOR, 8> GetCornerPointsV4_1() const;
	std::array<vec3, 8> GetCornerPointsV3() const;
};

struct Sphere
{
	Sphere(const vec3& _center, float _radius) : center(_center), radius(_radius) {}
	union
	{
		struct
		{
			vec3 center;
			float radius;
		};
		struct
		{
			vec3 c;
			float r;
		};
	};
};

struct CullMeshData
{
	FrustumPlaneset frustumPlanes;
	XMMATRIX matWorld; // rename to matTransform to be more generic?
	const std::vector<BoundingBox>* pLocalSpaceBoundingBoxes;

	const std::vector<MeshID>* pMeshIDs;
	MeshInstanceTransformationLookup* pMeshDrawData;
};

namespace VQEngine
{
	bool IsSphereInFrustum(const FrustumPlaneset& frustum, const Sphere& sphere);

	bool IsBoundingBoxVisibleFromFrustum(const FrustumPlaneset& frustum, const BoundingBox& aabb);

	bool IsBoundingBoxInsideSphere_Approx(const BoundingBox& aabb, const Sphere& sphere);

	bool IsIntersecting(const Sphere& s1, const Sphere& s2);

	// deprecated
	size_t CullMeshes(const FrustumPlaneset& frustumPlanes, const GameObject* pObj, MeshInstanceTransformationLookup& meshDrawData);


	// returns the indices of visible bounding boxes
	//
	std::vector<int> CullMeshes(CullMeshData& data);

	size_t CullGameObjects
	(
		const FrustumPlaneset&                  frustumPlanes
		, const std::vector<const GameObject*>& pObjs
		, std::vector<const GameObject*>&       pCulledObjs
	);
}