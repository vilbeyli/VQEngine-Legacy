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

#define KERNEL_SIZE 64
#define DEPTH_BIAS 0.025

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};


struct SceneVariables
{
	matrix matProjection;
	float2 screenSize;
	float3 samples[KERNEL_SIZE];
};

cbuffer cSceneVariables
{
    SceneVariables SSAO_constants;
};

Texture2D texViewSpaceNormals;
Texture2D texViewPositions;
Texture2D<float4> texNoise;
Texture2D texDepth;

SamplerState sNoiseSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	const float2 uv = In.uv;
	const float3 N = texViewSpaceNormals.Sample(sNoiseSampler, uv).xyz;
	const float3 P = texViewPositions.Sample(sNoiseSampler, uv).xyz;

	// tile noise texture (4x4) over whole screen by scaling UV coords (textures wrap)
    const float2 noiseScale = SSAO_constants.screenSize / 4.0f;
	const float3 noise = texNoise.Sample(sNoiseSampler, uv * noiseScale).xyz;

	// Gramm-Schmidt process for orthogonal basis creation: https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
	const float3 T = normalize(noise - N * dot(noise, N));
	const float3 B = cross(N, T);
	const float3x3 TBN = float3x3(T, B, N);

	float occlusion = 0.0;
	float radius = 0.5f;
	for (int i = 0; i < KERNEL_SIZE; ++i)
	{
        float3 kernelSample = mul(SSAO_constants.samples[i], TBN); // From tangent to view-space
		kernelSample = P + kernelSample * radius;

		// get the screenspace position of sample
		float4 offset = float4(kernelSample, 1.0f);
        offset = mul(offset, SSAO_constants.matProjection);
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5f + 0.5f;	// [0, 1]
		
		const float  D = texDepth.Sample(sNoiseSampler, uv + offset.xy).r;
		occlusion += (D >= kernelSample.z + DEPTH_BIAS ? 1.0f : 0.0f);
	}

	return occlusion.xxxx;
}