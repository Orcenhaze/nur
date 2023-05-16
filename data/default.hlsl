struct VS_INPUT
{
	float3 pos    : POSITION;
	float4 color  : COLOR;
	float3 normal : NORMAL;
	float2 uv     : TEXCOORD;
};

struct PS_INPUT
{
	float4 pos    : SV_POSITION;
	float4 color  : COLOR;
	float3 normal : NORMAL;
	float2 uv     : TEXCOORD;
};

cbuffer VS_Constants : register(b0)
{
	float4x4 object_to_proj_matrix;
}

sampler sampler0 : register(s0);

Texture2D<float4> texture0 : register(t0); 

PS_INPUT vs(VS_INPUT input)
{
	PS_INPUT output;
	output.pos    = mul(object_to_proj_matrix, float4(input.pos, 1.0f));
	output.color  = input.color;
	output.normal = input.normal;
	output.uv     = input.uv;
	return output;
}

cbuffer PS_Constants : register(b1)
{
	float4 placeholder;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
	float4 tex = texture0.Sample(sampler0, input.uv);
	return input.color * tex;
}