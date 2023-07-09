struct VS_INPUT
{
	float2 pos    : POSITION;
	float2 uv     : TEXCOORD;
};

struct PS_INPUT
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
	float4 color  : COLOR;
};

cbuffer Particle_Constant : register(b0)
{
	float4x4 object_to_proj_matrix;
    float4   color;
    float2   offset;
}

sampler sampler0 : register(s0);
Texture2D<float4> texture0 : register(t0); 

PS_INPUT vs(VS_INPUT input)
{
	PS_INPUT output;
	float scale  = 0.2f;
	output.pos   = mul(object_to_proj_matrix, float4((input.pos * scale) + offset, 0.0f, 1.0f));
	output.uv    = input.uv;
	output.color = color;
	return output;
}

float4 ps(PS_INPUT input) : SV_TARGET
{
	float4 tex = texture0.Sample(sampler0, input.uv);
	return input.color * tex;
}