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
	float2 texCoord : TEXCOORD0;
};

Texture2D InputTexture;
SamplerState BlurSampler;

cbuffer constants 
{
	int textureWidth;
	int textureHeight;
};


// gaussian kernel : src=https://learnopengl.com/#!Advanced-Lighting/Bloom
float4 PSMain(PSIn In) : SV_TARGET
{
	#include "GaussianKernels.hlsl"

	const float4 color = InputTexture.Sample(BlurSampler, In.texCoord);
	const float2 texOffset = float2(1.0f, 1.0f) / float2(textureWidth, textureHeight);
	
	int kernelOffset = 0;
	float3 result = KERNEL_WEIGHTS[kernelOffset] * color;	// use first weight 
#if HORIZONTAL_PASS
	{
		[unroll] for (kernelOffset = 1; kernelOffset < KERNEL_RANGE; ++kernelOffset)
		{
			const float2 weighedOffset = float2(texOffset.x * kernelOffset, 0.0f);
			result += InputTexture.Sample(BlurSampler, In.texCoord + weighedOffset).rgb * KERNEL_WEIGHTS[kernelOffset];
			result += InputTexture.Sample(BlurSampler, In.texCoord - weighedOffset).rgb * KERNEL_WEIGHTS[kernelOffset];
		}
	}
#else
	{
		[unroll] for (kernelOffset = 1; kernelOffset < KERNEL_RANGE; ++kernelOffset)
		{
			const float2 weighedOffset = float2(0.0f, texOffset.y * kernelOffset);
			result += InputTexture.Sample(BlurSampler, In.texCoord + weighedOffset).rgb * KERNEL_WEIGHTS[kernelOffset];
			result += InputTexture.Sample(BlurSampler, In.texCoord - weighedOffset).rgb * KERNEL_WEIGHTS[kernelOffset];
		}
	}
#endif
	return float4(result, 1.0f);
}