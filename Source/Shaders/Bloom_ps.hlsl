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

struct PSIn
{
	float4 position : SV_POSITION;
	float2 texCoord : TEXCOORD0;
};

struct PSOut
{
	float4 color		: SV_TARGET0;
	float4 brightColor	: SV_TARGET1;
};

Texture2D colorInput;
SamplerState samTriLinearSam
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = Wrap;
	AddressV = Wrap;
};

cbuffer BloomParams 
{
	float BrightnessThreshold;
};

PSOut PSMain(PSIn In) : SV_TARGET
{
    const float4 color = colorInput.Sample(samTriLinearSam, In.texCoord);
	
	// clamp values that is made very large by NDF in BRDF
	// this reduces bloom flickering
	const float BrightnessUpperThreshold = 6.0f;	
	const float brightness = dot(float3(0.2126, 0.7152, 0.0722), color.xyz); // luma conversion

	// source: https://learnopengl.com/#!Advanced-Lighting/Bloom
	PSOut _out;
	_out.color = color;
	if (brightness > BrightnessThreshold && brightness < BrightnessUpperThreshold)
		_out.brightColor = color;
	else
		_out.brightColor = float4(0, 0, 0, 1);
	return _out;
}