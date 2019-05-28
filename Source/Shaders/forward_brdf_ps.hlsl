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

// PARALLAX_MAPPING
//
// src: https://learnopengl.com/Advanced-Lighting/Parallax-Mapping

//#define PARALLAX_HEIGHT_INTENSITY 1.00f
#define PARALLAX_HEIGHT_INTENSITY 0.05f

float2 ParallaxUVs_Crude(float2 uv, float3 ViewVectorInTangentSpace, Texture2D HeightMap, SamplerState Sampler)
{
	// initial implementation: the hacky approach and first results for parallax mapping

    //const float height = 0.5f - HeightMap.Sample(Sampler, uv).r;
    //const float height = 1.0f - HeightMap.Sample(Sampler, uv).r;
    const float height = HeightMap.Sample(Sampler, uv).r;

    float2 uv_offset = ViewVectorInTangentSpace.xy / ViewVectorInTangentSpace.z * (height * PARALLAX_HEIGHT_INTENSITY);
    return uv - uv_offset;
}


float2 ParallaxUVs_Steep(float2 uv, float3 ViewVectorInTangentSpace, Texture2D HeightMap, SamplerState Sampler)
{
	
	// first iteration: Steep Parallax Mapping
#if 0
    const float numLayers = 10.0f;
#else
    const float minLayers = 8;
    const float maxLayers = 32;
    const float numLayers = lerp(maxLayers, minLayers, dot(float3(0, 0, 1), ViewVectorInTangentSpace));
#endif

    float layerDepthIncrement = 1.0f / numLayers;
    float currLayerDepth = 0.0f;

	// the amount to shift the texture coordinates per layer (from vector P)
    //float2 P = normalize(ViewVectorInTangentSpace.xy) * heightIntensity;
    float2 P = ViewVectorInTangentSpace.xy / ViewVectorInTangentSpace.z * PARALLAX_HEIGHT_INTENSITY;
    float2 uv_delta = P / numLayers;

    float2 currUVs = uv;
    float currHeightMapValue = HeightMap.Sample(Sampler, uv).r;

	[loop]
    while (currLayerDepth < currHeightMapValue)
    {
        currUVs -= uv_delta; // shift uv's along the direction of P
        currHeightMapValue = HeightMap.Sample(Sampler, currUVs).r;
        currLayerDepth += layerDepthIncrement;
    }

    return currUVs;
}

float2 ParallaxUVs_Occl(float2 uv, float3 ViewVectorInTangentSpace, Texture2D HeightMap, SamplerState Sampler)
{
	// second iteration: Parallax Occlusion Mapping

    const int INVERSE_HEIGHT_MAP = 1;

    const float minLayers = 8;
    const float maxLayers = 32;
    const float numLayers = lerp(maxLayers, minLayers, dot(float3(0, 0, 1), ViewVectorInTangentSpace));

    float layerDepthIncrement = 1.0f / numLayers;
    float currLayerDepth = 0.0f;

	// the amount to shift the texture coordinates per layer (from vector P)
    //float2 P = normalize(ViewVectorInTangentSpace.xy) * heightIntensity;
    float2 P = ViewVectorInTangentSpace.xy / ViewVectorInTangentSpace.z * PARALLAX_HEIGHT_INTENSITY;
    float2 uv_delta = P / numLayers;

    float2 currUVs = uv;
    float currHeightMapValue = INVERSE_HEIGHT_MAP > 0
		? HeightMap.Sample(Sampler, uv).r
		: (1.0f - HeightMap.Sample(Sampler, uv).r);

	[loop]
    while (currLayerDepth < currHeightMapValue)
    {
        currUVs -= uv_delta; // shift uv's along the direction of P
        currHeightMapValue = INVERSE_HEIGHT_MAP > 0
			? HeightMap.Sample(Sampler, uv).r
			: (1.0f - HeightMap.Sample(Sampler, uv).r);
        currLayerDepth += layerDepthIncrement;
    }

	// get texture coordinates before collision (reverse operations)
    float2 prevUVs = currUVs + uv_delta;

	// get depth after and before collision for linear interpolation
    float afterDepth = currHeightMapValue - currLayerDepth;
    float beforeDepth = (INVERSE_HEIGHT_MAP > 0
		? HeightMap.Sample(Sampler, uv).r
		: (1.0f - HeightMap.Sample(Sampler, uv).r)) - currLayerDepth + layerDepthIncrement;
 
	// interpolation of texture coordinates
    float weight = afterDepth / (afterDepth - beforeDepth);
    float2 finalTexCoords = prevUVs * weight + currUVs * (1.0 - weight);

    return finalTexCoords;
}

