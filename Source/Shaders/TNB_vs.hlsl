//	VQEngine | DirectX11 Renderer
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
	float2 texCoord : TEXCOORD0;
};

struct GSIn
{
	float3 T : TANGENT;
	float3 N : NORMAL;
	float3 B : BINORMAL;
	float3 WorldPosition : POSITION;
};

cbuffer perObj
{
	matrix normalMatrix;
	matrix world;
};

GSIn VSMain(VSIn In)
{
	const float3 B = normalize(cross(In.normal, In.tangent));

	GSIn Out;
	Out.T	 = normalize(mul(normalMatrix, In.tangent));
	Out.N	 = normalize(mul(normalMatrix, In.normal));
	Out.B	 = normalize(mul(normalMatrix, B));
	Out.WorldPosition = mul(world, float4(In.position, 1)).xyz;
	return Out;
}