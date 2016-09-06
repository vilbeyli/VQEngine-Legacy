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
	float3 worldPos : POSITION;
	float3 normal	: NORMAL;
	float2 texCoord : TEXCOORD0;
};

struct Light
{
	float3 position;
	float3 color;

	// spotlight

	//point
	float2 attenuation;
	float range;
};

cbuffer renderConsts
{
	//float gammaCorrection;
	float3 cameraPos;
	float pad;		// currently necessary to align 16-bit
	Light light;
//	float ambient;
};

cbuffer perObject
{
	float3 color;
};

float Attenuation(float2 coeffs, float dist)
{
	return 1.0f / (
		1.0f
	+	coeffs[0] * dist
	+	coeffs[1] * dist * dist
	);

	//return 1.0f / (	// range 13
	//	1.0f
	//+ 0.7f * dist
	//+ 1.8f * dist * dist
	//);

	//return 1.0f / (	// range 13
	//	1.0f
	//+ 0.0450f * dist
	//+ 0.0075f * dist * dist
	//);
}

float4 PSMain(PSIn In) : SV_TARGET
{
	// gamma correction
	//bool gammaCorrect = gammaCorrection > 0.99f;
	float gamma = 1.0/2.2;

	float3 N = normalize(In.normal);
	float3 L = normalize(light.position - In.worldPos);
	float3 V = normalize(cameraPos - In.worldPos);
	float3 R = normalize(2*N*dot(N, L) - L);
	//float3 R = normalize(2*N*max(dot(N, L), 0.0f) - L);
	//if (dot(R,L) < -0.99f)
	//	R = float3(0, 0, 0);


	// lighting parameters
	const float ambient = 0.005f;
	const float shininess = 40.0f;
	float diffuse = max(0.0f, dot(N, L));

	//float3 color = float3(1, 1, 1);
	float3 Ia = color * ambient;
	float3 Id = color * diffuse;
	float3 Is = light.color * pow(max(dot(R, V), 0.0f), shininess);

	// check range and compute attenuation
	float dist = length(light.position - In.worldPos);
	//float dist = length(float3(0,10,0) - In.worldPos);
	if (dist >= light.range)
	{
		Id = float3(0, 0, 0);
		//Id = -Ia;
	}
	else
	{
		Id *= Attenuation(light.attenuation, dist) * light.color;
	}

	// lighting
	float4 outColor = float4(Ia + Id + Is, 1);
	//float4 outColor = float4(N, 1);
	//float4 outColor = float4(V + Id * 0.0001, 1);
	//float4 outColor = float4(Is, 1);

	//if(gammaCorrect)
		return pow(outColor, float4(gamma,gamma,gamma,1.0f));
	//else
	//	return outColor;
}