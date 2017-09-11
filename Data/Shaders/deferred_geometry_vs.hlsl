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


struct VSIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT0;
	float2 uv		: TEXCOORD0;
};

struct PSIn
{
	float4 position			: SV_POSITION;
	float3 viewPosition		: POSITION0;
	float3 viewNormal		: NORMAL;
	float3 viewTangent		: TANGENT;
	float2 uv				: TEXCOORD1;
};

cbuffer perModel
{
	matrix worldView;
	matrix normalViewMatrix;
	matrix worldViewProj;
};

PSIn VSMain(VSIn In)
{
	const float4 pos = float4(In.position, 1);

	PSIn Out;
	Out.position		= mul(worldViewProj, pos);
	Out.viewPosition	= mul(worldView, pos).xyz;
	Out.viewNormal		= normalize(mul(normalViewMatrix, In.normal));
	Out.viewTangent		= normalize(mul(normalViewMatrix, In.tangent));
	Out.uv				= In.uv;
	return Out;
}