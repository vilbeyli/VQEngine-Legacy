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
	float3 diffuseColor;
	float3 specularColor;
	float shininess;
};


#define SHININESS_ADJUSTER 1.0f

// CBUFFERS
//---------------------------------------------------------
#define LIGHT_COUNT 20  // don't forget to update CPU define too (SceneManager.cpp)
#define SPOT_COUNT 10   // ^
struct Light
{
	// COMMON
	float3 position;
	float pad1;
	float3 color;
	float brightness;

	// SPOTLIGHT
	float3 spotDir;
	float halfAngle;

	// POINT LIGHT
	float2 attenuation;
	float range;
	float pad3;
};

cbuffer renderConsts
{
	float gammaCorrection;
	float3 cameraPos;

	float lightCount;
	float spotCount;
	float2 padding;

	Light lights[LIGHT_COUNT];
	Light spots[SPOT_COUNT];
	//	float ambient;
};

cbuffer perObject
{
	float3 diffuse;
	float alpha;

	float3 specular;
	float shininess;

	float isDiffuseMap;
	float isNormalMap;
};

// TEXTURES & SAMPLERS
//---------------------------------------------------------
Texture2D gDiffuseMap;
Texture2D gNormalMap;
Texture2D gShadowMap;

SamplerState sShadowSampler;

// FUNCTIONS
//---------------------------------------------------------
// point light attenuation equation
float Attenuation(float2 coeffs, float dist)
{
	return 1.0f / (
		1.0f
		+ coeffs[0] * dist
		+ coeffs[1] * dist * dist
		);

	//return 1.0f / (	// range 13
	//	1.0f
	//+ 0.7f * dist
	//+ 1.8f * dist * dist
	//);

	//return 1.0f / (	// range 13
	//	1.0f
	//+ 0.0450f * dist
	//+ 0.0075f * dist * dist
	//);
}

// spotlight intensity calculataion
float Intensity(Light l, float3 worldPos)
{   // TODO: figure out halfAngle behavior
	float3 L = normalize(l.position - worldPos);
	float3 spotDir = normalize(-l.spotDir);
	float theta = dot(L, spotDir);
	float softAngle = l.halfAngle * 1.25f;
	float softRegion = softAngle - l.halfAngle;
	return clamp((theta - softAngle) / softRegion, 0.0f, 1.0f);
}

#define ATTENUATION_TEST
#define BLINN_PHONG

// returns diffuse and specular light
float3 Phong(Light light, Surface s, float3 V, float3 worldPos)
{
	const float3 N = s.N;
	const float3 L = normalize(light.position - worldPos);
	const float3 R = normalize(2 * N * dot(N, L) - L);
	

	float diffuse = max(0.0f, dot(N, L));   // lights
	float3 Id = light.color * s.diffuseColor  * diffuse;

#ifdef BLINN_PHONG
	const float3 H = normalize(L + V);
	float3 Is = light.color * s.specularColor * pow(max(dot(N, H), 0.0f), 4.0f * s.shininess) * diffuse;
#else
	float3 Is = light.color * s.specularColor * pow(max(dot(R, V), 0.0f), s.shininess) * diffuse;
#endif
	
	//float3 Is = light.color * pow(max(dot(R, V), 0.0f), 240) ;

	return Id + Is;
}

inline float3 UnpackNormals(float2 uv, float3 worldNormal, float3 worldTangent)
{
	// uncompressed normal in tangent space
	float3 SampledNormal = gNormalMap.Sample(sShadowSampler, uv).xyz;
	SampledNormal = normalize(SampledNormal * 2.0f - 1.0f);
	//SampledNormal.y *= -1.0f;
	//float3 T = normalize(worldTangent);	// after interpolation, T and N might not be orthonormal
	// make sure T is orthonormal to N by subtracting off any component of T along direction N.
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
	if (pxDepthInLSpace - 0.000001 > closestDepthInLSpace)
	{
		return 0.0f;
	}

	return 1.0f;
}

float3 ShadowTestDebug(float3 worldPos, float4 lightSpacePos, float3 illumination)
{
	float3 outOfFrustum = -illumination + float3(1, 1, 0);
	float3 inShadows = -illumination + float3(1, 0, 0);
	float3 noShadows = -illumination + float3(0, 1, 0);

	float3 projLSpaceCoords = lightSpacePos.xyz / lightSpacePos.w;
	//projLSpaceCoords.x *= -1.0f;

	float2 shadowTexCoords = float2(0.5f, 0.5f) + projLSpaceCoords.xy * float2(0.5f, -0.5f);	// invert Y
	float pxDepthInLSpace = projLSpaceCoords.z;
	float closestDepthInLSpace = gShadowMap.Sample(sShadowSampler, shadowTexCoords).x;

	if (projLSpaceCoords.x < -1.0f || projLSpaceCoords.x > 1.0f ||
		projLSpaceCoords.y < -1.0f || projLSpaceCoords.y > 1.0f ||
		projLSpaceCoords.z <  0.0f || projLSpaceCoords.z > 1.0f
		)
	{
		return outOfFrustum;
	}

	if (pxDepthInLSpace > closestDepthInLSpace)
	{
		return inShadows;
	}

	return noShadows;
}

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters
	float3 N = normalize(In.normal);
	float3 T = normalize(In.tangent);
	float3 V = normalize(cameraPos - In.worldPos);
	const float ambient = 0.035f;
	Surface s;
	s.N = (isNormalMap)* UnpackNormals(In.texCoord, N, T) +
		(1.0f - isNormalMap) * N;
	s.diffuseColor = diffuse *  (isDiffuseMap          * gDiffuseMap.Sample(sShadowSampler, In.texCoord).xyz +
		(1.0f - isDiffuseMap)   * float3(1,1,1));
	s.specularColor = specular;
	s.shininess = shininess * SHININESS_ADJUSTER;

	// illumination
	float3 Ia = s.diffuseColor * ambient;	// ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular
	for (int i = 0; i < lightCount; ++i)	// POINT Lights
		IdIs += Phong(lights[i], s, V, In.worldPos) * Attenuation(lights[i].attenuation, length(lights[i].position - In.worldPos));

	for (int j = 0; j < spotCount; ++j)		// SPOT Lights
		IdIs += Phong(spots[j], s, V, In.worldPos) * Intensity(spots[j], In.worldPos) * ShadowTest(In.worldPos, In.lightSpacePos);

	float3 illumination = Ia + IdIs;
	
	// --- debug --- 
	// illumination += ShadowTestDebug(In.worldPos, In.lightSpacePos, illumination);
	// --- debug --- 
	float4 outColor = float4(illumination, 1);	// alpha

	// gamma correction
	const bool gammaCorrect = gammaCorrection > 0.99f;
	//const float gamma = 1.0 / 2.2;
	//const float gamma = 1.0;
	const float gamma = 2.2;
	//const float gamma = 2.2f * (1.0f - isDiffuseMap) + isDiffuseMap * 1.0f;
	
	//return outColor;
	return pow(outColor, float4(gamma, gamma, gamma, 1.0f));
	//if (gammaCorrect)	
	//else        		return outColor;
}