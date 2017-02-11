//	DX11Renderer - VDemo | DirectX11 Renderer
//	Copyright(C) 2016  - Volkan Ilbeyli
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

cbuffer perLine
{
	float3 p1;
	float pad1;
	float3 p2;
	float pad2;
};

cbuffer renderConsts
{
	matrix view;
	matrix proj;
};

struct GSIn
{

};

struct PSIn
{
	float4 position : SV_POSITION;
};

// Note about 'maxvertexcount(N)'
// [NVIDIA08]:	peak perf when GS outputs 1-20 scalars
//				50% perf if 27-40 scalars are output
//				where scalar count = N * PSIn(scalarCount)
[maxvertexcount(2)]
void GSMain(point GSIn gin[1], inout LineStream<PSIn> lineStream)
{
	PSIn Out[2];
	Out[0].position = mul(proj, mul(view, float4(p1, 1)));
	Out[1].position = mul(proj, mul(view, float4(p2, 1)));

	lineStream.RestartStrip();
	lineStream.Append(Out[0]);
	lineStream.Append(Out[1]);
}