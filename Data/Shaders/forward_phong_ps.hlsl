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
// defines maximum number of dynamic lights
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

SamplerState samAnisotropic
{
    Filter = ANISOTROPIC;
    MaxAnisotropy = 4;
    AddressU = WRAP;
    AddressV = WRAP;
};

// FUNCTIONS
//---------------------------------------------------------
// point light attenuation equation
float Attenuation(float2 coeffs, float dist)
{
	return 1.0f / (
		1.0f
	+	coeffs[0] * dist
	+	coeffs[1] * dist * dist
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

// returns diffuse and specular light
float3 Phong(Light light, Surface s, float3 V, float3 worldPos)
{
    float3 N = s.N;
    float3 L = normalize(light.position - worldPos);
    float3 R = normalize(2 * N * dot(N, L) - L);
	//float3 R = normalize(2*N*max(dot(N, L), 0.0f) - L);
	//if (dot(R,L) < -0.99f)
	//	R = float3(0, 0, 0);

    float diffuse = max(0.0f, dot(N, L));   // lights
    float3 Id = light.color * s.diffuseColor  * diffuse;
    float3 Is = light.color * s.specularColor * pow(max(dot(R, V), 0.0f), s.shininess) * diffuse;
	//float3 Is = light.color * pow(max(dot(R, V), 0.0f), 240) ;

    return Id + Is;
}

inline float3 UnpackNormals(float2 uv, float3 worldNormal, float3 worldTangent)
{
	// uncompressed normal in tangent space
	float3 SampledNormal = gNormalMap.Sample(samAnisotropic, uv).xyz;
	SampledNormal = normalize(SampledNormal * 2.0f - 1.0f);

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
	float closestDepthInLSpace = gShadowMap.Sample(samAnisotropic, shadowTexCoords).x;

	// frustum check
	if ( projLSpaceCoords.x < -1.0f || projLSpaceCoords.x > 1.0f ||
		 projLSpaceCoords.y < -1.0f || projLSpaceCoords.y > 1.0f ||
		 projLSpaceCoords.z <  0.0f || projLSpaceCoords.z > 1.0f
		)
	{
		return -10.0f;
	}

	if (pxDepthInLSpace > closestDepthInLSpace)
	{
		return 0.0f;
	}

	return 1.0f;
	
}

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters
    float3 N = normalize(In.normal);
    float3 T = normalize(In.tangent);
    float3 V = normalize(cameraPos - In.worldPos);
    const float ambient = 0.00075f;
    Surface s;
    s.N             =   (isNormalMap       ) * UnpackNormals(In.texCoord, N, T) + 
                        (1.0f - isNormalMap) * N;
    s.diffuseColor  = diffuse *  ( isDiffuseMap          * gDiffuseMap.Sample(samAnisotropic, In.texCoord).xyz +
                                 (1.0f - isDiffuseMap)   * float3(1,1,1));
    s.specularColor = specular;
    s.shininess = shininess * SHININESS_ADJUSTER;

	// illumination
	float3 Ia = s.diffuseColor * ambient;	// ambient
    float3 IdIs = float3(0.0f, 0.0f, 0.0f);	// diffuse & specular
    for (int i = 0; i < lightCount; ++i)	// POINT Lights
        IdIs += Phong(lights[i], s, V, In.worldPos) * Attenuation(lights[i].attenuation, length(lights[i].position - In.worldPos));

    for (int j = 0; j < spotCount; ++j)		// SPOT Lights
        IdIs += Phong(spots[j], s, V, In.worldPos) * Intensity(spots[j], In.worldPos);

	float3 illumination = Ia + IdIs * ShadowTest(In.worldPos, In.lightSpacePos);
	float4 outColor = float4(illumination, 1);	// alpha

    // gamma correction
    const bool gammaCorrect = gammaCorrection > 0.99f;
    const float gamma = 1.0 / 2.2;
	if(gammaCorrect)	return pow(outColor, float4(gamma,gamma,gamma,1.0f));
	else        		return outColor;
}