float2 ParallaxUVs_Occl2(float2 uv, float3 ViewVectorInTangentSpace, Texture2D HeightMap, SamplerState Sampler, float NdotV)
{
	//src: https://www.gamedev.net/articles/programming/graphics/a-closer-look-at-parallax-occlusion-mapping-r3262/

    const float fHeigtMapScale = 0.005f;
    const float nMaxSamples = 32.0f;
    const float nMinSamples = 8.0f;

    float fParallaxLimit = -length(ViewVectorInTangentSpace.xy) / ViewVectorInTangentSpace.z;
    fParallaxLimit *= fHeigtMapScale;

    float2 vOffsetDir = normalize(ViewVectorInTangentSpace.xy);
    float2 vMaxOffset = vOffsetDir * fParallaxLimit;

    int nNumSamples = (int) lerp(nMaxSamples, nMinSamples, NdotV);

    float fStepSize = 1.0 / (float) nNumSamples;

    float2 dx = ddx(uv);
    float2 dy = ddy(uv);

    float fCurrRayHeight = 1.0;
    float2 vCurrOffset = float2(0, 0);
    float2 vLastOffset = float2(0, 0);

    float fLastSampledHeight = 1;
    float fCurrSampledHeight = 1;

    int nCurrSample = 0;

    while (nCurrSample < nNumSamples)
    {
        fCurrSampledHeight = HeightMap.SampleGrad(Sampler, uv + vCurrOffset, dx, dy).r;
        if (fCurrSampledHeight > fCurrRayHeight)
        {
            float delta1 = fCurrSampledHeight - fCurrRayHeight;
            float delta2 = (fCurrRayHeight + fStepSize) - fLastSampledHeight;

            float ratio = delta1 / (delta1 + delta2);

            vCurrOffset = (ratio) * vLastOffset + (1.0 - ratio) * vCurrOffset;

            nCurrSample = nNumSamples + 1;
        }
        else
        {
            nCurrSample++;

            fCurrRayHeight -= fStepSize;

            vLastOffset = vCurrOffset;
            vCurrOffset += fStepSize * vMaxOffset;

            fLastSampledHeight = fCurrSampledHeight;
        }
    }

    return uv + vCurrOffset;;
}

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

struct ObjectMatrices
{
    matrix worldViewProj;
    matrix world;
    matrix normal;
    matrix worldInverse;
};
cbuffer perModel
{
#ifdef INSTANCED
	ObjectMatrices ObjMatrices[INSTANCE_COUNT];
#else
    ObjectMatrices ObjMatrices;
#endif
};


