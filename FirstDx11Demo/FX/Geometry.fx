Texture2D GeometryColorMap : register(t0);
Texture2D GeometryColorMap1 : register(t1);
SamplerState GeometrySampler : register(s0);

SamplerState samWarp
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};
 
cbuffer constantBuffer : register(b0)
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projMatrix;
	matrix normalMatrix;
	matrix texScaleMatrix;
	float4 eyePos;
	float elapseTime;
	float deltaTime;
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
	//float4 eyePos : POSITION;
	float2 tex0 : TEXCOORD0;
};


PS_Input VS_Main( VS_Input vertex )
{
	PS_Input ps;
	ps.pos = mul(vertex.pos, worldMatrix);
	ps.pos = mul(ps.pos, viewMatrix);
	ps.pos = mul(ps.pos, projMatrix);
	ps.normal = mul(vertex.normal, normalMatrix);
	//ps.eyePos = mul(eyePos, worldMatrix);
	//ps.eyePos = mul(ps.eyePos, viewMatrix);
	//ps.eyePos = mul(ps.eyePos, projMatrix);
	ps.tex0 = vertex.tex0;
	ps.tex0.y += 0.00005f * elapseTime;
	ps.tex0.x += 0.0001f * elapseTime;
	//matrix texAniMatrix = mul(float4(ps.tex0, .0f, .0f), texScaleMatrix);
	matrix texAniMatrix = texScaleMatrix;
	texAniMatrix._m21 = ps.tex0.x;
	texAniMatrix._m22 = ps.tex0.y;
	ps.tex0 = mul(ps.tex0, texAniMatrix).xy;

	return ps;
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
	return GeometryColorMap.Sample(samWarp, frag.tex0) * GeometryColorMap1.Sample(samWarp, frag.tex0);
	//return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

technique11 Geometry
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, VS_Main() ) );
		SetPixelShader( CompileShader( ps_4_0, PS_Main() ) );
	}
}