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

#include "BRDF.hlsl"
#include "LightingCommon.hlsl"

struct PSIn
{
	float4 position		 : SV_POSITION;
	float2 uv			 : TEXCOORD0;
};

cbuffer SceneVariables
{
	float3 CameraWorldPosition;
	float pad0;
	float2 ScreenSize;
};

cbuffer PerLightData
{
	PointLight light;
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
	const float2 uv = In.position.xy / ScreenSize;

	// lighting & surface parameters
	const float3 P = texPosition.Sample(sNearestSampler, uv).xyz;
	const float3 N = texNormals.Sample(sNearestSampler, uv).xyz;
	const float3 V = normalize(CameraWorldPosition - P);
	const float3 L = light.position;

	const float4 diffuseRoughness = texDiffuseRoughnessMap.Sample(sNearestSampler, uv);
	const float4 specularMetalness = texSpecularMetalnessMap.Sample(sNearestSampler, uv);

	BRDF_Surface s;	
	s.N = N;
	s.diffuseColor = diffuseRoughness.rgb;
	s.specularColor = specularMetalness.rgb;
	s.roughness = diffuseRoughness.a;
	s.metalness = specularMetalness.a;
	
	// illumination
	float3 IdIs = float3(0.0f, 0.0f, 0.0f);		// diffuse & specular

	// integrate the lighting equation
	const float3 Wi = normalize(L - P);
	const float3 radiance = AttenuationPhong(light.attenuation, length(light.position - P))
		* light.color
		* light.brightness;

	IdIs += BRDF(Wi, s, V, P) * radiance;
	
	return float4(IdIs, 1.0f);
	//return float4(IdIs * 0.000001f + float3(1,0,0), 1.0f);
	//return float4(uv, 0, 1.0f);
	//return float4(N, 1.0f);
}