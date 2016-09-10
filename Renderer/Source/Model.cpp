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

const Material Material::ruby = Material(	XMFLOAT3(0.61424f, 0.04136f, 0.04136f),		// diffuse
											XMFLOAT3(0.727811f, 0.626959f, 0.626959f),	// specular
											76.8f);										// shininess


const Material Material::jade = Material(	XMFLOAT3(0.54f, 0.89f, 0.63f),				// diffuse
											XMFLOAT3(0.316228f, 0.316228f, 0.316228f),	// specular
											12.8f);										// shininess


const Material Material::bronze = Material(	XMFLOAT3(0.714f, 0.4284f, 0.18144f),		// diffuse
											XMFLOAT3(0.393548f, 0.271906f, 0.166721f),	// specular
											25.6f);										// shininess


const Material Material::gold = Material(	XMFLOAT3(0.75164f, 0.60648f, 0.22648f),		// diffuse
											XMFLOAT3(0.628281f, 0.555802f, 0.366065f),	// specular
											51.2f);										// shininess

Material::Material(const XMFLOAT3 & diffuse_in, const XMFLOAT3 & specular_in, float shininess_in)
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
	shininess(90.0f)
{}

void Material::SetMaterialConstants(Renderer * renderer) const
{
	renderer->SetConstant3f("diffuse", color.Value());
	renderer->SetConstant1f("shininess", shininess);
	renderer->SetConstant1f("alpha", alpha);
	renderer->SetConstant3f("specular", specular);
	//renderer->SetConstant1f("isDiffuseMap", diffuseMap.id == -1 ? 0.0f : 1.0f);
}
