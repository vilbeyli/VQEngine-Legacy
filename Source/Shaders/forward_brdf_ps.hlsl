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

#define _DEBUG

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
	float3 worldPos		 : POSITION;
	float4 lightSpacePos : POSITION1;	// array?
	float3 normal		 : NORMAL;
	float3 tangent		 : TANGENT;
	float2 texCoord		 : TEXCOORD4;
#ifdef INSTANCED
	uint instanceID	     : SV_InstanceID;
#endif
};

cbuffer SceneVariables
{
	float  isEnvironmentLightingOn;
	float3 cameraPos;
	
	float2 screenDimensions;
	float2 spotShadowMapDimensions;

	float directionalShadowMapDimension;
	float directionalDepthBias;
	float2 pad;
	
	SceneLighting Lights;

	float ambientFactor;
};
TextureCubeArray texPointShadowMaps;
Texture2DArray   texSpotShadowMaps;
Texture2DArray   texDirectionalShadowMaps;

cbuffer cbSurfaceMaterial
{
#ifdef INSTANCED
	SurfaceMaterial surfaceMaterial[INSTANCE_COUNT];
#else
	SurfaceMaterial surfaceMaterial;
#endif
};

Texture2D texDiffuseMap;
Texture2D texNormalMap;
Texture2D texSpecularMap;
Texture2D texAlphaMask;

Texture2D texAmbientOcclusion;

Texture2D tIrradianceMap;
TextureCube tPreFilteredEnvironmentMap;
Texture2D tBRDFIntegrationLUT;

SamplerState sShadowSampler;
SamplerState sLinearSampler;
SamplerState sEnvMapSampler;
SamplerState sNearestSampler;
SamplerState sWrapSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	// TODO: instanced alpha discard
#ifndef INSTANCED
	const float2 uv = In.texCoord * surfaceMaterial.uvScale;
	const float alpha = HasAlphaMask(surfaceMaterial.textureConfig) > 0 ? texAlphaMask.Sample(sLinearSampler, uv).r : 1.0f;
	if (alpha < 0.01f)
		discard;
#endif

	ShadowTestPCFData pcfTest;

	// base indices for indexing shadow views
	const int pointShadowsBaseIndex = 0;	// omnidirectional cubemaps are sampled based on light dir, texture is its own array
	const int spotShadowsBaseIndex = 0;		
	const int directionalShadowBaseIndex = spotShadowsBaseIndex + Lights.numSpotCasters;	// currently unused


	// lighting & surface parameters (World Space)
	const float3 P = In.worldPos;
	const float3 N = normalize(In.normal);
	const float3 T = normalize(In.tangent);
	const float3 V = normalize(cameraPos - P);
	const float2 screenSpaceUV = In.position.xy / screenDimensions;

	pcfTest.viewDistanceOfPixel = length(P - cameraPos);

	BRDF_Surface s;
#ifdef INSTANCED
	s.N = N;
		// HasNormalMap(surfaceMaterial.textureConfig) > 0
		// ? UnpackNormals(texNormalMap, sLinearSampler, uv, N, T)
		// : N;
	s.diffuseColor = surfaceMaterial[In.instanceID].diffuse;
		// HasDiffuseMap(surfaceMaterial.textureConfig) > 0
		// ? texDiffuseMap.Sample(sLinearSampler, uv).xyz
		// : surfaceMaterial.diffuse;
	s.specularColor = surfaceMaterial[In.instanceID].specular;
		// HasSpecularMap(surfaceMaterial.textureConfig) > 0
		// ? texSpecularMap.Sample(sLinearSampler, uv)
		// : surfaceMaterial.specular;
	s.roughness = surfaceMaterial[In.instanceID].roughness;
	s.metalness = surfaceMaterial[In.instanceID].metalness;
#else
	s.N = HasNormalMap(surfaceMaterial.textureConfig) > 0
		? UnpackNormals(texNormalMap, sLinearSampler, uv, N, T)
		: N;
	s.diffuseColor = HasDiffuseMap(surfaceMaterial.textureConfig) > 0
		? texDiffuseMap.Sample(sLinearSampler, uv).xyz
		: surfaceMaterial.diffuse;
	s.specularColor = HasSpecularMap(surfaceMaterial.textureConfig) > 0 
		? texSpecularMap.Sample(sLinearSampler, uv) 
		: surfaceMaterial.specular;
	s.roughness = surfaceMaterial.roughness;
	s.metalness = surfaceMaterial.metalness;
