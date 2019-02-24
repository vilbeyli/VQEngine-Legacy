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

#ifndef _SHADING_MATH_H
#define _SHADING_MATH_H

// Method for Hammersley Sequence creation
#define USE_BIT_MANIPULATION	

// constants
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define PI_OVER_TWO 1.5707963268f
#define PI_SQUARED 9.86960440109f


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


// additional sources: 
// - Converting to/from cubemaps: http://paulbourke.net/miscellaneous/cubemaps/
// - Convolution: https://learnopengl.com/#!PBR/IBL/Diffuse-irradiance
// - Projections: https://gamedev.stackexchange.com/questions/114412/how-to-get-uv-coordinates-for-sphere-cylindrical-projection
float2 SphericalSample(float3 v)
{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb509575(v=vs.85).aspx
	// The signs of the x and y parameters are used to determine the quadrant of the return values 
	// within the range of -PI to PI. The atan2 HLSL intrinsic function is well-defined for every point 
	// other than the origin, even if y equals 0 and x does not equal 0.
    float2 uv = float2(atan2(v.z, v.x), asin(-v.y));
    uv /= float2(-TWO_PI, PI);
    uv += float2(0.5, 0.5);
    return uv;
}

// the Hammersley Sequence,a random low-discrepancy sequence based on the Quasi-Monte Carlo method as carefully described by Holger Dammertz. 
// It is based on the Van Der Corpus sequence which mirrors a decimal binary representation around its decimal point.
// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html 
// https://www.scratchapixel.com/lessons/mathematics-physics-for-computer-graphics/monte-carlo-methods-in-practice/introduction-quasi-monte-carlo
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
    return float2(float(i) / float(count), 
#ifdef USE_BIT_MANIPULATION
		RadicalInverse_VdC(uint(i))
#else
		VanDerCorpus(uint(i), 2u)	// note: this crashes for some reason.
#endif
	);
}

inline float LinearDepth(in float zBufferSample, in float A, in float B)
{
	// src:
	// https://developer.nvidia.com/content/depth-precision-visualized
	// http://dev.theomader.com/linear-depth/
	// https://www.mvps.org/directx/articles/linear_z/linearz.htm
	// http://www.humus.name/index.php?ID=255
    return A / (zBufferSample - B);
}
inline float LinearDepth(in float zBufferSample, in matrix matProjInverse) { return LinearDepth(zBufferSample, matProjInverse[2][3], matProjInverse[2][2]); }

float3 ViewSpacePosition(in const float nonLinearDepth, const in float2 uv, const in matrix invProjection)
{
	// src: 
	// https://mynameismjp.wordpress.com/2009/03/10/reconstructing-position-from-depth/
	// http://www.derschmale.com/2014/01/26/reconstructing-positions-from-the-depth-buffer/

	const float x = uv.x * 2 - 1;			// [-1, 1]
    const float y = (1.0f - uv.y) * 2 - 1;	// [-1, 1]
	const float z = nonLinearDepth;			// [ 0, 1]

	float4 projectedPosition = float4(x, y, z, 1);
	float4 viewSpacePosition = mul(invProjection, projectedPosition);
	return viewSpacePosition.xyz / viewSpacePosition.w;
}

float4x4 inverse(float4x4 m)
{
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}

float3x3 inverse(float3x3 m)
{
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2];

    float t11 = - n23 * n32 + n22 * n33;
    float t12 = + n13 * n32 - n12 * n33;
    float t13 = - n13 * n22 + n12 * n23;

    float det = n11 * t11 + n21 * t12 + n31 * t13;
    float idet = 1.0f / det;

    float3x3 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (+ n23 * n31 - n21 * n33) * idet;
    ret[0][2] = (- n22 * n31 + n21 * n32) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (- n13 * n31 + n11 * n33) * idet;
    ret[1][2] = (+ n12 * n31 - n11 * n32) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (+ n13 * n21 - n11 * n23) * idet;
    ret[2][2] = (- n12 * n21 + n11 * n22) * idet;

    return ret;
}


#endif