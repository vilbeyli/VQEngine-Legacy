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

#if DEINTERLEAVE
Texture2D<float>		texInput;
RWTexture2DArray<float> texOutputs; // 4
#endif

#if INTERLEAVE
//Texture2DArray<float>	texInputs; // 4
Texture2D<float> texInput0;
Texture2D<float> texInput1;
Texture2D<float> texInput2;
Texture2D<float> texInput3; // TODO: use an array for these
RWTexture2D<float>		texOutput;
#endif

#if 0 // 2x2 thread group (0.6 ms 1080p)


[numthreads(2,2,1)]
void CSMain(
	uint3 dispatchTID : SV_DispatchThreadID,
	uint3 groupTID    : SV_GroupThreadID,
	uint3 groupID     : SV_GroupID,
	uint  groupIndex  : SV_GroupIndex
)
{
#if DEINTERLEAVE
	texOutputs[uint3(groupID.xy, groupTID.x + groupTID.y * 2)] = texInput[/*groupID.xy * 2.0f.xx + groupTID.xy*/dispatchTID.xy];
#endif

#if INTERLEAVE
	texOutput[dispatchTID.xy] = texInputs[uint3(groupID.xy, groupTID.x + groupTID.y * 2)];
#endif
}



#else // 4x4 thread group (0.1 ms 1080p)



[numthreads(4,4,1)]
void CSMain(
	uint3 dispatchTID : SV_DispatchThreadID,
	uint3 groupTID    : SV_GroupThreadID,
	uint3 groupID     : SV_GroupID,
	uint  groupIndex  : SV_GroupIndex
)
{
	const uint slice = (groupTID.x % 2) + (groupTID.y % 2) * 2;
	const uint2 threadGroupOffset = uint2(groupTID.x / 2, groupTID.y / 2);
#if DEINTERLEAVE
	texOutputs[uint3(groupID.xy * 2 + threadGroupOffset, slice)] = texInput[dispatchTID.xy];
#endif

#if INTERLEAVE
	switch (slice)
	{
	case 0: texOutput[dispatchTID.xy] = texInput0.Load(uint3(groupID.xy * 2 + threadGroupOffset, 0)); break;
	case 1: texOutput[dispatchTID.xy] = texInput1.Load(uint3(groupID.xy * 2 + threadGroupOffset, 0)); break;
	case 2: texOutput[dispatchTID.xy] = texInput2.Load(uint3(groupID.xy * 2 + threadGroupOffset, 0)); break;
	case 3: texOutput[dispatchTID.xy] = texInput3.Load(uint3(groupID.xy * 2 + threadGroupOffset, 0)); break;
	}
#endif
}


#endif