Texture2D colorMap : register(t0);
Texture2D colorMap2 : register(t1);
SamplerState colorSampler : register(s0);

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
	vs_out.pos = mul(vertex.pos, worldMatrix);
	vs_out.pos = mul(vs_out.pos, viewMatrix);
	vs_out.pos = mul(vs_out.pos, projMatrix);
	vs_out.tex0 = vertex.tex0;
    return vs_out;
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
    return 1.0f - colorMap.Sample(colorSampler, frag.tex0);
}

float4 PS_Shift(PS_Input frag) : SV_TARGET
{
	float4 color  = colorMap.Sample(colorSampler, frag.tex0);
	float4 outColor;

	outColor.x = color.y;
	outColor.y = color.z;
	outColor.z = color.x;
	outColor.w = color.w;

	return outColor;
}

float4 PS_MultiTexture(PS_Input frag) : SV_TARGET
{
	return colorMap.Sample(colorSampler, frag.tex0) * colorMap2.Sample(colorSampler, frag.tex0);
}

technique11 ColorInversion
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_4_0, VS_Main() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_Main() ) );

	}

	pass P1
	{
		SetVertexShader( CompileShader( vs_4_0, VS_Main() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_Shift() ) );
	}

	pass P2
	{
		SetVertexShader( CompileShader( vs_4_0, VS_Main() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS_MultiTexture() ) );
	}
}