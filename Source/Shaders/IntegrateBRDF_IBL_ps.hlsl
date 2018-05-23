//	VQEngine | DirectX11 Renderer
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
//	https://chetanjags.wordpress.com/2015/08/26/image-based-lighting/
//	http://www.trentreed.net/blog/physically-based-shading-and-image-based-lighting/
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

float4 PSMain(PSIn In) : SV_TARGET
{
	float2 integratedBRDF = IntegrateBRDF(In.uv.x, 1.0f - In.uv.y);
	return float4(integratedBRDF, 0, 1);
}
