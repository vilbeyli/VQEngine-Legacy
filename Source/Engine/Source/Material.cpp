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


BlinnPhong_Material MaterialPool::RandomBlinnPhongMaterial(MaterialID matID)
{
	assert(false);	// TODO: fix preset blinn-phong material IDs
	float r = MathUtil::RandF(0.0f, 1.0f);
	if (r >= 0.0000f && r < 0.1999f) return BlinnPhong_Material::ruby;
	if (r >= 0.2000f && r < 0.3999f) return BlinnPhong_Material::gold;
	if (r >= 0.4000f && r < 0.5999f) return BlinnPhong_Material::bronze;
	if (r >= 0.6000f && r < 0.7999f) return BlinnPhong_Material::jade;
	else return BlinnPhong_Material(matID);
}
BRDF_Material MaterialPool::RandomBRDFMaterial(MaterialID matID)
{
	BRDF_Material m = BRDF_Material(matID);
	if (MathUtil::RandI(0, 100) < 50)
	{	// PLASTIC
		m.metalness = MathUtil::RandF(0.0f, 0.15f);
		m.roughness = MathUtil::RandF(0.44f, 0.99f);
	}
	else
	{	// METAL
		m.metalness = MathUtil::RandF(0.7f, 1.0f);
		m.roughness = MathUtil::RandF(0.04f, 0.7f);
	}
	return m;
}


MaterialPool::MaterialPool(MaterialPool && other)
{
	BRDFs = std::move(other.BRDFs);
	Phongs = std::move(other.Phongs);
	pNextAvailableBRDF = other.pNextAvailableBRDF;
	pNextAvailablePhong = other.pNextAvailablePhong;
}


MaterialPool& MaterialPool::operator=(MaterialPool&& other)
{
	mv(BRDFs, other.BRDFs, pNextAvailableBRDF, other.pNextAvailableBRDF);
	mv(Phongs, other.Phongs, pNextAvailablePhong, other.pNextAvailablePhong);
	return *this;
}

void MaterialPool::Initialize(size_t poolSize)
{
	InitializePool(BRDFs, pNextAvailableBRDF, poolSize, EMaterialType::GGX_BRDF);
	CreateMaterial(EMaterialType::GGX_BRDF);
	mStatsBRDF.mObjectsFree = poolSize - 1;

	InitializePool(Phongs, pNextAvailablePhong, poolSize, EMaterialType::BLINN_PHONG);
	CreateMaterial(EMaterialType::BLINN_PHONG);
	mStatsPhong.mObjectsFree = poolSize - 1;
}

void MaterialPool::Clear()
{
	BRDFs.clear();
	Phongs.clear();
	mStatsPhong.mObjectsFree = 0;
	mStatsBRDF.mObjectsFree = 0;
	pNextAvailablePhong = nullptr;
	pNextAvailableBRDF = nullptr;
}

template<class T>
Material* Create(T*& pNext)
{
	assert(pNext);
	Material* pReturn = pNext;
	pNext = static_cast<T*>(pReturn->pNextAvailable);
	assert(pNext);
	assert(pNext->ID.ID > INVALID_MATERIAL_ID);
	return pReturn;
}

MaterialID MaterialPool::CreateMaterial(EMaterialType type)
{
	std::unique_lock<std::mutex> lock(mBufferMutex);
	MaterialID returnID = { INVALID_MATERIAL_ID };
	switch (type)
	{
	case GGX_BRDF:
		returnID = Create(pNextAvailableBRDF)->ID;
		--mStatsBRDF.mObjectsFree;
		break;
	case BLINN_PHONG:
		returnID = Create(pNextAvailablePhong)->ID;
		--mStatsPhong.mObjectsFree;
		break;
	default:
		Log::Error("Unknown material type: %d", type);
		break;
	}
	assert(returnID.ID > -1);
	return returnID;
}
MaterialID MaterialPool::CreateRandomMaterial(EMaterialType type)
{
	std::unique_lock<std::mutex> lock(mBufferMutex);
	Material* pNewRandomMaterial = nullptr;
	switch (type)
	{
	case GGX_BRDF:
	{
		pNewRandomMaterial = Create(pNextAvailableBRDF);
		BRDF_Material* pBRDF = static_cast<BRDF_Material*>(pNewRandomMaterial);
		*pBRDF = RandomBRDFMaterial(pNewRandomMaterial->ID);
	}
	break;
	case BLINN_PHONG:
	{
		pNewRandomMaterial = Create(pNextAvailablePhong);
		BlinnPhong_Material* pPhong = static_cast<BlinnPhong_Material*>(pNewRandomMaterial);
		*pPhong = RandomBlinnPhongMaterial(pNewRandomMaterial->ID);
	}
	break;
	}
	return pNewRandomMaterial->ID;
}

