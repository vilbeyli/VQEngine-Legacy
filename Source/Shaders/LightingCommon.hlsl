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

#define SHADOW_BIAS 0.0000005f

#if 0	// debug shortcut to cancel spot lights
#define SPOTLIGHT_BRIGHTNESS_SCALAR 0
#define SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG 0
#else
// some manual tuning...
#define SPOTLIGHT_BRIGHTNESS_SCALAR 0.001f
#define SPOTLIGHT_BRIGHTNESS_SCALAR_PHONG 0.00028f
#endif
#define POINTLIGHT_BRIGHTNESS_SCALAR_PHONG 0.002f

//----------------------------------------------------------
// LIGHTS
//----------------------------------------------------------
struct PointLight
{	// 48 bytes
	float3 position;
	float  range;
	float3 color;
	float  brightness;
	float3 attenuation;
	float  depthBias;
};

struct SpotLight
{	// 48 bytes
	float3 position;
	float  halfAngle;
	float3 color;
	float  brightness;
	float3 spotDir;
	float  depthBias;
};

struct DirectionalLight
{	// 32 bytes
	float3 lightDirection;
	float  brightness;
	float3 color;
	float shadowFactor;
};

// defines maximum number of dynamic lights  todo: shader defines
// don't forget to update CPU define too (RenderPasses.h)
#define NUM_POINT_LIGHT	100
#define NUM_SPOT_LIGHT	20

#define NUM_POINT_LIGHT_SHADOW	5
#define NUM_SPOT_LIGHT_SHADOW	5

#define LIGHT_INDEX_SPOT	0
#define LIGHT_INDEX_POINT	1

struct SceneLighting	
{
	// non-shadow caster counts
	int numPointLights;  
	int numSpots;
	// shadow caster counts
	int numPointCasters;  
	int numSpotCasters;
	//----------------------------------------------
	DirectionalLight directional;
	matrix shadowViewDirectional;
	//----------------------------------------------
	PointLight point_lights[NUM_POINT_LIGHT];
	PointLight point_casters[NUM_POINT_LIGHT_SHADOW];
	//----------------------------------------------
	SpotLight spots[NUM_SPOT_LIGHT];
	SpotLight spot_casters[NUM_SPOT_LIGHT_SHADOW];
	//----------------------------------------------
	matrix shadowViews[NUM_SPOT_LIGHT_SHADOW];
};


//----------------------------------------------------------
// MATERIAL
//----------------------------------------------------------
inline int HasDiffuseMap(int textureConfig)  { return ((textureConfig & 1) > 0 ? 1 : 0); }
inline int HasNormalMap(int textureConfig)	 { return ((textureConfig & 2) > 0 ? 1 : 0); }
inline int HasSpecularMap(int textureConfig) { return ((textureConfig & 4) > 0 ? 1 : 0); }
inline int HasAlphaMask(int textureConfig)   { return ((textureConfig & 8) > 0 ? 1 : 0); }

struct SurfaceMaterial
{
    float3 diffuse;
    float alpha;

    float3 specular;
    float roughness;

    float metalness;
    float shininess;
	float2 uvScale;

	int textureConfig;
	int pad0, pad1, pad2;
};

//----------------------------------------------------------
// LIGHTING FUNCTIONS
//----------------------------------------------------------
inline float AttenuationBRDF(float2 coeffs, float dist)
{
	return 1.0f / (dist * dist);	// quadratic attenuation (inverse square) is physically more accurate
}

inline float AttenuationPhong(float2 coeffs, float dist)
{
	return 1.0f / (
		1.0f
		+ coeffs[0] * dist
		+ coeffs[1] * dist * dist
		);
}



// LearnOpenGL: PBR Lighting
//
// You may still want to use the constant, linear, quadratic attenuation equation that(while not physically correct) 
// can offer you significantly more control over the light's energy falloff.
inline float Attenuation(float3 coeffs, float dist)
{
    return 1.0f / (
		coeffs[0]
		+ coeffs[1] * dist
		+ coeffs[2] * dist * dist
	);
}


// spotlight intensity calculataion
float SpotlightIntensity(SpotLight l, float3 worldPos)
{   
	const float3 L         = normalize(l.position - worldPos);
	const float3 spotDir   = normalize(-l.spotDir);
	const float theta      = dot(L, spotDir);
	const float softAngle  = l.halfAngle * 1.25f;
	const float softRegion = softAngle - l.halfAngle;
	return clamp((theta - softAngle) / softRegion, 0.0f, 1.0f);
}

