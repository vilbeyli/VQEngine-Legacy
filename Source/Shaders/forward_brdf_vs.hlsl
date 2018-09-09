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

struct ObjectMatrices
{
	matrix worldViewProj;
	matrix world;
	matrix normal;
};

cbuffer perModel
{
#ifdef INSTANCED
	ObjectMatrices ObjMatrices[INSTANCE_COUNT];
#else
	ObjectMatrices ObjMatrices;
#endif
};

cbuffer frame
{
	matrix lightSpaceMat;	// array?
	float fovH;
	float panini;
};


struct VSIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT0;
	float2 texCoord : TEXCOORD0;    
#ifdef INSTANCED
	uint instanceID : SV_InstanceID;
#endif
};

struct PSIn 
{
	float4 position		 : SV_POSITION;
	float3 worldPos		 : POSITION;
	float4 lightSpacePos : POSITION1;	// array?
	float3 normal		 : NORMAL;
    float3 tangent		 : TANGENT;
    float2 texCoord		 : TEXCOORD4;
#ifdef INSTANCED
	uint instanceID	     : SV_InstanceID;
#endif
};


#include "Panini.hlsl"

PSIn VSMain(VSIn In)
{
	const float4 pos = float4(In.position, 1);


	PSIn Out;
#if 0	// experimenting with panini projection, using unreal's implementation
	// src: https://docs.unrealengine.com/latest/INT/Engine/Rendering/PostProcessEffects/PaniniProjection/
	// panini projection
	const float2 screenPos	= Out.position.xy / Out.position.w;
	const float  depth		= Out.position.z  / Out.position.w;
	const float2 paniniScreenPosition = PaniniProjectionScreenPosition(screenPos, fovH);// *0.5f + 0.5f;

	//if(D > 0.01f)
	const float w = Out.position.w;
	const float z = Out.position.z;
	if(w > 0)
	    Out.position = float4(paniniScreenPosition * w, z, w) * panini 
	                 + float4(screenPos            * w, z, w) * (1.0f - panini);
#endif

#ifdef INSTANCED
	Out.position = mul(ObjMatrices[In.instanceID].worldViewProj, pos);
	Out.worldPos = mul(ObjMatrices[In.instanceID].world , pos).xyz;
    Out.normal	 = normalize(mul(ObjMatrices[In.instanceID].normal, In.normal));
    Out.tangent	 = normalize(mul(ObjMatrices[In.instanceID].normal, In.tangent));
	Out.instanceID = In.instanceID;
#else
	Out.position = mul(ObjMatrices.worldViewProj, pos);
	Out.worldPos = mul(ObjMatrices.world , pos).xyz;
    Out.normal	 = normalize(mul(ObjMatrices.normal, In.normal));
    Out.tangent	 = normalize(mul(ObjMatrices.normal, In.tangent));
#endif
	Out.texCoord = In.texCoord;
	Out.lightSpacePos = mul(lightSpaceMat, float4(Out.worldPos, 1));
	return Out;
}