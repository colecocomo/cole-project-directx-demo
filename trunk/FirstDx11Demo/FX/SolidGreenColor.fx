Texture2D colorMap : register(t0);
SamplerState colorSampler : register(s0);

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
	vs_out.pos = vertex.pos;
	vs_out.tex0 = vertex.tex0;
    return vs_out;
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
    return colorMap.Sample(colorSampler, frag.tex0);
}