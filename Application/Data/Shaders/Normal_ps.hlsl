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
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 texCoord : TEXCOORD4;
};

cbuffer renderConsts
{
	float gammaCorrection;
};

cbuffer perObj
{
    float isNormalMap;
};


Texture2D gNormalMap;
SamplerState samAnisotropic
{
    Filter = ANISOTROPIC;
    MaxAnisotropy = 4;
    AddressU = WRAP;
    AddressV = WRAP;
};

inline float3 UnpackNormals(float2 uv, float3 vertNormal, float3 vertTangent)
{
	// uncompressed normal in tangent space
    float3 TNormal = gNormalMap.Sample(samAnisotropic, uv).xyz;
	TNormal = TNormal.xzy;
	//TNormal.y *= -1.0f;
	//TNormal.z *= -1.0f;

	TNormal = normalize(TNormal * 2.0f - 1.0f);
	float3 N = normalize(vertNormal);


    // float3 T = WTangent;	// after interpolation, T and N might not be orthonormal
	// make sure T is orthonormal to N by subtracting off any component of T along direction N.
	float3 T = normalize(vertTangent - dot(vertNormal, vertTangent) * vertNormal);
    float3 B = normalize(cross(N, T));
	float3x3 TBN = float3x3(T, B, N);
    //float3x3 TBN = float3x3(T, N, B);
    
	
	//TBN = transpose(TBN);
	return mul(TBN, TNormal);
    //return TNormal;
    //return T;
}

float4 PSMain(PSIn In) : SV_TARGET
{
	// gamma correction
	bool gammaCorrect = gammaCorrection > 0.99f;
	float gamma = 1.0/2.2;

    float3 N = normalize(In.normal);
    float3 T = normalize(In.tangent);
    float3 n = (isNormalMap)        * UnpackNormals(In.texCoord, N, T) +
               (1.0f - isNormalMap) * N;
	float4 outColor = float4(n,1);
	

	if(gammaCorrect)
		return pow(outColor, float4(gamma,gamma,gamma,1.0f));
	else
		return outColor;
}