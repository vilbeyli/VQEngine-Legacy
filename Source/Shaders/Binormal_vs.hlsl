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

//cbuffer perFrame
//{
//	matrix view;
//	matrix	proj;
//}

cbuffer perModel
{
    matrix world;
	matrix worldViewProj;
}

struct VSIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float2 texCoord : TEXCOORD0;
	float3 tangent	: TANGENT0;
};

struct PSIn
{
	float4 position : SV_POSITION;
	float3 normal	: NORMAL;
    float3 tangent	: TANGENT;
	float2 texCoord : TEXCOORD4;
};

PSIn VSMain(VSIn In)
{
    float3x3 rotMatrix =
    {
        world._11_12_13,
		world._21_22_23,
		world._31_32_33
    };

	PSIn Out;
    Out.position = mul(worldViewProj, float4(In.position, 1));
    Out.normal   = mul(rotMatrix, In.normal);
    Out.tangent  = mul(rotMatrix, In.tangent);
    Out.texCoord = In.texCoord;
	
	return Out;
}