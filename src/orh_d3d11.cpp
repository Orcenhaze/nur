/* orh_d3d11.cpp - v0.01 - C++ D3D11 immediate mode renderer.

#include "orh.h" before this file.

CONVENTIONS:
* CCW winding order for front faces.
* Right-handed coordinate system. +Y is up.
* Matrices are row-major with column vectors.
* First pixel pointed to is top-left-most pixel in image.
* UV-coords origin is at top-left corner (DOESN'T match with vertex coordinates).
* When storing paths, if string name has "folder" in it, then it ends with '/' or '\\'.

TODO:
[] World-space text rendering.
[] Make a generic orh_renderer.h that the game can use without directly talking to specific graphics API.

NOTE:
sRGB --> linear:    pow(color, 2.2)        make numbers smaller    (ShaderResourceView with _SRGB format).
linear --> sRGB:    pow(color, 1.0/2.2)    make numbers bigger     (RenderTargetView with _SRGB format).

*/

#include <d3d11.h>       // D3D interface
#include <dxgi1_3.h>     // DirectX driver interface
#include <d3dcompiler.h> // Shader compiler

//#pragma comment(lib, "dxgi.lib")        
#pragma comment(lib, "d3d11.lib")       // direct3D library
#pragma comment(lib, "dxguid")          // directx graphics interface
#pragma comment(lib, "d3dcompiler.lib") // shader compiler

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#if !DEVELOPER // Because ImGui already defines this.
#    define STB_TRUETYPE_IMPLEMENTATION
#    include "stb/stb_truetype.h"
#endif

#if DEVELOPER
#    define STB_IMAGE_WRITE_IMPLEMENTATION
#    include "stb/stb_image_write.h"
#endif

//
// Window dimensions.
//
GLOBAL DWORD current_window_width;
GLOBAL DWORD current_window_height;

// Projection matrices.
//
GLOBAL M4x4         object_to_world_matrix = m4x4_identity();
GLOBAL M4x4_Inverse world_to_view_matrix   = {m4x4_identity(), m4x4_identity()};
GLOBAL M4x4_Inverse view_to_proj_matrix    = {m4x4_identity(), m4x4_identity()};;
GLOBAL M4x4         object_to_proj_matrix  = m4x4_identity();

//
// D3D11 objects.
//
GLOBAL ID3D11Device             *device;
GLOBAL ID3D11DeviceContext      *device_context;
GLOBAL IDXGISwapChain1          *swap_chain;
GLOBAL IDXGISwapChain2          *swap_chain2;
GLOBAL ID3D11DepthStencilView   *depth_stencil_view;
GLOBAL ID3D11RenderTargetView   *render_target_view;
GLOBAL D3D11_VIEWPORT            viewport;
GLOBAL HANDLE                    frame_latency_waitable_object;

GLOBAL ID3D11RasterizerState    *rasterizer_state;
GLOBAL ID3D11RasterizerState    *rasterizer_state_solid;
GLOBAL ID3D11RasterizerState    *rasterizer_state_wireframe;
GLOBAL ID3D11BlendState         *blend_state;
GLOBAL ID3D11BlendState         *blend_state_one;
GLOBAL ID3D11DepthStencilState  *depth_state;
GLOBAL ID3D11SamplerState       *sampler0;
GLOBAL ID3D11ShaderResourceView *texture0;

//
// Textures.
//
struct Texture
{
    String8 full_path;
    
    s32 width, height;
    s32 bpp;
    
    ID3D11ShaderResourceView *view;
};
GLOBAL Texture white_texture;

//
// Fonts.
//
struct Font
{
    String8 full_path;
    Texture atlas;
    s32    *sizes;
    s32     sizes_count;
    s32     first;
    s32     w, h;
    u8     *pixels; // Only R-channel (1 bpp).
    
    stbtt_fontinfo     info;
    stbtt_pack_context pack_context;
    stbtt_packedchar **char_data;
    s32                char_count;
};
GLOBAL Font consolas;

////////////////////////////////
// Immediate mode renderer info.
//
struct Immediate_VS_Constants
{
    M4x4 object_to_proj_matrix;
};
struct Immediate_PS_Constants
{
    V2  drawing_rect_size;
    V2  line_p0;
    V2  line_p1;
    b32 is_line;
};
GLOBAL Immediate_PS_Constants immediate_ps_constants;
GLOBAL ID3D11InputLayout     *immediate_input_layout;
GLOBAL ID3D11Buffer          *immediate_vbo;
GLOBAL ID3D11Buffer          *immediate_vs_cbuffer;
GLOBAL ID3D11Buffer          *immediate_ps_cbuffer;
GLOBAL ID3D11VertexShader    *immediate_vs;
GLOBAL ID3D11PixelShader     *immediate_ps;

