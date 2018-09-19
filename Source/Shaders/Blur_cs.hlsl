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
//#define THREAD_GROUP_SIZE_X 1024
//#define THREAD_GROUP_SIZE_Y 1024
//#define THREAD_GROUP_SIZE_Z 1


#if VERTICAL
#define PIXEL_CACHE_SIZE    IMAGE_SIZE_Y
#else
#define PIXEL_CACHE_SIZE    IMAGE_SIZE_X
#endif

groupshared half4 gColorLine[PIXEL_CACHE_SIZE];

[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, THREAD_GROUP_SIZE_Z)]
void CSMain(
	uint3 groupID     : SV_GroupID,
	uint3 groupTID    : SV_GroupThreadID,
	uint3 dispatchTID : SV_DispatchThreadID,
	uint  groupIndex  : SV_GroupIndex
)
{
	// gaussian kernel : src=https://learnopengl.com/#!Advanced-Lighting/Bloom
	//
	const half WEIGHTS[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
	// 	0.053514	0.045235	0.027318	0.011785	0.003631	0.000799	0.000125	0.000014	0.000001	0


	// https://twvideo01.ubm-us.net/o1/vault/gdc09/slides/100_Handout%206.pdf

	// READ INPUT TEXTURE AND SAVE INTO SHARED MEM
	//
	const uint TEXTURE_READ_COUNT = (uint) ceil(min(
		  ((float)IMAGE_SIZE_X) / THREAD_GROUP_SIZE_X
		, ((float)IMAGE_SIZE_Y) / THREAD_GROUP_SIZE_Y));

	[unroll] for (uint i = 0; i < TEXTURE_READ_COUNT; ++i)
	{
#if VERTICAL
		const uint2 outTexel = dispatchTID.xy + uint2(0, THREAD_GROUP_SIZE_Y * i);
		const int idxColorLine = outTexel.y >= IMAGE_SIZE_Y ? IMAGE_SIZE_Y -1 : outTexel.y;
#else
		const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * i, 0);
		const int idxColorLine = outTexel.x >= IMAGE_SIZE_X ? IMAGE_SIZE_X - 1 : outTexel.x;
#endif

		const float2 uv = float2(outTexel.xy) / float2(IMAGE_SIZE_X, IMAGE_SIZE_Y);
		const half4 color = texColorIn.SampleLevel(sSampler, uv, 0);

		gColorLine[idxColorLine] = color;
	}



	// SYNC UP SHARED-MEM SO IT'S READY TO READ
	//
	GroupMemoryBarrierWithGroupSync();


	// RUN THE HORIZONTAL/VERTICAL BLUR KERNEL
	//
	[unroll] for (uint px = 0; px < TEXTURE_READ_COUNT; ++px)
	{
#if VERTICAL
		const uint2 outTexel = dispatchTID.xy + uint2(0, THREAD_GROUP_SIZE_Y * px);
		int idxColorLine = outTexel.y;
#else
		const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * px, 0);
		int idxColorLine = outTexel.x;
#endif


		// use first weight 
		half4 result = half4(gColorLine[idxColorLine].xyz * WEIGHTS[0], 1);

		// and tap the next and previous pixels in increments
		[unroll] for (int i = 1; i < 5; ++i)
		{
#if HORIZONTAL
			bool bKernelSampleOutOfBounds = ((outTexel.x + i) >= IMAGE_SIZE_X);
			idxColorLine = bKernelSampleOutOfBounds ? 0 : outTexel.x + i;
			result += gColorLine[idxColorLine] * (bKernelSampleOutOfBounds ? 0.0f : WEIGHTS[i]);
			
			bKernelSampleOutOfBounds = ((outTexel.x - i) < 0);
			idxColorLine = bKernelSampleOutOfBounds ? 0 : outTexel.x - i;

			result += gColorLine[idxColorLine] * (bKernelSampleOutOfBounds ? 0 : WEIGHTS[i]);
#endif

#if VERTICAL
			bool bKernelSampleOutOfBounds = ((outTexel.y + i) >= IMAGE_SIZE_Y);
			idxColorLine = bKernelSampleOutOfBounds ? 0 : outTexel.y + i;
			result += gColorLine[idxColorLine] * (bKernelSampleOutOfBounds ? 0 : WEIGHTS[i]);

			bKernelSampleOutOfBounds = ((outTexel.y - i) < 0);
			idxColorLine = bKernelSampleOutOfBounds ? 0 : outTexel.y - i;

			result += gColorLine[idxColorLine] * (bKernelSampleOutOfBounds ? 0 : WEIGHTS[i]);
#endif
		}

		texColorOut[outTexel] = half4(result.xyz, 1.0f);
	}
}
