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

struct GSIn
{
	float3 T : TANGENT;
	float3 N : NORMAL;
	float3 B : BINORMAL;
	float3 wPos : POSITION;
};

cbuffer renderConsts
{
	matrix view;
	matrix proj;
};

struct PSIn
{
	float4 position : SV_POSITION;
	float3 color : COLOR;
};

// Note about 'maxvertexcount(N)'
// [NVIDIA08]:	peak perf when GS outputs 1-20 scalars
//				50% perf if 27-40 scalars are output
//				where scalar count = N * PSIn(scalarCount)
[maxvertexcount(6)]
void GSMain(point GSIn gin[1], inout LineStream<PSIn> lineStream)
{
	const matrix vp = mul(proj, view);
	const float4 VertPos = mul(vp, float4(gin[0].wPos, 1));
	const float4 TPos = mul(vp, float4(gin[0].wPos + gin[0].T, 1));
	const float4 NPos = mul(vp, float4(gin[0].wPos + gin[0].N, 1));
	const float4 BPos = mul(vp, float4(gin[0].wPos + gin[0].B, 1));

	const float3 red	= float3(1, 0, 0);
	const float3 blue	= float3(0, 0, 1);
	const float3 green	= float3(0, 1, 0);

	PSIn Out[6];

	// T
	Out[0].position = VertPos;
	Out[1].position = TPos;
	Out[0].color = red;
	Out[1].color = red;
	lineStream.RestartStrip();
	lineStream.Append(Out[0]);
	lineStream.Append(Out[1]);

	// N
	Out[2].position = VertPos;
	Out[3].position = NPos;
	Out[2].color = blue;
	Out[3].color = blue;
	lineStream.RestartStrip();
	lineStream.Append(Out[2]);
	lineStream.Append(Out[3]);

	// B
	Out[4].position = VertPos;
	Out[5].position = BPos;
	Out[4].color = green;
	Out[5].color = green;
	lineStream.RestartStrip();
	lineStream.Append(Out[4]);
	lineStream.Append(Out[5]);

}