// Vertex info.
//
struct Vertex_XCNU
{
    V3 position;
    V4 color;
    V3 normal;
    V2 uv;
};
GLOBAL s32           num_immediate_vertices;
GLOBAL const s32     MAX_IMMEDIATE_VERTICES = 2400;
GLOBAL Vertex_XCNU   immediate_vertices[MAX_IMMEDIATE_VERTICES];

// State.
//
// @Note: If true: vertices we push to buffer are pixel positions relative to drawing rect.
GLOBAL b32 is_using_pixel_coords;

////////////////////////////////
////////////////////////////////

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

////////////////////////////////
////////////////////////////////

FUNCTION void d3d11_load_texture(Texture *texture, s32 w, s32 h, u8 *color_data)
{
    if (!color_data)
        return;
    
    texture->width  = w;
    texture->height = h;
    texture->bpp    = 4;
    
    //
    // Create texture as shader resource and create view.
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width      = texture->width;
    desc.Height     = texture->height;
    desc.MipLevels  = 1;
    desc.ArraySize  = 1;
    desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.SampleDesc = {1, 0};
    desc.Usage      = D3D11_USAGE_IMMUTABLE;
    desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;
    
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem     = color_data;
    data.SysMemPitch = texture->width * texture->bpp;
    
    ID3D11Texture2D *texture2d;
    device->CreateTexture2D(&desc, &data, &texture2d);
    device->CreateShaderResourceView(texture2d, 0, &texture->view);
    texture2d->Release();
}

FUNCTION void d3d11_load_texture(Texture *texture, String8 full_path)
{
	texture->full_path = full_path;
    u8 *color_data = stbi_load((const char*)full_path.data, 
                               &texture->width, &texture->height, 
                               &texture->bpp, 4);
    if (color_data) {
        //
        // Create texture as shader resource and create view.
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width      = texture->width;
        desc.Height     = texture->height;
        desc.MipLevels  = 1;
        desc.ArraySize  = 1;
        desc.Format     = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        desc.SampleDesc = {1, 0};
        desc.Usage      = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;
        
        D3D11_SUBRESOURCE_DATA data = {};
        data.pSysMem     = color_data;
        data.SysMemPitch = texture->width * texture->bpp;
        
        ID3D11Texture2D *texture2d;
        device->CreateTexture2D(&desc, &data, &texture2d);
        device->CreateShaderResourceView(texture2d, 0, &texture->view);
        texture2d->Release();
    } else {
        print("STBI ERROR: failed to load image %S\n", texture->full_path);
    }
    
    stbi_image_free(color_data);
}

FUNCTION void d3d11_load_font(Font *font, String8 full_path, s32 ascii_start, s32 ascii_end, s32 *sizes, s32 sizes_count)
{
    // @Note: From Wassimulator's SimplyRend. 
    Arena_Temp scratch = get_scratch(0, 0);
    defer(free_scratch(scratch));
    
    font->full_path = full_path;
    String8 file    = os->read_entire_file(full_path);
    defer(os->free_file_memory(file.data));
    
    stbtt_InitFont(&font->info, file.data, 0);
    
    s32 char_count   = ascii_end - ascii_start;
    font->char_count = char_count;
    stbtt_pack_range *ranges = PUSH_ARRAY_ZERO(scratch.arena, stbtt_pack_range, sizes_count);
    font->sizes              = PUSH_ARRAY_ZERO(os->permanent_arena, s32, sizes_count);
    font->sizes_count        = sizes_count;
    for (s32 i = 0; i < sizes_count; i++) {
        ranges[i].font_size = (f32)sizes[i];
        font->sizes[i]      = sizes[i];
    }
    
    font->char_data = PUSH_ARRAY_ZERO(os->permanent_arena, stbtt_packedchar*, char_count * sizes_count);
    for (s32 i = 0; i < sizes_count; i++) {
        ranges[i].first_unicode_codepoint_in_range = ascii_start;
        ranges[i].num_chars                        = char_count;
        ranges[i].array_of_unicode_codepoints      = 0;
        font->char_data[i]                         = PUSH_ARRAY_ZERO(os->permanent_arena, stbtt_packedchar, char_count + 1);
        ranges[i].chardata_for_range               = font->char_data[i]; 
    }
    font->first = ascii_start;
    
    s32 w = 2000, h = 2000;
    font->w = w;
    font->h = h;
    font->pixels = PUSH_ARRAY_ZERO(os->permanent_arena, u8, w * h);
    stbtt_PackBegin(&font->pack_context, font->pixels, w, h, 0, 1, 0);
    stbtt_PackSetOversampling(&font->pack_context, 1, 1);
    stbtt_PackFontRanges(&font->pack_context, file.data, 0, ranges, sizes_count);
    stbtt_PackEnd(&font->pack_context);
    
    u8 *pixels_rgba = PUSH_ARRAY(scratch.arena, u8, w * h * 4);
    MEMORY_SET(pixels_rgba, 1, w * h * 4);
    for (s32 i = 0; i < w * h; i++) {
        pixels_rgba[i * 4 + 0] |= font->pixels[i];
        pixels_rgba[i * 4 + 1] |= font->pixels[i];
        pixels_rgba[i * 4 + 2] |= font->pixels[i];
        pixels_rgba[i * 4 + 3] |= font->pixels[i];
        
        if (pixels_rgba[i * 4] == 0)
            pixels_rgba[i * 4 + 3] = 0;
    }
    
    d3d11_load_texture(&font->atlas, w, h, pixels_rgba);
    
#if DEVELOPER
    String8 target = sprint(scratch.arena, "%Sarial_atlas.png", os->data_folder);
    stbi_write_png((char*)target.data, w, h, 4, pixels_rgba, 0);
#endif
}

