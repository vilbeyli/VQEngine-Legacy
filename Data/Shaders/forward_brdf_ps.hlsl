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

// constants
//#define PI 3.14159265359f
#define SHININESS_ADJUSTER 1.0f		// temp hack
//#define EPSILON 0.000000000001f
#define SHADOW_BIAS 0.00000001f

// defines
#define _DEBUG
#define DIRECT_LIGHTING

struct PSIn
{
	float4 position		 : SV_POSITION;
	float3 worldPos		 : POSITION;
	float4 lightSpacePos : POSITION1;	// array?
	float3 normal		 : NORMAL;
	float3 tangent		 : TANGENT;
	float2 texCoord		 : TEXCOORD4;
};

struct Surface
{
	float3 N;
	float roughness;
	float3 diffuseColor;
	float metalness;
	float3 specularColor;
};



// CBUFFERS
//---------------------------------------------------------
// defines maximum number of dynamic lights
#define LIGHT_COUNT 20  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^
struct Light
{
	// COMMON
	float3 position;
	float  pad1;
	float3 color;
	float  brightness;

	// SPOTLIGHT
	float3 spotDir;
	float  halfAngle;

	// POINT LIGHT
	float2 attenuation;
	float  range;
	float  pad3;
};

cbuffer SceneVariables
{
	float  pad0;
	float3 cameraPos;

	float  lightCount;
	float  spotCount;
	float2 padding;

	Light lights[LIGHT_COUNT];
	Light spots[SPOT_COUNT];
	//	float ambient;
};

cbuffer SurfaceMaterial
{
	float3 diffuse;
	float  alpha;

	float3 specular;
	float  roughness;

	float  isDiffuseMap;
	float  isNormalMap;
	float  metalness;
};

// TEXTURES & SAMPLERS
//---------------------------------------------------------
Texture2D gDiffuseMap;
Texture2D gNormalMap;
Texture2D gShadowMap;

SamplerState sShadowSampler;
SamplerState sNormalSampler;

// FUNCTIONS
//---------------------------------------------------------
// point light attenuation equation
inline float Attenuation(float2 coeffs, float dist)
{
#if 0
	return 1.0f / (
		1.0f
		+ coeffs[0] * dist
		+ coeffs[1] * dist * dist
		);
#else
	// quadratic attenuation (inverse square) is physically more accurate
	return 1.0f / (dist * dist);
#endif
}

// spotlight intensity calculataion
float Intensity(Light l, float3 worldPos)
{   
	const float3 L         = normalize(l.position - worldPos);
	const float3 spotDir   = normalize(-l.spotDir);
	const float theta      = dot(L, spotDir);
	const float softAngle  = l.halfAngle * 1.25f;
	const float softRegion = softAngle - l.halfAngle;
	return clamp((theta - softAngle) / softRegion, 0.0f, 1.0f);
}



inline float3 UnpackNormals(float2 uv, float3 worldNormal, float3 worldTangent)
{
	// uncompressed normal in tangent space
	float3 SampledNormal = gNormalMap.Sample(sNormalSampler, uv).xyz;
	SampledNormal = normalize(SampledNormal * 2.0f - 1.0f);

	const float3 T = normalize(worldTangent - dot(worldNormal, worldTangent) * worldNormal);
	const float3 N = normalize(worldNormal);
	const float3 B = normalize(cross(T, N));
	const float3x3 TBN = float3x3(T, B, N);
	return mul(SampledNormal, TBN);
}

float ShadowTest(float3 worldPos, float4 lightSpacePos)
{
	// homogeneous position after interpolation
	float3 projLSpaceCoords = lightSpacePos.xyz / lightSpacePos.w;

	// clip space [-1, 1] --> texture space [0, 1]
	float2 shadowTexCoords = float2(0.5f, 0.5f) + projLSpaceCoords.xy * float2(0.5f, -0.5f);	// invert Y

	float pxDepthInLSpace = projLSpaceCoords.z;
	float closestDepthInLSpace = gShadowMap.Sample(sShadowSampler, shadowTexCoords).x;

	// frustum check
	if (projLSpaceCoords.x < -1.0f || projLSpaceCoords.x > 1.0f ||
		projLSpaceCoords.y < -1.0f || projLSpaceCoords.y > 1.0f ||
		projLSpaceCoords.z <  0.0f || projLSpaceCoords.z > 1.0f
		)
	{
		return 0.0f;
	}

	// depth check
	if (pxDepthInLSpace - SHADOW_BIAS > closestDepthInLSpace)
	{
		return 0.0f;
	}

	return 1.0f;
}

#include "BRDF.hlsl"

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters
	const float3 P = In.worldPos;
	const float3 N = normalize(In.normal);
	const float3 T = normalize(In.tangent);
	const float3 V = normalize(cameraPos - P);
	const float ambient = 0.00005f;

	Surface s;	// surface 
	s.N = (isNormalMap)* UnpackNormals(In.texCoord, N, T) +
		(1.0f - isNormalMap) * N;
	s.diffuseColor = diffuse *  (isDiffuseMap          * gDiffuseMap.Sample(sShadowSampler, In.texCoord).xyz +
		(1.0f - isDiffuseMap)   * float3(1,1,1));
	s.specularColor = specular;
	s.roughness = roughness;
	s.metalness = metalness;

	// illumination
	const float3 Ia = s.diffuseColor * ambient;	// ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);		// diffuse & specular

	// integrate the lighting equation
	const int steps = 10;
	const float dW = 1.0f / steps;
	for (int iter = 0; iter < steps; ++iter)
	{
		// POINT Lights
		// brightness default: 300
		//---------------------------------
		for (int i = 0; i < lightCount; ++i)		
		{
			const float3 Wi       = normalize(lights[i].position - P);
			const float3 radiance = Attenuation(lights[i].attenuation, length(lights[i].position - P)) * lights[i].color * lights[i].brightness;
			IdIs += BRDF(Wi, s, V, P) * radiance * dW;
		}

		// SPOT Lights (shadow)
		// todo: intensity/brightness
		//---------------------------------
		for (int j = 0; j < spotCount; ++j)			
		{
			const float3 Wi       = normalize(spots[j].position - P);
			const float3 radiance = Intensity(spots[j], P) * spots[j].color;
			IdIs += BRDF(Wi, s, V, P) * radiance * ShadowTest(P, In.lightSpacePos) * dW;
		}
	}
	
	const float3 illumination = Ia + IdIs;
	return float4(illumination, 1);
}