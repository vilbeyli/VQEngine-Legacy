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

#include "Model.h"
#include "Renderer.h"


const Material Material::ruby = Material(	vec3(0.61424f, 0.04136f, 0.04136f),		// diffuse
											vec3(0.727811f, 0.626959f, 0.626959f),	// specular
											76.8f);									// shininess


const Material Material::jade = Material(	vec3(0.54f, 0.89f, 0.63f),				// diffuse
											vec3(0.316228f, 0.316228f, 0.316228f),	// specular
											12.8f);									// shininess


const Material Material::bronze = Material(	vec3(0.714f, 0.4284f, 0.18144f),		// diffuse
											vec3(0.393548f, 0.271906f, 0.166721f),	// specular
											25.6f);									// shininess


const Material Material::gold = Material(	vec3(0.75164f, 0.60648f, 0.22648f),		// diffuse
											vec3(0.628281f, 0.555802f, 0.366065f),	// specular
											51.2f);									// shininess


Material Material::RandomMaterial()
{
	float r = RandF(0.0f, 1.0f);
	if (r >= 0.0000f && r < 0.1999f) return ruby;
	if (r >= 0.2000f && r < 0.3999f) return gold;
	if (r >= 0.4000f && r < 0.5999f) return bronze;
	if (r >= 0.6000f && r < 0.7999f) return jade;
	else return Material();
}

Material::Material(const vec3 & diffuse_in, const vec3 & specular_in, float shininess_in)
{
	color = diffuse_in;
	alpha = 1.0f;
	specular = specular_in;
	shininess = shininess_in;
}

Material::Material()
	:
	color(Color::white),
	alpha(1.0f),
	specular(Color::white.Value()),
	shininess(90.0f),
	roughness(0.5f),
	metalness(0.01f)
{}

void Material::SetMaterialConstants(Renderer* renderer, SHADERS shader) const
{
	renderer->SetConstant3f("diffuse", color.Value());
	renderer->SetConstant1f("alpha", alpha);
	renderer->SetConstant3f("specular", specular);
	switch(shader)
	{
	case SHADERS::FORWARD_PHONG:
	/*case SHADERS::FORWARD_PHONG:*/	// Todo: deferred
		renderer->SetConstant1f("shininess", shininess);
		break;
		
	case SHADERS::FORWARD_BRDF:
	/*case SHADERS::FORWARD_BRDF:*/
		renderer->SetConstant1f("metalness", metalness);
		renderer->SetConstant1f("roughness", roughness);
		break;
	}

	
	renderer->SetConstant1f("isDiffuseMap", diffuseMap._id == -1 ? 0.0f : 1.0f);
	renderer->SetConstant1f("isNormalMap" , normalMap._id  == -1 ? 0.0f : 1.0f);
	if (diffuseMap._id>=0) renderer->SetTexture("gDiffuseMap", diffuseMap._id);
	if ( normalMap._id>=0) renderer->SetTexture("gNormalMap" , normalMap._id);
}
