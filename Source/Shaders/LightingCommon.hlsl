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

//struct SpotLight
//
//struct PointLight
//{
//	float3 position;
//	float 
//};

// defines maximum number of dynamic lights  todo: shader defines
#define NUM_POINT_LIGHT 20			// don't forget to update CPU define too (SceneManager.cpp)
#define NUM_POINT_LIGHT_SHADOW 20   // ^
#define NUM_SPOT_LIGHT 20			// ^
#define NUM_SPOT_LIGHT_SHADOW 20	// ^

struct SceneLighting	//  5152 bytes
{
	int numPointLights;		// non-shadow caster count
	int numSpots;
	int numDirectionals;
	int numPointCasters;		// shadow caster count
	int numSpotCasters;
	int numDirectionalCasters;
	int pad0, pad1;	// 32 bytes

	// todo: break up light into point and spot. add directional as well.
	Light point_lights[NUM_POINT_LIGHT];
	Light point_casters[NUM_POINT_LIGHT_SHADOW];
	// dir

	Light spots[NUM_SPOT_LIGHT];
	Light spot_casters[NUM_SPOT_LIGHT_SHADOW];
	// dir
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
inline float AttenuationBRDF(float2 coeffs, float dist)
{
	// quadratic attenuation (inverse square) is physically more accurate
	return 1.0f / (dist * dist);
}

inline float AttenuationPhong(float2 coeffs, float dist)
{
    return 1.0f / (
			1.0f
			+ coeffs[0] * dist
			+ coeffs[1] * dist * dist
			);
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

// returns diffuse and specular components of phong illumination model
float3 Phong(Light light, PHONG_Surface s, float3 V, float3 worldPos)
{
    const float3 N = s.N;
    const float3 L = normalize(light.position - worldPos);
    const float3 R = normalize(2 * N * dot(N, L) - L);
	

    float diffuse = max(0.0f, dot(N, L)); // lights
    float3 Id = light.color * s.diffuseColor * diffuse;

#ifdef BLINN_PHONG
	const float3 H = normalize(L + V);
	float3 Is = light.color * s.specularColor * pow(max(dot(N, H), 0.0f), 4.0f * s.shininess) * diffuse;
#else
    float3 Is = light.color * s.specularColor * pow(max(dot(R, V), 0.0f), s.shininess) * diffuse;
#endif
	
	//float3 Is = light.color * pow(max(dot(R, V), 0.0f), 240) ;

    return Id + Is;
}