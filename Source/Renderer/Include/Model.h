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

#pragma once

#include "Color.h"
#include "Texture.h"
#include "Shader.h"

class Renderer;

struct Material
{
#if 0
	union
	{
		Color	albedo;
		Color	color;
	};
#else
	Color		color;
#endif
	float		alpha;	
	vec3		specular;
	float		roughness;

	// store brdf/phong both for being able to togge between different lighting models
	float		metalness;	// cook-torrence brdf
	float		shininess;	// phong

	TextureID	diffuseMap;
	TextureID	normalMap;

	// phong default materials
	static const Material jade, ruby, bronze, gold;
	static Material RandomMaterial();


	Material(const vec3& diffuse, const vec3& specular, float shininess);
	Material();
	~Material();
	void SetMaterialConstants(Renderer* renderer, EShaders shader) const;
};

class Model
{
public:
	Model() : m_mesh(-1) {}
	~Model() {}

public:
	BufferID m_mesh;
	Material m_material;
};

