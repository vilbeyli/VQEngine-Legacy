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

//cbuffer perFrame
//{
//	matrix view;
//	matrix	proj;
//}

cbuffer perModel
{
    matrix world;
	matrix normalMatrix;
	matrix worldViewProj;
}

cbuffer frame
{
	matrix lightSpaceMat;	// array?
};


struct VSIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT0;
	float2 texCoord : TEXCOORD0;    
};

struct PSIn 
{
	float4 position		 : SV_POSITION;
	float3 worldPos		 : POSITION;
	float4 lightSpacePos : POSITION1;	// array?
	float3 normal		 : NORMAL;
    float3 tangent		 : TANGENT;
    float2 texCoord		 : TEXCOORD4;
};

PSIn VSMain(VSIn In)
{
	const float4 pos = float4(In.position, 1);

	PSIn Out;
	Out.position		= mul(worldViewProj, pos);
	Out.worldPos		= mul(world        , pos).xyz;
    Out.normal			= normalize(mul(normalMatrix, In.normal));
	Out.lightSpacePos	= mul(lightSpaceMat, pos);
    Out.tangent			= normalize(mul(normalMatrix, In.tangent));
	Out.texCoord		= In.texCoord;
	return Out;
}