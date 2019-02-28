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



// LINEARLY TRANSFORMED COSINES (LTC)
//
// "Real-Time Polygonal-Light Shading with Linearly Transformed Cosines", by Eric Heitz et al,
// https://eheitzresearch.wordpress.com/415-2/
//
// sources: 
//
// - GPU Zen
// - https://blog.selfshadow.com/publications/s2017-shading-course/#course_content
// - Paper: https://drive.google.com/file/d/0BzvWIdpUpRx_d09ndGVjNVJzZjA/view
// - Slides: https://drive.google.com/file/d/0BzvWIdpUpRx_Z2pZWWFtam5xTFE/view
//


#include "ShadingMath.hlsl"

// Illumination = Integral( BRDF * cos(theta) * dw );
//
// D = BRDF * cos
//
float D(float3 w, in float3x3 Minv)
{
	// brdf * cos
    float3 wo = mul(Minv, w);
    float lo = length(wo);
    return 1.0f / PI * (saturate(wo.z / lo) * abs(determinant(Minv))) / (lo * lo * lo);
}

float D(float3 w)
{
    float3x3 Minv = float3x3(1, 0, 0, 0, 1, 0, 0, 0, 1);
    return D(w, Minv);
}

//--------------------------------------------------------------------------------
// UTILITIES
//--------------------------------------------------------------------------------
float SamplePhi(int i, int iMax) { return TWO_PI * float(i) / float(iMax); }

// code from [Frisvad2012]
void BuildOrthonormalBasis(float3 N, out float3 T, out float3 B)
{
	if(N.z < -0.999999)
    {
        T = float3(0, -1, 0);
        B = float3(-1, 0, 0);
        return;
    }
    float a = 1.0f / (1.0f + N.z);
    float b = -N.x * N.y * a;
    T = float3(1.0f - N.x * N.x * a, b, -N.x);
    B = float3(b, 1.0f - N.y * N.y * a, -N.y);
}

// LUT is 32x32
#define LTC_LUT_SIZE 32.0f
float2 GetLTC_UVs(float cosTheta, float roughness)
{
    const float theta = acos(cosTheta);
    float2 uv = float2(roughness, theta / (0.5f - PI));

	// scale + bias for proper filtering
    uv = uv * (LTC_LUT_SIZE - 1.0f) / LTC_LUT_SIZE + (0.5 / LTC_LUT_SIZE);
    return uv;
}

float3x3 GetLTC_Matrix(Texture2D texLUT, SamplerState linearSampler, float2 uv)
{
    const float4 t = texLUT.Sample(linearSampler, uv);
    return float3x3(
		  1.0f,  0.0f,  t.y
		, 0.0f,  t.z ,  0.0f
	    , t.w ,  0.0f,  t.x
	);
}

//--------------------------------------------------------------------------------
// NUMERICAL INTEGRATIONS
//--------------------------------------------------------------------------------
float I_cylinder_numerical(float3 p1, float3 p2, float R, in float3x3 matTBN)
{
	// Initialize the orthonormal basis vectors
    float L = length(p2 - p1);
    float3 wt = normalize(p2 - p1);
    float3 wt1, wt2;
    BuildOrthonormalBasis(wt, wt1, wt2);
	
	// illumination
    float I = 0.0f; 

	// integral discretization
    const int nSamplesPhi = 20;
    const int nSamplesL   = 100;
    for (int i = 0; i < nSamplesPhi; ++i)
    {
        for (int j = 0; j < nSamplesL; ++j)
        {
			// normal vector
            float phi = SamplePhi(i, nSamplesPhi);
            float3 wn = cos(phi) * wt1 + sin(phi) * wt2;

			// position
            float l = L * float(j) / float(nSamplesL);
            float3 p = p1 + l * wt + R * wn;

			// normalize the direction
            float3 wp = normalize(p);

            //wp = mul(matTBN, wp);
            //wn = mul(matTBN, wn);
            //wp = mul(wp, matTBN);
            //wn = mul(wn, matTBN);

			// integrate
            I += D(wp) * saturate(dot(-wp, wn)) / dot(p, p);
            //I += D(wp, matTBN) * saturate(dot(-wp, wn)) / dot(p, p);
            //I += D(wp, transpose(matTBN)) * saturate(dot(-wp, wn)) / dot(p, p);
        }
    }

    I *= TWO_PI * R * L / float(nSamplesPhi * nSamplesL);
	return I;
}

