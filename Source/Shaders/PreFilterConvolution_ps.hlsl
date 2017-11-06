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
	float2 uv : TEXCOORD0;
};

Texture2D tEnvironmentMap;
SamplerState sLinear;

cbuffer cb
{
	float roughness;
};

// source: https://learnopengl.com/#!PBR/IBL/Specular-IBL
float4 PSMain(PSIn In) : SV_TARGET
{
    float3 N = float3(0, 0, 1);
	float3 R = N;
	float3 V = R;

	const int PREFILTER_SAMPLE_COUNT = 1024;
	float totalWeight = 0.0f;
	float3 preFilteredColor = 0.0f.xxx;

	for(int i = 0; i < PREFILTER_SAMPLE_COUNT; ++i)
	{
		float2 Xi = Hammersley(i, PREFILTER_SAMPLE_COUNT);
		float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0f * dot(V, H) * H - V);

		float NdotL = max(dot(N,L), 0.0f);
		if(NdotL > 0.0f)
        {
			//const float2 uv = SphericalSample(normalize(In.LPos));
			preFilteredColor += tEnvironmentMap.Sample(sLinear, In.uv).xyz;
			totalWeight += NdotL;
        }
    }

	preFilteredColor /= totalWeight;
	return float4(preFilteredColor, 1.0f);
	//return tEnvironmentMap.Sample(sLinear, In.uv);
	//return float4(1,0,0.5,1);
}