FUNCTION s32 find_font_size_index(Font *font, s32 size)
{
    s32 result = 0;
    b32 found  = false;
    
    while (!found) {
        for (s32 i = 0; i < font->sizes_count; i++) {
            if (font->sizes[i] == size) {
                found  = true;
                result = i;
            }
        }
        
        size--;
        size = CLAMP(font->sizes[0], size, 100);
    }
    
    return result;
}

FUNCTION f32 get_text_width(Font *font, s32 vh, char *format, ...)
{
    // @Note: Gets width of text in pixels.
    //
    // @Note: vh is percentage [0-100] relative to drawing_rect height.
    //
    
    char text[256];
    va_list arg_list;
    va_start(arg_list, format);
    u64 text_length = string_format_list(text, sizeof(text), format, arg_list);
    va_end(arg_list);
    
    // Calculate font size based on vh and get corresponding index of that size.
    s32 size = (s32)(get_height(os->drawing_rect) * vh * 0.01f);
    size     = find_font_size_index(font, size);
    
    f32 result = 0;
    for (s32 i = 0; i < text_length; i++) {
        s32 char_index      = text[i] - font->first;
        stbtt_packedchar d  = font->char_data[size][char_index];
        result             += d.xadvance;
    }
    
    return result;
}

FUNCTION void d3d11_compile_shader(String8 hlsl_path, D3D11_INPUT_ELEMENT_DESC element_desc[], UINT element_count, ID3D11InputLayout **input_layout_out, ID3D11VertexShader **vs_out, ID3D11PixelShader **ps_out)
{
    String8 file = os->read_entire_file(hlsl_path);
    
    UINT flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if defined(_DEBUG)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
    
    ID3DBlob *vs_blob = 0, *ps_blob = 0, *error_blob = 0;
    HRESULT hr = D3DCompile(file.data, file.count, 0, 0, 0, "vs", "vs_5_0", flags, 0, &vs_blob, &error_blob);
    if (FAILED(hr))  {
        const char *message = (const char *) error_blob->GetBufferPointer();
        OutputDebugStringA(message);
        ASSERT(!"Failed to compile vertex shader!");
    }
    
    hr = D3DCompile(file.data, file.count, 0, 0, 0, "ps", "ps_5_0", flags, 0, &ps_blob, &error_blob);
    if (FAILED(hr))  {
        const char *message = (const char *) error_blob->GetBufferPointer();
        OutputDebugStringA(message);
        ASSERT(!"Failed to compile pixel shader!");
    }
    
    device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), 0, vs_out);
    device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), 0, ps_out);
    device->CreateInputLayout(element_desc, element_count, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), input_layout_out);
    
    vs_blob->Release(); ps_blob->Release();
    os->free_file_memory(file.data);
}

FUNCTION void create_immediate_shader()
{
    // Immediate shader input layout.
    D3D11_INPUT_ELEMENT_DESC layout_desc[] = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex_XCNU, position), D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, offsetof(Vertex_XCNU, color),    D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, offsetof(Vertex_XCNU, normal),   D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, offsetof(Vertex_XCNU, uv),       D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    //
    // Immediate vertex buffer.
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = sizeof(immediate_vertices);
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&desc, 0, &immediate_vbo);
    }
    
    //
    // Constant buffers.
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = ALIGN_UP(sizeof(Immediate_VS_Constants), 16);
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        
        device->CreateBuffer(&desc, 0, &immediate_vs_cbuffer);
    }
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth      = ALIGN_UP(sizeof(Immediate_PS_Constants), 16);
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&desc, 0, &immediate_ps_cbuffer);
    }
    
    Arena_Temp scratch = get_scratch(0, 0);
    String8 hlsl_path  = sprint(scratch.arena, "%Simmediate.hlsl", os->data_folder);
    d3d11_compile_shader(hlsl_path, layout_desc, ARRAYSIZE(layout_desc), &immediate_input_layout, &immediate_vs, &immediate_ps);
    free_scratch(scratch);
}

