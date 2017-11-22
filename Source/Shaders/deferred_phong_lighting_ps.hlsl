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

#define BLINN_PHONG

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};


// CBUFFERS
//---------------------------------------------------------
cbuffer SceneVariables
{
	matrix matView;
	matrix matViewToWorld;

	matrix lightSpaceMat; // todo [arr]
	SceneLighting sceneLightData;
};


Texture2D texSpotShadowMap;	// todo array


// TEXTURES & SAMPLERS
//---------------------------------------------------------
Texture2D texDiffuseRoughnessMap;
Texture2D texSpecularMetalnessMap;
Texture2D texNormals;
Texture2D texPosition;	// to be removed
//Texture2DArray texPointShadowMaps;

SamplerState sShadowSampler;
SamplerState sNearestSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters (View Space Lighting)
	const float3 P = texPosition.Sample(sNearestSampler, In.uv).xyz;
	const float3 N = texNormals.Sample(sNearestSampler, In.uv).xyz;
	const float3 V = normalize(- P);
	
	const float3 Pw = mul(matViewToWorld, float4(P, 1)).xyz;
    const float4 Pl = mul(lightSpaceMat, float4(Pw, 1));
    const float3 Vw = mul(matViewToWorld, float4(V, 0)).xyz;
    const float3 Nw = mul(matViewToWorld, float4(N, 0)).xyz;

	const float4 diffuseRoughness  = texDiffuseRoughnessMap.Sample(sNearestSampler, In.uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sNearestSampler, In.uv);

    PHONG_Surface s;
    s.N = Nw;
	s.diffuseColor = diffuseRoughness.rgb;
	s.specularColor = specularMetalness.rgb;
	s.shininess = diffuseRoughness.a;	// shininess is stored in alpha channel

	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular

	for (int i = 0; i < sceneLightData.numPointLights; ++i)	// POINT Lights w/o shadows
    {
		float3 Lw = normalize(sceneLightData.point_lights[i].position - Pw);
        IdIs += 
		Phong(s, Lw, Vw, sceneLightData.point_lights[i].color)
		* AttenuationPhong(sceneLightData.point_lights[i].attenuation, length(sceneLightData.point_lights[i].position - Pw))
		* sceneLightData.point_lights[i].brightness 
		* POINTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }
	
	for (int j = 0; j < sceneLightData.numSpots; ++j)		// SPOT Lights w/o shadows
    {
		float3 Lw = normalize(sceneLightData.spots[i].position - Pw);
        IdIs +=
		Phong(s, Lw, Vw, sceneLightData.spots[j].color)
		* Intensity(sceneLightData.spots[j], Pw)
		* sceneLightData.spots[j].brightness 
		* SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }

	for (int k = 0; k < sceneLightData.numSpotCasters; ++k)	// SPOT Lights
    {
		float3 Lw = normalize(sceneLightData.spot_casters[i].position - Pw);
        IdIs +=
		Phong(s, Lw, Vw, sceneLightData.spot_casters[k].color)
		* Intensity(sceneLightData.spot_casters[k], Pw)
		* ShadowTest(Pw, Pl, texSpotShadowMap, sShadowSampler)
		* sceneLightData.spot_casters[k].brightness 
		* SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }


	const float3 illumination = IdIs;
	return float4(illumination, 1);
}