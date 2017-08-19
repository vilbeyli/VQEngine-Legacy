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
#define PI 3.14159265359f
#define EPSILON 0.000000000001f

// defines
#define _DEBUG
#define DIRECT_LIGHTING

// ================================== BRDF NOTES =========================================
//
//	src: https://learnopengl.com/#!PBR/Theory
//	ref: http://blog.selfshadow.com/publications/s2012-shading-course/hoffman/s2012_pbs_physics_math_notes.pdf
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
inline float NormalDistribution(float3 N, float3 H, float roughness)
{	// approximates microfacets :	approximates the amount the surface's microfacets are
	//								aligned to the halfway vector influenced by the roughness
	//								of the surface
	//							:	determines the size, brightness, and shape of the specular highlight
	// more: http://reedbeta.com/blog/hows-the-ndf-really-defined/
	//
	// NDF_GGXTR(N, H, roughness) = roughness^2 / ( PI * ( dot(N, H))^2 * (roughness^2 - 1) + 1 )^2
	const float a = roughness * roughness;
	const float a2 = a * a;
	const float nh2 = pow(max(dot(N, H), 0), 2);
	const float denom = (PI * pow((nh2 * (a2 - 1.0f) + 1.0f), 2));
	if (denom < EPSILON) return 1.0f;
#if 0
	return min(a2 / denom, 10);
#else
	return a2 / denom;
#endif
}

// Smith's Schlick-GGX
inline float Geometry_Smiths_SchlickGGX(float3 N, float3 V, float roughness)
{	// describes self shadowing of geometry
	//
	// G_ShclickGGX(N, V, k) = ( dot(N,V) ) / ( dot(N,V)*(1-k) + k )
	//
	// k		 :	remapping of roughness based on wheter we're using geometry function 
	//				for direct lighting or IBL
	// k_direct	 = (roughness + 1)^2 / 8
	// k_IBL	 = roughness^2 / 2
	//
#ifdef DIRECT_LIGHTING
	const float k = pow((roughness + 1.0f), 2) / 8.0f;
#else	// IBL
	const float k = pow(roughness, 2) / 2.0f;
#endif
	const float NV = max(0.0f, dot(N, V));
	const float denom = (NV * (1.0f - k) + k) + 0.0001f;
	//if (denom < EPSILON) return 1.0f;
	return NV / denom;
}

#ifdef _DEBUG
float Geometry(float3 N, float3 V, float3 L, float k)
{	// essentially a multiplier [0, 1] measuring microfacet shadowing
	float geomNV = Geometry_Smiths_SchlickGGX(N, V, k);
	float geomNL = Geometry_Smiths_SchlickGGX(N, L, k);
	return  geomNV * geomNL;
}
#else
inline float Geometry(float3 N, float3 V, float3 L, float k)
{	// essentially a multiplier [0, 1] measuring microfacet shadowing
	return  Geometry_Smiths_SchlickGGX(N, V, k) * Geometry_Smiths_SchlickGGX(N, L, k);
}
#endif

// Fresnel-Schlick approximation descrribes reflection
inline float3 Fresnel(float3 N, float3 V, float3 F0)
{	// F_Schlick(N, V, F0) = F0 - (1-F0)*(1 - dot(N,V))^5
	return F0 + (float3(1,1,1) - F0) * pow(1.0f - max(0.0f, dot(N, V)), 5.0f);
}

inline float3 F_LambertDiffuse(float3 kd)
{
	return kd / PI;
}

float3 BRDF(float3 Wi, Surface s, float3 V, float3 worldPos)
{
	// vectors
	const float3 Wo = V;
	const float3 N  = s.N;
	const float3 H = normalize(Wo + Wi);
	const float  NL = max(0.0f, dot(N, Wi));

	// surface
	const float3 albedo = s.diffuseColor;
	const float  roughness = s.roughness;
	const float  metalness = s.metalness;
	const float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metalness);	

	// Fresnel_Cook-Torrence BRDF
	const float3 F   = Fresnel(H, V, F0);
	const float  G   = Geometry(N, Wo, Wi, roughness);
	const float  NDF = NormalDistribution(N, H, roughness);
	const float  denom = (4.0f * max(0.0f, dot(Wo, N)) * max(0.0f, dot(Wi, N))) + 0.0001f;
	const float3 specular = NDF * F * G / denom;
	const float3 Is = specular * s.specularColor;

	// Diffuse BRDF
	const float3 kS = F;
	const float3 kD = (float3(1, 1, 1) - kS) * (1.0f - metalness) * albedo;
	const float3 Id = F_LambertDiffuse(kD);
	
	return (Id + Is) * NL;
}