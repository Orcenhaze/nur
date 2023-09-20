#ifndef BACKGROUND_H
#define BACKGROUND_H

////////////////////////////////
// Renderer info.
//
struct Background_PS_Constants
{
    f32 time;
    f32 value; 
};
GLOBAL ID3D11InputLayout  *background_input_layout;
GLOBAL ID3D11Buffer       *background_vbo;
GLOBAL ID3D11Buffer       *background_ps_cbuffer;
GLOBAL ID3D11VertexShader *background_vs;
GLOBAL ID3D11PixelShader  *background_ps;

#endif //BACKGROUND_H
