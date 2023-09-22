
FUNCTION void create_background_shader()
{
    // Shader input layout.
    D3D11_INPUT_ELEMENT_DESC layout_desc[] = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0,          0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(V2), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    f32 screen_quad[] =
    {
        // Vertex origin in bottom-left. 
        // UV origin is top-left.
        
        // POSITION    TEXCOORDS
        -1.0f, -1.0f, 0.0f,  1.0f,
        1.0f, -1.0f,  1.0f,  1.0f,
        1.0f, 1.0f,   1.0f,  0.0f,
        
        -1.0f, -1.0f, 0.0f,  1.0f,
        1.0f, 1.0f,   1.0f,  0.0f,
        -1.0f, 1.0f,  0.0f,  0.0f,
    };
    
    //
    // Vertex buffer.
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = sizeof(screen_quad);
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        desc.Usage          = D3D11_USAGE_IMMUTABLE;
        
        D3D11_SUBRESOURCE_DATA data = { screen_quad }; 
        device->CreateBuffer(&desc, &data, &background_vbo);
    }
    
    //
    // Constant buffers.
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = ALIGN_UP(sizeof(Background_PS_Constants), 16);
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        device->CreateBuffer(&desc, 0, &background_ps_cbuffer);
    }
    
    Arena_Temp scratch = get_scratch(0, 0);
    String8 hlsl_path  = sprint(scratch.arena, "%Sbackground.hlsl", os->data_folder);
    d3d11_compile_shader(hlsl_path, layout_desc, ARRAYSIZE(layout_desc), &background_input_layout, &background_vs, &background_ps);
    free_scratch(scratch);
}

FUNCTION void background_init()
{
    create_background_shader();
}

FUNCTION void background_draw()
{
    // Bind Input Assembler.
    device_context->IASetInputLayout(background_input_layout);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(V4);
    UINT offset = 0;
    device_context->IASetVertexBuffers(0, 1, &background_vbo, &stride, &offset);
    
    // Vertex shader.
    device_context->VSSetShader(background_vs, 0, 0);
    
    // Rasterizer Stage.
    device_context->RSSetViewports(1, &viewport);
    device_context->RSSetState(rasterizer_state);
    
    // Pixel Shader.
    Background_PS_Constants constants = { (f32)os->time };
    D3D11_MAPPED_SUBRESOURCE mapped;
    device_context->Map(background_ps_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    MEMORY_COPY(mapped.pData, &constants, sizeof(constants));
    device_context->Unmap(background_ps_cbuffer, 0);
    device_context->PSSetConstantBuffers(0, 1, &background_ps_cbuffer);
    device_context->PSSetSamplers(0, 1, &sampler0);
    device_context->PSSetShader(background_ps, 0, 0);
    
    // Output Merger.
    device_context->OMSetDepthStencilState(depth_state, 0);
    device_context->OMSetBlendState(blend_state, 0, 0XFFFFFFFFU);
    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);
    
    // Draw.
    device_context->Draw(6, 0);
}
