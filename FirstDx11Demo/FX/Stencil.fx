Texture2D WallColorMap : register(t0);
Texture2D FloorColorMap : register(t1);

SamplerState sampleWrap
{
	filter = min_mag_mip_linear;
	addressu = wrap;
	addressv = wrap;
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

cbuffer constantBuffer1 : register(b1)
{	
	int  isWall;
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

PS_Input VS_Main_Common( VS_Input vertex )
{
	PS_Input ret;
	ret.pos = mul(vertex.pos, worldMatrix);
	ret.pos = mul(ret.pos, viewMatrix);
	ret.pos = mul(ret.pos, projMatrix);

	ret.normal = mul(vertex.normal, normalMatrix);
	ret.tex0 = mul(vertex.tex0, texScaleMatrix);

	ret.eyePos = mul(eyePos, worldMatrix);
	ret.eyePos = mul(ret.eyePos, viewMatrix);
	ret.eyePos = mul(ret.eyePos, projMatrix);

	return ret;
}


float4 PS_Main_Common( PS_Input frag ) : SV_TARGET
{
	float4 ret;
	if(isWall != 0)
	{
		ret = WallColorMap.Sample(sampleWrap, frag.tex0);
	}
	else
	{
		ret = FloorColorMap.Sample(sampleWrap, frag.tex0);
	}

	return ret;
}

technique11 Mirror
{
	pass p0
	{
		SetVertexShader( CompileShader(vs_4_0, VS_Main_Common()) );
		SetPixelShader( CompileShader(ps_4_0, PS_Main_Common()) );
	}
};
