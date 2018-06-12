//	VQEngine | DirectX11 Renderer
//	Copyright(C) 2016  - Volkan Ilbeyli
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

#include "Utilities/Color.h"
#include "Application/HandleTypedefs.h"
#include "Renderer/RenderingEnums.h"

class Renderer;

struct MaterialID
{
	int ID;
	EMaterialType GetType() const;
	
};

struct BlinnPhong_Material;
struct BRDF_Material;


struct Material				// 56 Bytes
{
	MaterialID	ID;			// 4 Bytes
	LinearColor	diffuse;	// 12 Bytes
	float		alpha;		// 4 Bytes
	vec3		specular;	// 12 Bytes

	vec2		tiling;	// default=(1,1)

	TextureID	diffuseMap;
	TextureID	normalMap;

	Material* pNextAvailable = nullptr;	// making this class a pool object

	Material(MaterialID _ID);
	~Material();
	void SetMaterialConstants(Renderer* renderer, EShaders shader, bool bIsDeferredRendering) const;
	virtual void SetMaterialSpecificConstants(Renderer* renderer, EShaders shader, bool bIsDeferredRendering) const = 0;
	virtual void Clear() = 0;
};

struct BRDF_Material : public Material	
{	// Cook-Torrence BRDF
	float		metalness;
	float		roughness;
	
	BRDF_Material() : Material({ -1 }), metalness(0.0f), roughness(0.0f) {}
	void SetMaterialSpecificConstants(Renderer* renderer, EShaders shader, bool bIsDeferredRendering) const override;
	void Clear() override;

private:
	friend class MaterialPool;	// only MaterialPool can create Material instances
	BRDF_Material(MaterialID _ID) : Material(_ID), metalness(0.1f), roughness(0.6f) {}
};

struct BlinnPhong_Material : public Material
{
	float		shininess;

	BlinnPhong_Material() : Material(MaterialID{ -1 }), shininess(0) {}
	void SetMaterialSpecificConstants(Renderer* renderer, EShaders shader, bool bIsDeferredRendering) const override;
	void Clear() override;

	static const BlinnPhong_Material jade, ruby, bronze, gold;	// todo: handle preset materials in scene
private:
	friend class MaterialPool;	// only MaterialPool can create Material instances
	BlinnPhong_Material(MaterialID _ID) : Material(_ID), shininess(90) {}
	BlinnPhong_Material(MaterialID _ID, const vec3& diffuse, const vec3& specular, float shininess);
};


// Each new type of material currently requires its own Create/Get/Random/pNextAvailable/vector<>
// This is quite inflexible at this moment, however, this was done in favor of data oriented design.
//
// MaterialPool can be refactored later using some macro/template functionality.
//
struct PoolStats
{
	size_t mObjectsFree = 0;
};

class MaterialPool
{
public:
	MaterialPool() = default;
	MaterialPool(MaterialPool&& other);
	MaterialPool& operator=(MaterialPool&& other);
	void Initialize(size_t poolSize);
	void Clear();

	MaterialID CreateMaterial(EMaterialType type);
	MaterialID CreateRandomMaterial(EMaterialType type);

	Material* GetMaterial(MaterialID matID);
	const Material* GetMaterial_const(MaterialID matID) const;
	const Material* GetDefaultMaterial(EMaterialType type) const;

	inline Material* CreateAndGetMaterial(EMaterialType type) { return GetMaterial(CreateMaterial(type)); };
	inline Material* CreateAndGetRandomMaterial(EMaterialType type) { return GetMaterial(CreateRandomMaterial(type)); };

private:
	static BRDF_Material RandomBRDFMaterial(MaterialID matID);
	static BlinnPhong_Material RandomBlinnPhongMaterial(MaterialID matID);

private:
	std::vector<BRDF_Material>			BRDFs;
	std::vector<BlinnPhong_Material>	Phongs;

	BRDF_Material* pNextAvailableBRDF = nullptr;
	BlinnPhong_Material* pNextAvailablePhong = nullptr;

	PoolStats mStatsBRDF;
	PoolStats mStatsPhong;

private:
	template<class T>
	void InitializePool(std::vector<T>& pool, T*& pNextObject, const size_t poolSz, EMaterialType type)
	{
		pool.resize(poolSz);
		pNextObject = &pool[0];
		for (size_t i = 0; i < pool.size() - 1; ++i)
		{
			pool[i].pNextAvailable = &pool[i + 1];
			pool[i].ID = GenerateMaterialID(type, i);
		}
		pool.back().ID = GenerateMaterialID(type, pool.size() - 1);
		pool.back().pNextAvailable = nullptr;
	}
	template<class T>
	void mv(std::vector<T>& vTarget, std::vector<T>& vSource, T*& pNext, T* pNextOther)
	{
		std::move(vSource.begin(), vSource.end(), vTarget.begin());
		pNext = &vTarget[GetBufferIndex(pNextOther->ID)];
		for (size_t i = 1; i < vTarget.size(); ++i)
		{
			vTarget[i - 1].pNextAvailable = &vTarget[i];
		}
	}
};

constexpr size_t SZ_MATERIAL = sizeof(Material);
constexpr size_t SZ_MATERIAL_BRDF = sizeof(BRDF_Material);
constexpr size_t SZ_MATERIAL_PHONG = sizeof(BlinnPhong_Material);
constexpr size_t SZ_MATERIAL_BUFFER = sizeof(MaterialPool);
constexpr size_t SZ_MATERIAL_ID = sizeof(MaterialID);
constexpr size_t SZ_LINEAR_COLOR = sizeof(LinearColor);
