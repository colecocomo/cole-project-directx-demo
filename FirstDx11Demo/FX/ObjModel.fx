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
	float4 normal : NORMAL;
};

struct PS_Input
{
	float4 pos : SV_POSITION;
};


PS_Input VS_Main( VS_Input vertex )
{
	PS_Input vs_out;
	vs_out.pos = mul(vertex.pos, worldMatrix);
	vs_out.pos = mul(vs_out.pos, viewMatrix);
	vs_out.pos = mul(vs_out.pos, projMatrix);
    return vs_out;
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
    //return colorMap.Sample(colorSampler, frag.tex0);
	float4 ret;
	ret.w = 255;
	ret.x = 112;
	ret.y = 146;
	ret.z = 190;
	return ret;
}