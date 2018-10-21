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


// http://developer.amd.com/wordpress/media/2013/05/GCNPerformanceTweets.pdf

#include "ShadingMath.hlsl"

// this helps removing the false occlusion due to 
// low tesselation (sponza arches)
#define SSAO_DEPTH_BIAS 1.2f	// bias to apply during depth testing

struct PSIn
{
	float4 position	: SV_POSITION;
	float2 uv		: TEXCOORD0;
};


struct SceneVariables
{
	matrix matProjection;
	matrix matProjectionInv;
	float2 screenSize;
	float  radius;
	float  intensity;
	float3 samples[KERNEL_SIZE];
};

cbuffer cSceneVariables
{
    SceneVariables SSAO_constants;
#if INTERLEAVED_TEXTURING
	int slice;
#endif
};

Texture2D texViewSpaceNormals;
Texture2D<float2> texNoise;

#if INTERLEAVED_TEXTURING
Texture2DArray texDepth;
#else
Texture2D texDepth;
#endif

SamplerState sNoiseSampler;
SamplerState sPointSampler;
SamplerState sLinearSampler;

float PSMain(PSIn In) : SV_TARGET
{
	const float2 uv = In.uv;

	float3 N = texViewSpaceNormals.Sample(sPointSampler, uv).xyz;
	if(dot(N, N) < 0.00001) return 1.0f;
	N = normalize(N);

#if INTERLEAVED_TEXTURING
	const float zBufferSample = texDepth.Sample(sPointSampler, float3(uv, slice)).x;	// non-linear depth
#else
	const float zBufferSample = texDepth.Sample(sPointSampler, uv).x;	// non-linear depth
#endif
	const float3 P = ViewSpacePosition(zBufferSample, uv, SSAO_constants.matProjectionInv);

	// tile noise texture (4x4) over whole screen by scaling UV coords (textures wrap)
	const float2 noiseScale = SSAO_constants.screenSize / 4.0f;
	const float3 noise = float3(texNoise.Sample(sNoiseSampler, uv * noiseScale).xy, 0.0f);

	// Gramm-Schmidt process for orthogonal basis creation: https://en.wikipedia.org/wiki/Gram%E2%80%93Schmidt_process
	// in short: project noise on N vector, and subtract that projection from the noise vector
	//			 this would give us a vector perpendicular to N. see gif in link 
	const float3 T = normalize(noise - N * dot(noise, N));
	const float3 B = cross(T, N);
	const float3x3 TBN = float3x3(T, B, N);

	float occlusion = 0.0;

	[unroll]	// when unrolled, VGPR usage skyrockets and reduces #waves in flight
	for (int i = 0; i < KERNEL_SIZE; ++i)
	{
		float3 kernelSample = P + mul(SSAO_constants.samples[i], TBN) * SSAO_constants.radius; // From tangent to view-space

		// get the screenspace position of the sample
		float4 offset = float4(kernelSample, 1.0f);
		offset = mul(SSAO_constants.matProjection, offset);
		offset.xy = ((offset.xy / offset.w) * float2(1.0f, -1.0f)) * 0.5f + 0.5f; // [0, 1]
		
		// sample depth
		const float sampleDepth = LinearDepth(
#if INTERLEAVED_TEXTURING
			texDepth.Sample(sPointSampler, float3(offset.xy, slice)).r
#else
			texDepth.Sample(sPointSampler, offset.xy).r
#endif
			, SSAO_constants.matProjection[2][3]
			, SSAO_constants.matProjection[2][2]);

		// range check & accumulate
		if (smoothstep(0.0f, 1.0f, SSAO_constants.radius / abs(P.z - sampleDepth)) > 0)
		{
			if (sampleDepth < kernelSample.z - SSAO_DEPTH_BIAS)
			{
#if 1
				occlusion += 1.0f;
#else
				const float depthDistance = abs(sampleDepth - kernelSample.z);
				occlusion += pow(depthDistance, -1.05f);
#endif
			}
		}
	}
	occlusion = 1.0 - (occlusion / KERNEL_SIZE);

	return pow(occlusion, SSAO_constants.intensity);
}