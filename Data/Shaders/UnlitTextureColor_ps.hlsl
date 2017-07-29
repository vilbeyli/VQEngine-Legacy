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

struct PSIn
{
	float4 position : SV_POSITION;
	float3 normal	: NORMAL;
	float2 texCoord : TEXCOORD0;
};

cbuffer renderConsts
{
	float gammaCorrection;
};

cbuffer perObject
{
	float3 diffuse;
    float isDiffuseMap;
};

Texture2D gDiffuseMap;
SamplerState samAnisotropic
{
    Filter = ANISOTROPIC;
    MaxAnisotropy = 4;
    AddressU = WRAP;
    AddressV = WRAP;
};

float4 PSMain(PSIn In) : SV_TARGET
{
	// gamma correction
	bool gammaCorrect = gammaCorrection > 0.99f;
	//const float gammaa = 1.0 / 2.2;
	//const float gamma = 1.0 / 2.2;
	//const float gamma = 2.2;
	//const float gamma = 1.0;

	float2 uv = In.texCoord;
    float4 outColor = isDiffuseMap          * gDiffuseMap.Sample(samAnisotropic, uv) * float4(diffuse, 1) 
                    + (1.0f - isDiffuseMap) * float4(diffuse,1);
	
	const float gamma = 2.2f * (1.0f - isDiffuseMap) + isDiffuseMap * 1.0f;

	if(gammaCorrect)
		return pow(outColor, float4(gamma,gamma,gamma,1.0f));
	else
		//return pow(outColor, float4(gammaa, gammaa, gammaa, 1.0f));
	return outColor;
}