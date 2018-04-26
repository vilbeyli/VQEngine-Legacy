//	DX11Renderer - VDemo | DirectX11 Renderer
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
#include "Renderer.h"

const BlinnPhong_Material BlinnPhong_Material::ruby = BlinnPhong_Material(
	vec3(0.61424f, 0.04136f, 0.04136f),		// diffuse
	vec3(0.727811f, 0.626959f, 0.626959f),	// specular
	76.8f									// shininess
);


const BlinnPhong_Material BlinnPhong_Material::jade = BlinnPhong_Material(
	vec3(0.54f, 0.89f, 0.63f),				// diffuse
	vec3(0.316228f, 0.316228f, 0.316228f),	// specular
	12.8f									// shininess
);


const BlinnPhong_Material BlinnPhong_Material::bronze = BlinnPhong_Material(
	vec3(0.714f, 0.4284f, 0.18144f),		// diffuse
	vec3(0.393548f, 0.271906f, 0.166721f),	// specular
	25.6f									// shininess
);


const BlinnPhong_Material BlinnPhong_Material::gold = BlinnPhong_Material(
	vec3(0.75164f, 0.60648f, 0.22648f),		// diffuse
	vec3(0.628281f, 0.555802f, 0.366065f),	// specular
	51.2f									// shininess
);


BlinnPhong_Material Material::RandomBlinnPhongMaterial()
{
	float r = RandF(0.0f, 1.0f);
	if (r >= 0.0000f && r < 0.1999f) return BlinnPhong_Material::ruby;
	if (r >= 0.2000f && r < 0.3999f) return BlinnPhong_Material::gold;
	if (r >= 0.4000f && r < 0.5999f) return BlinnPhong_Material::bronze;
	if (r >= 0.6000f && r < 0.7999f) return BlinnPhong_Material::jade;
	else return BlinnPhong_Material();
}
BRDF_Material Material::RandomBRDFMaterial()
{
	BRDF_Material m = BRDF_Material();
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


BlinnPhong_Material::BlinnPhong_Material(const vec3 & diffuse_in, const vec3 & specular_in, float shininess_in)
	:
	Material()
{
	diffuse = diffuse_in;
	specular = specular_in;
	shininess = shininess_in;
}

Material::Material()
	:
	diffuse(LinearColor::white),
	alpha(1.0f),
	specular(LinearColor::white.Value()),
	tiling(1, 1),
	diffuseMap(-1),
	normalMap(-1)
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