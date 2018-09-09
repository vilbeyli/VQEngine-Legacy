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

#include "Phong.hlsl"
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
	matrix matPorjInverse;
	
	float2 spotShadowMapDimensions;
	float2 pad;
	
	SceneLighting sceneLightData;
};
TextureCubeArray texPointShadowMaps;
Texture2DArray   texSpotShadowMaps;
Texture2DArray   texDirectionalShadowMaps;


// TEXTURES & SAMPLERS
//---------------------------------------------------------
Texture2D texDiffuseRoughnessMap;
Texture2D texSpecularMetalnessMap;
Texture2D texNormals;
Texture2D texDepth;

SamplerState sShadowSampler;
SamplerState sLinearSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	ShadowTestPCFData pcfTest;
	pcfTest.depthBias = 0.0000005f;
	

	// base indices for indexing shadow views
	const int pointShadowsBaseIndex = 0;	// omnidirectional cubemaps are sampled based on light dir, texture is its own array
	const int spotShadowsBaseIndex = 0;		
	const int directionalShadowBaseIndex = spotShadowsBaseIndex + sceneLightData.numSpotCasters;	// currently unused

	// lighting & surface parameters (View Space Lighting)
    const float3 P = ViewSpacePosition(texDepth.Sample(sLinearSampler, In.uv).r, In.uv, matPorjInverse);
    const float3 N = normalize(texNormals.Sample(sLinearSampler, In.uv).xyz);
	const float3 V = normalize(- P);
	
	const float3 Pw = mul(matViewToWorld, float4(P, 1)).xyz;
    const float3 Vw = mul(matViewToWorld, float4(V, 0)).xyz;
    const float3 Nw = mul(matViewToWorld, float4(N, 0)).xyz;

	const float4 diffuseRoughness  = texDiffuseRoughnessMap.Sample(sLinearSampler, In.uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sLinearSampler, In.uv);

    PHONG_Surface s;
    s.N = Nw;
	s.diffuseColor = diffuseRoughness.rgb;
	s.specularColor = specularMetalness.rgb;
	s.shininess = diffuseRoughness.a;	// shininess is stored in alpha channel

	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular
	
	// POINT Lights w/o shadows
	for (int i = 0; i < sceneLightData.numPointLights; ++i)	
    {
		float3 Lw = normalize(sceneLightData.point_lights[i].position - Pw);
        float NdotL = saturate(dot(Nw, Lw));
        IdIs += 
		Phong(s, Lw, Vw, sceneLightData.point_lights[i].color)
		* AttenuationPhong(sceneLightData.point_lights[i].attenuation, length(sceneLightData.point_lights[i].position - Pw))
		* sceneLightData.point_lights[i].brightness 
		* NdotL
		* POINTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }
	
	// SPOT Lights w/o shadows
	for (int j = 0; j < sceneLightData.numSpots; ++j)		
    {
		float3 Lw = normalize(sceneLightData.spots[j].position - Pw);
        float NdotL = saturate(dot(Nw, Lw));
        IdIs +=
		Phong(s, Lw, Vw, sceneLightData.spots[j].color)
		* SpotlightIntensity(sceneLightData.spots[j], Pw)
		* sceneLightData.spots[j].brightness 
		* NdotL
		* SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }

	// SPOT Lights w/ shadows
	for (int k = 0; k < sceneLightData.numSpotCasters; ++k)	
    {
		const matrix matShadowView = sceneLightData.shadowViews[spotShadowsBaseIndex + k];
		pcfTest.lightSpacePos = mul(matShadowView, float4(Pw, 1));
		float3 Lw = normalize(sceneLightData.spot_casters[k].position - Pw);
		pcfTest.NdotL = saturate(dot(Nw, Lw));
        IdIs +=
		Phong(s, Lw, Vw, sceneLightData.spot_casters[k].color)
		* SpotlightIntensity(sceneLightData.spot_casters[k], Pw)
		* ShadowTestPCF(pcfTest, texSpotShadowMaps, sShadowSampler, spotShadowMapDimensions, k)
		* sceneLightData.spot_casters[k].brightness 
		* pcfTest.NdotL
		* SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }


	const float3 illumination = IdIs;
	return float4(illumination, 1);
}