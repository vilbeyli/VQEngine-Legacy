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

#define PI		3.14159265359f
#define EPSILON 0.000000000001f

#define _DEBUG

struct PSIn
{
	float4 position			: SV_POSITION;
	float3 viewPosition	: POSITION0;
	float3 viewNormal		: NORMAL;
	float3 viewTangent		: TANGENT;
	float2 uv				: TEXCOORD1;
};

struct PSOut	// G-BUFFER
{
	float4 diffuseRoughness  : SV_TARGET0;
	float4 specularMetalness : SV_TARGET1;
	float3 normals			 : SV_TARGET2;
	float3 position			 : SV_TARGET3;
};

// CBUFFERS
//---------------------------------------------------------

cbuffer SurfaceMaterial	// todo: use BRDF_Surface struct (match on CPU side)
{
	float3 diffuse;
	float  alpha;

	float3 specular;
	float  roughness;

	float  isDiffuseMap;
	float  isNormalMap;
	float  metalness;
};

// TEXTURES & SAMPLERS
//---------------------------------------------------------
Texture2D texDiffuseMap;
Texture2D texNormalMap;

SamplerState sNormalSampler;

inline float3 UnpackNormals(float2 uv, float3 viewNormal, float3 viewTangent)
{
	// uncompressed normal in tangent space
	float3 SampledNormal = texNormalMap.Sample(sNormalSampler, uv).xyz;
	SampledNormal = normalize(SampledNormal * 2.0f - 1.0f);

	const float3 T = normalize(viewTangent - dot(viewNormal, viewTangent) * viewNormal);
	const float3 N = normalize(viewNormal);
	const float3 B = normalize(cross(T, N));
	const float3x3 TBN = float3x3(T, B, N);
	return mul(SampledNormal, TBN);
}

struct Surface
{
	float3 N;
	float  roughness;
	float3 diffuseColor;
	float  metalness;
	float3 specularColor;
};

PSOut PSMain(PSIn In) : SV_TARGET
{
	PSOut GBuffer;

	// lighting & surface parameters
	const float3 P = In.viewPosition;
	const float3 N = normalize(In.viewNormal);
	const float3 T = normalize(In.viewTangent);
	const float3 V = normalize(-P);

	Surface s;
	s.N				= (isNormalMap) * UnpackNormals(In.uv, N, T) +	(1.0f - isNormalMap) * N;
	s.diffuseColor	= diffuse *  (isDiffuseMap * texDiffuseMap.Sample(sNormalSampler, In.uv).xyz +
					(1.0f - isDiffuseMap)   * diffuse);
	s.specularColor = specular;
	s.roughness = roughness;
	s.metalness = metalness;

	GBuffer.diffuseRoughness	= float4(s.diffuseColor, s.roughness);
	GBuffer.specularMetalness	= float4(s.specularColor, s.metalness);
	GBuffer.normals				= s.N;
	GBuffer.position			= P;
	return GBuffer;
}