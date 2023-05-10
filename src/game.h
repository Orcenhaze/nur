#ifndef GAME_H
#define GAME_H

GLOBAL b32 game_loaded = false;
enum
{
    M_LOGO,
    M_MENU,
    M_GAME,
    M_EDITOR
} GLOBAL main_mode = M_GAME;

#define WRAP_D(d) (((d) + 8) % 8)
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
    v2s(1, 0),
};


enum 
{
    T_EMPTY,
    T_EMITTER,
    T_MIRROR,
    T_BENDER, // For this one: the reflected light is always compatible with Dir, but the object itself is visaully 30 degrees off of Dir.
    T_SPLITTER,
    T_DETECTOR,
    T_DOOR,
    T_DOOR_OPEN,
};

V2s obj_sprite[] = {
    v2s(7, 7),
    v2s(0, 1),
    v2s(0, 2),
    v2s(0, 3),
    v2s(0, 4), // Draw T_SPLITTER with (s + dir) % 4
    v2s(0, 5),
    v2s(0, 6),
    v2s(1, 6),
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
    v4(1, 1, 0, 1), // Yellow
    v4(1, 0, 1, 1), // Magenta
    v4(0, 1, 1, 1), // Cyan
};

struct Obj
{
    // @Note: Color in each direction. Some objs don't use this.
    u8 color[8];
    u8 type;
    u8 dir;
    u8 c; // The actual color of the object. Used for emitters, detectors and doors.
};

struct Player
{
    s32 x, y;
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

// @Note: Square coords under the cursor in the current frame.
// src_m is used as fall back if we drop picked obj on a wrong square.
//
// @Cleanup: 
//
GLOBAL s32 mx; GLOBAL s32 my;
GLOBAL s32 src_mx; GLOBAL s32 src_my;
GLOBAL Obj picked_obj;
GLOBAL s32 px; GLOBAL s32 py; GLOBAL u8 pdir; // Player
GLOBAL s32 rx; GLOBAL s32 ry; // Room (bottom left coords).
GLOBAL V2 camera;

struct Loaded_Game
{
    Obj obj_map[NUM_Y*SIZE_Y][NUM_X*SIZE_X];
    u8  tile_map[NUM_Y*SIZE_Y][NUM_X*SIZE_X];
    
    // @Note: From player xy, we calculate room xy. 
    //        And from room xy, we calculate camera xy.
    //
    Player player;
};

struct Game_State
{
    Loaded_Game loaded_game;
    
    V2 delta_mouse;
    V2 mouse_ndc_old;
    V2 mouse_world;
    
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
