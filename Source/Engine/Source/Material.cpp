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

#include "Material.h"
#include "Renderer/Renderer.h"

#include "Utilities/Log.h"

const BlinnPhong_Material BlinnPhong_Material::ruby = BlinnPhong_Material(
	MaterialID{ -1 },
	vec3(0.61424f, 0.04136f, 0.04136f),		// diffuse
	vec3(0.727811f, 0.626959f, 0.626959f),	// specular
	76.8f									// shininess
);


const BlinnPhong_Material BlinnPhong_Material::jade = BlinnPhong_Material(
	MaterialID{ -1 },
	vec3(0.54f, 0.89f, 0.63f),				// diffuse
	vec3(0.316228f, 0.316228f, 0.316228f),	// specular
	12.8f									// shininess
);


const BlinnPhong_Material BlinnPhong_Material::bronze = BlinnPhong_Material(
	MaterialID{ -1 },
	vec3(0.714f, 0.4284f, 0.18144f),		// diffuse
	vec3(0.393548f, 0.271906f, 0.166721f),	// specular
	25.6f									// shininess
);


const BlinnPhong_Material BlinnPhong_Material::gold = BlinnPhong_Material(
	MaterialID{ -1 },
	vec3(0.75164f, 0.60648f, 0.22648f),		// diffuse
	vec3(0.628281f, 0.555802f, 0.366065f),	// specular
	51.2f									// shininess
);

MaterialID GenerateMaterialID(EMaterialType type, size_t idx)
{
#ifdef _DEBUG
	// todo: debug assert
	//assert((1ULL << unsigned long long(type) * 16ULL) + static_cast<unsigned long long>(idx) < max(int)); 
#endif
	if (type == EMaterialType::GGX_BRDF) return MaterialID{ static_cast<int>(idx) };
	else return MaterialID{ (1 << (int(type) * 16)) + static_cast<int>(idx) };
}
constexpr int TYPE_MASK = (1 << 16);
size_t GetBufferIndex(MaterialID id)
{
	//								 ^
	// this means 8 bit per material instance - 256 different materials supported.

	return (id.ID & TYPE_MASK) ? (id.ID ^ TYPE_MASK) : id.ID;
	// try this to see if it makes speed difference:
	// return id.ID & ( (id.ID & TYPE_MASK) ? (~TYPE_MASK) : 0xFFFFFFFF);

#if 0	// readable version
	if (id.ID & TYPE_MASK)
		return id.ID & (~TYPE_MASK);
	else
		return id.ID;
#endif
}
EMaterialType GetMaterialType(MaterialID matID)
{
	//if(matID.ID & TYPE_MASK)
	//return EMaterialType::MATERIAL_TYPE_COUNT;
	return matID.ID & TYPE_MASK ? EMaterialType::BLINN_PHONG : EMaterialType::GGX_BRDF;
}


BlinnPhong_Material MaterialBuffer::RandomBlinnPhongMaterial(MaterialID matID)
{
	assert(false);	// TODO: fix preset blinn-phong material IDs
	float r = RandF(0.0f, 1.0f);
	if (r >= 0.0000f && r < 0.1999f) return BlinnPhong_Material::ruby;
	if (r >= 0.2000f && r < 0.3999f) return BlinnPhong_Material::gold;
	if (r >= 0.4000f && r < 0.5999f) return BlinnPhong_Material::bronze;
	if (r >= 0.6000f && r < 0.7999f) return BlinnPhong_Material::jade;
	else return BlinnPhong_Material(matID);
}
BRDF_Material MaterialBuffer::RandomBRDFMaterial(MaterialID matID)
{
	BRDF_Material m = BRDF_Material(matID);
	if (RandI(0, 100) < 50)
	{	// PLASTIC
		m.metalness = RandF(0.0f, 0.15f);
		m.roughness = RandF(0.44f, 0.99f);
	}
	else
	{	// METAL
		m.metalness = RandF(0.7f, 1.0f);
		m.roughness = RandF(0.04f, 0.7f);
	}
	return m;
}
MaterialID MaterialBuffer::CreateRandomMaterial(EMaterialType type)
{
	long long newIndex = -1;
	MaterialID returnID = { -1 };
	switch (type)
	{
	case GGX_BRDF:
		newIndex = static_cast<long long>(BRDFs.size());
		BRDFs.push_back(RandomBRDFMaterial(GenerateMaterialID(type, newIndex)));
		returnID = BRDFs.back().ID;
		break;
	case BLINN_PHONG:
		newIndex = static_cast<long long>(Phongs.size());
		Phongs.push_back(RandomBlinnPhongMaterial(GenerateMaterialID(type, newIndex)));
		returnID = Phongs.back().ID;
		break;
	}
	return returnID;
}
MaterialID MaterialBuffer::CreateMaterial(EMaterialType type)
{
	long long newIndex = -1;
	MaterialID returnID = { -1 };
	switch (type)
	{
	case GGX_BRDF:
		newIndex = static_cast<long long>(BRDFs.size());
		BRDFs.push_back(BRDF_Material(GenerateMaterialID(type, newIndex)));
		returnID = BRDFs.back().ID;
		break;
	case BLINN_PHONG:
		newIndex = static_cast<long long>(Phongs.size());
		Phongs.push_back(BlinnPhong_Material(GenerateMaterialID(type, newIndex)));
		returnID = Phongs.back().ID;
		break;
	default:
		Log::Error("Unknown material type: %d", type);
		break;
	}
	return returnID;
}

