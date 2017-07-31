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

#define PI 3.14159265359f
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

}

// spotlight intensity calculataion
float Intensity(Light l, float3 worldPos)
{   
	float3 L = normalize(l.position - worldPos);
	float3 spotDir = normalize(-l.spotDir);
	float theta = dot(L, spotDir);
	float softAngle = l.halfAngle * 1.25f;
	float softRegion = softAngle - l.halfAngle;
	return clamp((theta - softAngle) / softRegion, 0.0f, 1.0f);
}



inline float3 UnpackNormals(float2 uv, float3 worldNormal, float3 worldTangent)
{
	// uncompressed normal in tangent space
	float3 SampledNormal = gNormalMap.Sample(sShadowSampler, uv).xyz;
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
	if (pxDepthInLSpace - 0.000001 > closestDepthInLSpace)
	{
		return 0.0f;
	}

	return 1.0f;
}

// ================================== BRDF NOTES =========================================
//
//	src: https://learnopengl.com/#!PBR/Theory
//
// Rendering Equation
// ------------------
//
// L_o(p, w_o) = L_e(p) + Integral_Omega()[f_r(p, w_i, w_o) * L_i(p, w_i) * dot(N, w_i)]dw_i
//
// L_o		: Radiance leaving surface point P along the direction w_o (V=eye)
// L_e		: Emissive Radiance leaving surface point p | we're not gonna use emissive materials for now 
// L_i		: Irradiance (Radiance coming) coming from a light source at point p 
// f_r()	: reflectance equation representing the material property
// Integral : all incoming radiance over hemisphere omega, reflected towards the direction of w_o<->(V=eye) 
//			  by the surface material
//
// The integral is numerically solved.
// 
// BRDF						: Bi-directional Reflectance Distribution Function
// The Cook-Torrence BRDF	: f_r = k_d * f_lambert + k_s * f_cook-torrence
//							f_lambert = albedo / PI;

// Trowbridge-Reitz GGX Distribution
inline float NormalDistribution(float3 N, float3 H, float alpha)
{	// approximates microfacets :	approximates the amount the surface's microfacets are
	//								aligned to the halfway vector influenced by the roughness
	//								of the surface

	// NDF_GGXTR(N, H, alpha) = alpha^2 / ( PI * ( dot(N, H))^2 * (alpha^2 - 1) + 1 )^2
	const float a2 = alpha * alpha;
	const float nh2 = pow(max(dot(N, H), 0), 2);
	return a2 / (PI * pow((nh2 * (a2 - 1) + 1), 2));
}

// Smith's Schlick-GGX
inline float Geometry_Smiths_SchlickGGX(float3 N, float3 V, float alpha)
{	// describes self shadowing of geometry
	//
	// G_ShclickGGX(N, V, k) = ( dot(N,V) ) / ( dot(N,V)*(1-k) + k )
	//
	// k		 :	remapping of alpha based on wheter we're using geometry function 
	//				for direct lighting or IBL
	// k_direct	 = (alpha + 1)^2 / 8
	// k_IBL	 = alpha^2 / 2
	//
#ifdef DIRECT_LIGHTING
	const float k = pow((alpha + 1), 2) / 8.0f;
#else	// IBL
	const float k = pow(alpha, 2) / 2.0f;
#endif
	const float NV = max(0.0f, dot(N, V));
	return NV / (NV * (1.0f - k) + k);
}

inline float Geometry(float3 N, float3 V, float3 L, float k)
{	// essentially a multiplier [0, 1] measuring microfacet shadowing
	return Geometry_Smiths_SchlickGGX(N, V, k) * Geometry_Smiths_SchlickGGX(N, L, k);
}

// Fresnel-Schlick approximation
inline float3 Fresnel(float3 N, float3 V)
{	// descrribes reflection
	//
	// F_Schlick(N, V, F0) = F0 - (1-F0)*(1 - dot(N,V))^5
	//
	// approx F0=vec3(0.4f), see src for more
	const float3 F0 = float3(0.4f, 0.4f, 0.4f);

	return F0 + (float3(1,1,1) - F0) * pow(1.0f - max(0.0f, dot(N, V)), 5.0f);
}

#define _DEBUG
#ifdef _DEBUG
float3 F_CookTorrence(float3 Wo, float3 N, float3 Wi, float alpha)
{
	const float3 H = normalize(Wo + Wi);
	float3 fresnel = Fresnel(N, Wo);
	float geometry = Geometry(N, H, Wi, alpha);
	float normalDistr = NormalDistribution(N, H, alpha);
	float denom = (4.0f * max(0.0f, dot(Wo, N)) * max(0.0f, dot(Wi, N)));
	return  normalDistr * fresnel * geometry / denom;
}
#else
inline float3 F_CookTorrence(float3 Wo, float3 N, float3 Wi, float alpha)
{
	const float3 H = normalize(Wo + Wi);
	return NormalDistribution(N, H, alpha) * Fresnel(N, Wo) * Geometry(N, H, Wi, alpha) / (4.0f * max(0.0f, dot(Wo, N)) * max(0.0f, dot(Wi, N)));
}
#endif

inline float3 F_LambertDiffuse(float3 kd)
{
	return kd / PI;
}

float3 BRDF(Light light, Surface s, float3 V, float3 worldPos)
{
	const float3 Wo = V;
	const float3 Wi = normalize(light.position - worldPos);
	const float3 N  = s.N;
	const float NL = max(0.0f, dot(N, Wi));

	const float3 kd = s.diffuseColor;
	const float3 ks = s.specularColor;
	const float  shininess = s.shininess;

	float3 Id = F_LambertDiffuse(kd);
	float3 Is = ks * F_CookTorrence(Wo, N, Wi, shininess);

	return light.color * (Id + Is * 0.00000001f) * NL;
}
// ================================== BRDF END =========================================

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters
	const float3 N = normalize(In.normal);
	const float3 T = normalize(In.tangent);
	const float3 V = normalize(cameraPos - In.worldPos);
	const float ambient = 0.035f;
	const float gamma = 2.2;

	Surface s;	// surface 
	s.N = (isNormalMap)* UnpackNormals(In.texCoord, N, T) +
		(1.0f - isNormalMap) * N;
	s.diffuseColor = diffuse *  (isDiffuseMap          * gDiffuseMap.Sample(sShadowSampler, In.texCoord).xyz +
		(1.0f - isDiffuseMap)   * float3(1,1,1));
	s.specularColor = specular;
	s.shininess = shininess * SHININESS_ADJUSTER;

	// illumination
	const float3 Ia = s.diffuseColor * ambient;	// ambient
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);		// diffuse & specular
	for (int i = 0; i < lightCount; ++i)		// POINT Lights
		IdIs += BRDF(lights[i], s, V, In.worldPos) * Attenuation(lights[i].attenuation, length(lights[i].position - In.worldPos));

	for (int j = 0; j < spotCount; ++j)			// SPOT Lights
		IdIs += BRDF(spots[j], s, V, In.worldPos) * Intensity(spots[j], In.worldPos) * ShadowTest(In.worldPos, In.lightSpacePos);

	const float3 illumination = Ia + IdIs;
	
	const float4 outColor = float4(illumination, 1);
	return pow(outColor, float4(gamma, gamma, gamma, 1.0f));
}