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
#ifdef INSTANCED
	uint instanceID			: SV_InstanceID;
#endif
};

struct PSOut	// G-BUFFER
{
	float4 diffuseRoughness  : SV_TARGET0;
	float4 specularMetalness : SV_TARGET1;
	half3  normals			 : SV_TARGET2;
	float3 emissiveColor	 : SV_TARGET3;
};


cbuffer cbSurfaceMaterial
{
#ifdef INSTANCED
    SurfaceMaterial surfaceMaterial[INSTANCE_COUNT];
#else
	SurfaceMaterial surfaceMaterial;
#endif
    float BRDFOrPhong;
};

Texture2D texDiffuseMap;
Texture2D texNormalMap;
Texture2D texSpecularMap;
//Texture2D texAOMap;        // ?
Texture2D texMetallicMap;
Texture2D texRoughnessMap;
Texture2D texEmissiveMap;

Texture2D texAlphaMask;


SamplerState sNormalSampler;
SamplerState sAnisoSampler;

PSOut PSMain(PSIn In) : SV_TARGET
{
#ifndef INSTANCED
	const float2 uv = In.uv * surfaceMaterial.uvScale;
	const float alpha = HasAlphaMask(surfaceMaterial.textureConfig) > 0 ? texAlphaMask.Sample(sNormalSampler, uv).r : 1.0f;
	if (alpha < 0.01f)
		discard;
#endif

	PSOut GBuffer;

	// lighting & surface parameters
	const float3 P = In.viewPosition;
	const float3 N = normalize(In.viewNormal);
	const float3 T = normalize(In.viewTangent);
	const float3 V = normalize(-P);

#ifdef INSTANCED
	const float  metalness            = surfaceMaterial[In.instanceID].metalness;
	const float3 finalDiffuse         = surfaceMaterial[In.instanceID].diffuse;
	const float3 finalSpecular        = surfaceMaterial[In.instanceID].specular;
	const float  roughnessORshininess = surfaceMaterial[In.instanceID].roughness * BRDFOrPhong 
	                                  + surfaceMaterial[In.instanceID].shininess * (1.0f - BRDFOrPhong);
	const float3 emissive             = surfaceMaterial[In.instanceID].emissiveColor * surfaceMaterial[In.instanceID].emissiveIntensity;
	const float3 finalNormal = N;
#else
    const float3 sampledDiffuse = pow(texDiffuseMap.Sample(sAnisoSampler, uv).xyz, 2.2f);
	const float3 surfaceDiffuse = surfaceMaterial.diffuse;

	const float3 finalDiffuse   = HasDiffuseMap(surfaceMaterial.textureConfig) > 0 
		? sampledDiffuse 
		: surfaceDiffuse;

	const float3 finalNormal    = HasNormalMap (surfaceMaterial.textureConfig) > 0 
		? UnpackNormals(texNormalMap, sAnisoSampler, uv, N, T)
		: N;

	const float3 finalSpecular  = HasSpecularMap(surfaceMaterial.textureConfig) > 0 
		? texSpecularMap.Sample(sNormalSampler, uv).xxx  // .xxx := quick hack for single channel specular maps (to be removed)
		: surfaceMaterial.specular;

	const float roughnessORshininess = HasRoughnessMap(surfaceMaterial.textureConfig) > 0
		? texRoughnessMap.Sample(sNormalSampler, uv)
		: surfaceMaterial.roughness * BRDFOrPhong + surfaceMaterial.shininess * (1.0f - BRDFOrPhong);

	const float metalness = HasMetallicMap(surfaceMaterial.textureConfig) > 0
		? texMetallicMap.Sample(sNormalSampler, uv)
		: surfaceMaterial.metalness;

    const float3 emissive = (HasEmissiveMap(surfaceMaterial.textureConfig) > 0
		? texEmissiveMap.Sample(sNormalSampler, uv)
		: surfaceMaterial.emissiveColor) * surfaceMaterial.emissiveIntensity;
#endif

	GBuffer.diffuseRoughness	= float4(finalDiffuse, roughnessORshininess);
	GBuffer.specularMetalness	= float4(finalSpecular, metalness);
	GBuffer.normals				= finalNormal;
    GBuffer.emissiveColor		= emissive;
	return GBuffer;
}