#endif
	const float3 R = reflect(-V, s.N);

	const float texAO = texAmbientOcclusion.Sample(sNearestSampler, screenSpaceUV).x;
	const float ao = texAO * ambientFactor;

	// illumination
    const float3 Ia = s.diffuseColor * ao;	// ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular
	float3 IEnv = 0.0f.xxx;					// environment lighting


	//-- POINT LIGHTS --------------------------------------------------------------------------------------------------------------------------
#if ENABLE_POINT_LIGHTS
	for (int i = 0; i < Lights.numPointLights; ++i)		
	{
		const float3 Lw       = Lights.point_lights[i].position;
		const float3 Wi       = normalize(Lw - P);
		const float D		  = length(Lw - P);
		const float NdotL	  = saturate(dot(s.N, Wi));
		const float3 radiance = 
			AttenuationBRDF(Lights.point_lights[i].attenuation, D)
			* Lights.point_lights[i].color 
			* Lights.point_lights[i].brightness;

		if (D < Lights.point_lights[i].range)
			IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}
#endif
#if ENABLE_POINT_LIGHTS_SHADOW
	for (int l = 0; l < Lights.numPointCasters; ++l)
	{
		const float3 Lw = Lights.point_casters[l].position;
		const float3 Wi = normalize(Lw - P);
		const float  D = length(Lw - P);
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
				(Lw - P),
				Lights.point_casters[l].range
			);
			IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
		}
	}
#endif


	
	//-- SPOT LIGHTS ---------------------------------------------------------------------------------------------------------------------------
#if ENABLE_SPOT_LIGHTS_SHADOW
	pcfTest.depthBias = 0.0000005f; // TODO
	for (int k = 0; k < Lights.numSpotCasters; ++k)
	{
		const matrix matShadowView = Lights.shadowViews[spotShadowsBaseIndex + k];
		pcfTest.lightSpacePos  = mul(matShadowView, float4(P, 1));
		const float3 Lw        = Lights.spot_casters[k].position;
		const float3 Wi        = normalize(Lw - P);
		const float3 radiance  = SpotlightIntensity(Lights.spot_casters[k], P) * Lights.spot_casters[k].color * Lights.spot_casters[k].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		pcfTest.NdotL	       = saturate(dot(s.N, Wi));
		const float3 shadowing = ShadowTestPCF(pcfTest, texSpotShadowMaps, sShadowSampler, spotShadowMapDimensions, k);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
	}
#endif
#if ENABLE_SPOT_LIGHTS
	for (int j = 0; j < Lights.numSpots; ++j)
	{
		const float3 Lw        = Lights.spots[j].position;
		const float3 Wi        = normalize(Lw - P);
		const float3 radiance  = SpotlightIntensity(Lights.spots[j], P) * Lights.spots[j].color * Lights.spots[j].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		const float NdotL	   = saturate(dot(s.N, Wi));
		IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}
#endif




	//-- DIRECTIONAL LIGHT --------------------------------------------------------------------------------------------------------------------------
#if ENABLE_DIRECTIONAL_LIGHTS
	if (Lights.directional.enabled != 0)
	{
		pcfTest.lightSpacePos = mul(Lights.shadowViewDirectional, float4(P, 1));
		const float3 Wi = -Lights.directional.lightDirection;
		const float3 radiance
			= Lights.directional.color
			* Lights.directional.brightness;
		pcfTest.NdotL = saturate(dot(s.N, Wi));
		pcfTest.depthBias = directionalDepthBias; // TODO
		const float shadowing = (Lights.directional.shadowing == 0)
			? 1.0f
			: ShadowTestPCF(
			  pcfTest
			, texDirectionalShadowMaps
			, sShadowSampler
			, float2(directionalShadowMapDimension, directionalShadowMapDimension)
			, 0);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * pcfTest.NdotL;
	}
#endif

	// ENVIRONMENT Map
	//---------------------------------
	if(isEnvironmentLightingOn > 0.001f)
    {
        const float NdotV = max(0.0f, dot(s.N, V));

        const float2 equirectangularUV = SphericalSample(s.N);
        const float3 environmentIrradience = tIrradianceMap.Sample(sWrapSampler, equirectangularUV).rgb;
        const float3 environmentSpecular = tPreFilteredEnvironmentMap.SampleLevel(sEnvMapSampler, R, s.roughness * MAX_REFLECTION_LOD).rgb;
        const float2 F0ScaleBias = tBRDFIntegrationLUT.Sample(sNearestSampler, float2(NdotV, 1.0f - s.roughness)).rg;
        IEnv = EnvironmentBRDF(s, V, ao, environmentIrradience, environmentSpecular, F0ScaleBias);
		IEnv -= Ia; // cancel ambient lighting
    }
	const float3 illumination = Ia + IdIs + IEnv;

	return float4(illumination, 1.0f);
}