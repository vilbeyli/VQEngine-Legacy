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

// defines maximum number of dynamic lights
#define LIGHT_COUNT 50  // update CPU define too (SceneManager.cpp)
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
	float lightCount;	// alignment
	Light lights[LIGHT_COUNT];
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

// returns diffuse and specular light
float3 Phong(Light light, float3 N, float3 V, float3 worldPos)
{
    // material properties
    const float shininess = 40.0f;

    float3 L = normalize(light.position - worldPos);
    float3 R = normalize(2 * N * dot(N, L) - L);
    
    float diffuse = max(0.0f, dot(N, L));   // lights
    float3 Id = color * diffuse;
	float3 Is = light.color * pow(max(dot(R, V), 0.0f), shininess) * diffuse;

	// check range and compute attenuation
	float dist = length(light.position - worldPos);
	if (dist >= light.range)
	{
		Id = float3(0, 0, 0);
	}
	else
	{
		Id *= Attenuation(light.attenuation, dist) * light.color;
	}

    return Id + Is;
}

float4 PSMain(PSIn In) : SV_TARGET
{
	// gamma correction
	//bool gammaCorrect = gammaCorrection > 0.99f;
	float gamma = 1.0/2.2;

    float3 N = normalize(In.normal);
    float3 V = normalize(cameraPos - In.worldPos);
	//float3 R = normalize(2*N*max(dot(N, L), 0.0f) - L);
	//if (dot(R,L) < -0.99f)
	//	R = float3(0, 0, 0);


	// lighting parameters
	const float ambient = 0.005f;

	//float3 color = float3(1, 1, 1);
	float3 Ia = color * ambient;
    float3 IdIs = float3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < lightCount; ++i)
    {
        IdIs += Phong(lights[i], N, V, In.worldPos);
    }

	// lighting
        float4 outColor = float4(Ia + IdIs, 1);
	//float4 outColor = float4(N, 1);

	//if(gammaCorrect)
		return pow(outColor, float4(gamma,gamma,gamma,1.0f));
	//else
	//	return outColor;
}