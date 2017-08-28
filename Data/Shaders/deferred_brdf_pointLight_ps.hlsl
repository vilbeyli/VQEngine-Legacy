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

#include "LightingCommon.hlsl"
#include "BRDF.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};

cbuffer SceneVariables
{
	float3 cameraPos;
};

// GBuffer
Texture2D texDiffuseRoughnessMap;
Texture2D texSpecularMetalnessMap;
Texture2D texNormals;
Texture2D texPosition;

// Shadow
Texture2D texShadowMap;

SamplerState sShadowSampler;
SamplerState sNearestSampler;

float4 PSMain(PSIn In) : SV_TARGET
{
	// lighting & surface parameters
	const float3 P = texPosition.Sample(sNearestSampler, In.uv);
	const float3 N = texNormals.Sample(sNearestSampler, In.uv);
	const float3 V = normalize(cameraPos - P);

	const float4 diffuseRoughness = texDiffuseRoughnessMap.Sample(sNearestSampler, In.uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sNearestSampler, In.uv);

	BRDF_Surface s;	
	s.N = N;
	s.diffuseColor = diffuseRoughness.rgb;
	s.specularColor = specularMetalness.rgb;
	s.roughness = diffuseRoughness.a;
	s.metalness = specularMetalness.a;
	
	// illumination
	const float ambient = 0.2f;
	const float3 Ia = s.diffuseColor * ambient;	// ambient
	//return float4(Ia, 1.0f);
	return float4(0, 0, 0.002, 1);// +float4(Ia * 0.00001, 1.0f);
}