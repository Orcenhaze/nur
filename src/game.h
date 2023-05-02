#ifndef GAME_H
#define GAME_H

#define WORLD_ORIGIN v2(-0.5f) 

enum
{
    M_LOGO,
    M_MENU,
    M_GAME,
    M_EDITOR
} GLOBAL main_mode = M_GAME;

enum
{
    Dir_E,
    Dir_NE,
    Dir_N,
    Dir_NW,
    Dir_W,
    Dir_SW,
    Dir_S,
    Dir_SE
};

// @Todo: Separate to xdir and ydir.
V2s dirs[8] = {
    v2s( 1,  0),
    v2s( 1,  1),
    v2s( 0,  1),
    v2s(-1,  1),
    v2s(-1,  0),
    v2s(-1, -1),
    v2s( 0, -1),
    v2s( 1, -1),
};

#define TILE_SIZE 16  // In pixels!
Texture tex;

enum
{
    Tile_FLOOR,
    Tile_WALL,
};

V2s tile_sprite[] = {
    v2s(0, 0),
    v2s(1, 0)
};


enum 
{
    T_EMPTY,
    T_EMITTER,
    T_MIRROR,
    
};

V2s obj_sprite[] = {
    v2s(7, 7),
    v2s(0, 1),
    v2s(0, 2),
};

enum
{
    Color_WHITE,
    Color_RED,
    Color_GREEN,
    Color_BLUE,
    Color_YELLOW,
    Color_MAGENTA,
    Color_CYAN,
};

V4 colors[8] = {
    v4(1, 1, 1, 1),
    v4(1, 0, 0, 1),
    v4(0, 1, 0, 1),
    v4(0, 0, 1, 1),
    v4(1, 1, 0, 1),
    v4(1, 0, 1, 1),
    v4(0, 1, 1, 1),
};

struct Obj
{
    // Color in each direction. Only one color is set for the EMITTER and it never changes.
    u8 color[8];
    u8 type;
    u8 dir;
    
    // @Todo: It might be a good idea to have a color for the object itself besides the color in each direction.
};

// The size of a square/cell is 1 unit!
//
#define NUM_X  4   // number of rooms.
#define NUM_Y  4
#define SIZE_X 8   // size of each room (in squares).
#define SIZE_Y 8
#define WORLD_EDGE_X NUM_X*SIZE_X // In units! World range [0, WORLD_EDGE].
#define WORLD_EDGE_Y NUM_Y*SIZE_X

GLOBAL u8  tilemap[NUM_Y*SIZE_Y][NUM_X*SIZE_X];
GLOBAL Obj objmap[NUM_Y*SIZE_Y][NUM_X*SIZE_X];

struct Game_State
{
    // @Note: These should be in a separte struct, for serailization and de-serialization.
    u8  tile_map[NUM_Y*SIZE_Y][NUM_X*SIZE_X];
    Obj obj_map[NUM_Y*SIZE_Y][NUM_X*SIZE_X];
    
    V2 delta_mouse;
    V2 mouse_ndc_old;
    V2 mouse_world;
    V2 camera_position;
    
#if DEVELOPER
    // @Note: We want to select using one key (left mouse), so we will store the type is one variable 
    // and set a boolean to say which one it is.
    //
    b32 is_tile_selected;
    u8  selected_tile_or_obj;
    s32 selected_color;
#endif
};
GLOBAL Game_State *game;

#endif //GAME_H
