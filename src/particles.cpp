
FUNCTION void create_particle_shader()
{
    // Shader input layout.
    D3D11_INPUT_ELEMENT_DESC layout_desc[] = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,          D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, sizeof(V2), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    f32 unit_quad[] =
    {
        // Vertex origin in bottom-left. 
        // UV origin is top-left.
        
        // POSITION     TEXCOORD
        -0.5f, -0.5f,  0.0f,  1.0f,
        0.5f,  -0.5f,  1.0f,  1.0f,
        0.5f,   0.5f,  1.0f,  0.0f,
        
        -0.5f, -0.5f,  0.0f,  1.0f,
        0.5f,   0.5f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.0f,  0.0f,
    };
    
    //
    // Vertex buffer.
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = sizeof(unit_quad);
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        desc.Usage          = D3D11_USAGE_IMMUTABLE;
        
        D3D11_SUBRESOURCE_DATA data = { unit_quad }; 
        device->CreateBuffer(&desc, &data, &particle_vbo);
    }
    
    //
    // Constant buffers.
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = ALIGN_UP(sizeof(Particle_Constants), 16);
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        device->CreateBuffer(&desc, 0, &particle_vs_cbuffer);
    }
    
    Arena_Temp scratch = get_scratch(0, 0);
    String8 hlsl_path  = sprint(scratch.arena, "%Sparticle.hlsl", os->data_folder);
    d3d11_compile_shader(hlsl_path, layout_desc, ARRAYSIZE(layout_desc), &particle_input_layout, &particle_vs, &particle_ps);
    free_scratch(scratch);
}

FUNCTION void obj_emitter_init(s32 amount)
{
    Particle_Emitter *e = &game->obj_emitter;
    
    array_init(&e->particles, amount);
    e->amount = amount;
    
    for (s32 i = 0; i < SLOT_COUNT; i++) {
        if (e->texture[i].view == 0)
            e->texture[i] = white_texture;
    }
    
    for (s32 i = 0; i < amount; i++) {
        Particle p = {};
        p.color    = v4(1.0f);
        array_add(&e->particles, p);
    }
}

FUNCTION void particles_init()
{
    create_particle_shader();
    obj_emitter_init(500);
}

FUNCTION s32 obj_emitter_get_first_unused()
{
    Particle_Emitter *e = &game->obj_emitter;
    
    // Quick check.
    LOCAL_PERSIST s32 last_used = 0;
    for (s32 i = last_used; i < e->amount; i++) {
        if (e->particles[i].life <= 0.0f) {
            last_used = i;
            return i;
        }
    }
    
    // Linear check.
    for (s32 i = 0; i < e->amount; i++) {
        if (e->particles[i].life <= 0.0f) {
            last_used = i;
            return i;
        }
    }
    
    // All particles are being used. Overwrite the first one.
    last_used = 0;
    return 0;
}

FUNCTION void obj_emitter_respawn_particle(Particle *p, s32 type, Emitter_Texture_Slot slot, V2 offset)
{
    p->type = type;
    p->slot = slot;
    
    if (type == ParticleType_ROTATE) {
        V2 random   = random_range_v2(&game->rng, v2(-0.45f), v2(0.45f));
        f32 r_col   = random_rangef(&game->rng, 0.5f, 1.0f);
        p->position = offset + random;
        p->color    = v4(r_col, r_col, r_col, 1.0f);
        p->velocity = v2(0.0f, 0.5f);
        p->life     = 1.0f;
        p->scale    = random_rangef(&game->rng, 0.15f, 0.25f);
    } else if (type == ParticleType_WALK) {
        p->position = offset + v2(0);
        p->color    = v4(1.0f);
        p->velocity = -fdirs[pdir] * 1.5f;
        p->life     = 0.75f;
        p->scale    = 0.45f;
    }
}

FUNCTION void obj_emitter_emit(s32 num_new_particles, s32 type, Emitter_Texture_Slot slot, V2 offset = v2(0))
{
    Particle_Emitter *e = &game->obj_emitter;
    
    // Add new particles.
    for (s32 i = 0; i < num_new_particles; i++) {
        s32 unused_particle = obj_emitter_get_first_unused();
        obj_emitter_respawn_particle(&e->particles[unused_particle], type, slot, offset);
    }
}

FUNCTION void obj_emitter_update_particles()
{
    Particle_Emitter *e = &game->obj_emitter;
    f32 dt = os->dt;
    
    // Update all particles.
    for (s32 i = 0; i < e->amount; i++) {
        Particle *p = &e->particles[i];
        p->life -= dt;
        
        if (p->life > 0.0f) {
            
            if (p->type == ParticleType_ROTATE) {
                f32 intensity = 0.5f;
                V2 shake      = intensity * random_range_v2(&game->rng, v2(-1), v2(1));
                p->position += (p->velocity + shake) * dt;
                p->velocity += shake * dt * 0.6f;
                p->color.a  -= 1.0f * dt;
            } else if (p->type == ParticleType_WALK) {
                p->position += p->velocity * dt;
                //p->velocity += p->velocity * dt * 1.5f;
                p->color.a  -= 2.2f * dt;
                p->scale    -= 1.2f * dt;
            }
            
        }
    }
}

FUNCTION void obj_emitter_draw_particles()
{
    Particle_Emitter *e = &game->obj_emitter;
    
    // Bind Input Assembler.
    device_context->IASetInputLayout(particle_input_layout);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(V4);
    UINT offset = 0;
    device_context->IASetVertexBuffers(0, 1, &particle_vbo, &stride, &offset);
    
    // Vertex shader.
    device_context->VSSetConstantBuffers(0, 1, &particle_vs_cbuffer);
    device_context->VSSetShader(particle_vs, 0, 0);
    
    // Rasterizer Stage.
    device_context->RSSetViewports(1, &viewport);
    device_context->RSSetState(rasterizer_state);
    
    // Pixel Shader.
    device_context->PSSetSamplers(0, 1, &sampler0);
    //device_context->PSSetShaderResources(0, 1, &e->texture.view);
    device_context->PSSetShader(particle_ps, 0, 0);
    
    // Output Merger.
    device_context->OMSetDepthStencilState(depth_state, 0);
    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);
    
    // For each alive particle.
    for (s32 i = 0; i < e->amount; i++) {
        Particle *p = &e->particles[i];
        if (p->life > 0.0f) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            device_context->Map(particle_vs_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
            Particle_Constants *constants = (Particle_Constants *) mapped.pData;
            
            constants->object_to_proj_matrix = view_to_proj_matrix.forward * world_to_view_matrix.forward;
            constants->color  = p->color;
            constants->offset = p->position;
            constants->scale  = p->scale;
            
            device_context->Unmap(particle_vs_cbuffer, 0);
            
            // Set texture.
            device_context->PSSetShaderResources(0, 1, &e->texture[p->slot].view);
            
            // Output Merger.
            if (p->type == ParticleType_WALK)
                device_context->OMSetBlendState(blend_state, 0, 0XFFFFFFFFU);
            else
                device_context->OMSetBlendState(blend_state_one, 0, 0XFFFFFFFFU);
            
            // Draw.
            device_context->Draw(6, 0);
        }
    }
}
