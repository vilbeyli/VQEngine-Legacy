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
//#define THREAD_GROUP_SIZE_X 512
//#define THREAD_GROUP_SIZE_Y 512
//#define THREAD_GROUP_SIZE_Z 1


#if VERTICAL
#define COLOR_LINE_WIDTH IMAGE_SIZE_Y
#else
#define COLOR_LINE_WIDTH IMAGE_SIZE_X
#endif

groupshared float4 gColorLine[COLOR_LINE_WIDTH];

// gaussian kernel : src=https://learnopengl.com/#!Advanced-Lighting/Bloom
const float WEIGHTS[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };


[numthreads(THREAD_GROUP_SIZE_X, THREAD_GROUP_SIZE_Y, THREAD_GROUP_SIZE_Z)]
void CSMain(
	uint3 groupID     : SV_GroupID,
	uint3 groupTID    : SV_GroupThreadID,
	uint3 dispatchTID : SV_DispatchThreadID,
	uint  groupIndex  : SV_GroupIndex
)
{
	// READ INPUT TEXTURE AND SAVE INTO SHARED MEM
	//
	const uint TEXTURE_READ_COUNT = (uint) ceil(min(
		  IMAGE_SIZE_X / THREAD_GROUP_SIZE_X
		, IMAGE_SIZE_Y / THREAD_GROUP_SIZE_Y)) + 1;

	[unroll] for (uint i = 0; i < TEXTURE_READ_COUNT; ++i)
	{
#if VERTICAL
		const uint2 outTexel = dispatchTID.xy + uint2(0, THREAD_GROUP_SIZE_Y * i);
		const int idxColorLine = outTexel.y;
#else
		const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * i, 0);
		const int idxColorLine = outTexel.x;
#endif

		// test color output ------------------------------------------------
		// texColorOut[outTexel] = float4(
		// 	color.x, 
		// 	color.y, 
		// 	((float)groupIndex) / (THREAD_GROUP_SIZE_X * THREAD_GROUP_SIZE_Y), 
		// 	1).xzyw;
		// test color output ------------------------------------------------

		const float2 uv = float2(outTexel.xy) / float2(IMAGE_SIZE_X, IMAGE_SIZE_Y);
		const float4 color = texColorIn.SampleLevel(sSampler, uv, 0);
		
		gColorLine[idxColorLine] = color;
	}



	// SYNC UP SHARED-MEM SO IT'S READY TO READ
	//
	GroupMemoryBarrierWithGroupSync();



	// RUN THE HORIZONTAL/VERTICAL BLUR KERNEL
	//


	[unroll] for (uint i = 0; i < TEXTURE_READ_COUNT; ++i)
	{
#if VERTICAL
		const uint2 outTexel = dispatchTID.xy + uint2(0, THREAD_GROUP_SIZE_Y * i);
		const int idxColorLine = outTexel.y;
#else
		const uint2 outTexel = dispatchTID.xy + uint2(THREAD_GROUP_SIZE_X * i, 0);
		const int idxColorLine = outTexel.x;
#endif

		const float3 color = gColorLine[idxColorLine].xyz;
		float3 result = color * 1.0f;//WEIGHTS[0];	// use first weight 

#if HORIZONTAL
		for (int i = 1; i < 5; ++i)
		{
			//const float2 weighedOffset = float2(texOffset.x * i, 0.0f);
			//result += InputTexture.Sample(BlurSampler, In.texCoord + weighedOffset).rgb * WEIGHTS[i];
			//result += InputTexture.Sample(BlurSampler, In.texCoord - weighedOffset).rgb * WEIGHTS[i];

			//result += gColorLine[idxColorLine]
		}
#endif
		
#if VERTICAL
		for (int i = 1; i < 5; ++i)
		{
		}
#endif


		texColorOut[outTexel] = float4(result, 1.0f);
	}
}
