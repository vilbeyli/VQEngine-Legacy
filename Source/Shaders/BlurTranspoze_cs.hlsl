
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
	// gaussian kernel : src
	// https://learnopengl.com/#!Advanced-Lighting/Bloom
	// https://twvideo01.ubm-us.net/o1/vault/gdc09/slides/100_Handout%206.pdf
	const half KERNEL_WEIGHTS[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
	// 	0.053514	0.045235	0.027318	0.011785	0.003631	0.000799	0.000125	0.000014	0.000001	0



	// READ INPUT TEXTURE AND SAVE INTO SHARED MEM
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


#if 1
	// RUN THE HORIZONTAL/VERTICAL BLUR KERNEL
	//
	[unroll] for (uint passCount = 0; passCount < PASS_COUNT; ++passCount)
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
			[unroll] for (int i = 1; i < 5; ++i)
			{
#if HORIZONTAL
				bool bKernelSampleOutOfBounds = ((outTexel.x + i) >= IMAGE_SIZE_X);
				idxColorLine = bKernelSampleOutOfBounds ? 0 : outTexel.x + i;
				result += gColorLine[idxColorLine] * (bKernelSampleOutOfBounds ? 0.0f : KERNEL_WEIGHTS[i]);

				bKernelSampleOutOfBounds = ((outTexel.x - i) < 0);
				idxColorLine = bKernelSampleOutOfBounds ? 0 : outTexel.x - i;

				result += gColorLine[idxColorLine] * (bKernelSampleOutOfBounds ? 0 : KERNEL_WEIGHTS[i]);
#endif

			}

			// save the blurred pixel value
			texColorOut[outTexel.yx] = half4(result, 0.0f);
		}
		if (passCount == PASS_COUNT - 1) break;


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
#endif
}
