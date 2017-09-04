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
#include "BRDF.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};


// CBUFFERS
//---------------------------------------------------------
// defines maximum number of dynamic lights
#define LIGHT_COUNT 20  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^

cbuffer SceneVariables
{
	matrix lightSpaceMat; // [arr?]
	float  pad0;
	float3 CameraWorldPosition;

	float  lightCount;
	float  spotCount;
	float2 padding;

	Light lights[LIGHT_COUNT];
	Light spots[SPOT_COUNT];
	
};


// TEXTURES & SAMPLERS
//---------------------------------------------------------
Texture2D texDiffuseRoughnessMap;
Texture2D texSpecularMetalnessMap;
Texture2D texNormals;
Texture2D texPosition;
Texture2D texShadowMap;

SamplerState sShadowSampler;
SamplerState sNearestSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters
	const float3 P = texPosition.Sample(sNearestSampler, In.uv);
	const float3 N = texNormals.Sample(sNearestSampler, In.uv);
	//const float3 T = normalize(In.tangent);
	const float3 V = normalize(CameraWorldPosition - P);
	const float ambient = 0.00005f;

	const float4 diffuseRoughness  = texDiffuseRoughnessMap.Sample(sNearestSampler, In.uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sNearestSampler, In.uv);

    BRDF_Surface s;
    s.N = N;
	s.diffuseColor = diffuseRoughness.rgb;
	s.specularColor = specularMetalness.rgb;
	s.roughness = diffuseRoughness.a;
	s.metalness = specularMetalness.a;

#if 1
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);		// diffuse & specular

	// POINT Lights
	// brightness default: 300
	//---------------------------------
	for (int i = 0; i < lightCount; ++i)		
	{
		const float3 Wi       = normalize(lights[i].position - P);
		const float3 radiance = Attenuation(lights[i].attenuation, length(lights[i].position - P), false) * lights[i].color * lights[i].brightness;
		IdIs += BRDF(Wi, s, V, P) * radiance;
	}

#if 1
	// SPOT Lights (shadow)
	//---------------------------------
	float4 lightSpacePos = mul(lightSpaceMat, float4(P,1));
	for (int j = 0; j < spotCount; ++j)
	{
		const float3 Wi       = normalize(spots[j].position - P);
		const float3 radiance = Intensity(spots[j], P) * spots[j].color;
		const float3 shadowing = ShadowTest(P, lightSpacePos, texShadowMap, sShadowSampler);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing;
	}
#endif

	const float3 illumination = IdIs;
	return float4(illumination, 1);
#endif
}