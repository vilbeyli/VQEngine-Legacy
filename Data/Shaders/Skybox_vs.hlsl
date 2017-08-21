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

// source: http://richardssoftware.net/Home/Post/25

struct VSIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT0;
	float2 texCoord : TEXCOORD0;
};

struct PSIn
{
	float4 HPos : SV_POSITION;
	float3 LPos : POSITION;
};

cbuffer matrices {
	matrix worldViewProj;
	float fovH;
};

#include "Panini.hlsl"

PSIn VSMain(VSIn In)
{
	const float4 projectedPos = mul(worldViewProj, In.position).xyww;
	
	PSIn Out;
	Out.LPos = In.position;	// sample direction
	Out.HPos = projectedPos;	// z=w makes depth 1 -> drawn at infinte

	// panini projection
	const float2 screenPos	= projectedPos.xy / projectedPos.w;
	const float  depth		= projectedPos.w  / projectedPos.w;
	const float2 paniniScreenPosition = PaniniProjectionScreenPosition(screenPos, fovH);// *0.5f + 0.5f;

	const float panini = 0.0f;
#if 0
	Out.HPos = float4(paniniScreenPosition * Out.HPos.w, Out.HPos.w, Out.HPos.w);
#else
	Out.HPos = float4(screenPos * Out.HPos.w, Out.HPos.w, Out.HPos.w);
#endif
	//Out.HPos = float4(paniniScreenPosition, depth, 1.0f) * Out.HPos.w * panini 
	//	     + float4(screenPos           , depth, 1.0f) * Out.HPos.w * (1.0f - panini);

	return Out;
}