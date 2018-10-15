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
Texture2DArray<float>	texInputs; // 4
RWTexture2D<float>		texOutput;
#endif

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

