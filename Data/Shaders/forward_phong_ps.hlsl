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

#define BLINN_PHONG

#define LIGHT_COUNT 20  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^

#include "LightingCommon.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float3 worldPos		 : POSITION;
	float4 lightSpacePos : POSITION1;	// array?
	float3 normal		 : NORMAL;
	float3 tangent		 : TANGENT;
	float2 texCoord		 : TEXCOORD4;
};

cbuffer SceneConstants
{
	float padding0;
	float3 cameraPos;

	float lightCount;
	float spotCount;
	float2 padding;

	Light lights[LIGHT_COUNT];
	Light spots[SPOT_COUNT];
	//	float ambient;
};

cbuffer SufaceMaterial
{
	float3 diffuse;
	float alpha;

	float3 specular;
	float shininess;

	float isDiffuseMap;
	float isNormalMap;
};

Texture2D gDiffuseMap;
Texture2D gNormalMap;
Texture2D gShadowMap;

SamplerState sShadowSampler;
SamplerState sNormalSampler;

// returns diffuse and specular light
float3 Phong(Light light, PHONG_Surface s, float3 V, float3 worldPos)
{
	const float3 N = s.N;
	const float3 L = normalize(light.position - worldPos);
	const float3 R = normalize(2 * N * dot(N, L) - L);
	

	float diffuse = max(0.0f, dot(N, L));   // lights
	float3 Id = light.color * s.diffuseColor  * diffuse;

#ifdef BLINN_PHONG
	const float3 H = normalize(L + V);
	float3 Is = light.color * s.specularColor * pow(max(dot(N, H), 0.0f), 4.0f * s.shininess) * diffuse;
#else
	float3 Is = light.color * s.specularColor * pow(max(dot(R, V), 0.0f), s.shininess) * diffuse;
#endif
	
	//float3 Is = light.color * pow(max(dot(R, V), 0.0f), 240) ;

	return Id + Is;
}

float4 PSMain(PSIn In) : SV_TARGET
{
	const bool bUsePhongAttenuation = true;
	// lighting & surface parameters
	float3 N = normalize(In.normal);
	float3 T = normalize(In.tangent);
	float3 V = normalize(cameraPos - In.worldPos);
	const float ambient = 0.005f;
	PHONG_Surface s;
	s.N = (isNormalMap)        * UnpackNormals(gNormalMap, sNormalSampler, In.texCoord, N, T) +
		  (1.0f - isNormalMap) * N;
	s.diffuseColor = diffuse *  (isDiffuseMap          * gDiffuseMap.Sample(sShadowSampler, In.texCoord).xyz +
		                        (1.0f - isDiffuseMap)  * float3(1,1,1));
	s.specularColor = specular;
	s.shininess = shininess;

	// illumination
	float3 Ia = s.diffuseColor * ambient;	// ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular
	for (int i = 0; i < lightCount; ++i)	// POINT Lights
		IdIs += Phong(lights[i], s, V, In.worldPos) * Attenuation(lights[i].attenuation, length(lights[i].position - In.worldPos), bUsePhongAttenuation);

	for (int j = 0; j < spotCount; ++j)		// SPOT Lights
		IdIs += Phong(spots[j], s, V, In.worldPos) * Intensity(spots[j], In.worldPos) * ShadowTest(In.worldPos, In.lightSpacePos, gShadowMap, sShadowSampler);

	float3 illumination = Ia + IdIs;
	
	// --- debug --- 
	// illumination += ShadowTestDebug(In.worldPos, In.lightSpacePos, illumination, gShadowMap, sShadowSampler);
	// --- debug --- 
	return float4(illumination, 1);	
}