FUNCTION void create_particle_shader()
{
    // Shader input layout.
    D3D11_INPUT_ELEMENT_DESC layout_desc[] = 
    {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,          D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, sizeof(V2), D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    
    f32 unit_cube[] =
    {
        // Vertex origin in bottom-left. 
        // UV origin is top-left.
        
        // POSITION    TEXCOORD
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
        desc.ByteWidth      = sizeof(unit_cube);
        desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        desc.Usage          = D3D11_USAGE_IMMUTABLE;
        
        D3D11_SUBRESOURCE_DATA data = { unit_cube }; 
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

FUNCTION void d3d11_init(HWND window)
{
    // @Note: Kudos Martins:
    // https://gist.github.com/mmozeiko/5e727f845db182d468a34d524508ad5f
    
    //
    // Create device and context.
    {
        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_SINGLETHREADED;
#if defined(_DEBUG)
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0};
        HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, levels, ARRAYSIZE(levels),
                                       D3D11_SDK_VERSION, &device, NULL, &device_context);
        ASSERT_HR(hr);
    }
    
#if defined(_DEBUG)
    //
    // Enable debug break on API errors.
    {
        ID3D11InfoQueue* info;
        HRESULT hr = device->QueryInterface(__uuidof(ID3D11InfoQueue), (void**) &info);
        ASSERT_HR(hr);
        hr         = info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        ASSERT_HR(hr);
        hr         = info->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        ASSERT_HR(hr);
        info->Release();
    }
    // after this there's no need to check for any errors on device functions manually
    // so all HRESULT return values in this code will be ignored
    // debugger will break on errors anyway
#endif
    
    //
    // Create swapchain.
    {
        // get DXGI device from D3D11 device
        IDXGIDevice *dxgi_device;
        HRESULT hr = device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
        ASSERT_HR(hr);
        
        // get DXGI adapter from DXGI device
        IDXGIAdapter *dxgi_adapter;
        hr = dxgi_device->GetAdapter(&dxgi_adapter);
        ASSERT_HR(hr);
        
        // get DXGI factory from DXGI adapter
        IDXGIFactory2 *factory;
        hr = dxgi_adapter->GetParent(IID_IDXGIFactory2, (void**)&factory);
        ASSERT_HR(hr);
        
        DXGI_SWAP_CHAIN_DESC1 desc = {};
        desc.Format                = DXGI_FORMAT_B8G8R8A8_UNORM;
        desc.SampleDesc            = {1, 0};
        desc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount           = 2;
        desc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        desc.Scaling               = DXGI_SCALING_NONE;
        desc.Flags                 = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        
        hr = factory->CreateSwapChainForHwnd((IUnknown*)device, window, &desc, NULL, NULL, &swap_chain);
        ASSERT_HR(hr);
        
        // disable Alt+Enter changing monitor resolution to match window size.
        factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER);
        
        factory->Release();
        dxgi_adapter->Release();
        dxgi_device->Release();
        
        // Create swapchain2, set maximum frame latency and get waitable object.
        hr = swap_chain->QueryInterface(IID_IDXGISwapChain2, (void**)&swap_chain2);
        ASSERT_HR(hr);
        
        swap_chain2->SetMaximumFrameLatency(1);
        frame_latency_waitable_object = swap_chain2->GetFrameLatencyWaitableObject();
    }
    
    //
    // Create rasterize states.
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.FillMode              = D3D11_FILL_SOLID;
        desc.CullMode              = D3D11_CULL_NONE;
        //desc.FrontCounterClockwise = TRUE;
        desc.MultisampleEnable     = TRUE;
        device->CreateRasterizerState(&desc, &rasterizer_state_solid);
    }
    {
        D3D11_RASTERIZER_DESC desc = {};
        desc.FillMode              = D3D11_FILL_WIREFRAME;
        desc.CullMode              = D3D11_CULL_NONE;
        //desc.FrontCounterClockwise = TRUE;
        desc.MultisampleEnable     = TRUE;
        device->CreateRasterizerState(&desc, &rasterizer_state_wireframe);
    }
    rasterizer_state = rasterizer_state_solid;
    
    //
    // Create blend states.
    {
        D3D11_BLEND_DESC desc = {};
        desc.RenderTarget[0].BlendEnable           = TRUE;
        desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        
        device->CreateBlendState(&desc, &blend_state);
        
        D3D11_BLEND_DESC desc2 = desc;
        desc2.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
        device->CreateBlendState(&desc2, &blend_state_one);
    }
    
    //
    // Create depth state.
    {
        D3D11_DEPTH_STENCIL_DESC desc = {};
        desc.DepthEnable      = FALSE;
        desc.DepthWriteMask   = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc        = D3D11_COMPARISON_LESS_EQUAL; // @Note: Use GREATER if using reverse-z
        desc.StencilEnable    = FALSE;
        desc.StencilReadMask  = D3D11_DEFAULT_STENCIL_READ_MASK;
        desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        // desc.FrontFace = ... 
        // desc.BackFace = ...
        device->CreateDepthStencilState(&desc, &depth_state);
    }
    
    //
    // Create texture sampler state.
    {
        D3D11_SAMPLER_DESC desc = {};
        desc.Filter   = D3D11_FILTER_MIN_MAG_MIP_POINT;
        desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        
        device->CreateSamplerState(&desc, &sampler0);
    }
    
    //
    // White texture.
    {
        u8 data[] = {0xFF, 0xFF, 0xFF, 0xFF};
        d3d11_load_texture(&white_texture, 1, 1, data);
    }
    
    //
    // Default font.
    {
        s32 sizes[] = {6, 12, 16, 20, 24, 32, 40, 48, 52, 64, 72, 80, 86, 92};
        d3d11_load_font(&consolas, S8LIT("C:/Windows/Fonts/consolab.ttf"), ' ', S8_MAX, sizes, ARRAY_COUNT(sizes));
    }
    
    //
    // Shaders.
    {    
        create_immediate_shader();
        create_particle_shader();
    }
}

