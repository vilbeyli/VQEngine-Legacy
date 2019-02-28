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
#include "LTC.hlsl"

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
	matrix matProjInverse;
	
	//float2 pointShadowMapDimensions;
	float2 spotShadowMapDimensions;
	float  directionalShadowMapDimension;
    float dummy;
	matrix directionalProj;
	
	SceneLighting Lights;
};

// TEXTURES & SAMPLERS
//-----------------------------------------------------------------------------------------------------------------------------------------
Texture2D texDiffuseRoughnessMap;
Texture2D texSpecularMetalnessMap;
Texture2D texNormals;
Texture2D texEmissiveMap;
Texture2D texDepth;

TextureCubeArray texPointShadowMaps;
Texture2DArray   texSpotShadowMaps;
Texture2DArray   texDirectionalShadowMaps;

Texture2D texLTC_LUT;

SamplerState sShadowSampler;
SamplerState sLinearSampler;


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
	const float3 P = ViewSpacePosition(nonLinearDepth, In.uv, matProjInverse);
	const float3 N = texNormals.Sample(sLinearSampler, In.uv);
	//const float3 T = normalize(In.tangent);
	const float3 V = normalize(- P);
	
	const float3 Pw = mul(matViewToWorld, float4(P, 1)).xyz;	// world position of the shaded pixel
	const float3 Pcam = float3(matViewToWorld._14, matViewToWorld._24, matViewToWorld._34);// world position of the camera
	pcfTest.viewDistanceOfPixel = length(Pw - Pcam);
    
	const float3 Vw = normalize(Pcam - Pw);
    const float3 Nw = mul(matViewToWorld, float4(N, 0));

	const float4 diffuseRoughness  = texDiffuseRoughnessMap.Sample(sLinearSampler, In.uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sLinearSampler, In.uv);

    BRDF_Surface s;
    s.N = N;
	s.diffuseColor = diffuseRoughness.rgb;
	s.specularColor = specularMetalness.rgb;
	s.roughness = diffuseRoughness.a;
	s.metalness = specularMetalness.a;
    s.P = P;

	float3 IdIs = float3(0.0f, 0.0f, 0.0f);		// diffuse & specular
    float3 Ie = texEmissiveMap.Sample(sLinearSampler, In.uv);

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
		if( D < Lights.point_lights[i].range )
			IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}
#endif
#if ENABLE_POINT_LIGHTS_SHADOW
	for (int l = 0; l < Lights.numPointCasters; ++l)
	{
		const float3 Lw		  = Lights.point_casters[l].position;
		const float3 Lv       = mul(matView, float4(Lw, 1));
		const float3 Wi       = normalize(Lv - P);
		const float  D        = length(Lights.point_casters[l].position - Pw);
		const float3 radiance = 
			AttenuationBRDF(Lights.point_casters[l].attenuation, D)
			* Lights.point_casters[l].color 
			* Lights.point_casters[l].brightness;

		pcfTest.NdotL = saturate(dot(s.N, Wi));
		pcfTest.depthBias = Lights.point_casters[l].depthBias;

		if (D < Lights.point_casters[l].range)
		{
			const float3 shadowing = OmnidirectionalShadowTestPCF(
				pcfTest,
				texPointShadowMaps,
				sShadowSampler,
				spotShadowMapDimensions,
				l,
				(Lw - Pw),
				Lights.point_casters[l].range
			);
			IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
		}
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
		pcfTest.depthBias      = Lights.spot_casters[k].depthBias;
		const float3 shadowing = ShadowTestPCF(pcfTest, texSpotShadowMaps, sShadowSampler, spotShadowMapDimensions, k);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
	}
#endif



//-- DIRECTIONAL LIGHT ----------------------------------------------------------------------------------------------------------------------
#if ENABLE_DIRECTIONAL_LIGHTS
	if (Lights.directional.enabled != 0)
	{
		pcfTest.lightSpacePos = mul(Lights.shadowViewDirectional, float4(Pw, 1));
		const float3 Lv = mul(matView, float4(Lights.directional.lightDirection, 0.0f));
		const float3 Wi = normalize(-Lv);
		const float3 radiance
			= Lights.directional.color
			* Lights.directional.brightness;
		pcfTest.NdotL = saturate(dot(s.N, Wi));
		pcfTest.depthBias = Lights.directional.depthBias;
		const float shadowing = (Lights.directional.shadowing == 0)
			? 1.0f 
			: ShadowTestPCF_Directional(
				pcfTest
				, texDirectionalShadowMaps
				, sShadowSampler
				, directionalShadowMapDimension
				, 0
				, directionalProj
			);

		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
	}
#endif

	
//-- AREA LIGHTS ----------------------------------------------------------------------------------------------------------------------
#if !LTC_USE_VIEW_SPACE
    s.N = normalize(Nw);
	s.P = Pw;
	IdIs += EvalCylinder(s, Vw, Lights.cylinderLight, texLTC_LUT, sLinearSampler, matView);
#else
    s.N = N;
    s.P = P;
    IdIs += EvalCylinder(s, V, Lights.cylinderLight, texLTC_LUT, sLinearSampler, matView);
#endif

	const float3 illumination = Ie + IdIs;
	return float4(illumination, 1);
}