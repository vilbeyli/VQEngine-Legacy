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
#if 1
    return float3x3(
		  1.0f,  0.0f,  t.y
		, 0.0f,  t.z ,  0.0f
	    , t.w ,  0.0f,  t.x
	);
#else
    return float3x3(
                float3(t.x, 0, t.y),
                float3(0, 1, 0),
                float3(t.z, 0, t.w)
            );
#endif
}