FUNCTION void d3d11_resize()
{
    if (render_target_view) {
        // Release old swap chain buffers.
        device_context->ClearState();
        render_target_view->Release();
        depth_stencil_view->Release();
        
        render_target_view = 0;
    }
    
    UINT window_width  = os->window_size.w; 
    UINT window_height = os->window_size.h;
    
    // Resize to new size for non-zero size.
    if ((window_width != 0) && (window_height != 0)) {
        HRESULT hr = swap_chain->ResizeBuffers(0, window_width, window_height, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
        ASSERT_HR(hr);
        
        //
        // Create render_target_view for new render target texture.
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width      = window_width;
            desc.Height     = window_height;
            desc.MipLevels  = 1;
            desc.ArraySize  = 1;
            desc.Format     = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            desc.SampleDesc = {4, 0};
            desc.BindFlags  = D3D11_BIND_RENDER_TARGET;
            
            ID3D11Texture2D *texture;
            device->CreateTexture2D(&desc, 0, &texture);
            device->CreateRenderTargetView(texture, 0, &render_target_view);
            texture->Release();
        }
        
        //
        // Create new depth stencil texture and new depth_stencil_view.
        {
            D3D11_TEXTURE2D_DESC desc = {};
            desc.Width      = window_width;
            desc.Height     = window_height;
            desc.MipLevels  = 1;
            desc.ArraySize  = 1;
            desc.Format     = DXGI_FORMAT_D32_FLOAT; // Use DXGI_FORMAT_D32_FLOAT_S8X24_UINT if you need stencil.
            desc.SampleDesc = {4, 0};
            desc.Usage      = D3D11_USAGE_DEFAULT;
            desc.BindFlags  = D3D11_BIND_DEPTH_STENCIL;
            
            ID3D11Texture2D *depth;
            device->CreateTexture2D(&desc, 0, &depth);
            device->CreateDepthStencilView(depth, 0, &depth_stencil_view);
            depth->Release();
        }
    }
    
    current_window_width  = window_width;
    current_window_height = window_height;
}

// Call in beginning of frame.
FUNCTION void d3d11_viewport(FLOAT top_left_x, FLOAT top_left_y, FLOAT width, FLOAT height)
{
    // Resize swap chain if needed.
    if ((render_target_view == 0)                   || 
        (os->window_size.w != current_window_width) || 
        (os->window_size.h != current_window_height)) {
        d3d11_resize();
    }
    
    viewport.TopLeftX = top_left_x;
    viewport.TopLeftY = top_left_y;
    viewport.Width    = width;
    viewport.Height   = height;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
}

FUNCTION void d3d11_clear(FLOAT r, FLOAT g, FLOAT b, FLOAT a)
{
    // @Note: Go linear; using SRGB framebuffer (render_target_view).
    V4 color  = v4(r, g, b, a);
    color.rgb = pow(color.rgb, 2.2f);
    
    device_context->ClearRenderTargetView(render_target_view, color.I);
    
    // @Note: Clear depth buffer to the "far" value (0.0f instead of 1.0f if using reverse-z perspective projection).
    device_context->ClearDepthStencilView(depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

FUNCTION void d3d11_clear() 
{ 
    d3d11_clear(0.0f, 0.0f, 0.0f, 1.0f); 
}

FUNCTION HRESULT d3d11_present(b32 vsync)
{
    ID3D11Texture2D *backbuffer, *texture;
    swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**) &backbuffer);
    render_target_view->GetResource((ID3D11Resource **) &texture);
    device_context->ResolveSubresource(backbuffer, 0, texture, 0, DXGI_FORMAT_B8G8R8A8_UNORM);
    backbuffer->Release();
    texture->Release();
    
    HRESULT hr = swap_chain->Present(vsync? 1 : 0, 0);
    return hr;
}

