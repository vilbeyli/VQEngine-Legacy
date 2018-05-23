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
	float isHDR;
};

// src
//	: reference	: https://learnopengl.com/#!Advanced-Lighting/HDR
//	: tonemap	: https://photo.stackexchange.com/questions/7630/what-is-tone-mapping-how-does-it-relate-to-hdr
//	: hdr		: https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting
//	: hdr		: https://gamedev.stackexchange.com/questions/62836/does-hdr-rendering-have-any-benefits-if-bloom-wont-be-applied
//  : gamma		: http://blog.johnnovak.net/2016/09/21/what-every-coder-should-know-about-gamma/
float4 PSMain(PSIn In) : SV_TARGET
{
	const float3 color = ColorTexture.Sample(Sampler, In.texCoord);
	const float gamma = 1.0f / 2.2f;

#ifdef DO_TONEMAPPING
	const float3 HDRToneMapped = float3(1, 1, 1) - exp(-color * exposure);
	const float3 LDRToneMapped = color / (color + float3(1, 1, 1));
	float3 toneMapped = HDRToneMapped * isHDR + LDRToneMapped * (1.0f - isHDR);
#else
	float3 toneMapped = color;
#endif

	toneMapped = pow(toneMapped, float3(gamma, gamma, gamma));
	return float4(toneMapped, 1.0f);
}