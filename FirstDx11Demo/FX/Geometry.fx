Texture2D GeometryColorMap : register(t0);
SamplerState GeometrySampler : register(s0);
 
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
	PS_Input ps;
	ps.pos = mul(vertex.pos, worldMatrix);
	ps.pos = mul(ps.pos, viewMatrix);
	ps.pos = mul(ps.pos, projMatrix);
	ps.normal = mul(vertex.normal, normalMatrix);
	ps.eyePos = mul(eyePos, worldMatrix);
	ps.eyePos = mul(ps.eyePos, viewMatrix);
	ps.eyePos = mul(ps.eyePos, projMatrix);

	return ps;
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
	//float4 ret = tex2D();
	return float4(.0f, .0f, .0f, .0f);
}

technique11 Geometry
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, VS_Main() ) );
		SetPixelShader( CompileShader( ps_4_0, PS_Main() ) );
	}
}