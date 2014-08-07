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
};

struct PS_Input
{
	float4 pos : SV_POSITION;
	float3 normal : NORMAL;
	float4 eyePos : POSITION;
};


PS_Input VS_Main( VS_Input vertex )
{
	PS_Input vs_out;
	vs_out.pos = mul(vertex.pos, worldMatrix);
	vs_out.pos = mul(vs_out.pos, viewMatrix);
	vs_out.pos = mul(vs_out.pos, projMatrix);

	vs_out.normal = mul(vertex.normal, normalMatrix);

	vs_out.eyePos = mul(eyePos, worldMatrix);
	vs_out.eyePos = mul(vs_out.eyePos, viewMatrix);
	vs_out.eyePos = mul(vs_out.eyePos, projMatrix);
    return vs_out;
}


float4 PS_Main( PS_Input frag ) : SV_TARGET
{
	float4 ambient = float4(.08f, .08f, .08f, 0);

	float4 directionLight = float4(.678f, .569f, .329f, 0);
	float3 direction = float3(.0f, 1.0f, .0f );
	float factor = max(dot(direction, frag.normal), 0);

	float4 specularLight = float4(1.0f, .0f, .0f, 0);
	float3 sepcularDirection = float3(.0f, .0f, 1.0f);
	float3 reflectDirect = reflect(sepcularDirection, frag.normal);
	reflectDirect = normalize(reflectDirect);
	float3 eyeDirection = frag.eyePos - frag.pos;
	eyeDirection = normalize(eyeDirection);
	float sepcularFactor = max(dot(reflectDirect, eyeDirection), 0);

	//return ambient + factor * directionLight + pow(sepcularFactor, 1) * specularLight;

	// µ„π‚‘¥
	float4 pointLightPos = float4(10.0f, 20.0f, 0.f, .0f);
	float4 pointLightColr = float4(.678f, .569f, .329f, 0);
	float4 pointLightDirection = pointLightPos - frag.pos;
	float dis = length(pointLightDirection);
	pointLightDirection = normalize(pointLightDirection);
	float pointLightFactor = max(dot(pointLightDirection, frag.normal), 0);

	// with toon light
 	if(pointLightFactor < .0001f)
 	{
	 		pointLightFactor = 0.4;
 	}
 	else if(pointLightFactor > .0001f && pointLightFactor <= .5f)
 	{
 		pointLightFactor = 0.6;
 	}
 	else if(pointLightFactor > .5f && pointLightFactor <= 1.0f)
 	{
 		pointLightFactor = 1.0;
 	}

	float3 pointLightRelectDirect = reflect(pointLightDirection * -1, frag.normal);
	float pointLightSpecularFactor = max(dot(pointLightRelectDirect, eyeDirection), 0);

	// with toon light
 	if(pointLightSpecularFactor <= .1f && pointLightSpecularFactor >= .0f)
 	{
 		pointLightSpecularFactor = .0;
 	}
 	else if(pointLightSpecularFactor > .1f && pointLightSpecularFactor <= .8f)
 	{
 		pointLightSpecularFactor = 0.5;
 	}
 	else if(pointLightSpecularFactor > .8f && pointLightSpecularFactor <= 1.0f)
 	{
 		pointLightSpecularFactor = 0.8;
 	}

	//return ambient + (pointLightFactor * pointLightColr + pow(pointLightSpecularFactor, 4) * pointLightColr) / (1/* + 2 * dis + 3 * pow(dis,2)*/);

	// spot light
	float4 spotLightPos = float4(420.0f, 294.80f, -10.f, 1.0f);
	float4 spotLightColr = float4(.678f, .0f, .0f, 1.0f);
	float4 spotLightOriginDirection = float4(.0f, .0f, 1.0f, 1.0f);
	float4 spotLightDirection = spotLightPos - frag.pos;
	float spotDis = length(spotLightDirection);
	spotLightDirection = normalize(spotLightDirection);
	float spotLightFactor = max(dot(spotLightDirection, frag.normal), 0);

	float3 spotLightRelectDirect = reflect(spotLightDirection * -1, frag.normal);
	float spotLightSpecularFactor = max(dot(spotLightRelectDirect, eyeDirection), 0);

	float kSpot = max(dot(spotLightDirection * -1, spotLightOriginDirection), 0);// pow(spotLightFactor, 1);
	kSpot = pow(kSpot, 3);

	return kSpot * (ambient + (spotLightFactor * spotLightColr + pow(spotLightSpecularFactor, 4) * spotLightColr) / (1/* + 2 * dis + 3 * pow(dis,2)*/));
}