cbuffer SceneVariables
{
	float  isEnvironmentLightingOn;
	float3 cameraPos;
	
	float2 screenDimensions;
	float2 spotShadowMapDimensions;

	float directionalShadowMapDimension;
	float3 pad;
	
	SceneLighting Lights;
	
    matrix directionalProj;
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
Texture2D texMetallicMap;
Texture2D texRoughnessMap;
Texture2D texEmissiveMap;
Texture2D texHeightMap;

Texture2D texAmbientOcclusion;

Texture2D tIrradianceMap;
TextureCube tPreFilteredEnvironmentMap;
Texture2D tBRDFIntegrationLUT;

SamplerState sShadowSampler;
SamplerState sLinearSampler;
SamplerState sEnvMapSampler;
SamplerState sNearestSampler;
SamplerState sWrapSampler;

SamplerState sAnisoSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters (World Space)
    const float3 P = In.worldPos;
    const float3 N = normalize(In.normal);
    float3 T = normalize(In.tangent);
    const float3 V = normalize(cameraPos - P);
    const float2 screenSpaceUV = In.position.xy / screenDimensions;
	
	
    //T = normalize(T - dot(N, T) * N);
    float3 B = normalize(cross(T, N));
    float3x3 TBN = float3x3(T, B, N);


	// TODO: instanced alpha discard
    float3 ViewVectorInTangentSpace = 0.0f.xxx;
#ifndef INSTANCED
    ViewVectorInTangentSpace = mul(V, TBN);

    const float2 sclaeBiasedUV = In.texCoord * surfaceMaterial.uvScale;
#if ENABLE_PARALLAX_MAPPING
    const float NdotV = max(0, dot(N, V));

    const float2 uv = HasHeightMap(surfaceMaterial.textureConfig)
		//? ParallaxUVs_Occl(sclaeBiasedUV, ViewVectorInTangentSpace, texHeightMap, sLinearSampler)
		? ParallaxUVs_Occl2(sclaeBiasedUV, ViewVectorInTangentSpace, texHeightMap, sLinearSampler, NdotV)
		: sclaeBiasedUV;
#else
    const float2 uv = sclaeBiasedUV;
#endif //ENABLE_PARALLAX_MAPPING

	const float alpha = HasAlphaMask(surfaceMaterial.textureConfig) > 0 
		? texAlphaMask.Sample(sLinearSampler, uv).r 
		: 1.0f;

	if (alpha < 0.01f)
		discard;
#endif

	ShadowTestPCFData pcfTest;

	// base indices for indexing shadow views
	const int pointShadowsBaseIndex = 0;	// omnidirectional cubemaps are sampled based on light dir, texture is its own array
	const int spotShadowsBaseIndex = 0;		
	const int directionalShadowBaseIndex = spotShadowsBaseIndex + Lights.numSpotCasters;	// currently unused
	
	pcfTest.viewDistanceOfPixel = length(P - cameraPos);

	BRDF_Surface s = (BRDF_Surface)0;
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
	s.emissiveColor = surfaceMaterial[In.instanceID].emissiveColor * surfaceMaterial[In.instanceID].emissiveIntensity;
#else
	s.N = HasNormalMap(surfaceMaterial.textureConfig) > 0
		? UnpackNormals(texNormalMap, sAnisoSampler, uv, N, T)
		: N;
	s.diffuseColor = HasDiffuseMap(surfaceMaterial.textureConfig) > 0
		? pow(texDiffuseMap.Sample(sAnisoSampler, uv).xyz, 2.2f)
		: surfaceMaterial.diffuse;
	s.specularColor = HasSpecularMap(surfaceMaterial.textureConfig) > 0 
		? texSpecularMap.Sample(sLinearSampler, uv).xxx // .xxx := quick hack for single channel specular maps (to be removed)
		: surfaceMaterial.specular;
	s.roughness = HasRoughnessMap(surfaceMaterial.textureConfig) > 0
		? texRoughnessMap.Sample(sLinearSampler, uv)
		: surfaceMaterial.roughness;
	s.metalness = HasMetallicMap(surfaceMaterial.textureConfig) > 0
		? texMetallicMap.Sample(sLinearSampler, uv)
		: surfaceMaterial.metalness;
    s.emissiveColor = (HasEmissiveMap(surfaceMaterial.textureConfig) > 0
		? texEmissiveMap.Sample(sLinearSampler, uv).rgb
		: surfaceMaterial.emissiveColor) * surfaceMaterial.emissiveIntensity;
#endif
	const float3 R = reflect(-V, s.N);

	const float texAO = texAmbientOcclusion.Sample(sNearestSampler, screenSpaceUV).x;
	const float ao = texAO * ambientFactor;

	// illumination
    const float3 Ia = s.diffuseColor * ao;	// ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular
	float3 IEnv = 0.0f.xxx;					// environment lighting
    float3 Ie = s.emissiveColor;


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
	for (int k = 0; k < Lights.numSpotCasters; ++k)
	{
		const matrix matShadowView = Lights.shadowViews[spotShadowsBaseIndex + k];
		pcfTest.lightSpacePos  = mul(matShadowView, float4(P, 1));
		const float3 Lw        = Lights.spot_casters[k].position;
		const float3 Wi        = normalize(Lw - P);
		const float3 radiance  = SpotlightIntensity(Lights.spot_casters[k], P) * Lights.spot_casters[k].color * Lights.spot_casters[k].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		pcfTest.NdotL	       = saturate(dot(s.N, Wi));
        pcfTest.depthBias = Lights.spot_casters[k].depthBias;
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
		const float3 Wi = normalize(-Lights.directional.lightDirection);
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
    const float3 illumination = Ie + Ia + IdIs + IEnv;
    return float4(illumination, 1.0f);// * 0.000001f + float4(ViewVectorInTangentSpace, 1.0f);
    

}