FUNCTION DWORD d3d11_wait_on_swapchain()
{
    DWORD result = WaitForSingleObjectEx(frame_latency_waitable_object,
                                         1000, // 1 second timeout (shouldn't ever occur)
                                         true);
    return result;
}

////////////////////////////////
////////////////////////////////

////////////////////////////////
// Immediate mode renderer functions.
//
FUNCTION void set_texture(Texture *texture)
{
    if (texture)
        texture0 = texture->view;
    else   
        texture0 = white_texture.view;
}

FUNCTION void set_view_to_proj()
{
    f32 ar = (f32)os->render_size.w / (f32)os->render_size.h;
    view_to_proj_matrix = perspective(120.0f * DEGS_TO_RADS, ar, 0.01f, 10.0f);
}

FUNCTION void set_world_to_view(V3 camera_position)
{
    world_to_view_matrix = look_at(camera_position,
                                   camera_position + v3(0, 0, -1),
                                   v3(0, 1, 0));
}

FUNCTION void set_object_to_world(V3 position, Quaternion orientation)
{
    // @Note: immediate_() family functions (like immediate_quad(), immediate_triangle(), etc.. ) MUST 
    // use object-space coordinates for their geometry after calling this function. 
    // Points are usually passed in world space to immediate_() functions. But after using this function,
    // you should pass the world position here and use local-space points in the immediate_() functions.
    //
    // Example:
    //
    // USUALLY:
    // V3 center = v3(5, 7, 0);  // POINT IN WORLD SPACE
    // immediate_begin();
    // immediate_quad(center, v3(1, 1, 0), v4(1));
    // immediate_end();
    //
    //
    // WHEN WE WANT TO USE set_object_transform():
    // immediate_begin();
    // set_object_transform(center, quaternion_identity());
    // immediate_quad(v3(0), v3(1, 1, 0), v4(1));
    // immediate_end();
    //
    // Notice that we passed v3(0) to immediate_quad() after using set_object_transform(), which means
    // that the quad object is in it's own local space and it will be transformed to the world in the
    // vertex shader.
    
    M4x4 m = m4x4_identity();
    m._14  = position.x;
    m._24  = position.y;
    m._34  = position.z;
    
    M4x4 r = m4x4_from_quaternion(orientation);
    
    object_to_world_matrix = m * r;
}

FUNCTION V2 pixel_to_ndc(V2 pixel)
{
    // @Note: pixel relative to drawing_rect.
    //
    f32 w = get_width(os->drawing_rect);
    f32 h = get_height(os->drawing_rect);
    
    V2 p = hadamard_div(pixel, v2(w, h));
    p    = p*2 - v2(1);
    p.y *= -1;
    
    return p;
}

FUNCTION V2 world_to_ndc(V2 point)
{
    // Clip space is same as proj space.
    M4x4 world_to_proj = view_to_proj_matrix.forward * world_to_view_matrix.forward;
    V4 point_clip      = world_to_proj * v4(point.x, point.y, 0, 1);
    
    V2 result;
    result.x = point_clip.x / point_clip.w;
    result.y = point_clip.y / point_clip.w;
    
    return result;
}

