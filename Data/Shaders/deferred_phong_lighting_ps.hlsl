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

#define BLINN_PHONG

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
	//float  pad0;
	//float3 CameraWorldPosition;
	matrix matView;
	matrix matViewToWorld;

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
	// lighting & surface parameters (View Space Lighting)
	const float3 P = texPosition.Sample(sNearestSampler, In.uv).xyz;
	const float3 N = texNormals.Sample(sNearestSampler, In.uv).xyz;
	const float3 V = normalize(- P);
	
	const float3 Pw = mul(matViewToWorld, float4(P, 1)).xyz;
    const float3 Vw = mul(matViewToWorld, float4(V, 0)).xyz;
    const float3 Nw = mul(matViewToWorld, float4(N, 0)).xyz;
    const float4 lightSpacePos = mul(lightSpaceMat, float4(Pw, 1));

	const float4 diffuseRoughness  = texDiffuseRoughnessMap.Sample(sNearestSampler, In.uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sNearestSampler, In.uv);

    PHONG_Surface s;
    s.N = Nw;
	s.diffuseColor = diffuseRoughness.rgb;
	s.specularColor = specularMetalness.rgb;
	s.shininess = diffuseRoughness.a;	// shininess is stored in alpha channel

	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular

	for (int i = 0; i < lightCount; ++i)	// POINT Lights
    {
        IdIs += 
		Phong(lights[i], s, Vw, Pw)
		* AttenuationPhong(lights[i].attenuation, length(lights[i].position - Pw))
		* lights[i].brightness 
		* POINTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }

#if 1
	for (int j = 0; j < spotCount; ++j)		// SPOT Lights
    {
        IdIs +=
		Phong(spots[j], s, Vw, Pw)
		* Intensity(spots[j], Pw)
		* ShadowTest(Pw, lightSpacePos, texShadowMap, sShadowSampler)
		* spots[j].brightness 
		* SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }
#endif

	const float3 illumination = IdIs;
	return float4(illumination, 1);
}