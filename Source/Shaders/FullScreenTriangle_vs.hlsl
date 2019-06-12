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

struct PSIn 
{
    centroid float4 position : SV_POSITION;
    centroid noperspective
    float2 uv : TEXCOORD0;
};

PSIn VSMain(uint vertexID : SV_VertexID)
{
	PSIn Out;

	// ((0&1) << 1   = x=0 
	// ((1&1) << 1   = x=2 
	// ((2&1) << 1   = x=0 
	//---------------------------------
	// ((0&2) << 1   = y=0 
	// ((1&2) << 1   = y= 
	// ((2&2) << 1   = y= 

    int x = ((vertexID & 1) << 2);
    int y = ((vertexID & 2) << 1);

    float u = ((vertexID & 1) << 1);
    float v = -float(vertexID & 2);

	Out.position = float4(
	  -1 + x
	, -1 + y
	, 0
	, 1);
	Out.uv		 = float2(0 + u, 1 + v);
	return Out;
}