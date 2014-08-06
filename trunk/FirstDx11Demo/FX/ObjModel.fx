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

	float4 pointLightPos = float4(10.0f, 20.0f, 0.f, .0f);
	float4 pointLightColr = float4(.678f, .569f, .329f, 0);
	float4 pointLightDirection = pointLightPos - frag.pos;
	float dis = length(pointLightDirection);
	pointLightDirection = normalize(pointLightDirection);
	float pointLightFactor = max(dot(pointLightDirection, frag.normal), 0);
	float3 pointLightRelectDirect = reflect(pointLightDirection * -1, frag.normal);
	float pointLightSpecularFactor = max(dot(pointLightRelectDirect, eyeDirection), 0);
	return ambient + (pointLightFactor * pointLightColr + pow(pointLightSpecularFactor, 4) * pointLightColr) / (1/* + 2 * dis + 3 * pow(dis,2)*/);
}