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
	
	void SetMaterialSpecificConstants(Renderer* renderer, EShaders shader, bool bIsDeferredRendering) const override;
	void Clear() override;

	friend class MaterialBuffer;	// only MaterialBuffer can create Material instances
private:
	BRDF_Material(MaterialID _ID) : Material(_ID), metalness(0.1f), roughness(0.6f) {}
};

struct BlinnPhong_Material : public Material
{
	static const BlinnPhong_Material jade, ruby, bronze, gold;	// todo: handle preset materials in scene

	float		shininess;
	
	void SetMaterialSpecificConstants(Renderer* renderer, EShaders shader, bool bIsDeferredRendering) const override;
	void Clear() override;

	friend class MaterialBuffer;	// only MaterialBuffer can create Material instances
private:
	BlinnPhong_Material(MaterialID _ID) : Material(_ID), shininess(90) {}
	BlinnPhong_Material(MaterialID _ID, const vec3& diffuse, const vec3& specular, float shininess);
};



class MaterialBuffer
{
public:
	MaterialID CreateMaterial(EMaterialType type);
	MaterialID CreateRandomMaterial(EMaterialType type);

	Material* GetMaterial(MaterialID matID);
	const Material* GetMaterial_const(MaterialID matID) const;

	inline Material* CreateAndGetMaterial(EMaterialType type) { return GetMaterial(CreateMaterial(type)); };
	inline Material* CreateAndGetRandomMaterial(EMaterialType type) { return GetMaterial(CreateRandomMaterial(type)); };

private:
	static BRDF_Material RandomBRDFMaterial(MaterialID matID);
	static BlinnPhong_Material RandomBlinnPhongMaterial(MaterialID matID);

private:
	std::vector<BRDF_Material>			BRDFs;
	std::vector<BlinnPhong_Material>	Phongs;
};

constexpr size_t SZ_MATERIAL = sizeof(Material);
constexpr size_t SZ_MATERIAL_BRDF = sizeof(BRDF_Material);
constexpr size_t SZ_MATERIAL_PHONG = sizeof(BlinnPhong_Material);
constexpr size_t SZ_MATERIAL_BUFFER = sizeof(MaterialBuffer);
constexpr size_t SZ_MATERIAL_ID = sizeof(MaterialID);
constexpr size_t SZ_LINEAR_COLOR = sizeof(LinearColor);
