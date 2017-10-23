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

#include "LightingCommon.hlsl"
#include "BRDF.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};


// IMAGE-BASED LIGHTING - BRDF Response Pre-Calculation
// ===========================================================================================================================
// src: 
//	https://learnopengl.com/#!PBR/IBL/Specular-IBL
//	http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
// ---------------------------------------------------------------------------------------------------------
// The focus is on the specular part of the lighting equation
//
//		L_o(p, w_o) = Integral[ fr(p, w_i, w_o) * L_i(p, w_i) * (n . w_i) * dw_i]
//
//   where
//
//		fr(p, w_i, w_o) = DFG / ( 4 * (w_o . n) * (w_i . n) )
//
// The integral cannot be solved in real-time so we precompute the integral.
// The problem is, the specular integral depends on w_i and w_o (p irrelevant). 
// Unlike the irradiance integral, it doesn't depend only on w_i.
//
// Epic Games' split sum approximation solves the issue by splitting the pre-computation into 2 individual parts 
// that we can later combine to get the resulting pre-computed result we're after. The split sum approximation 
// splits the specular integral into two separate integrals:
//
//		L_o(p, w_o) = Integral[ L(p, w_i) * d_wi] * Integral[ fr(p, w_i, w_o) * (n . w_i) * dw_i]
//
// The first integral (when convoluted) is known as the pre-filtered environment map
// The second integral is the BRDF part of the specular integral, assuming L(p, w_i) = 1.0 (constant radiance).
//
// The idea is to pre-calculate the BRDF's response given an input roughness and an input angle between 
// the surface normal N and the incoming light direction w_i. The BRDF Integration map contains a 
// scale and bias vector to the F0 term of Fresnel in red and green channels, respectively. 
// Cleverly rearranging the BRDF Integral, we get the split sum integrals for the fr() (see link for details).
//
//		Integral[ fr(p, w_i, w_o) * (n . w_i) * dw_i] = F0 * Integral1 + Integral2
//
//		Integral1 = Integral[ fr(p, w_i, w_o) * (1 - (1 - (w_o . h))^5) * (n . w_i) * dw_i]
//		Integral2 = Integral[ fr(p, w_i, w_o) * (1 - (w_o . h))^5 * (n . w_i) * dw_i]
//
// The two resulting integrals represent a scale and a bias to F0 respectively. 
// Note that as fr(p, w_i, w_o) already contains a term for F they both cancel out, removing F from fr. (??)
//
//

#define SAMPLE_COUNT 1024
#define USE_BIT_MANIPULATION

// the Hammersley Sequence,a random low-discrepancy sequence based on the Quasi-Monte Carlo method as carefully described by Holger Dammertz. 
// It is based on the Van Der Corpus sequence which mirrors a decimal binary representation around its decimal point.
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
#ifdef USE_BIT_MANIPULATION
float RadicalInverse_VdC(uint bits) //  Van Der Corpus
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

#else
// the non-bit-manipulation version of the above function
float VanDerCorpus(uint n, uint base)
{
    float invBase = 1.0 / float(base);
    float denom = 1.0;
    float result = 0.0;

    for (uint i = 0u; i < 32u; ++i)
    {
        if (n > 0u)
        {
            denom = n % 2.0f;
            result += denom * invBase;
            invBase = invBase / 2.0;
            n = uint(float(n) / 2.0);
        }
    }

    return result;
}
#endif

float2 Hammersley(int i, int count)
{
#ifdef USE_BIT_MANIPULATION
	return float2(float(i) / float(count), RadicalInverse_VdC(i));
#else
	return float2(float(i) / float(count), VanDerCorpus(i, 2u));
#endif
}

// Instead of uniformly or randomly (Monte Carlo) generating sample vectors over the integral's hemisphere, we'll generate 
// sample vectors biased towards the general reflection orientation of the microsurface halfway vector based on the surface's roughness. 
// This gives us a sample vector somewhat oriented around the expected microsurface's halfway vector based on some input roughness 
// and the low-discrepancy sequence value Xi. Note that Epic Games uses the squared roughness for better visual results as based on 
// Disney's original PBR research.
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness)
{
    const float a = roughness * roughness;

    const float phi = 2.0f * PI * Xi.x;
    const float cosTheta = sqrt((1.0f - Xi.y) / (1.0f + (a*a - 1.0f)) * Xi.y);
    const float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

	// from sphreical coords to cartesian coords
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

	// from tangent-space to world space
    const float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    const float3 tangent = normalize(cross(up, N));
    const float3 bitangent = cross(N, tangent);

    const float3 sample = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sample);
}

float2 IntegrateBRDF(float NdotV, float roughness)
{
    //return float2(NdotV, roughness);

    float3 V;
    V.x = sqrt(1.0f - NdotV * NdotV);
    V.y = 0;
    V.z = NdotV;

    float F0Scale = 0;	// Integral1
    float F0Bias = 0;	// Integral2
	
    const float3 N = float3(0, 0, 1);
    for (int i = 0; i < SAMPLE_COUNT; ++i)
    {
        const float2 Xi = Hammersley(i, SAMPLE_COUNT);
        const float3 H = ImportanceSampleGGX(Xi, N, roughness);
        const float3 L = normalize(reflect(V, H));

        const float NdotL = max(L.z, 0);
        const float NdotH = max(H.z, 0);
        const float VdotH = max(dot(V, H), 0);

		if(NdotL > 0.0f)
        {
            const float G = GeometryEnvironmentMap(N, V, L, roughness);
            const float G_Vis = (G * VdotH) / ((NdotH * NdotV) + 0.0001);
            const float Fc = pow(1.0 - VdotH, 5.0f);

			F0Scale += (1.0f - Fc) * G_Vis;
            F0Bias += Fc * G_Vis;
        }
    }
    return float2(F0Scale, F0Bias) / SAMPLE_COUNT;
}

float4 PSMain(PSIn In) : SV_TARGET
{
	float2 integratedBRDF = IntegrateBRDF(In.uv.x, In.uv.y);
	return float4(integratedBRDF, 0, 1);
}
