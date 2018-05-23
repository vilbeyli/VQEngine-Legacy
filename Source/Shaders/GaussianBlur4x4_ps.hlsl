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
	float2 uv		: TEXCOORD0;
};

Texture2D tOcclusion;
SamplerState BlurSampler;

cbuffer cb
{
	float2 inputTextureDimensions;
};

float PSMain(PSIn In) : SV_TARGET
{
	const float2 texelSize = 1.0f / inputTextureDimensions;

	float result = 0.0f;
	for (int x = -2; x < 2; ++x)
	{
		for (int y = -2; y < 2; ++y)
		{
			const float2 offset = float2(x, y) * texelSize;
			result += tOcclusion.Sample(BlurSampler, In.uv + offset).r;
		}
	}
	return result / 16;	// =4x4 kernel size
}