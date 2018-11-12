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
#ifdef INSTANCED
	matrix matWorld[INSTANCE_COUNT];
	matrix wvp[INSTANCE_COUNT];
#else
	matrix matWorld;
	matrix wvp;
#endif
};

cbuffer perObjec
{
	ObjectMatrices ObjMats;
};

struct VSIn
{
	float3 position : POSITION;

#ifdef INSTANCED
	uint instanceID : SV_InstanceID;
#endif
};

struct PSIn
{
	float4 svPosition     : SV_POSITION;
	float4 worldPosition  : POSITION0;
};


PSIn VSMain(VSIn In)
{
	PSIn vsOut;

#ifdef INSTANCED
	const matrix mWorld = ObjMats.matWorld[instanceID];
	const matrix mWVP = ObjMats.wvp[instanceID];
#else
	const matrix mWorld = ObjMats.matWorld;
	const matrix mWVP = ObjMats.wvp;
#endif

	const float4 vPos = float4(In.position, 1.0f);

	vsOut.worldPosition = mul(mWorld, vPos);
	vsOut.svPosition    = mul(mWVP  , vPos);

	return vsOut;
}