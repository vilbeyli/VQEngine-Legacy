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

#include "BRDF.hlsl"
#include "LightingCommon.hlsl"

#define ENABLE_POINT_LIGHTS 1
#define ENABLE_POINT_LIGHTS_SHADOW 1

#define ENABLE_SPOT_LIGHTS 1
#define ENABLE_SPOT_LIGHTS_SHADOW 1

#define ENABLE_DIRECTIONAL_LIGHTS 1

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};


// CBUFFERS
//-----------------------------------------------------------------------------------------------------------------------------------------
cbuffer SceneVariables	// frame constants
{
	matrix matView;
	matrix matViewToWorld;
	matrix matPorjInverse;
	
	//float2 pointShadowMapDimensions;
	float2 spotShadowMapDimensions;
	float directionalShadowMapDimension;
	float directionalDepthBias;
	
	SceneLighting Lights;
};

TextureCubeArray texPointShadowMaps;
Texture2DArray   texSpotShadowMaps;
Texture2DArray   texDirectionalShadowMaps;



// TEXTURES & SAMPLERS
//-----------------------------------------------------------------------------------------------------------------------------------------
Texture2D texDiffuseRoughnessMap;
Texture2D texSpecularMetalnessMap;
Texture2D texNormals;
Texture2D texDepth;

SamplerState sShadowSampler;
SamplerState sLinearSampler;



// SHADOW TEST
//
float ShadowTestPCF(
	in ShadowTestPCFData pcfTestLightData
	, TextureCubeArray shadowCubeMapArr
	, SamplerState shadowSampler
	, float2 shadowMapDimensions
	, int shadowMapIndex
	, float3 lightDirection
	, in matrix lightProjectionMat
)
{
	const float BIAS = pcfTestLightData.depthBias * tan(acos(pcfTestLightData.NdotL));
	float shadow = 0.0f;

	// depth check
	const float far_plane = 80;

#if 1
	const float closestDepthInLSpace = shadowCubeMapArr.Sample(shadowSampler, float4(-lightDirection, shadowMapIndex)).x;
	const float closestDepthInWorldSpace = LinearDepth(closestDepthInLSpace, lightProjectionMat);
	shadow += (length(lightDirection) - 1.9525 > closestDepthInWorldSpace) ? 1.0f : 0.0f;
#else
	const float closestDepthInLSpace = shadowCubeMapArr.Sample(shadowSampler, float4(-lightDirection, shadowMapIndex)).x * far_plane;
	//shadow += (length(lightDirection) - 1.15 > closestDepthInLSpace) ? 1.0f : 0.0f;
	shadow += (length(lightDirection) - 0.005 > closestDepthInLSpace) ? 1.0f : 0.0f;
#endif

	return 1.0 - shadow;
}



// ENTRY POINT
//
float4 PSMain(PSIn In) : SV_TARGET
{
	ShadowTestPCFData pcfTest;

	// base indices for indexing shadow views
	const int pointShadowsBaseIndex = 0;	// omnidirectional cubemaps are sampled based on light dir, texture is its own array
	const int spotShadowsBaseIndex = 0;		
	const int directionalShadowBaseIndex = spotShadowsBaseIndex + Lights.numSpotCasters;
	const float2 dirShadowMapDimensions = directionalShadowMapDimension.xx;

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
	

	float3 IdIs = float3(0.0f, 0.0f, 0.0f);		// diffuse & specular

//-- POINT LIGHTS --------------------------------------------------------------------------------------------------------------------------
#if ENABLE_POINT_LIGHTS
	// brightness default: 300
	for (int i = 0; i < Lights.numPointLights; ++i)		
	{
		const float3 Lv       = mul(matView, float4(Lights.point_lights[i].position, 1));
		const float3 Wi       = normalize(Lv - P);
		const float D = length(Lights.point_lights[i].position - Pw);
		const float NdotL	  = saturate(dot(s.N, Wi));
		const float3 radiance = 
			AttenuationBRDF(Lights.point_lights[i].attenuation, D)
			* Lights.point_lights[i].color 
			* Lights.point_lights[i].brightness;
		IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}
#endif
#if ENABLE_POINT_LIGHTS_SHADOW
	for (int l = 0; l < Lights.numPointCasters; ++l)
	{
		const float3 Lw		  = Lights.point_lights[l].position;
		const float3 Lv       = mul(matView, float4(Lw, 1));
		const float3 Wi       = normalize(Lv - P);
		const float3 radiance = 
			AttenuationBRDF(Lights.point_lights[l].attenuation, length(Lights.point_lights[l].position - Pw))
			* Lights.point_lights[l].color 
			* Lights.point_lights[l].brightness;

		// shadow view?
		const matrix matShadowView = Lights.shadowViews[spotShadowsBaseIndex + l];
		pcfTest.lightSpacePos = mul(matShadowView, float4(Pw, 1));

		pcfTest.NdotL = saturate(dot(s.N, Wi));
		pcfTest.depthBias = 0.0000005f;
		const float3 shadowing = ShadowTestPCF(pcfTest, texPointShadowMaps, sShadowSampler, spotShadowMapDimensions, l, (Lw - Pw), Lights.pointLightProjMats[l]);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
	}
#endif



//-- SPOT LIGHTS ---------------------------------------------------------------------------------------------------------------------------
#if ENABLE_SPOT_LIGHTS
	for (int j = 0; j < Lights.numSpots; ++j)
	{
		const float3 Lv        = mul(matView, float4(Lights.spots[j].position, 1));
		const float3 Wi        = normalize(Lv - P);
		const float3 radiance  = SpotlightIntensity(Lights.spots[j], Pw) * Lights.spots[j].color * Lights.spots[j].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		const float NdotL	   = saturate(dot(s.N, Wi));
		IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}
#endif
#if ENABLE_SPOT_LIGHTS_SHADOW
	for (int k = 0; k < Lights.numSpotCasters; ++k)
	{
		const matrix matShadowView = Lights.shadowViews[spotShadowsBaseIndex + k];
		pcfTest.lightSpacePos  = mul(matShadowView, float4(Pw, 1));
		const float3 Lv        = mul(matView, float4(Lights.spot_casters[k].position, 1));
		const float3 Wi        = normalize(Lv - P);
		const float3 radiance  = SpotlightIntensity(Lights.spot_casters[k], Pw) * Lights.spot_casters[k].color * Lights.spot_casters[k].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		pcfTest.NdotL          = saturate(dot(s.N, Wi));
		pcfTest.depthBias = 0.0000005f;
		const float3 shadowing = ShadowTestPCF(pcfTest, texSpotShadowMaps, sShadowSampler, spotShadowMapDimensions, k);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
	}
#endif



//-- DIRECTIONAL LIGHT ----------------------------------------------------------------------------------------------------------------------
#if ENABLE_DIRECTIONAL_LIGHTS
	if (Lights.directional.shadowFactor > 0.0f)
	{
		pcfTest.lightSpacePos = mul(Lights.shadowViewDirectional, float4(Pw, 1));
		const float3 Lv = mul(matView, float4(Lights.directional.lightDirection, 0.0f));
		const float3 Wi = normalize(-Lv);
		const float3 radiance
			= Lights.directional.color
			* Lights.directional.brightness;
		pcfTest.NdotL = saturate(dot(s.N, Wi));
		pcfTest.depthBias = directionalDepthBias;
		const float3 shadowing = ShadowTestPCF(pcfTest, texDirectionalShadowMaps, sShadowSampler, dirShadowMapDimensions, 0);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
	}
#endif




	const float3 illumination = IdIs;
	return float4(illumination, 1);
}