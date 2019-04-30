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


struct VSIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT0;
	float2 uv		: TEXCOORD0;
#ifdef INSTANCED
	uint instanceID : SV_InstanceID;
#endif
};

struct PSIn
{
	float4 position			: SV_POSITION;
	float3 viewPosition		: POSITION0;
	float3 viewNormal		: NORMAL;
	float3 viewTangent		: TANGENT;
	float2 uv				: TEXCOORD1;
#ifdef INSTANCED
	uint instanceID			: SV_InstanceID;
#endif
};

struct ObjectMatrices
{
	matrix worldView;
	matrix normalViewMatrix;
	matrix worldViewProj;
};


#define ENABLE_HEIGHTMAPPING 0

#if ENABLE_HEIGHTMAPPING
#include "LightingCommon.hlsl"
cbuffer cbSurfaceMaterial
{
#ifdef INSTANCED
    SurfaceMaterial surfaceMaterial[INSTANCE_COUNT];
#else
	SurfaceMaterial surfaceMaterial;
#endif
    float BRDFOrPhong;
};

Texture2D texHeightMap;
SamplerState sNormalSampler;
#endif

cbuffer perModel
{
#ifdef INSTANCED
	ObjectMatrices ObjMatrices[INSTANCE_COUNT];
#else
	ObjectMatrices ObjMatrices;
#endif
};


PSIn VSMain(VSIn In)
{
	const float4 pos = float4(In.position, 1);

	PSIn Out = (PSIn)0;
#ifdef INSTANCED
	Out.position	 = mul(ObjMatrices[In.instanceID].worldViewProj, pos);
	Out.viewPosition = mul(ObjMatrices[In.instanceID].worldView, pos).xyz;
	Out.viewNormal	 = normalize(mul(ObjMatrices[In.instanceID].normalViewMatrix, In.normal));
	Out.viewTangent	 = normalize(mul(ObjMatrices[In.instanceID].normalViewMatrix, In.tangent));
	Out.instanceID	 = In.instanceID;
#else
	//Out.position	 = mul(ObjMatrices.worldViewProj, pos);
	float4 clipPos   = mul(ObjMatrices.worldViewProj, pos);
	Out.viewPosition = mul(ObjMatrices.worldView, pos).xyz;
	Out.viewNormal	 = normalize(mul(ObjMatrices.normalViewMatrix, half4(In.normal , 0))).rgb;
	Out.viewTangent	 = normalize(mul(ObjMatrices.normalViewMatrix, float4(In.tangent, 0))).rgb;

	const float fHeightIntensity = 0.90f;
#if ENABLE_HEIGHTMAPPING
    if(HasHeightMap(surfaceMaterial.textureConfig) != 0)
    {
        float Height = texHeightMap.SampleLevel(sNormalSampler, In.uv, 0).r;
		float4 bumpedPos = float4(pos + normalize(In.normal) * Height * fHeightIntensity, 1.0f);
		Out.position = mul(ObjMatrices.worldViewProj, bumpedPos);
		Out.viewPosition = mul(ObjMatrices.worldView, bumpedPos).xyz;
		//Out.position.xyz = clipPos.xyz;
		//Out.position.w = clipPos.w;
    }
	else
#endif
	{
		Out.position = clipPos;
	}
#endif
	Out.uv				= In.uv;
	return Out;
}