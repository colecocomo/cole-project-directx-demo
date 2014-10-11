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
	float4 posW : POSITION;
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
	ret.posW = mul(vertex.pos, worldMatrix);;

	return ret;
}


float4 PS_Main_Common( PS_Input frag ) : SV_TARGET
{
	float4 ret;
	if(isWall == 1)
	{
		ret = WallColorMap.Sample(sampleWrap, frag.tex0);
	}
	else if(isWall == 2)
	{
		float4 ambient = float4(.08f, .08f, .08f, 0);
		float4 directionLight = float4(.678f, .569f, .329f, 0);
		float3 direction = float3(.0f, 1.0f, .0f );
		frag.normal = normalize(frag.normal);
		float factor = max(dot(direction, frag.normal), 0);

		float4 specularLight = float4(1.0f, .0f, .0f, 0);
		float3 sepcularDirection = float3(.0f, .0f, 1.0f);
		float3 reflectDirect = reflect(sepcularDirection, frag.normal);
		reflectDirect = normalize(reflectDirect);
		float3 eyeDirection = eyePos - frag.posW;
		eyeDirection = normalize(eyeDirection);
		float sepcularFactor = max(dot(reflectDirect, eyeDirection), 0);

		ret = ambient + factor * directionLight + pow(sepcularFactor, 1) * specularLight;
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
