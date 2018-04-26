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

#include "BRDF.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};


// CBUFFERS
//---------------------------------------------------------
// defines maximum number of dynamic lights
#define LIGHT_COUNT 100  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^

cbuffer SceneVariables	// frame constants
{
	matrix matView;
	matrix matViewToWorld;
	matrix matPorjInverse;
	
	//float2 pointShadowMapDimensions;
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
	// base indices for indexing shadow views
	const int pointShadowsBaseIndex = 0;	// omnidirectional cubemaps are sampled based on light dir, texture is its own array
	const int spotShadowsBaseIndex = 0;		
	const int directionalShadowBaseIndex = spotShadowsBaseIndex + sceneLightData.numSpotCasters;	// currently unused

	// lighting & surface parameters (View Space Lighting)
    const float nonLinearDepth = texDepth.Sample(sLinearSampler, In.uv).r;
	const float3 P = ViewSpacePosition(nonLinearDepth, In.uv, matPorjInverse);
	const float3 N = texNormals.Sample(sLinearSampler, In.uv);
	//const float3 T = normalize(In.tangent);
	const float3 V = normalize(- P);
	
	const float3 Pw = mul(matViewToWorld, float4(P, 1)).xyz;

	const float4 diffuseRoughness  = texDiffuseRoughnessMap.Sample(sLinearSampler, In.uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sLinearSampler, In.uv);

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
	for (int i = 0; i < sceneLightData.numPointLights; ++i)		
	{
		const float3 Lv       = mul(matView, float4(sceneLightData.point_lights[i].position, 1));
		const float3 Wi       = normalize(Lv - P);
		const float D = length(sceneLightData.point_lights[i].position - Pw);
		const float NdotL	  = saturate(dot(s.N, Wi));
		const float3 radiance = 
			AttenuationBRDF(sceneLightData.point_lights[i].attenuation, D)
			* sceneLightData.point_lights[i].color 
			* sceneLightData.point_lights[i].brightness;
		IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}

	// todo shadow caster points
	//for (int i = 0; i < sceneLightData.numPointLights; ++i)		
	//{
	//	const float3 Lv       = mul(matView, float4(sceneLightData.point_lights[i].position, 1));
	//	const float3 Wi       = normalize(Lv - P);
	//	const float3 radiance = 
	//		AttenuationBRDF(sceneLightData.point_lights[i].attenuation, length(sceneLightData.point_lights[i].position - Pw))
	//		* sceneLightData.point_lights[i].color 
	//		* sceneLightData.point_lights[i].brightness;
	//	IdIs += BRDF(Wi, s, V, P) * radiance;
	//}

	
	// SPOT Lights
	//---------------------------------
	for (int j = 0; j < sceneLightData.numSpots; ++j)
	{
		const float3 Lv        = mul(matView, float4(sceneLightData.spots[j].position, 1));
		const float3 Wi        = normalize(Lv - P);
		const float3 radiance  = Intensity(sceneLightData.spots[j], Pw) * sceneLightData.spots[j].color * sceneLightData.spots[j].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		const float NdotL	   = saturate(dot(s.N, Wi));
		IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}

	for (int k = 0; k < sceneLightData.numSpotCasters; ++k)
	{
		const matrix matShadowView = sceneLightData.shadowViews[spotShadowsBaseIndex + k];
		const float4 Pl		   = mul(matShadowView, float4(Pw, 1));
		const float3 Lv        = mul(matView, float4(sceneLightData.spot_casters[k].position, 1));
		const float3 Wi        = normalize(Lv - P);
		const float3 radiance  = Intensity(sceneLightData.spot_casters[k], Pw) * sceneLightData.spot_casters[k].color * sceneLightData.spot_casters[k].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		const float  NdotL	   = saturate(dot(s.N, Wi));
		const float3 shadowing = ShadowTestPCF(Pw, Pl, texSpotShadowMaps, k, sShadowSampler, NdotL, spotShadowMapDimensions);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * NdotL;
	}

	const float3 illumination = IdIs;
	return float4(illumination, 1);
#endif
}