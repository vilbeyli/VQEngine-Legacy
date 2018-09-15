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

RWTexture2D<float4> texSSAOOutput;
Texture2D  texDepth;
SamplerState sPointSampler;
//Texture2D<float4> texNoise;

#define WorkgroupDimensionX  16
#define WorkgroupDimensionY  16
#define WorkgroupDimensionZ  1

[numthreads(WorkgroupDimensionX, WorkgroupDimensionY, WorkgroupDimensionZ)]
void CSMain(
	uint3 dispatchTID : SV_DispatchThreadID,
	uint3 groupTID    : SV_GroupThreadID,
	uint3 groupID     : SV_GroupID,
	uint  groupIndex  : SV_GroupIndex
)
{
	// -------------------------------------------------------------------
	// THREAD UNIT
	// -------------------------------------------------------------------
	//outColor[dispatchTID.xy] = float4( 
	//    ((float)dispatchTID.x) / 1920.f
	//	, ((float)dispatchTID.y) / 1080.0f
	//	, ((float)dispatchTID.z) / 1.0f
	//	, 1);
	// -------------------------------------------------------------------

	// -------------------------------------------------------------------
	// WORKGROUP - WARP - WAVEFRONT - WAVE
	// -------------------------------------------------------------------
	//texSSAOOutput[dispatchTID.xy] = float4(
	//	  ((float)groupTID.x) / max(float(WorkgroupDimensionX - 1), 1.0f)
	//	, ((float)groupTID.y) / max(float(WorkgroupDimensionY - 1), 1.0f)
	//	, ((float)groupTID.z) / max(float(WorkgroupDimensionZ - 1), 1.0f)
	//	, 1);
	// -------------------------------------------------------------------

#if 0
	const float depth = texDepth.Load(uint3(dispatchTID.x, dispatchTID.y, 0)).x;
#endif

#if 1
	float2 uv = float2(dispatchTID.xy) / float2(1920, 1080);
	const float depth = 
		(texDepth.SampleLevel(sPointSampler, uv, 0).x == 0.0f || true)
		? 0.19f 
		: texDepth.SampleLevel(sPointSampler, uv, 0).x * 1000000 + 0.15;
#else
	const float depth = 0.19f;
#endif
	texSSAOOutput[dispatchTID.xy] = float4(depth, depth, depth, 1.0f);

	//GroupMemoryBarrierWithGroupSync();
	return;
}

//-----------------------------------------------------------------------------------
// Notes:
//
// dispatchTID = current pixel for indexing output resource (per pixel).
//
// groupTID    = current position in the thread group (16x16 in this case).
//               this is important for indexing <group shader memory> !
//
// groupID     = index of the thread group Dispatch()ed by the CPU
//               can be usd to determine which work group (which block 
//               of threads on screen).
//
//-----------------------------------------------------------------------------------