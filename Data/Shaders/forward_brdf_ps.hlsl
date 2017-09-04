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

#define _DEBUG
#define DIRECT_LIGHTING

// defines maximum number of dynamic lights
#define LIGHT_COUNT 20  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^

#include "LightingCommon.hlsl"
#include "BRDF.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float3 worldPos		 : POSITION;
	float4 lightSpacePos : POSITION1;	// array?
	float3 normal		 : NORMAL;
	float3 tangent		 : TANGENT;
	float2 texCoord		 : TEXCOORD4;
};

cbuffer SceneVariables
{
	float  pad0;
	float3 cameraPos;

	float  lightCount;
	float  spotCount;
	float2 padding;

	Light lights[LIGHT_COUNT];
	Light spots[SPOT_COUNT];
	//	float ambient;
};

cbuffer SurfaceMaterial
{
	float3 diffuse;
	float  alpha;

	float3 specular;
	float  roughness;

	float  isDiffuseMap;
	float  isNormalMap;
	float  metalness;
};

Texture2D gDiffuseMap;
Texture2D gNormalMap;
Texture2D gShadowMap;

SamplerState sShadowSampler;
SamplerState sNormalSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	const bool bUsePhongAttenuation = false; // use brdf attenuation
	// lighting & surface parameters
	const float3 P = In.worldPos;
	const float3 N = normalize(In.normal);
	const float3 T = normalize(In.tangent);
	const float3 V = normalize(cameraPos - P);
	const float ambient = 0.00005f;

	BRDF_Surface s;	// surface 
	s.N = (isNormalMap)* UnpackNormals(gNormalMap, sNormalSampler, In.texCoord, N, T) +
		(1.0f - isNormalMap) * N;
	s.diffuseColor = diffuse *  (isDiffuseMap          * gDiffuseMap.Sample(sShadowSampler, In.texCoord).xyz +
		(1.0f - isDiffuseMap)   * float3(1,1,1));
	s.specularColor = specular;
	s.roughness = roughness;
	s.metalness = metalness;

	// illumination
	const float3 Ia = s.diffuseColor * ambient;	// ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);		// diffuse & specular

	
	// POINT Lights
	// brightness default: 300
	//---------------------------------
	for (int i = 0; i < lightCount; ++i)		
	{
		const float3 Wi       = normalize(lights[i].position - P);
		const float3 radiance = Attenuation(lights[i].attenuation, length(lights[i].position - P), bUsePhongAttenuation) * lights[i].color * lights[i].brightness;
		IdIs += BRDF(Wi, s, V, P) * radiance;
	}

	// SPOT Lights (shadow)
	// todo: intensity/brightness
	//---------------------------------
	for (int j = 0; j < spotCount; ++j)			
	{
		const float3 Wi       = normalize(spots[j].position - P);
		const float3 radiance = Intensity(spots[j], P) * spots[j].color;
		const float shadowing = ShadowTest(P, In.lightSpacePos, gShadowMap, sShadowSampler);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing;
	}
	
	const float3 illumination = Ia + IdIs;
	return float4(illumination, 1);
}