float I_line_numerical(float3 p1, float3 p2)
{
    float L = length(p2 - p1);
    float3 wt = normalize(p2 - p1);

	// illumination
    float I = 0.0f;

	// integral discretization
    const int nSamples = 100;
    for (int i = 0; i < nSamples; ++i)
    {
		// position
        float3 p = p1 + L + float(i) / float(nSamples - 1) * wt;

		// normalize the direction
        float wp = normalize(p);

		// integrate
        I += 2.0f * D(wp) * length(cross(wp, wt)) / dot(p, p);
    }

    I *= L / float(nSamples);
    return I;
}


float I_disks_numerical(float3 p1, float3 p2, float R)
{
	// orthonormal base
    float L = length(p2 - p1);
    float3 wt = normalize(p2 - p1);
    float3 wt1, wt2;
    BuildOrthonormalBasis(wt, wt1, wt2);

	// illumination
    float Idisks = 0.0f;

	// integration
    const int nSamplesPhi = 20;
    const int nSamplesR = 200;
    for (int i = 0; i < nSamplesPhi; ++i)
    {
        for (int j = 0; j < nSamplesR; ++j)
        {
            float phi = SamplePhi(i, nSamplesPhi);
            float r = R * float(j) / float(nSamplesR - 1);
            float3 p, wp;

            p = p1 + r * (cos(phi) * wt1 + sin(phi) * wt2);
            wp = normalize(p);
            Idisks += r * D(wp) * saturate(dot(wp, +wt)) * dot(p, p);
			
            p = p2 + r * (cos(phi) * wt1 + sin(phi) * wt2);
            wp = normalize(p);
            Idisks += r * D(wp) * saturate(dot(wp, -wt)) * dot(p, p);
        }
    }

    Idisks *= TWO_PI * R / float(nSamplesPhi * nSamplesR);
	return Idisks;
}

// Most accurate with:
//
// - cylinders of small radius
// - cylinders far from the shading point
// - low frequency sampling (high roughness materials)
//
float I_cylinder_approx(float3 p1, float3 p2, float R)
{
	// if the radius @R is small enough, we can use the line integral as an approximation.
	// 
	//		I_cyl(R) ~ R * I_line
	//
	// In cases where @R is too large, the approximation could be inaccurate and overshoot.
	// As we use normalized distributions, we can clamp it to 1.0f to prevent overly high values.
	//
    return min(1.0f, R * I_line_numerical(p1, p2));
}



//--------------------------------------------------------------------------------
// LTC INTEGRATIONS
//--------------------------------------------------------------------------------
float Fpo(float d, float l)
{
    const float d2 = d * d; 
	const float l2 = l * l;
    return l / (d * (d2 + l2)) + atan(l / d) / d2;
}
float Fwt(float d, float l)
{
    const float d2 = d * d;
    const float l2 = l * l;
    return l2 / (d * (d2 + l2));
}
float I_diffuse_line(float3 p1, float3 p2)
{
	// tangent
    float3 wt = normalize(p2 - p1);

	// clamping
    if (p1.z <= 0.0f && p2.z < 0.0f)	return 0.0f;	// clamp below horizon
	if (p1.z <= 0.0f)	p1 = (+p1 * p2.z - p2 * p1.z) / (+p2.z - p1.z);
    if (p1.z <= 0.0f)	p2 = (-p1 * p2.z + p2 * p1.z) / (-p2.z + p1.z);

	// shading point orthonormal projection on line
    float3 po = p1 - wt * dot(p1, wt);

	// distance to line
    float d = length(po);

	// parameterization
    float l1 = dot(p1 - po, wt);
    float l2 = dot(p2 - po, wt);

	// integral
    float I =	(Fpo(d, l2) - Fpo(d, l1)) * po.z +
				(Fwt(d, l2) - Fwt(d, l1)) * wt.z;

    return I / PI;
}