Material* MaterialBuffer::GetMaterial(MaterialID matID)
{
	const size_t bufferIndex = GetBufferIndex(matID);
	const EMaterialType type = GetMaterialType(matID);

	Material* ptr(nullptr);
	switch (type)
	{
	case GGX_BRDF:
		ptr = &BRDFs[bufferIndex];
		break;
	case BLINN_PHONG:
		ptr = &Phongs[bufferIndex];
		break;
	default:
		Log::Error("Incorrect material type: %d from material ID: %d", type, matID.ID);
		break;
	}
	return ptr;
}
const Material* MaterialBuffer::GetMaterial_const(MaterialID matID) const
{
	const size_t bufferIndex = GetBufferIndex(matID);
	const EMaterialType type = GetMaterialType(matID);

	const Material* ptr(nullptr);
	switch (type)
	{
	case GGX_BRDF:
		ptr = &BRDFs[bufferIndex];
		break;
	case BLINN_PHONG:
		break;
		ptr = &Phongs[bufferIndex];
	default:
		Log::Error("Incorrect material type: %d from material ID: %d", type, matID.ID);
		break;
	}
	return ptr;
}
BlinnPhong_Material::BlinnPhong_Material(MaterialID _ID, const vec3 & diffuse_in, const vec3 & specular_in, float shininess_in)
	:
	Material(_ID)
{
	diffuse = diffuse_in;
	specular = specular_in;
	shininess = shininess_in;
}

Material::Material(MaterialID _ID)
	:
	diffuse(LinearColor::white),
	alpha(1.0f),
	specular(LinearColor::white.Value()),
	tiling(1, 1),
	diffuseMap(-1),
	normalMap(-1),
	ID(_ID)
{}

Material::~Material() {}


struct SurfaceMaterial
{
	vec3  diffuse;
	float alpha;

	vec3  specular;
	float roughness;

	// todo: remove is*Map after shader permutation is implemented
	float isDiffuseMap;
	float isNormalMap;

	float metalness;
	float shininess;
	vec2 tiling;
};

void Material::SetMaterialConstants(Renderer * renderer, EShaders shader, bool bIsDeferredRendering) const
{
	switch (shader)
	{
	case EShaders::NORMAL:
	case EShaders::Z_PREPRASS:
		renderer->SetConstant2f("uvScale", tiling);
		renderer->SetConstant1f("isNormalMap", normalMap == -1 ? 0.0f : 1.0f);
		if (normalMap != -1) renderer->SetTexture("texNormalMap", normalMap);
		break;
	case EShaders::UNLIT:
		renderer->SetConstant3f("diffuse", diffuse);
		renderer->SetConstant1f("isDiffuseMap", diffuseMap == -1 ? 0.0f : 1.0f);
		if (diffuseMap != -1) renderer->SetTexture("texDiffuseMap", diffuseMap);
		break;
	default:
		if (diffuseMap >= 0) renderer->SetTexture("texDiffuseMap", diffuseMap);
		if (normalMap >= 0) renderer->SetTexture("texNormalMap", normalMap);
		break;
	}

	SetMaterialSpecificConstants(renderer, shader, bIsDeferredRendering);
}

void BRDF_Material::Clear() 
{
	diffuse = vec3(1,1,1);
	metalness = 0.0f;
	specular = vec3(1,1,1);
	roughness = 0.04f;
	diffuseMap = -1;
	normalMap = -1;
	alpha = 1.0f;
}

void BlinnPhong_Material::Clear()
{
	diffuse = vec3(1,1,1);
	shininess = 0.0f;
	specular = vec3(1,1,1);
	diffuseMap = -1;
	normalMap = -1;
	alpha = 1.0f;
}

void BRDF_Material::SetMaterialSpecificConstants(Renderer* renderer, EShaders shader, bool bIsDeferredRendering) const
{
	switch (shader)
	{
	case EShaders::NORMAL:
	case EShaders::Z_PREPRASS:
		renderer->SetConstant2f("uvScale", tiling);
		break;
	case EShaders::UNLIT:
		break;
	default:
		SurfaceMaterial mat{
			diffuse.Value(),
			alpha,

			specular,
			roughness,

			diffuseMap == -1 ? 0.0f : 1.0f,
			normalMap == -1 ? 0.0f : 1.0f,
			metalness,
			0.0f,
			tiling
		};
		renderer->SetConstantStruct("surfaceMaterial", &mat);

		if (bIsDeferredRendering)
		{
			renderer->SetConstant1f("BRDFOrPhong", 1.0f);
		}
		else
		{

		}

		break;
	}
}



void BlinnPhong_Material::SetMaterialSpecificConstants(Renderer* renderer, EShaders shader, bool bIsDeferredRendering) const
{
	switch (shader)
	{
	case EShaders::NORMAL:
	case EShaders::Z_PREPRASS:
	case EShaders::UNLIT:
		break;

	default:
		SurfaceMaterial mat{
			diffuse.Value(),
			alpha,

			specular,
			0.0f,	// brdf roughness

			diffuseMap == -1 ? 0.0f : 1.0f,
			normalMap == -1 ? 0.0f : 1.0f,
			0.0f,	// brdf metalness
			shininess,
			tiling
		};
		renderer->SetConstantStruct("surfaceMaterial", &mat);
		if (bIsDeferredRendering)
		{
			renderer->SetConstant1f("BRDFOrPhong", 0.0f);
		}
		else
		{

		}
		break;
	}
}