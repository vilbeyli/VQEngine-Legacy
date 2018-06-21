//	VQEngine | DirectX11 Renderer
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
	float  isEnvironmentLightingOn;
	float3 cameraPos;
	
	float2 screenDimensions;
	float2 spotShadowMapDimensions;

	//float2 pointShadowMapDimensions;
	//float2 pad;
	
	SceneLighting sceneLightData;

	float ambientFactor;
};
TextureCubeArray texPointShadowMaps;
Texture2DArray   texSpotShadowMaps;
Texture2DArray   texDirectionalShadowMaps;

cbuffer cbSurfaceMaterial
{
    SurfaceMaterial surfaceMaterial;
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
	// base indices for indexing shadow views
	const int pointShadowsBaseIndex = 0;	// omnidirectional cubemaps are sampled based on light dir, texture is its own array
	const int spotShadowsBaseIndex = 0;		
	const int directionalShadowBaseIndex = spotShadowsBaseIndex + sceneLightData.numSpotCasters;	// currently unused
	const float2 uv = In.texCoord * surfaceMaterial.uvScale;

	// lighting & surface parameters (World Space)
	const float3 P = In.worldPos;
	const float3 N = normalize(In.normal);
	const float3 T = normalize(In.tangent);
    const float3 V = normalize(cameraPos - P);
    const float2 screenSpaceUV = In.position.xy / screenDimensions;

	BRDF_Surface s;
    //s.N = (surfaceMaterial.isNormalMap) * UnpackNormals(texNormalMap, sLinearSampler, uv, N, T) + (1.0f - surfaceMaterial.isNormalMap) * N;
	s.N = HasNormalMap(surfaceMaterial.textureConfig) > 0 ? UnpackNormals(texNormalMap, sLinearSampler, uv, N, T) : N;
    const float3 R = reflect(-V, s.N);

	// there's a weird issue here if garbage texture is bound and isDiffuseMap is 0.0f. 
	//s.diffuseColor	= surfaceMaterial.diffuse * (HasDiffuseMap(surfaceMaterial.textureConfig) * texDiffuseMap.Sample(sNormalSampler, uv).xyz + (1.0f - sHasDiffuseMap(surfaceMaterial.textureConfig)) * surfaceMaterial.diffuse);
	//s.N				= (surfaceMaterial.isNormalMap) * UnpackNormals(texNormalMap, sNormalSampler, uv, N, T) + (1.0f - surfaceMaterial.isNormalMap) * N;

	// workaround -> use if else for selecting rather then blending. for some reason, the vector math fails and (0,0,0) + (1,1,1) ends up being (0,1,1). Not sure why.
	float3 sampledDiffuse = HasDiffuseMap(surfaceMaterial.textureConfig) * texDiffuseMap.Sample(sLinearSampler, uv).xyz;
	float3 surfaceDiffuse = surfaceMaterial.diffuse;
	float3 finalDiffuse = HasDiffuseMap(surfaceMaterial.textureConfig) > 0 ? sampledDiffuse : surfaceDiffuse;
	s.diffuseColor = finalDiffuse;

    s.specularColor = HasSpecularMap(surfaceMaterial.textureConfig) > 0 ? texSpecularMap.Sample(sLinearSampler, uv) : surfaceMaterial.specular;
    s.roughness = surfaceMaterial.roughness;
    s.metalness = surfaceMaterial.metalness;

	const float texAO = texAmbientOcclusion.Sample(sNearestSampler, screenSpaceUV).x;
	const float ao = texAO * ambientFactor;

	// illumination
    const float3 Ia = s.diffuseColor * ao;	// ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular
	float3 IEnv = 0.0f.xxx;					// environment lighting
	
	// POINT Lights
	// brightness default: 300
	//---------------------------------
	for (int i = 0; i < sceneLightData.numPointLights; ++i)		
	{
		const float3 Lw       = sceneLightData.point_lights[i].position;
		const float3 Wi       = normalize(Lw - P);
		const float D		  = length(Lw - P);
		const float NdotL	  = saturate(dot(s.N, Wi));
		const float3 radiance = 
			AttenuationBRDF(sceneLightData.point_lights[i].attenuation, D)
			* sceneLightData.point_lights[i].color 
			* sceneLightData.point_lights[i].brightness;
		IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}

	// SPOT Lights - Shadowing
	//---------------------------------
	for (int k = 0; k < sceneLightData.numSpotCasters; ++k)
	{
		const matrix matShadowView = sceneLightData.shadowViews[spotShadowsBaseIndex + k];
		const float4 Pl		   = mul(matShadowView, float4(P, 1));
		const float3 Lw        = sceneLightData.spot_casters[k].position;
		const float3 Wi        = normalize(Lw - P);
		const float3 radiance  = Intensity(sceneLightData.spot_casters[k], P) * sceneLightData.spot_casters[k].color * sceneLightData.spot_casters[k].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		const float  NdotL	   = saturate(dot(s.N, Wi));
		const float3 shadowing = ShadowTestPCF(P, Pl, texSpotShadowMaps, k, sShadowSampler, NdotL, spotShadowMapDimensions);
		IdIs += BRDF(Wi, s, V, P) * radiance * shadowing * NdotL;
	}
	
	// SPOT Lights - Non-shadowing
	//---------------------------------
	for (int j = 0; j < sceneLightData.numSpots; ++j)
	{
		const float3 Lw        = sceneLightData.spots[j].position;
		const float3 Wi        = normalize(Lw - P);
		const float3 radiance  = Intensity(sceneLightData.spots[j], P) * sceneLightData.spots[j].color * sceneLightData.spots[j].brightness * SPOTLIGHT_BRIGHTNESS_SCALAR;
		const float NdotL	   = saturate(dot(s.N, Wi));
		IdIs += BRDF(Wi, s, V, P) * radiance * NdotL;
	}

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

	float alpha = HasAlphaMask(surfaceMaterial.textureConfig) ? texAlphaMask.Sample(sLinearSampler, uv).r : 1.0f;
	//float alpha = 1.0f;

	return float4(illumination * alpha, alpha);
}