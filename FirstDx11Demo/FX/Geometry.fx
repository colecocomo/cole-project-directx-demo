cbuffer constantBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projMatrix;
	matrix normalMatrix;
	float4 eyePos;
	float elapseTime;
}; 

struct VS_Input
{
	float4 pos : POSITION;
	float3 normal : NORMAL;
	float2 tex0 : TEXCOORD0;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float4 eyePos : POSITION;
};


PS_Input VS_Main( VS_Input vertex )
{
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
	
}