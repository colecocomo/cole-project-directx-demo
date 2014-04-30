cbuffer perFramView : register(b0)
{
	matrix viewMatrix;
}; 

cbuffer perFramProj : register(b1)
{
	matrix projMatrix;
}; 

cbuffer perFramWorld : register(b2)
{
	matrix worldMatrix;
}; 

struct VS_Input
{
	float4 pos : POSITION;
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
	ret.xyzw = 255;
	return ret;
}