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

#define DO_TONEMAPPING

struct PSIn
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

Texture2D ColorTexture;
SamplerState Sampler;

cbuffer constants 
{
	float exposure;
};

float4 PSMain(PSIn In) : SV_TARGET
{
	const float3 color = ColorTexture.Sample(Sampler, In.texCoord);

#ifdef DO_TONEMAPPING
	float3 toneMapped = float3(1, 1, 1) - exp(-color * exposure);
#else
	float3 toneMapped = color;
#endif
	
#if 1
	const float gamma = 1.0f / 2.2f;
#else
	const float gamma = 1.0f;
#endif

	toneMapped = pow(toneMapped, float3(gamma, gamma, gamma));
	return float4(toneMapped, 1.0f);
}