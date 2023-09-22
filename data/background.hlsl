// Thanks to afl_ext
// 5D wave noise: https://www.shadertoy.com/view/lscyD7

struct VS_INPUT
{
	float2 pos    : POSITION;
	float2 uv     : TEXCOORD;
};

struct PS_INPUT
{
	float4 pos    : SV_POSITION;
	float2 uv     : TEXCOORD;
};

cbuffer PS_Constants : register(b0)
{
	float time;
}

PS_INPUT vs(VS_INPUT input)
{
	PS_INPUT output;
	output.pos = float4(input.pos, 0.0, 1.0);
	output.uv  = input.uv;
	return output;
}

float gw5d_hash(float p) 
{
    return frac(4768.1232345456 * sin(p));
}

float2 gw5d_wavedx(float4 position, float4 direction, float speed, float frequency, float timeshift)
{
    float x    = dot(direction, position) * frequency + timeshift * speed;
    float wave = exp(sin(x) - 1.0);
    float dx   = wave * cos(x);
    return float2(wave, -dx);
}

float4 gw5d_rand_waves()
{
	static float gw5d_seed_waves = 0.0f;
    float x = gw5d_hash(gw5d_seed_waves); gw5d_seed_waves += 1.0;
    float y = gw5d_hash(gw5d_seed_waves); gw5d_seed_waves += 1.0;
    float z = gw5d_hash(gw5d_seed_waves); gw5d_seed_waves += 1.0;
    float w = gw5d_hash(gw5d_seed_waves); gw5d_seed_waves += 1.0;
    return float4(x,y,z,w) * 2.0 - 1.0;
}

float getwaves5d(float4 position, float dragmult, float timeshift)
{
    float iter   = 0.0;
    float phase  = 6.0;
    float speed  = 2.0;
    float weight = 1.0;
    float w      = 0.0;
    float ws     = 0.0;
    for(int i = 0; i < 20; i++) {
    	float4 p   = gw5d_rand_waves() * 1.21;
        float2 res = gw5d_wavedx(position, p, speed, phase, 0.0 + timeshift);
        position  -= normalize(position - p) * res.y * weight * dragmult * 0.01;
        w         += res.x * weight;
        iter      += 12.0;
        ws        += weight;
        weight     = lerp(weight, 0.0, 0.2);
        phase     *= 1.2;
        speed     *= 1.02;
    }

    return w / ws;
}

float3 polar_to_xyz(float2 xy)
{
    xy     *= float2(2.0 *3.1415,  3.1415);
    float z = cos(xy.y);
    float x = cos(xy.x)*sin(xy.y);
    float y = sin(xy.x)*sin(xy.y);
    return normalize(float3(x,y,z));
}

float3 palette(float t) {
    float3 a = float3( 0.208, 0.248, 0.298);
    float3 b = float3( 0.068, 0.028, -0.022);
    float3 c = float3(0.028, 0.338, 1.188);
    float3 d = float3(0.000, 0.333, 0.667);

    return a + b*cos(6.28318*(c*t+d));
}

float4 ps(PS_INPUT input) : SV_TARGET
{
	float2 p = input.uv;

	float waves  = getwaves5d(float4(polar_to_xyz(p), 1.0), 17.0, time/5.2);
    float3 color = palette(time/90.0) * waves;

    return float4(color, 1.0);
}