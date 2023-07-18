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
    
    CONFIRM,
    
    RESTART_LEVEL,
};

GLOBAL s32 binds[][MAX_BINDS_PER_INPUT] = {
    {Key_D, Key_RIGHT},
    {Key_W, Key_UP},
    {Key_A, Key_LEFT},
    {Key_S, Key_DOWN},
    
    {Key_Q},
    {Key_E},
    
    {Key_Z},
    
    {Key_ENTER, Key_SPACE},
    
    {Key_R}
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
GLOBAL b32 game_started = false;
enum
{
    M_MENUS,
    M_GAME,
    M_EDITOR
} GLOBAL current_mode = M_MENUS;

enum Menu_Page
{
    MAIN_MENU,
    PAUSE,
    SETTINGS,
    RESTART_CONFIRMATION,
    CONTROLS,
};
GLOBAL s32 selection;
GLOBAL s32 prev_selection;
GLOBAL Menu_Page page;
GLOBAL Menu_Page prev_page;

////////////////////////////////
////////////////////////////////
// Textures
//
#define TILE_SIZE 16  // In pixels!
Texture tex;

#define FONT_TILE_W 6 // In pixels!
#define FONT_TILE_H 8
Texture font_tex;

Texture controls_tex;

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

V2 fdirs[8] = {
    v2( 1.0f,  0.0f),
    v2( 1.0f,  1.0f),
    v2( 0.0f,  1.0f),
    v2(-1.0f,  1.0f),
    v2(-1.0f,  0.0f),
    v2(-1.0f, -1.0f),
    v2( 0.0f, -1.0f),
    v2( 1.0f, -1.0f),
};

enum
{
    Tile_FLOOR,
    Tile_WALL,
    Tile_OBJ_FRAME,
};

V2s tile_sprite[] = {
    v2s(0, 0),
    v2s(1, 0),
    v2s(3, 0),
};

enum 
{
    T_EMPTY,
    T_LASER,
    T_MIRROR,
    T_BENDER, // For this one: the reflected light is always compatible with Dir, but the object itself is visaully 30 degrees off of Dir.
    T_SPLITTER,
    T_DETECTOR,
    T_DOOR,
    T_DOOR_OPEN,
    T_TELEPORTER,
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
    v2s(2, 0),
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
    ObjFlags_NEVER_ROTATE    = 1 << 2,
    ObjFlags_IS_DEAD         = 1 << 3,
};

struct Obj
{
    // @Note: 
    // color[8] is color in each direction. Some objs don't interact with beams, so they use
    // it in a special way:
    // T_DOOR: color[0] is number of detectors in room required to be lit in order to open it.
    //         color[1] is used every frame as counter of number of detectors currently lit.
    //         color[1] must be cleared every frame before we update the doors and detectors.
    //
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

////////////////////////////////
////////////////////////////////
// Global current state.
//
// current_level metadata (we're storing each field independently).
GLOBAL Arena *current_level_arena;
GLOBAL s32 NUM_X;
GLOBAL s32 NUM_Y;
GLOBAL s32 SIZE_X;
GLOBAL s32 SIZE_Y;
GLOBAL u8  **tilemap;
GLOBAL Obj **objmap;

// Square position of mouse cursor.
GLOBAL s32 mx; GLOBAL s32 my;

// Room (bottom left square) that player is in.
GLOBAL s32 rx; GLOBAL s32 ry;

// Camera discrete and animated positions.
GLOBAL V2 camera; GLOBAL V2 camera_pos;

// Pushed obj discrete and animated positions.
GLOBAL V2 pushed_obj; GLOBAL V2 pushed_obj_pos;

// Player state.
GLOBAL f32 PLAYER_ANIMATION_SPEED = 8.0f;
#define NUM_ANIMATION_FRAMES 4
#define ANIMATION_FRAME_DURATION ((1.0f / PLAYER_ANIMATION_SPEED) / NUM_ANIMATION_FRAMES)
GLOBAL f32 animation_timer;
GLOBAL s32 px; GLOBAL s32 py; GLOBAL u8 pdir; GLOBAL V2 ppos; GLOBAL V2s psprite; GLOBAL u8 pcolor;
GLOBAL b32 dead;

// Settings.
GLOBAL b32 draw_grid;
GLOBAL b32 prompt_user_on_restart = true;

// Other state.
GLOBAL f32 zoom_level; 


////////////////////////////////
////////////////////////////////

#if 0
#include "levels.h"
#else
GLOBAL String8 level_names[] = 
{
    S8LIT("invalid_level"),
    S8LIT("aaa_placeholder1"),
    S8LIT("aaa_placeholder2"),
    S8LIT("aaa_placeholder3"),
    
    S8LIT("mirror_intro"),
    S8LIT("red_intro"),
    S8LIT("splitter_intro"),
    S8LIT("teach_rotation"),
    S8LIT("block_the_source"),
    S8LIT("mirror_first_challenge"),
    
    S8LIT("primary_mixing_intro"),
    S8LIT("trapped_mirror_intro"),
    
    S8LIT("bender_intro"),
    S8LIT("tie"),
    S8LIT("mirror_bender_relationship1"),
    S8LIT("mirror_bender_relationship2"),
    
    S8LIT("bender_red"),
    S8LIT("bender_locked_red"),
    S8LIT("bender_locked_red_double"),
    
    S8LIT("push"),
    S8LIT("make_way"),
    S8LIT("sneaky_corner"),
    S8LIT("sneaky_corner_hard"),
    S8LIT("splitter_first_challenge"),
    
    S8LIT("bender_splitter_triangle"),
    S8LIT("bender_splitter_relationship"),
    S8LIT("tie_cyan"),
    S8LIT("tie_hard"),
    
    S8LIT("connected_rooms"),
    
    S8LIT("splitter_locked1"),
    S8LIT("splitter_locked2"),
    S8LIT("splitter_locked3"),
    
    S8LIT("secondary_mixing_intro"),
    S8LIT("secondary_mixing_easy"),
    
    S8LIT("crossing_red_intro1"),
    S8LIT("make_white_beam"),
    S8LIT("crossing_red_intro2"),
    S8LIT("crossing_red_medium"),
    S8LIT("crossing_magenta"),
    
    S8LIT("sokobaning"),
    S8LIT("stack"),
    S8LIT("stack2"),
    S8LIT("stack_hard"),
    
    S8LIT("colored_splitter_intro"),
    S8LIT("colored_splitter_primary_mixing"),
    S8LIT("colored_splitter_crossing_red"),
    
    S8LIT("chain_small"),
    S8LIT("chain_big"),
    S8LIT("loop_medium"),
    S8LIT("loop_hard"),
};
#endif

enum 
{
    SaveFileVersion_INIT,
    
    SaveFileVersion_COUNT,
};

struct Settings
{
    // Not gonna use flags because meh.
    b32 fullscreen;
    b32 draw_grid;
    b32 prompt_user_on_restart;
};

enum
{
    LevelVersion_INIT,
    LevelVersion_REMOVE_ID,
    LevelVersion_ADD_NAME,
    
    LevelVersion_COUNT,
};

struct Loaded_Level
{
    String8 name;
    s32 num_x, num_y;   // Number of rooms.
    s32 size_x, size_y; // Size of each room (in squares).
    Player player;
    Obj **obj_map;
    u8  **tile_map;
};

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

struct Game_State
{
    Arena *loaded_level_arena;
    Loaded_Level loaded_level;
    
    Particle_Emitter obj_emitter;
    
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
    s32 num_detectors_required;
#endif
};
GLOBAL Game_State *game;

#endif //GAME_H