// todo: ESM - http://www.cad.zju.edu.cn/home/jqfeng/papers/Exponential%20Soft%20Shadow%20Mapping.pdf
struct ShadowTestPCFData
{
	//-------------------------
	float4 lightSpacePos;
	//-------------------------
	float  depthBias;
	float  NdotL;
	//...
	//-------------------------
};
float ShadowTestPCF(in ShadowTestPCFData pcfTestLightData, Texture2DArray shadowMapArr, SamplerState shadowSampler, float2 shadowMapDimensions, int shadowMapIndex)
//float ShadowTestPCF(float3 worldPos, float4 lightSpacePos, Texture2DArray shadowMapArr, int shadowMapIndex, SamplerState shadowSampler, float NdotL, float2 shadowMapDimensions)
{
	// homogeneous position after interpolation
	const float3 projLSpaceCoords = pcfTestLightData.lightSpacePos.xyz / pcfTestLightData.lightSpacePos.w;

	// frustum check
	if (projLSpaceCoords.x < -1.0f || projLSpaceCoords.x > 1.0f ||
		projLSpaceCoords.y < -1.0f || projLSpaceCoords.y > 1.0f ||
		projLSpaceCoords.z <  0.0f || projLSpaceCoords.z > 1.0f
		)
	{
		return 1.0f;
	}


	const float BIAS = pcfTestLightData.depthBias * tan(acos(pcfTestLightData.NdotL));
	float shadow = 0.0f;

    const float2 texelSize = 1.0f / (shadowMapDimensions);
	
	// clip space [-1, 1] --> texture space [0, 1]
	const float2 shadowTexCoords = float2(0.5f, 0.5f) + projLSpaceCoords.xy * float2(0.5f, -0.5f);	// invert Y
	const float pxDepthInLSpace = projLSpaceCoords.z;


	// PCF
	const int rowHalfSize = 2;
    for (int x = -rowHalfSize; x <= rowHalfSize; ++x)
    {
		for (int y = -rowHalfSize; y <= rowHalfSize; ++y)
        {
			float2 texelOffset = float2(x,y) * texelSize;
			float closestDepthInLSpace = shadowMapArr.Sample(shadowSampler, float3(shadowTexCoords + texelOffset, shadowMapIndex)).x;

			// depth check
			shadow += (pxDepthInLSpace - BIAS> closestDepthInLSpace) ? 1.0f : 0.0f;
        }
    }

    shadow /= (rowHalfSize * 2 + 1) * (rowHalfSize * 2 + 1);

	return 1.0 - shadow;
}



float ShadowTestPCF_Directional(float3 worldPos, float4 lightSpacePos, Texture2D shadowMap, SamplerState shadowSampler, float NdotL, float2 shadowMapDimensions)
{
	// homogeneous position after interpolation
	const float3 projLSpaceCoords = lightSpacePos.xyz / lightSpacePos.w;

	// frustum check
	if (projLSpaceCoords.x < -1.0f || projLSpaceCoords.x > 1.0f ||
		projLSpaceCoords.y < -1.0f || projLSpaceCoords.y > 1.0f ||
		projLSpaceCoords.z <  0.0f || projLSpaceCoords.z > 1.0f
		)
	{
		return 1.0f;
	}

	const float2 texelSize = 1.0f / (shadowMapDimensions);

	// clip space [-1, 1] --> texture space [0, 1]
	const float2 shadowTexCoords = float2(0.5f, 0.5f) + projLSpaceCoords.xy * float2(0.5f, -0.5f);	// invert Y

	const float BIAS = SHADOW_BIAS * tan(acos(NdotL));
	const float pxDepthInLSpace = projLSpaceCoords.z;

	float shadow = 0.0f;
	const int rowHalfSize = 2;

	// PCF
	for (int x = -rowHalfSize; x <= rowHalfSize; ++x)
	{
		for (int y = -rowHalfSize; y <= rowHalfSize; ++y)
		{
			float2 texelOffset = float2(x, y) * texelSize;
			float closestDepthInLSpace = shadowMap.Sample(shadowSampler, shadowTexCoords + texelOffset).x;

			// depth check
			shadow += (pxDepthInLSpace - BIAS> closestDepthInLSpace) ? 1.0f : 0.0f;
		}
	}

	shadow /= (rowHalfSize * 2 + 1) * (rowHalfSize * 2 + 1);
	return 1.0 - shadow;
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
