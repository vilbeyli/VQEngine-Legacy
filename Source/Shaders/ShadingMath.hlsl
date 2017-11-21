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

// Method for Hammersley Sequence creation
#define USE_BIT_MANIPULATION	
#define PI 3.14159265359f

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

float2 SphericalSample(float3 v)
{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb509575(v=vs.85).aspx
	// The signs of the x and y parameters are used to determine the quadrant of the return values 
	// within the range of -PI to PI. The atan2 HLSL intrinsic function is well-defined for every point 
	// other than the origin, even if y equals 0 and x does not equal 0.
    float2 uv = float2(atan2(v.z, v.x), asin(-v.y));
    uv /= float2(2 * PI, PI);
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
#ifdef USE_BIT_MANIPULATION
    return float2(float(i) / float(count), RadicalInverse_VdC(uint(i)));
#else
	// note: this crashes for some reason.
    return float2(float(i) / float(count), VanDerCorpus(uint(i), 2u));
#endif
}