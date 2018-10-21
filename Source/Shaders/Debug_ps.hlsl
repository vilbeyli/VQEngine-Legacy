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
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

Texture2D inputTexture;

// todo get from app
SamplerState samAnisotropic
{
	Filter = ANISOTROPIC;
	MaxAnisotropy = 4;
	AddressU = WRAP;
	AddressV = WRAP;
};

cbuffer c{
	float isDepthTexture;
	int numChannels;
};

float4 PSMain(PSIn In) : SV_TARGET
{
	const float depthExponent = 300;
	float4 color = inputTexture.Sample(samAnisotropic, In.texCoord);
	if (isDepthTexture > 0)
	{
		color.yzw = float3(color.x, color.x, color.x);
		color = pow(color, float4(depthExponent, depthExponent, depthExponent, 1));
	}
	if (numChannels == 1)
		return float4(color.xxx, color.w);
	else
		return color;
}