Material* MaterialPool::GetMaterial(MaterialID matID)
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
const Material* MaterialPool::GetMaterial_const(MaterialID matID) const
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
const Material * MaterialPool::GetDefaultMaterial(EMaterialType type) const
{
	const Material* pMat = nullptr;
	switch (type)
	{
	case GGX_BRDF:
		pMat = static_cast<const Material*>(&BRDFs[0]);
		break;
	case BLINN_PHONG:
		pMat = static_cast<const Material*>(&Phongs[0]);
		break;
	default:
		Log::Error("Incorrect material type: %d from material", type);
		break;
	}
	return pMat;
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
	
	: diffuse(LinearColor::white)
	, alpha(1.0f)
	, specular(LinearColor::white.Value())
	, tiling(1, 1)
	, emissiveColor(LinearColor::black)
	, emissiveIntensity(1.0f)

	, diffuseMap(INVALID_TEXTURE_ID)
	, normalMap(INVALID_TEXTURE_ID)
	, heightMap(INVALID_TEXTURE_ID)
	, specularMap(INVALID_TEXTURE_ID)
	, mask(INVALID_TEXTURE_ID)
	, roughnessMap(INVALID_TEXTURE_ID)
	, metallicMap(INVALID_TEXTURE_ID)
	, emissiveMap(INVALID_TEXTURE_ID)
	
	, ID(_ID)
{}

Material::~Material() {}


void Material::SetMaterialConstants(Renderer * renderer, EShaders shader, bool bIsDeferredRendering) const
{
	// todo: this function and material design needs to be redone.
	switch (shader)
	{
	case EShaders::NORMAL:
		renderer->SetConstant2f("uvScale", tiling);
		renderer->SetConstant1f("isNormalMap", normalMap == -1 ? 0.0f : 1.0f);
		if (normalMap != -1) renderer->SetTexture("texNormalMap", normalMap);
		if (mask >= 0)		renderer->SetTexture("texAlphaMask", mask);

		break;
	case EShaders::UNLIT:
		renderer->SetConstant3f("diffuse", diffuse);
		renderer->SetConstant1f("isDiffuseMap", diffuseMap == -1 ? 0.0f : 1.0f);
		if (diffuseMap != -1) renderer->SetTexture("texDiffuseMap", diffuseMap);
		break;
	default:
		if (diffuseMap >= 0)	renderer->SetTexture("texDiffuseMap", diffuseMap);
		if (normalMap >= 0)		renderer->SetTexture("texNormalMap", normalMap);
		if (specularMap >= 0)	renderer->SetTexture("texSpecularMap", specularMap);
		if (mask >= 0)			renderer->SetTexture("texAlphaMask", mask);
		if (metallicMap >= 0)	renderer->SetTexture("texMetallicMap", metallicMap);
		if (roughnessMap >= 0)	renderer->SetTexture("texRoughnessMap", roughnessMap);
		break;
	}


	switch (shader)
	{
	case EShaders::NORMAL:
		renderer->SetConstant2f("uvScale", tiling);
		renderer->SetConstant1i("textureConfig", GetTextureConfig());
		break;
	case EShaders::UNLIT:
		break;
	default:
		const SurfaceMaterial mat = GetShaderFriendlyStruct();
		
		renderer->SetConstantStruct("surfaceMaterial", &mat);
		if (bIsDeferredRendering)
		{
			renderer->SetConstant1f("BRDFOrPhong", 1.0f);	// assume brdf for now
			//renderer->SetConstant1f("BRDFOrPhong", 0.0f);
		}
		else
		{

		}
		break;
	}
}

bool Material::IsTransparent() const
{
	return alpha != 1.0f;
}

int Material::GetTextureConfig() const
{
	int textureConfig = 0;
	textureConfig |= diffuseMap == -1	? 0 : (1 << 0);
	textureConfig |= normalMap == -1	? 0 : (1 << 1);
	textureConfig |= specularMap == -1	? 0 : (1 << 2);
	textureConfig |= mask == -1			? 0 : (1 << 3);
	textureConfig |= roughnessMap == -1 ? 0 : (1 << 4);
	textureConfig |= metallicMap == -1  ? 0 : (1 << 5);
	textureConfig |= heightMap == -1    ? 0 : (1 << 6);
	textureConfig |= emissiveMap == -1  ? 0 : (1 << 7);
	return textureConfig;
}

void BRDF_Material::Clear() 
{
	diffuse = vec3(1,1,1);
	metalness = 0.0f;
	specular = vec3(1,1,1);
	roughness = 0.04f;
	emissiveColor = vec3(0,0,0);
	emissiveIntensity = 0.0f;
	diffuseMap = -1;
	normalMap = -1;
	specularMap = -1;
	mask = -1;
	roughnessMap = -1;
	metallicMap = -1;
	alpha = 1.0f;
	emissiveMap = -1;
	emissiveIntensity = 1.0f;
}

void BlinnPhong_Material::Clear()
{
	diffuse = vec3(1,1,1);
	alpha = 1.0f;
	specular = vec3(1,1,1);
	shininess = 0.0f;
	emissiveColor = vec3(0, 0, 0);
	emissiveIntensity = 0.0f;
	diffuseMap = -1;
	normalMap = -1;
	mask = -1;
	roughnessMap = -1;
	specularMap = -1;
	metallicMap = -1;
	emissiveMap = -1;
	emissiveIntensity = 1.0f;
}

SurfaceMaterial BRDF_Material::GetShaderFriendlyStruct() const
{	
	return SurfaceMaterial
	{
			diffuse.Value(),
			alpha,

			specular,
			roughness,
			
			metalness,
			0.0f,
			tiling,

			GetTextureConfig(),
			(int)0xDEADBEEF,(int)0xDEADBEEF,(int)0xDEADBEEF,

			emissiveColor,
			emissiveIntensity
	};
}

SurfaceMaterial BlinnPhong_Material::GetShaderFriendlyStruct() const
{

	return SurfaceMaterial
	{
		diffuse.Value(),
		alpha,

		specular,
		0.0f,	// brdf roughness

		0.0f,	// brdf metalness
		shininess,
		tiling,

		GetTextureConfig(),
		(int)0xDEADBEEF,(int)0xDEADBEEF,(int)0xDEADBEEF,

		emissiveColor,
		emissiveIntensity
	};
}
