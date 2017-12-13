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

#include "LightingCommon.hlsl"
#include "ShadingMath.hlsl"

#define EPSILON 0.000000000001f

#define _DEBUG


struct PSIn
{
	float4 position			: SV_POSITION;
	float3 viewPosition		: POSITION0;
	float3 viewNormal		: NORMAL;
	float3 viewTangent		: TANGENT;
	float2 uv				: TEXCOORD1;
};

struct PSOut	// G-BUFFER
{
	float4 diffuseRoughness  : SV_TARGET0;
	float4 specularMetalness : SV_TARGET1;
	float3 normals			 : SV_TARGET2;
};

cbuffer cbSurfaceMaterial
{
    SurfaceMaterial surfaceMaterial;
    float BRDFOrPhong;
};

Texture2D texDiffuseMap;
Texture2D texNormalMap;

SamplerState sNormalSampler;

PSOut PSMain(PSIn In) : SV_TARGET
{
	PSOut GBuffer;

	// lighting & surface parameters
	const float3 P = In.viewPosition;
	const float3 N = normalize(In.viewNormal);
	const float3 T = normalize(In.viewTangent);
	const float3 V = normalize(-P);
	float2 uv = In.uv * surfaceMaterial.uvScale;

    BRDF_Surface s;
	
	// there's a weird issue here if garbage texture is bound and isDiffuseMap is 0.0f. 
	//s.diffuseColor	= surfaceMaterial.diffuse * (surfaceMaterial.isDiffuseMap * texDiffuseMap.Sample(sNormalSampler, uv).xyz + (1.0f - surfaceMaterial.isDiffuseMap) * surfaceMaterial.diffuse);
	//s.N				= (surfaceMaterial.isNormalMap) * UnpackNormals(texNormalMap, sNormalSampler, uv, N, T) + (1.0f - surfaceMaterial.isNormalMap) * N;

	// workaround -> use if else for selecting rather then blending. for some reason, the vector math fails and (0,0,0) + (1,1,1) ends up being (0,1,1). Not sure why.
	float3 sampledDiffuse = texDiffuseMap.Sample(sNormalSampler, uv).xyz;
	float3 surfaceDiffuse = surfaceMaterial.diffuse;
	float3 finalDiffuse = surfaceMaterial.isDiffuseMap > 0.0f ? sampledDiffuse : surfaceDiffuse;
	
	s.diffuseColor = finalDiffuse;
	s.N = surfaceMaterial.isNormalMap > 0.0f ? UnpackNormals(texNormalMap, sNormalSampler, uv, N, T) : N;
	
	s.specularColor = surfaceMaterial.specular;
    s.roughness = // use s.roughness for either roughness (PBR) or shininess (Phong)
	surfaceMaterial.roughness * BRDFOrPhong + 
	surfaceMaterial.shininess * (1.0f - BRDFOrPhong);
	s.metalness = surfaceMaterial.metalness;

	GBuffer.diffuseRoughness	= float4(s.diffuseColor, s.roughness);
	GBuffer.specularMetalness	= float4(s.specularColor, s.metalness);
	GBuffer.normals				= s.N;
	return GBuffer;
}