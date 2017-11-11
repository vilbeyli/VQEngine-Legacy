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
	float4 HPos : SV_POSITION;
	float3 LPos : POSITION;
};

struct PSOut
{
	float4 cubeMapFace : SV_TARGET0;
};

Texture2D tEnvironmentMap;
SamplerState sLinear;

cbuffer cb
{
	float roughness;
};

float2 SphericalSample(float3 v)
{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb509575(v=vs.85).aspx
	// The signs of the x and y parameters are used to determine the quadrant of the return values 
	// within the range of -PI to PI. The atan2 HLSL intrinsic function is well-defined for every point 
	// other than the origin, even if y equals 0 and x does not equal 0.
	float2 uv = float2(atan2(v.z, v.x), asin(-v.y));
	uv /= float2(2*PI, PI);
	uv += float2(0.5, 0.5);
	return uv;
}

float4 Convolve(float3 N, float3 V, float3 R)
{
	const int PREFILTER_SAMPLE_COUNT = 1024;

	float totalWeight = 0.0f;
	float3 preFilteredColor = 0.0f.xxx;
	for(int i = 0; i < PREFILTER_SAMPLE_COUNT; ++i)
	{
		float2 Xi = Hammersley(i, PREFILTER_SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, max(0.04, roughness));
	    float3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = max(dot(N,L), 0.0f);
		if(NdotL > 0.0f)
	    {
			const float2 uv = SphericalSample(L);
			preFilteredColor += tEnvironmentMap.Sample(sLinear, uv).xyz;
			totalWeight += NdotL;
	    }
	}
	
    preFilteredColor /= max(totalWeight, 0.0001f);
    return float4(preFilteredColor, 1);
}

// source: https://learnopengl.com/#!PBR/IBL/Specular-IBL
PSOut PSMain(PSIn In)
{
	PSOut cubeMap;
	
    float3 N = normalize(In.LPos);
	
    const float3 R = N;
    const float3 V = R;
	
	//const float4 prefilteredColor = Convolve(N, V, R);
    cubeMap.cubeMapFace = Convolve(N, V, R);
	return cubeMap;
}