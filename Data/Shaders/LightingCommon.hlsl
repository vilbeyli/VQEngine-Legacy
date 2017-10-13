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

#define SHADOW_BIAS 0.0000005f

#if 0	// debug shortcut to cancel spot lights
#define SPOTLIGHT_BRIGHTNESS_SCALAR 0
#define SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG 0
#else
#define SPOTLIGHT_BRIGHTNESS_SCALAR 0.001f
#define SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG 0.00028f
#endif
#define POINTLIGHT_BRIGHTNESS_SCALAR_PHONG 0.002f

// STRUCTS
//----------------------------------------------------------
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

// CPU - GPU struct for both lighting models
struct SurfaceMaterial
{
    float3 diffuse;
    float alpha;

    float3 specular;
    float roughness;

    float isDiffuseMap;
    float isNormalMap;
    float metalness;
    float shininess;
};

struct BRDF_Surface
{
	float3 N;
	float roughness;
	float3 diffuseColor;
	float metalness;
	float3 specularColor;
};

struct PHONG_Surface
{
	float3 N;
	float3 diffuseColor;
	float3 specularColor;
	float shininess;
};

// FUNCTIONS
//----------------------------------------------------------
inline float Attenuation(float2 coeffs, float dist, bool phong)
{
	if (phong)
	{
		return 1.0f / (
			1.0f
			+ coeffs[0] * dist
			+ coeffs[1] * dist * dist
			);
	}

	// quadratic attenuation (inverse square) is physically more accurate
	// used for BRDF
	return 1.0f / (dist * dist);
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

inline float3 UnpackNormals(Texture2D normalMap, SamplerState normalSampler, float2 uv, float3 worldNormal, float3 worldTangent)
{
	// uncompressed normal in tangent space
	float3 SampledNormal = normalMap.Sample(normalSampler, uv).xyz;
	SampledNormal = normalize(SampledNormal * 2.0f - 1.0f);

	const float3 T = normalize(worldTangent - dot(worldNormal, worldTangent) * worldNormal);
	const float3 N = normalize(worldNormal);
	const float3 B = normalize(cross(T, N));
	const float3x3 TBN = float3x3(T, B, N);
	return mul(SampledNormal, TBN);
}

float ShadowTest(float3 worldPos, float4 lightSpacePos, Texture2D shadowMap, SamplerState shadowSampler)
{
	// homogeneous position after interpolation
	float3 projLSpaceCoords = lightSpacePos.xyz / lightSpacePos.w;

	// clip space [-1, 1] --> texture space [0, 1]
	float2 shadowTexCoords = float2(0.5f, 0.5f) + projLSpaceCoords.xy * float2(0.5f, -0.5f);	// invert Y

	float pxDepthInLSpace = projLSpaceCoords.z;
	float closestDepthInLSpace = shadowMap.Sample(shadowSampler, shadowTexCoords).x;

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

float3 ShadowTestDebug(float3 worldPos, float4 lightSpacePos, float3 illumination, Texture2D shadowMap, SamplerState shadowSampler)
{
	float3 outOfFrustum = -illumination + float3(1, 1, 0);
	float3 inShadows = -illumination + float3(1, 0, 0);
	float3 noShadows = -illumination + float3(0, 1, 0);

	float3 projLSpaceCoords = lightSpacePos.xyz / lightSpacePos.w;
	//projLSpaceCoords.x *= -1.0f;

	float2 shadowTexCoords = float2(0.5f, 0.5f) + projLSpaceCoords.xy * float2(0.5f, -0.5f);	// invert Y
	float pxDepthInLSpace = projLSpaceCoords.z;
	float closestDepthInLSpace = shadowMap.Sample(shadowSampler, shadowTexCoords).x;

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
