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

TextureCube tEnvironmentMap;
SamplerState sLinear;

cbuffer cb
{
	float roughness;
	float resolution;
};


// src: 
//	https://learnopengl.com/#!PBR/IBL/Specular-IBL
//	https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch20.html
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
		
		float HdotV = max(dot(H,V), 0.0f);
		float NdotH = max(dot(N,H), 0.0f);
		float NdotL = max(dot(N,L), 0.0f);
		if(NdotL > 0.0f)
	    {
			float D = NormalDistributionGGX(N, H, roughness);
            float pdf = D * NdotH / (4.0f * HdotV + 0.00001) + 0.0001;
            float saSample = 1.0f / ((PREFILTER_SAMPLE_COUNT * pdf) + 0.000001); // source cubemap face resolution
            float saTexel = 4.0f * PI / (6.0f * resolution * resolution);

            float mipLevel = roughness == 0.0f ? 0.0f : log2(saSample / saTexel);
			preFilteredColor += tEnvironmentMap.SampleLevel(sLinear, L, mipLevel).xyz * NdotL;
			totalWeight += NdotL;
	    }
	}
	
    preFilteredColor /= max(totalWeight, 0.0001f);
    return float4(preFilteredColor, 1);
}
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