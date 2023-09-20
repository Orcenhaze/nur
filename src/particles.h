#ifndef PARTICLES_H
#define PARTICLES_H

enum Emitter_Texture_Slot
{
    SLOT0,
    SLOT1,
    SLOT2,
    SLOT3,
    
    SLOT_COUNT
};

enum Particle_Type
{
    ParticleType_NONE,
    
    ParticleType_ROTATE,
    ParticleType_WALK,
};

struct Particle
{
    V2  position;
    V2  velocity;
    V4  color;
    f32 scale;
    f32 life;
    s32 type;
    
    // @Note: Emitters can store multiple textures and this tells us which texture to render for this particle.
    Emitter_Texture_Slot slot;
};

struct Particle_Emitter
{
    Array<Particle> particles;
    s32 amount;
    
    // @Cleanup: Maybe there's a better way to do this..?
    Texture texture[SLOT_COUNT];
    
    // @Todo: Add own Random_PCG.
};

////////////////////////////////
// Particles renderer info.
//
struct Particle_Constants
{
    M4x4 object_to_proj_matrix;
    V4   color;
    V2   offset;
    f32  scale;
};
GLOBAL ID3D11InputLayout  *particle_input_layout;
GLOBAL ID3D11Buffer       *particle_vbo;
GLOBAL ID3D11Buffer       *particle_vs_cbuffer;
GLOBAL ID3D11VertexShader *particle_vs;
GLOBAL ID3D11PixelShader  *particle_ps;

#endif //PARTICLES_H
