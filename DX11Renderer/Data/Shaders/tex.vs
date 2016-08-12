cbuffer MatrixBuffer
{
	matrix world;
	matrix view;
	matrix	proj;
}

struct VSIn
{
	float3 position : POSITION;
	float3 normal	: NORMAL;
	float2 texCoord : TEXCOORD0;
};

struct PSIn
{
	float4 position : SV_POSITION;
	float3 normal	: NORMAL;
	float2 texCoord : TEXCOORD0;
};

PSIn VSMain(VSIn In)
{
	PSIn Out;
	
	Out.position = float4(In.position, 1);
	Out.normal = In.normal;
	Out.texCoord = In.texCoord;
	
	return Out;
}