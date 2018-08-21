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

//todo: worldViewProj

struct FrameMatrices
{
	matrix viewProj;
	matrix view;
	matrix proj;
};
struct ObjectMatrices
{
	matrix world;
	matrix normalMatrix;
};

cbuffer perFrame
{
	FrameMatrices FrameMats;
}

#define INSTANCE_COUNT 64

cbuffer perModel
{
#ifdef INSTANCED
	ObjectMatrices ObjMats[INSTANCE_COUNT];
#else
	ObjectMatrices ObjMats;
#endif
}

struct VSIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float3 tangent	: TANGENT0;
	float2 texCoord : TEXCOORD0;    

	uint instanceID : SV_InstanceID;
};

struct PSIn
{
	float4 position : SV_POSITION;
};

#ifdef INSTANCED
PSIn VSMain(VSIn In)
#else
PSIn VSMain(VSIn In)
#endif
{
#ifdef INSTANCED
	matrix wvp = mul(FrameMats.proj, mul(FrameMats.view, ObjMats[In.instanceID].world));
#else
	matrix wvp = mul(FrameMats.proj, mul(FrameMats.view, ObjMats.world));
#endif
	PSIn Out;
	Out.position = mul(wvp  , float4(In.position, 1));
	return Out;
}