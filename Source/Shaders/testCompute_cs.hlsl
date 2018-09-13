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

RWTexture2D<float4> outColor;

[numthreads(16,16,1)]
void CSMain(
	uint3 dispatchTID : SV_DispatchThreadID,
	uint3 groupTID    : SV_GroupThreadID,
	uint3 groupID     : SV_GroupID,
	uint  groupIndex  : SV_GroupIndex
)
{
	//outColor[dispatchTID.xy] = float4(((float)groupIndex) / 255.0f, 1, 0, 1);
	
	//outColor[dispatchTID.xy] = float4( 
	//	((float)dispatchTID.x) / 1920.f
	//	, ((float)dispatchTID.y) / 1080.0f
	//	, ((float)dispatchTID.z) / 1.0f
	//	, 1);

	outColor[dispatchTID.xy] = float4(
		((float)groupTID.x) / 120.0f
		, ((float)groupTID.y) / 68.0f
		, ((float)groupTID.z) / 1.0f
		, 1);

	return;
}