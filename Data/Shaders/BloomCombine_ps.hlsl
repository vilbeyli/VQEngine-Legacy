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

Texture2D ColorTexture;
Texture2D BloomTexture;
SamplerState BlurSampler;

cbuffer constants 
{
	float exposure;
};

float4 PSMain(PSIn In) : SV_TARGET
{
	float3 outColor;
	const float3 color = ColorTexture.Sample(BlurSampler, In.texCoord);
	const float3 bloom = BloomTexture.Sample(BlurSampler, In.texCoord);
	
	if (length(bloom) != 0.0f)
		outColor = color + bloom;
	else
		outColor = color;

	float3 toneMapped = float3(1, 1, 1) - exp(-outColor * exposure);
	
	//const float gamma = 1.0f / 2.2f;
	const float gamma = 1.0f;
	toneMapped = pow(toneMapped, float3(gamma, gamma, gamma));
	return float4(toneMapped, 1.0f);
}