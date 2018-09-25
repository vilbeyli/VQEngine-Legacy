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

#define BLINN_PHONG

#define LIGHT_COUNT 100  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^

#include "Phong.hlsl"
#include "LightingCommon.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float3 worldPos		 : POSITION;
	float3 normal		 : NORMAL;
	float3 tangent		 : TANGENT;
	float2 texCoord		 : TEXCOORD4;
};

cbuffer SceneConstants
{
	float padding0;
	float3 cameraPos;
	
	float2 screenDimensions;
	float2 spotShadowMapDimensions;

	//float2 pointShadowMapDimensions;
	//float2 pad;
	
	SceneLighting Lights;
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
Texture2D texAmbientOcclusion;

SamplerState sShadowSampler;
SamplerState sLinearSampler;
SamplerState sNormalSampler;


float4 PSMain(PSIn In) : SV_TARGET
{	// base indices for indexing shadow views
	const int pointShadowsBaseIndex = 0;	// omnidirectional cubemaps are sampled based on light dir, texture is its own array
	const int spotShadowsBaseIndex = 0;		
	const int directionalShadowBaseIndex = spotShadowsBaseIndex + Lights.numSpotCasters;	// currently unused

	ShadowTestPCFData pcfTest;

	// lighting & surface parameters
	const float3 Nw = normalize(In.normal);
	const float3 T = normalize(In.tangent);
	const float3 Vw = normalize(cameraPos - In.worldPos);
    const float2 screenSpaceUV = In.position.xy / screenDimensions;
	const float3 Pw = In.worldPos;
	const float2 uv = In.texCoord * surfaceMaterial.uvScale;

	PHONG_Surface s;
    s.N = (HasNormalMap(surfaceMaterial.textureConfig)) * UnpackNormals(texNormalMap, sLinearSampler, uv, Nw, T) +
		  (1.0f - HasNormalMap(surfaceMaterial.textureConfig)) * Nw;
    
	s.diffuseColor = surfaceMaterial.diffuse              * (HasDiffuseMap(surfaceMaterial.textureConfig) * texDiffuseMap.Sample(sLinearSampler, uv).xyz +
					(1.0f - HasDiffuseMap(surfaceMaterial.textureConfig)));

	s.specularColor = surfaceMaterial.specular;
    s.shininess = surfaceMaterial.shininess;

	// illumination
    float3 Ia = s.diffuseColor * ambientFactor * texAmbientOcclusion.Sample(sNormalSampler, screenSpaceUV).xxx; // ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular
    s.N = normalize(s.N);

	// POINT Lights w/o shadows
	for (int i = 0; i < Lights.numPointLights; ++i)	
    {
		float3 Lw = normalize(Lights.point_lights[i].position - Pw);
        float NdotL = saturate(dot(s.N, Lw));
        IdIs += 
		Phong(s, Lw, Vw, Lights.point_lights[i].color)
		* AttenuationPhong(Lights.point_lights[i].attenuation, length(Lights.point_lights[i].position - Pw))
		* Lights.point_lights[i].brightness 
		* NdotL
		* POINTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }
	
	// SPOT Lights w/o shadows;
	pcfTest.depthBias = 0.0000005f;
	pcfTest.lightSpacePos = 0.0f.xxxx;
	for (int j = 0; j < Lights.numSpots; ++j)		
    {
		float3 Lw = normalize(Lights.spots[j].position - Pw);
		pcfTest.NdotL = saturate(dot(s.N, Lw));
        IdIs +=
		Phong(s, Lw, Vw, Lights.spots[j].color)
		* SpotlightIntensity(Lights.spots[j], Pw)
		* Lights.spots[j].brightness 
		* pcfTest.NdotL
		* SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }

	// SPOT Lights w/ shadows
	for (int k = 0; k < Lights.numSpotCasters; ++k)	
    {
		const matrix matShadowView = Lights.shadowViews[spotShadowsBaseIndex + k];
		pcfTest.lightSpacePos = mul(matShadowView, float4(Pw, 1));
		float3 Lw = normalize(Lights.spot_casters[k].position - Pw);
		pcfTest.NdotL = saturate(dot(s.N, Lw));;
		pcfTest.depthBias = 0.0000005f;
        IdIs +=
		Phong(s, Lw, Vw, Lights.spot_casters[k].color)
		* SpotlightIntensity(Lights.spot_casters[k], Pw)
		* ShadowTestPCF(pcfTest, texSpotShadowMaps, sShadowSampler, spotShadowMapDimensions, k)
		* Lights.spot_casters[k].brightness 
		* pcfTest.NdotL
		* SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }

	float3 illumination = Ia + IdIs;
	
	// --- debug --- 
	// illumination += ShadowTestDebug(In.worldPos, In.lightSpacePos, illumination, texShadowMap, sShadowSampler);
	// --- debug --- 
	return float4(illumination, 1);	
}