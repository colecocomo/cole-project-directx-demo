Texture2D WaterColorMap : register(t0);
Texture2D WaterColorMap1 : register(t1);
Texture2D GeometryColorMap : register(t2);

SamplerState samWarp
{
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
};

BlendState blend
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = SRC_COLOR;
	DestBlend[0] = DEST_COLOR;
	BlendOp[0] = ADD;
	SrcBlendAlpha[0] = ZERO;
	DestBlendAlpha[0] = ZERO;
	BlendOpAlpha[0] = ADD;
	RenderTargetWriteMask[0] = 0xf;
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
	float4 eyePos : POSITION;
	float2 tex0 : TEXCOORD0;
};


PS_Input VS_Main( VS_Input vertex )
{
	PS_Input ps;
	ps.pos = mul(vertex.pos, worldMatrix);
	ps.pos = mul(ps.pos, viewMatrix);
	ps.pos = mul(ps.pos, projMatrix);
	ps.normal = mul(vertex.normal, normalMatrix);
	//ps.normal = vertex.normal;
	ps.normal = normalize(ps.normal);
	ps.eyePos = mul(eyePos, worldMatrix);
	ps.eyePos = mul(ps.eyePos, viewMatrix);
	ps.eyePos = mul(ps.eyePos, projMatrix);
	ps.tex0 = vertex.tex0;
	ps.tex0 = mul(ps.tex0, texScaleMatrix).xy;

	return ps;
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
	float4 light = float4(1.0f, 1.0f, 1.0f, .0f);
	float3 lightDirection = float3(-1.0f, 1.0f, -1.0f);
	float factor = dot(frag.normal, lightDirection);
	return GeometryColorMap.Sample(samWarp, frag.tex0) * light * factor;
}

PS_Input Water_VS_Main( VS_Input vertex )
{
	PS_Input ps;
	ps.pos = mul(vertex.pos, worldMatrix);
	ps.pos = mul(ps.pos, viewMatrix);
	ps.pos = mul(ps.pos, projMatrix);
	//ps.normal = mul(vertex.normal, normalMatrix);
	ps.normal = vertex.normal;
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
	ps.tex0 = mul(ps.tex0, texScaleMatrix).xy;

	return ps;
}


float4 Water_PS_Main( PS_Input frag ) : SV_TARGET
{
	float4 ret = float4(1.0f, 1.0f, 1.0f, .5f);
	ret.xyz = (WaterColorMap.Sample(samWarp, frag.tex0) * WaterColorMap1.Sample(samWarp, frag.tex0)).xyz;
	return ret;
}

technique11 Geometry
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, VS_Main() ) );
		SetPixelShader( CompileShader( ps_4_0, PS_Main() ) );
	}

	pass p1
	{
		SetVertexShader( CompileShader( vs_4_0, Water_VS_Main() ) );
		SetPixelShader( CompileShader( ps_4_0, Water_PS_Main() ) );
		SetBlendState(blend, float4(.0f, .0f, .0f, .0f), 0xffffffff);
	}
}