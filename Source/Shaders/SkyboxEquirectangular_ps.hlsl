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

#include "ShadingMath.hlsl"

struct PSIn
{
	float4 HPos : SV_POSITION;
	float3 LPos : POSITION;
};

Texture2D texSkybox;
SamplerState samWrap;

float4 PSMain(PSIn In) : SV_TARGET
{
	//return float4(In.LPos, 1);
	const float2 uv = SphericalSample(normalize(In.LPos));
//	return float4(texSkybox.Sample(samWrap, uv).xyz, 1);
    return float4(pow(texSkybox.Sample(samWrap, uv).xyz, 2.2f), 1);
	//return float4(uv, 0, 1);	// test
}
