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

struct PSIn
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

Texture2D InputTexture;
SamplerState BlurSampler;

//cbuffer constants 
//{
//	int isHorizontal;
//	int textureWidth;
//	int textureHeight;
//};

float4 PSMain(PSIn In) : SV_TARGET
{
#if 0
	const float4 color = InputTexture.Sample(BlurSampler, In.texCoord);
	const float2 texOffset = float2(1.0f, 1.0f) / float2(textureWidth, textureHeight);
	
	// gaussian kernel : src=https://learnopengl.com/#!Advanced-Lighting/Bloom
	const float weight[5] = { 0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216 };
	
	float3 result = weight[0] * color;	// use first weight 
	if(isHorizontal)
	{
		for (int i = 1; i < 5; ++i)
		{
			const float2 weighedOffset = float2(texOffset.x * i, 0.0f);
			result += InputTexture.Sample(BlurSampler, In.texCoord + weighedOffset).rgb * weight[i];
			result += InputTexture.Sample(BlurSampler, In.texCoord - weighedOffset).rgb * weight[i];
		}
	}
	else
	{
		for (int i = 1; i < 5; ++i)
		{
			const float2 weighedOffset = float2(0.0f, texOffset.y * i);
			result += InputTexture.Sample(BlurSampler, In.texCoord + weighedOffset).rgb * weight[i];
			result += InputTexture.Sample(BlurSampler, In.texCoord - weighedOffset).rgb * weight[i];
		}
	}

	return float4(result, 1.0f);
#endif
	// TODO: implement bilateral blur
    return 1.0f.xxxx;
}