FUNCTION void update_render_transform()
{
    if (is_using_pixel_coords)
        object_to_proj_matrix = m4x4_identity();
    else
        object_to_proj_matrix = view_to_proj_matrix.forward * world_to_view_matrix.forward * object_to_world_matrix;
    
    // Do vs constants.
    //
    Immediate_VS_Constants vs_constants;
    vs_constants.object_to_proj_matrix = object_to_proj_matrix;
    
    D3D11_MAPPED_SUBRESOURCE mapped;
    device_context->Map(immediate_vs_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    MEMORY_COPY(mapped.pData, &vs_constants, sizeof(vs_constants));
    device_context->Unmap(immediate_vs_cbuffer, 0);
}

FUNCTION void immediate_end()
{
    if (!num_immediate_vertices)  {
        return;
    }
    
    // Bind Input Assembler.
    device_context->IASetInputLayout(immediate_input_layout);
    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(Vertex_XCNU);
    UINT offset = 0;
    D3D11_MAPPED_SUBRESOURCE mapped;
    device_context->Map(immediate_vbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    MEMORY_COPY(mapped.pData, immediate_vertices, stride * num_immediate_vertices);
    device_context->Unmap(immediate_vbo, 0);
    device_context->IASetVertexBuffers(0, 1, &immediate_vbo, &stride, &offset);
    
    // Vertex Shader.
    update_render_transform();
    device_context->VSSetConstantBuffers(0, 1, &immediate_vs_cbuffer);
    device_context->VSSetShader(immediate_vs, 0, 0);
    
    // Rasterizer Stage.
    device_context->RSSetViewports(1, &viewport);
    device_context->RSSetState(rasterizer_state);
    
    // Pixel Shader.
    device_context->Map(immediate_ps_cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    MEMORY_COPY(mapped.pData, &immediate_ps_constants, sizeof(immediate_ps_constants));
    device_context->Unmap(immediate_ps_cbuffer, 0);
    device_context->PSSetConstantBuffers(1, 1, &immediate_ps_cbuffer);
    device_context->PSSetSamplers(0, 1, &sampler0);
    device_context->PSSetShaderResources(0, 1, &texture0);
    device_context->PSSetShader(immediate_ps, 0, 0);
    
    // Output Merger.
    device_context->OMSetBlendState(blend_state, 0, 0XFFFFFFFFU);
    device_context->OMSetDepthStencilState(depth_state, 0);
    device_context->OMSetRenderTargets(1, &render_target_view, depth_stencil_view);
    
    // Draw.
    device_context->Draw(num_immediate_vertices, 0);
    
    // Reset state.
    num_immediate_vertices = 0;
    is_using_pixel_coords  = false;
    object_to_world_matrix = m4x4_identity();
    immediate_ps_constants = {};
    set_texture(0);
}

FUNCTION void immediate_begin(b32 wireframe = false)
{
    if (wireframe) rasterizer_state = rasterizer_state_wireframe;
    else           rasterizer_state = rasterizer_state_solid;
    
    immediate_end();
}

FUNCTION Vertex_XCNU* immediate_vertex_ptr(s32 index)
{
    if (index == MAX_IMMEDIATE_VERTICES) ASSERT(!"Maximum allowed vertices reached.\n");
    
    Vertex_XCNU *result = immediate_vertices + index;
    return result;
}

FUNCTION void immediate_vertex(V2 position, V4 color)
{
    if (num_immediate_vertices == MAX_IMMEDIATE_VERTICES) immediate_end();
    
    if (is_using_pixel_coords)
        position = pixel_to_ndc(position);
    
    // @Note: Go linear; using SRGB framebuffer.
    color.rgb = pow(color.rgb, 2.2f);
    
    Vertex_XCNU *v = immediate_vertex_ptr(num_immediate_vertices);
    v->position    = v3(position, 0);
    v->color       = color;
    v->normal      = v3(0, 0, 1);
    v->uv          = v2(0, 0);
    
    num_immediate_vertices += 1;
}

FUNCTION void immediate_vertex(V2 position, V2 uv, V4 color)
{
    if (num_immediate_vertices == MAX_IMMEDIATE_VERTICES) immediate_end();
    
    if (is_using_pixel_coords)
        position = pixel_to_ndc(position);
    
    // @Note: Go linear; using SRGB framebuffer.
    color.rgb = pow(color.rgb, 2.2);
    
    Vertex_XCNU *v = immediate_vertex_ptr(num_immediate_vertices);
    v->position    = v3(position, 0);
    v->color       = color;
    v->normal      = v3(0, 0, 1);
    v->uv          = uv;
    
    num_immediate_vertices += 1;
}

FUNCTION void immediate_triangle(V2 p0, V2 p1, V2 p2, V4 color)
{
    if ((num_immediate_vertices + 3) > MAX_IMMEDIATE_VERTICES) immediate_end();
    
    immediate_vertex(p0, color);
    immediate_vertex(p1, color);
    immediate_vertex(p2, color);
}

FUNCTION void immediate_quad(V2 p0, V2 p1, V2 p2, V2 p3, V4 color)
{
    // CCW starting bottom-left.
    
    if ((num_immediate_vertices + 6) > MAX_IMMEDIATE_VERTICES) immediate_end();
    
    immediate_triangle(p0, p1, p2, color);
    immediate_triangle(p0, p2, p3, color);
}

FUNCTION void immediate_quad(V2 p0, V2 p1, V2 p2, V2 p3, 
                             V2 uv0, V2 uv1, V2 uv2, V2 uv3, 
                             V4 color)
{
    // CCW starting bottom-left.
    
    if ((num_immediate_vertices + 6) > MAX_IMMEDIATE_VERTICES) immediate_end();
    
    immediate_vertex(p0, uv0, color);
    immediate_vertex(p1, uv1, color);
    immediate_vertex(p2, uv2, color);
    
    immediate_vertex(p0, uv0, color);
    immediate_vertex(p2, uv2, color);
    immediate_vertex(p3, uv3, color);
}

FUNCTION void immediate_rect(V2 center, V2 half_size, V4 color)
{
    ASSERT(is_using_pixel_coords == false);
    
    V2 p0 = center - half_size;
    V2 p2 = center + half_size;
    V2 p1 = v2(p2.x, p0.y);
    V2 p3 = v2(p0.x, p2.y);
    
    immediate_quad(p0, p1, p2, p3, color);
}

FUNCTION void immediate_rect(V2 center, V2 half_size, V2 uv_min, V2 uv_max, V4 color)
{
    // CCW starting bottom-left.
    
    V2 p0 = center - half_size;
    V2 p2 = center + half_size;
    V2 p1 = v2(p2.x, p0.y);
    V2 p3 = v2(p0.x, p2.y);
    
    // @Note: UV coordinates origin is top-left corner.
    V2 uv3 = uv_min;
    V2 uv1 = uv_max;
    V2 uv0 = v2(uv3.x, uv1.y);
    V2 uv2 = v2(uv1.x, uv3.y);
    
    immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
}

FUNCTION void immediate_rect_tl(V2 top_left, V2 size, V2 uv_min, V2 uv_max, V4 color)
{
    // @Note: We provide top left position, so we need to be careful with winding order.
    
    // CCW starting bottom-left.
    
    V2 p3 = top_left;
    V2 p1 = top_left + size;
    V2 p0 = v2(p3.x, p1.y);
    V2 p2 = v2(p1.x, p3.y);
    
    // @Note: UV coordinates origin is top-left corner.
    V2 uv3 = uv_min;
    V2 uv1 = uv_max;
    V2 uv0 = v2(uv3.x, uv1.y);
    V2 uv2 = v2(uv1.x, uv3.y);
    
    immediate_quad(p0, p1, p2, p3, uv0, uv1, uv2, uv3, color);
}

FUNCTION void immediate_text(Font *font, V2 top_left, s32 vh, V4 color, char *format, ...)
{
    // @Note: vh is percentage [0-100] relative to drawing_rect height.
    //
    
    // is_using_pixel_coords = true;
    
    char text[256];
    va_list arg_list;
    va_start(arg_list, format);
    u64 text_length = string_format_list(text, sizeof(text), format, arg_list);
    va_end(arg_list);
    
    // Calculate size based on vh and get corresponding index of that size.
    s32 size = (s32)(get_height(os->drawing_rect) * vh * 0.01f);
    size     = find_font_size_index(font, size);
    f32 x    = top_left.x;
    f32 y    = top_left.y; 
    for (s32 i = 0; i < text_length; i++) {
        s32 char_index      = text[i] - font->first;
        stbtt_packedchar d  = font->char_data[size][char_index];
        
        V2 uv_min = hadamard_div(v2((f32)d.x0, (f32)d.y0), v2((f32)font->w, (f32)font->h));
        V2 uv_max = hadamard_div(v2((f32)d.x1, (f32)d.y1), v2((f32)font->w, (f32)font->h));
        V2 s      = v2((f32)(d.x1 - d.x0), (f32)(d.y1 - d.y0));
        V2 p      = v2(x + d.xoff, y + d.yoff);
        
        immediate_rect_tl(p, s, uv_min, uv_max, color);
        
        x += d.xadvance;
    }
}

FUNCTION void immediate_line_2d(V2 p0, V2 p1, V4 color, f32 thickness = 0.1f)
{
    V2 a[2];
    V2 b[2];
    
    V2 line_d       = p1 - p0;
    f32 line_length = length(line_d);
    line_d          = normalize(line_d);
    
    // Half-thickness.
    f32 half_thickness = thickness * 0.5f;
    
    V2 d = perp(line_d);
    V2 p = p0;
    for(s32 sindex = 0; sindex < 2; sindex++)
    {
        a[sindex] = p + d*half_thickness;
        b[sindex] = p - d*half_thickness;
        
        p = p + line_length*line_d;
    }
    
    immediate_quad(b[0], b[1], a[1], a[0], color);
}

FUNCTION void immediate_grid(V2 bottom_left, u32 grid_width, u32 grid_height, f32 cell_size, V4 color, f32 line_thickness = 0.025f)
{
    V2 p0 = bottom_left;
    V2 p1;
    
    // Horizontal lines.
    for(u32 i = 0; i <= grid_height; i++)
    {
        p1  = p0 + v2(1, 0)*((f32)grid_width * cell_size);
        immediate_line_2d(p0, p1, color, line_thickness);
        p0 += v2(0, 1)*cell_size;
    }
    
    p0 = bottom_left;
    
    // Vertical lines.
    for(u32 i = 0; i <= grid_width; i++)
    {
        p1  = p0 + v2(0, 1)*((f32)grid_height * cell_size);
        immediate_line_2d(p0, p1, color, line_thickness);
        p0 += v2(1, 0)*cell_size;
    }
}