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
	float2 screenDimensions;

	Light lights[LIGHT_COUNT];
	Light spots[SPOT_COUNT];
	//	float ambient;
};

cbuffer cbSurfaceMaterial
{
    SurfaceMaterial surfaceMaterial;
};

Texture2D texDiffuseMap;
Texture2D texNormalMap;
Texture2D texShadowMap;
Texture2D texAmbientOcclusion;

Texture2D tIrradianceMap;
TextureCube tPreFilteredEnvironmentMap;
Texture2D tBRDFIntegrationLUT;

SamplerState sShadowSampler;
SamplerState sNormalSampler;
SamplerState sEnvMapSampler;
SamplerState sNearestSampler;
SamplerState sWrapSampler;

#define MAX_REFLECTION_LOD 4

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters (World Space)
	const float3 P = In.worldPos;
	const float3 N = normalize(In.normal);
	const float3 T = normalize(In.tangent);
    const float3 V = normalize(cameraPos - P);
    const float3 R = reflect(-V, N);
    const float2 screenSpaceUV = In.position.xy / screenDimensions;
	const float ambient = 0.01f;

	BRDF_Surface s;
    s.N = (surfaceMaterial.isNormalMap) * UnpackNormals(texNormalMap, sNormalSampler, In.texCoord, N, T) + (1.0f - surfaceMaterial.isNormalMap) * N;
    s.diffuseColor = surfaceMaterial.diffuse * (surfaceMaterial.isDiffuseMap * texDiffuseMap.Sample(sNormalSampler, In.texCoord).xyz +
					(1.0f - surfaceMaterial.isDiffuseMap) * surfaceMaterial.diffuse);
    s.specularColor = surfaceMaterial.specular;
    s.roughness = surfaceMaterial.roughness;
    s.metalness = surfaceMaterial.metalness;

	const float tAO = texAmbientOcclusion.Sample(sNormalSampler, screenSpaceUV).x;

	// illumination
    const float3 Ia = s.diffuseColor * ambient * tAO; // ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);				// diffuse & specular

	
	// POINT Lights
	// brightness default: 300
	//---------------------------------
	for (int i = 0; i < lightCount; ++i)		
	{
		const float3 Wi       = normalize(lights[i].position - P);
        const float3 radiance = AttenuationBRDF(lights[i].attenuation, length(lights[i].position - P)) * lights[i].color * lights[i].brightness;
		IdIs += BRDF(Wi, s, V, P) * radiance;
	}

	// SPOT Lights (shadow)
	//---------------------------------
	for (int j = 0; j < spotCount; ++j)	
	{
		const float3 Wi       = normalize(spots[j].position - P);
        const float3 radiance = Intensity(spots[j], P) * spots[j].color * spots[j].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		const float shadowing = ShadowTest(P, In.lightSpacePos, texShadowMap, sShadowSampler);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing;
	}
	
	// ENVIRONMENT Map
	//---------------------------------
    const float NdotV = max(0.0f, dot(N, V));

    const float2 equirectangularUV = SphericalSample(N);
    const float3 environmentIrradience = tIrradianceMap.Sample(sWrapSampler, equirectangularUV).rgb;
    const float3 environmentSpecular = tPreFilteredEnvironmentMap.SampleLevel(sEnvMapSampler, R, s.roughness * MAX_REFLECTION_LOD).rgb;
	const float2 F0ScaleBias = tBRDFIntegrationLUT.Sample(sNearestSampler, float2(NdotV, 1.0f - s.roughness)).rg;
	float3 IEnv = EnvironmentBRDF(s, V, tAO, environmentIrradience, environmentSpecular, F0ScaleBias);

	const float3 illumination = Ia + IdIs + IEnv;
	return float4(illumination, 1);
}