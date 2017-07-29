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

struct PSIn
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

Texture2D worldRenderTarget;
SamplerState samTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};


float4 PSMain(PSIn In) : SV_TARGET
{
	float4 color = worldRenderTarget.Sample(samTriLinearSam, In.texCoord);

	//const float3 clipValue = float3(0.5, 0.5, 0.5);
	//float4 finalColor = float4(color * float3(10, 1, 1), 1);
	//float4 finalColor = float4(max(clipValue, color), 1);
	//return finalColor;
	return color;
	const float c = In.texCoord.x;
	//const float g = 1.0 / 2.2;
	//const float g =  2.2;
	const float g =  1.0;
	return pow(float4(c, c, c, 1), float4(g,g,g,1));
}