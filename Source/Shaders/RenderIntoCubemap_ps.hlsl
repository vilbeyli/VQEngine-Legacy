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

#define PI 3.14159265359f
float2 SphericalSample(float3 v)
{
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb509575(v=vs.85).aspx
	// The signs of the x and y parameters are used to determine the quadrant of the return values 
	// within the range of -PI to PI. The atan2 HLSL intrinsic function is well-defined for every point 
	// other than the origin, even if y equals 0 and x does not equal 0.
    float2 uv = float2(atan2(v.z, v.x), asin(-v.y));
    uv /= float2(2 * PI, PI);
    uv += float2(0.5, 0.5);
    return uv;
}

struct PSIn
{
	float4 HPos : SV_POSITION;
	float3 LPos : POSITION;
};

struct PSOut
{
	float4 cubeMapFace : SV_TARGET0;
};

Texture2D tEnvironmentMap;
SamplerState sLinear;

PSOut PSMain(PSIn In)
{
	PSOut cubeMap;
	const float2 uv = SphericalSample(normalize(In.LPos));
	cubeMap.cubeMapFace = tEnvironmentMap.Sample(sLinear, uv);
	return cubeMap;
}