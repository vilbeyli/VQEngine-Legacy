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

#define BLINN_PHONG

#define LIGHT_COUNT 20  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^

#include "LightingCommon.hlsl"
#include "ShadingMath.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float3 worldPos		 : POSITION;
	float4 lightSpacePos : POSITION1;	// array?
	float3 normal		 : NORMAL;
	float3 tangent		 : TANGENT;
	float2 texCoord		 : TEXCOORD4;
};

cbuffer SceneConstants
{
	float padding0;
	float3 cameraPos;

	float lightCount;
	float spotCount;
    float2 screenDimensions;

	Light lights[LIGHT_COUNT];
	Light spots[SPOT_COUNT];
	//	float ambient;
};

cbuffer cbSurfaceMaterial
{
    SurfaceMaterial surfaceMaterial;
};


Texture2D texDiffuseMap;
Texture2D texNormalMap;
Texture2D texShadowMap;
Texture2D texAmbientOcclusion;

SamplerState sShadowSampler;
SamplerState sNormalSampler;


float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters
	const float3 N = normalize(In.normal);
	const float3 T = normalize(In.tangent);
	const float3 V = normalize(cameraPos - In.worldPos);
    const float2 screenSpaceUV = In.position.xy / screenDimensions;

	const float ambient = 0.005f;

	PHONG_Surface s;
    s.N = (surfaceMaterial.isNormalMap) * UnpackNormals(texNormalMap, sNormalSampler, In.texCoord, N, T) +
		  (1.0f - surfaceMaterial.isNormalMap) * N;
    
	s.diffuseColor = (surfaceMaterial.isDiffuseMap * texDiffuseMap.Sample(sShadowSampler, In.texCoord).xyz +
		             (1.0f - surfaceMaterial.isDiffuseMap) * float3(1, 1, 1)
	) * surfaceMaterial.diffuse;

	s.specularColor = surfaceMaterial.specular;
    s.shininess = surfaceMaterial.shininess;

	// illumination
    float3 Ia = s.diffuseColor * ambient * texAmbientOcclusion.Sample(sNormalSampler, screenSpaceUV).xxx; // ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular

	for (int i = 0; i < lightCount; ++i)	// POINT Lights
    {
        IdIs += 
		Phong(lights[i], s, V, In.worldPos) 
		* AttenuationPhong(lights[i].attenuation, length(lights[i].position - In.worldPos)) 
		* lights[i].brightness 
		* POINTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }

	for (int j = 0; j < spotCount; ++j)		// SPOT Lights
    {
        IdIs +=
		Phong(spots[j], s, V, In.worldPos)
		* Intensity(spots[j], In.worldPos)
		* ShadowTest(In.worldPos, In.lightSpacePos, texShadowMap, sShadowSampler)
		* spots[j].brightness 
		* SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG;
    }

	float3 illumination = Ia + IdIs;
	
	// --- debug --- 
	// illumination += ShadowTestDebug(In.worldPos, In.lightSpacePos, illumination, texShadowMap, sShadowSampler);
	// --- debug --- 
	return float4(illumination, 1);	
}