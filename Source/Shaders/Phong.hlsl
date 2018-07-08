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

#include "ShadingMath.hlsl"

// constants
#define EPSILON 0.000000000001f

// defines
#define _DEBUG


struct PHONG_Surface
{
	float3 N;
	float3 diffuseColor;
	float3 specularColor;
	float shininess;
};


// returns diffuse and specular components of phong illumination model
float3 Phong(PHONG_Surface s, float3 L, float3 V, float3 lightColor)
{
	const float3 N = s.N;
	const float3 R = normalize(2 * N * dot(N, L) - L);
	float diffuse = max(0.0f, dot(N, L));

	float3 Id = lightColor * s.diffuseColor * diffuse;

#ifdef BLINN_PHONG
	const float3 H = normalize(L + V);
	float3 Is = lightColor * s.specularColor * pow(max(dot(N, H), 0.0f), 4.0f * s.shininess) * diffuse;
#else
	float3 Is = lightColor * s.specularColor * pow(max(dot(R, V), 0.0f), s.shininess) * diffuse;
#endif

	//float3 Is = light.color * pow(max(dot(R, V), 0.0f), 240) ;
	return Id + Is;
}