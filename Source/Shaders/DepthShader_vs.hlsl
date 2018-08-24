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
	matrix wvp;
};

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

#ifdef INSTANCED
	uint instanceID : SV_InstanceID;
#endif
};

struct PSIn
{
	float4 position : SV_POSITION;
};

PSIn VSMain(VSIn In)
{
	PSIn Out;
#ifdef INSTANCED
	Out.position = mul(ObjMats[In.instanceID].wvp, float4(In.position, 1));
#else
	Out.position = mul(ObjMats.wvp  , float4(In.position, 1));
#endif

	return Out;
}