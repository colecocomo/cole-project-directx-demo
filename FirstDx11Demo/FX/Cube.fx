Texture2D colorMap : register(t0);
SamplerState colorSampler : register(s0);

cbuffer constantBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projMatrix;
	float elapseTime;
}; 

struct VS_Input
{
	float4 pos : POSITION;
	float2 tex0 : TEXCOORD0;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float2 tex0 :  TEXCOORD0;
};


PS_Input VS_Main( VS_Input vertex )
{
	PS_Input vs_out;
	vertex.pos.xy += 0.5f*sin(vertex.pos.x)*sin(3.0f*elapseTime);
	vertex.pos.z *= 0.6f + 0.4f*sin(2.0f*elapseTime);
	vs_out.pos = mul(vertex.pos, worldMatrix);
	vs_out.pos = mul(vs_out.pos, viewMatrix);
	vs_out.pos = mul(vs_out.pos, projMatrix);
	vs_out.tex0 = vertex.tex0;
    return vs_out;
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
    return colorMap.Sample(colorSampler, frag.tex0);
}