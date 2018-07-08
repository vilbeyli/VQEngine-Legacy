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
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};

cbuffer SceneVariables
{
	float ambientFactor;
};


Texture2D tDiffuseRoughnessMap;
Texture2D tAmbientOcclusion;

SamplerState sNearestSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	const float3 diffuseColor     = tDiffuseRoughnessMap.Sample(sNearestSampler, In.uv).rgb;
	const float  ambientOcclusion = tAmbientOcclusion.Sample(sNearestSampler, In.uv).r;
	
	const float3 Ia = diffuseColor * ambientOcclusion * ambientFactor;
	return float4(Ia, 1.0f);
}