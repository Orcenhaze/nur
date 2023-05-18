#ifndef GAME_H
#define GAME_H

////////////////////////////////
// Game inputs.
//
#define MAX_BINDS_PER_INPUT 3

enum Game_Input
{
    MOVE_RIGHT,
    MOVE_UP,
    MOVE_LEFT,
    MOVE_DOWN,
    
    ROTATE_CCW,
    ROTATE_CW,
    
    UNDO,
};

GLOBAL s32 binds[][MAX_BINDS_PER_INPUT] = {
    {Key_D, Key_RIGHT},
    {Key_W, Key_UP},
    {Key_A, Key_LEFT},
    {Key_S, Key_DOWN},
    
    {Key_Q},
    {Key_E},
    
    {Key_Z}
};

FUNCTION b32 input_pressed(Game_Input input)
{
    for (s32 i = 0; i < MAX_BINDS_PER_INPUT; i++) {
        s32 key = binds[input][i];
        if (key_pressed(key))
            return true;
    }
    
    return false;
}

FUNCTION b32 input_held(Game_Input input)
{
    for (s32 i = 0; i < MAX_BINDS_PER_INPUT; i++) {
        s32 key = binds[input][i];
        if (key_held(key))
            return true;
    }
    
    return false;
}

FUNCTION b32 input_released(Game_Input input)
{
    for (s32 i = 0; i < MAX_BINDS_PER_INPUT; i++) {
        s32 key = binds[input][i];
        if (key_released(key))
            return true;
    }
    
    return false;
}

////////////////////////////////
////////////////////////////////
// Menu
//
GLOBAL b32 game_loaded = false;
enum
{
    M_MENU,
    M_GAME,
    M_EDITOR
} GLOBAL main_mode = M_GAME;

s32 menu_selection = 0;
GLOBAL char *choices[] = 
{
    "START GAME",
    "SAVE AND QUIT"
};

////////////////////////////////
////////////////////////////////
// Textures
//
#define TILE_SIZE 16  // In pixels!
Texture tex;

#define FONT_TILE_W 6 // In pixels!
#define FONT_TILE_H 8
Texture font_tex;

////////////////////////////////


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
    v2s(4, 4),
    v2s(5, 4),
    v2s(6, 4),
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

enum
{
    ObjFlags_NONE = 0,
    
    ObjFlags_ONLY_ROTATE_CCW = 1 << 0,
    ObjFlags_ONLY_ROTATE_CW  = 1 << 1,
};

struct Obj
{
    // @Note: Color in each direction. Some objs don't use this.
    // @Todo: Use bitfileds?
    u8 color[8];
    u8 flags;
    u8 type;
    u8 dir;
    u8 c; // The actual color of the object. Used for emitters, detectors and doors.
};

struct Player
{
    s32 x, y;
    u8 dir;
};

// The size of a square/cell is 1 unit!
//
#define NUM_X  4   // number of rooms.
#define NUM_Y  4
#define SIZE_X 10   // size of each room (in squares).
#define SIZE_Y 10

GLOBAL u8  tilemap[NUM_Y*SIZE_Y][NUM_X*SIZE_X];
GLOBAL Obj objmap[NUM_Y*SIZE_Y][NUM_X*SIZE_X];

// @Cleanup: What a mess!
//
#define PLAYER_ANIMATION_SPEED 5.0f
#define NUM_ANIMATION_FRAMES   4
#define ANIMATION_FRAME_DURATION ((1.0f / PLAYER_ANIMATION_SPEED) / NUM_ANIMATION_FRAMES)
GLOBAL s32 mx; GLOBAL s32 my;                                 // Mouse.
GLOBAL s32 px; GLOBAL s32 py; GLOBAL u8 pdir; GLOBAL V2 ppos; // Player
GLOBAL b32 dead = false;
GLOBAL s32 rx; GLOBAL s32 ry; // Room (bottom left square) that player is in.
GLOBAL V2 camera; GLOBAL V2 camera_pos;
GLOBAL V2 pushed_obj; GLOBAL V2 pushed_obj_pos; // Animate obj pushing.
GLOBAL b32 draw_grid;
GLOBAL f32 animation_timer;

#define DEFAULT_ZOOM (SIZE_X*0.55f)
GLOBAL f32 zoom_level; 

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
    
    Random_PCG rng;
    
#if DEVELOPER
    // @Note: We want to select using one key (left mouse), so we will store the type is one variable 
    // and set a boolean to say which one it is.
    //
    b32 is_tile_selected;
    u8  selected_tile_or_obj;
    s32 selected_color;
    s32 selected_flag;
#endif
};
GLOBAL Game_State *game;

#endif //GAME_H
