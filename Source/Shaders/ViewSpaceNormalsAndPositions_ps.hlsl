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

#include "LightingCommon.hlsl"
#include "ShadingMath.hlsl"

struct PSIn
{
	float4 position			: SV_POSITION;
	float3 viewPosition		: POSITION0;
	float3 viewNormal		: NORMAL;
	float3 viewTangent		: TANGENT;
	float2 uv				: TEXCOORD1;
};

struct PSOut
{
	float3 normals			 : SV_TARGET0;
};

cbuffer cbSurfaceMaterial
{
    float isNormalMap;
	float2 uvScale;
	int textureConfig;
};

Texture2D texNormalMap;
Texture2D texAlphaMask;

SamplerState sNormalSampler;

PSOut PSMain(PSIn In) : SV_TARGET
{
	PSOut GBuffer;

	// lighting & surface parameters
	const float3 P = In.viewPosition;
	const float3 N = normalize(In.viewNormal);
	const float3 T = normalize(In.viewTangent);
	const float3 V = normalize(-P);
	const float2 uv = In.uv * uvScale;
	
	const float alpha = HasAlphaMask(textureConfig) > 0 ? texAlphaMask.Sample(sNormalSampler, uv).r : 1.0f;
	if (alpha < 0.01f)
		discard;

	float3 surfaceN				= (isNormalMap) * UnpackNormals(texNormalMap, sNormalSampler, uv, N, T) +	(1.0f - isNormalMap) * N;
	GBuffer.normals				= surfaceN;
	return GBuffer;
}