float I_ltc_line(float3 p1, float3 p2, float3x3 Minv)
{
    float3 p1o = mul(Minv, p1);
    float3 p2o = mul(Minv, p2);

    float I_diffuse = I_diffuse_line(p1o, p2o);

    float3 ortho = normalize(cross(p1, p2));
    float w = 1.0f / length(mul(inverse(transpose(Minv)), ortho));

	return w * I_diffuse;
}

float I_ltc_line_rectangle(float3 p1, float3 p2, float3 wn, float3x3 Minv)
{
    float3 wortho = normalize(cross(p1, p2));
    float I = abs(dot(wortho, wn)) * I_ltc_line(p1, p2, Minv);
    return I;
}

float I_ltc_disks(float3 p1, float3 p2, float R, float3x3 Minv)
{
    float A = PI * R * R;
	
    float3 wt = normalize(p2 - p1);
    float3 wp1 = normalize(p1);
    float3 wp2 = normalize(p2);
	
    float Idisks = A * (
			D(wp1, Minv) * saturate(dot(+wt, wp1)) / dot(p1, p2) +
			D(wp2, Minv) * saturate(dot(-wt, wp2)) / dot(p2, p2)
		);
	return Idisks;
}




//--------------------------------------------------------------------------------
// AREA LIGHT ILLUMINATION
//--------------------------------------------------------------------------------
float f(float NdotV, float roughness)
{
    const float2 ltc_uvs = GetLTC_UVs(NdotV, roughness);
    //const float3x3 Minv = GetLTC_Matrix(texLUT, smpLinear, ltc_uvs);

}

// determines if the space of @s.N and @V vectors 
// will be in view space or world space.
#define LTC_USE_VIEW_SPACE 1
float3 EvalCylinder(BRDF_Surface s, float3 V, CylinderLight l, Texture2D texLUT, SamplerState smpLinear, in matrix matView)
{
	// get inverse transform matrix
    const float NdotV = saturate(dot(s.N, V));
    const float2 ltc_uvs = GetLTC_UVs(NdotV, s.roughness);
    const float3x3 Minv = GetLTC_Matrix(texLUT, smpLinear, ltc_uvs);

	// generate cylidner points
    float3 halfHeightVec = float3(l.height * 0.5f, 0, 0);
    //float3 halfHeightVec = float3(0, l.height * 0.5f, 0);
    //float3 halfHeightVec = float3(0, 0, l.height * 0.5f);

    float4 p1W = float4(l.position + halfHeightVec, 1.0f);
    float4 p2W = float4(l.position - halfHeightVec, 1.0f);
    
    float3 T1 = normalize(V - dot(s.N, V) * s.N);
    float3 T2 = cross(s.N, T1);
    float3x3 matTBN = float3x3(T1, T2, s.N);

	
#if LTC_USE_VIEW_SPACE
    float3 p1V = mul(matView, p1W);
    float3 p2V = mul(matView, p2W);
    float3 p1 = mul(float3(p1V.xyz - s.P.xyz), matTBN);
    float3 p2 = mul(float3(p2V.xyz - s.P.xyz), matTBN);
#else
    float3 p1 = mul(float3(p1W.xyz - s.P.xyz), matTBN);
    float3 p2 = mul(float3(p2W.xyz - s.P.xyz), matTBN);
#endif
	
    float3 wp1 = normalize(p1);
    float3 wp2 = normalize(p2);
    
    //return -normalize(p1W - s.P);
    //return NdotV;
    //return V / 10;
	//return s.P/ 1000;
	
    //return NdotV.xxx;

#if 0
    float3 I = l.brightness * l.color * I_ltc_line(p1, p2, Minv);
#else
    float3 I = l.brightness * l.color * I_cylinder_numerical(p1, p2, l.radius, matTBN);
#endif

    //return 0.0f; // temp
    //return max(0.0f, min(1.0f, I));
    return max(0.0f, I);
    //return I;
    //return abs(I);
}