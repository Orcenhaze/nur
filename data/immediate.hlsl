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
	float2 drawing_rect_size;
	float2 line_p0;
	float2 line_p1;
	int    is_line;
}

float sdf_line(float2 p, float2 a, float2 b)
{
    float2 pa = p-a, ba = b-a;
    float h   = clamp(dot(pa,ba)/dot(ba,ba), 0.0, 1.0);
    return length(pa - ba*h);
}

float4 ps(PS_INPUT input) : SV_TARGET
{
	if (is_line) {
		float2 p = input.uv * 2.0 - 1.0;
		
		float thickness = 0.0002;
		float d         = sdf_line(p, line_p0, line_p1) - thickness;
		
		d = smoothstep(0.0, 0.1, d);
		if (d >= 1.0)
			return 0.0;

		d = 0.02/d;
		return float4(input.color.xyz*d, d);
	} else {
		float4 tex = texture0.Sample(sampler0, input.uv);
		return input.color * tex;
	}
}