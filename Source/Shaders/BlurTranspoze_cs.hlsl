
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

RWTexture2D<float4> texColorOut;
Texture2D<float4>   texColorIn;
SamplerState        sSampler;
#if USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH
cbuffer BlurParametersBuffer
{
	struct Params
	{
		uint strength;
	} cBlurParameters;
};
#endif

// These are defined by the application compiling the shader
//#define THREAD_GROUP_SIZE_X 1
//#define THREAD_GROUP_SIZE_Y 1024
//#define THREAD_GROUP_SIZE_Z 1
//#define PASS_COUNT 6

#define PIXEL_CACHE_SIZE    IMAGE_SIZE_X

groupshared half3 gColorLine[PIXEL_CACHE_SIZE];

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, THREAD_GROUP_SIZE_Z)]
void CSMain(
	uint3 groupID     : SV_GroupID,
	uint3 groupTID : SV_GroupThreadID,
	uint3 dispatchTID : SV_DispatchThreadID,
	uint  groupIndex : SV_GroupIndex
)
{

	#include "GaussianKernels.hlsl"

	// READ INPUT TEXTURE AND SAVE INTO SHARED MEM (LDS = Local Data Share)
	//
	const uint TEXTURE_READ_COUNT = (uint) ceil(min(
		  ((float)IMAGE_SIZE_X) / THREAD_GROUP_SIZE_X
		, ((float)IMAGE_SIZE_Y) / THREAD_GROUP_SIZE_Y));

	[unroll] for (uint i = 0; i < TEXTURE_READ_COUNT; ++i)
	{
		const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * i, 0);
		if (outTexel.x >= IMAGE_SIZE_X)
			break;
		const int idxColorLine = outTexel.x;

		const float2 uv = float2(outTexel.xy) / float2(IMAGE_SIZE_X, IMAGE_SIZE_Y);
		const half4 color = texColorIn.SampleLevel(sSampler, uv, 0);

		gColorLine[idxColorLine] = color.xyz;
		texColorOut[outTexel.yx] = float4(0, 0, 0, 1);
	}


	// RUN THE HORIZONTAL/VERTICAL BLUR KERNEL
	//
#if USE_CONSTANT_BUFFER_FOR_BLUR_STRENGTH
	for (uint passCount = 0; passCount < cBlurParameters.strength; ++passCount)
#else
	[unroll] for (uint passCount = 0; passCount < PASS_COUNT; ++passCount)
#endif
	{
		// SYNC UP SHARED-MEM SO IT'S READY TO READ
		//
		GroupMemoryBarrierWithGroupSync();


		// BLUR
		//
		[unroll] for (uint px = 0; px < TEXTURE_READ_COUNT; ++px)
		{
			const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * px, 0);
			int idxColorLine = outTexel.x;

			// use first weight 
			half3 result = gColorLine[idxColorLine] * KERNEL_WEIGHTS[0];

			// and tap the next and previous pixels in increments
			[unroll] for (uint kernelOffset = 1; kernelOffset < KERNEL_RANGE; ++kernelOffset)
			{
				bool bKernelSampleOutOfBounds = ((outTexel.x + kernelOffset) >= IMAGE_SIZE_X);
				if (bKernelSampleOutOfBounds)
				{
					result += gColorLine[IMAGE_SIZE_X - 1] * KERNEL_WEIGHTS[kernelOffset];
				}
				else
				{
					result += gColorLine[outTexel.x + kernelOffset] * KERNEL_WEIGHTS[kernelOffset];
				}

				bKernelSampleOutOfBounds = ((outTexel.x - kernelOffset) < 0);
				if (bKernelSampleOutOfBounds)
				{
					result += gColorLine[0] * KERNEL_WEIGHTS[kernelOffset];
				}
				else
				{
					result += gColorLine[outTexel.x - kernelOffset] * KERNEL_WEIGHTS[kernelOffset];
				}
			}

			// save the blurred pixel value
			texColorOut[outTexel.yx] = half4(result, 0.0f);
		}


		// UPDATE LDS
		//
		GroupMemoryBarrierWithGroupSync();

		[unroll] for (uint px = 0; px < TEXTURE_READ_COUNT; ++px)
		{
			const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * px, 0);
			int idxColorLine = outTexel.x;

			gColorLine[idxColorLine] = texColorOut[outTexel.yx].xyz;
		}

	}
}
