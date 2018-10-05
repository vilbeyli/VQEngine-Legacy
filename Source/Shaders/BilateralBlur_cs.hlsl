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

RWTexture2D<float>  texOcclusion;
RWTexture2D<float3> texNormals;

RWTexture2D<float>  texBlurredOcclusionOut;

[numthreads(1,1,1)]
void CSMain(
	uint3 dispatchTID : SV_DispatchThreadID,
	uint3 groupTID    : SV_GroupThreadID,
	uint3 groupID     : SV_GroupID,
	uint  groupIndex  : SV_GroupIndex
)
{
	;
}