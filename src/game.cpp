
FUNCTION void set_default_zoom()
{
    f32 level_ar  = (f32)SIZE_X/(f32)SIZE_Y;
    f32 render_ar = (f32)os->render_size.width / (f32)os->render_size.height;
    
    if (level_ar >= render_ar)
        zoom_level = ((f32)SIZE_X + 1.0f) / (2.0f * render_ar);
    else
        zoom_level = ((f32)SIZE_Y + 1.0f) / (2.0f);
    
    f32 padding_factor = 1.1f;
    zoom_level *= padding_factor;
    
    set_view_to_proj(zoom_level);
}

FUNCTION void update_camera(b32 teleport = false)
{
    camera = v2((f32)(rx + SIZE_X/2), (f32)(ry + SIZE_Y/2));
    if (teleport)
        camera_pos = camera;
}

FUNCTION b32 player_is_at_rest()
{
    b32 result = length2(ppos - v2((f32)px, (f32)py)) < SQUARE(0.001f);
    return result;
}

FUNCTION b32 pushed_obj_is_at_rest()
{
    b32 result = length2(pushed_obj_pos - pushed_obj) < SQUARE(0.001f);
    return result;
}

FUNCTION void set_player_position(s32 x, s32 y, u8 dir, b32 teleport = false)
{
    s32 room_x = SIZE_X*(x/SIZE_X);
    s32 room_y = SIZE_Y*(y/SIZE_Y);
    if (room_x != rx || room_y != ry) {
        // We are in a new room.
        rx = room_x; 
        ry = room_y;
        
        // Calculate camera.
        update_camera();
    }
    
    if (teleport) {
        px = x;
        py = y;
        ppos.x = (f32)x;
        ppos.y = (f32)y;
    } else {
        ppos.x = (f32)px;
        ppos.y = (f32)py;
        px = x;
        py = y;
    }
    
    pdir = dir;
}

// @Todo: Must organize files.
#include "undo.h"
GLOBAL Undo_Handler undo_handler;


//
// @Cleanup: Cleanup serialization stuff.
// @Cleanup: Cleanup serialization stuff.
//
FUNCTION void save_game()
{
    if (!game_started)
        return;
    
    String_Builder sb = sb_init();
    defer(sb_free(&sb));
    
    Loaded_Level *lev = &game->loaded_level;
    
    //save_map();
    sb_append(&sb, &lev->name.count, sizeof(u32));
    sb_append(&sb, lev->name.data, lev->name.count);
    
    Arena_Temp scratch = get_scratch(0, 0);
    os->write_entire_file(sprint(scratch.arena, "%Ssave.dat", os->data_folder), sb_to_string(&sb));
    free_scratch(scratch);
}

FUNCTION void reload_map()
{
    Arena *a          = current_level_arena;
    Loaded_Level *lev = &game->loaded_level;
    arena_reset(a);
    
    NUM_X            = lev->num_x;
    NUM_Y            = lev->num_y;
    SIZE_X           = lev->size_x;
    SIZE_Y           = lev->size_y;
    Player player    = lev->player;
    set_player_position(player.x, player.y, player.dir, true);
    
    s32 num_rows = lev->num_y*lev->size_y;
    s32 num_cols = lev->num_x*lev->size_x;
    
    // Allocate memory for objmap.
    objmap = PUSH_ARRAY(a, Obj*, num_rows);
    for (s32 i = 0; i < num_rows; i++) 
        objmap[i] = PUSH_ARRAY_ZERO(a, Obj, num_cols);
    // Copy objmap.
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            objmap[y][x] = lev->obj_map[y][x];
        }
    }
    
    // Allocate memory for tilemap.
    tilemap = PUSH_ARRAY(a, u8*, num_rows);
    for (s32 i = 0; i < num_rows; i++) 
        tilemap[i] = PUSH_ARRAY_ZERO(a, u8, num_cols);
    // Copy tilemap.
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            tilemap[y][x] = lev->tile_map[y][x];
        }
    }
    
    dead = false;
    set_default_zoom();
    update_camera(true);
    undo_handler_reset(&undo_handler);
}

FUNCTION b32 load_level(String8 level_name)
{
#define RESTORE_FIELD(field, inclusion_version) \
if (version >= (inclusion_version)) \
get(&file, &field)
#define RESTORE_FIELD_ARRAY(field, size, inclusion_version) \
if (version >= (inclusion_version)) \
get(&file, &field, size)
#define IGNORE_FIELD(type, field_name, inclusion_version, removed_version) \
do { \
type field_name; \
if (version >= (inclusion_version) && version < (removed_version)) \
get(&file, &field_name); \
} while(0)
#define IGNORE_FIELD_ARRAY(type, field_name, size, inclusion_version, removed_version) \
do { \
type field_name; \
if (version >= (inclusion_version) && version < (removed_version)) \
get(&file, &field_name, size); \
} while(0)
    
    Arena_Temp scratch = get_scratch(0, 0);
    String8 file = os->read_entire_file(sprint(scratch.arena, "%Slevels/%S.nlf", os->data_folder, level_name));
    free_scratch(scratch);
    if (!file.data) {
        print("Couldn't load level: %S\n", level_name);
        return false;
    }
    defer(os->free_file_memory(file.data));
    
    Arena *a          = game->loaded_level_arena;
    Loaded_Level *lev = &game->loaded_level;
    s32 version       = 0;
    arena_reset(a);
    
    get(&file, &version);
    
    // @Todo: Once we implement package manager, we don't need to copy data. Instead, we load the 
    // package in memory and just point to it, because the package will always be alive in memory.
    //
    // @Todo: When writing strings to files, we should establish a convention to always write them
    // null-terminated. When loading, we load count, then in case of memcpy, we copy count+1 to 
    // account for null-terminator.
    // When advancing, we advance count+1 to account for null.
    /* 
        IGNORE_FIELD(s32, id, LevelVersion_INIT, LevelVersion_REMOVE_ID);
        if (version >= (LevelVersion_ADD_NAME)) { 
            u32 len;
            get(&file, &len);
            String8 stemp = string(file.data, len);
            advance(&file, len);
            lev->name = string_copy(stemp);
        } else {
            lev->name = string_copy(level_name);
        }
     */
    u32 len;
    get(&file, &len);
    String8 stemp = string(file.data, len);
    advance(&file, len);
    //advance(&file, len + 1); // Account for null terminator.
    lev->name = string_copy(stemp);
    
#if DEVELOPER
    if (lev->name != level_name) {
        print("\nName mismatch!\n"
              "expected %S but got %S\n"
              "Save the game again to solve the issue!\n\n", level_name, lev->name);
    }
#endif
    
    get(&file, &lev->num_x);
    get(&file, &lev->num_y);
    get(&file, &lev->size_x);
    get(&file, &lev->size_y);
    
    get(&file, &lev->player);
    
    s32 num_rows = lev->num_y*lev->size_y;
    s32 num_cols = lev->num_x*lev->size_x;
    
    // Allocate memory for obj_map.
    lev->obj_map = PUSH_ARRAY(a, Obj*, num_rows);
    for (s32 i = 0; i < num_rows; i++) 
        lev->obj_map[i] = PUSH_ARRAY_ZERO(a, Obj, num_cols);
    // Load obj_map.
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            Obj *dst = &lev->obj_map[y][x];
            
            RESTORE_FIELD(dst->color, LevelVersion_INIT);
            RESTORE_FIELD(dst->flags, LevelVersion_INIT);
            RESTORE_FIELD(dst->type, LevelVersion_INIT);
            RESTORE_FIELD(dst->dir, LevelVersion_INIT);
            RESTORE_FIELD(dst->c, LevelVersion_INIT);
        }
    }
    
    // Allocate memory for tile_map.
    lev->tile_map = PUSH_ARRAY(a, u8*, num_rows);
    for (s32 i = 0; i < num_rows; i++) 
        lev->tile_map[i] = PUSH_ARRAY_ZERO(a, u8, num_cols);
    // Load tile_map.
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            get(&file, &lev->tile_map[y][x]);
        }
    }
    
    reload_map();
    
    return true;
}

FUNCTION b32 load_game()
{
    Arena_Temp scratch = get_scratch(0, 0);
    String8 file = os->read_entire_file(sprint(scratch.arena, "%Ssave.dat", os->data_folder));
    free_scratch(scratch);
    if (!file.data) {
        print("Save file not present!\n");
        return false;
    }
    defer(os->free_file_memory(file.data));
    
    u32 name_length;
    get(&file, &name_length);
    String8 name = string(file.data, name_length);
    //advance(&file, name_length + 1);
    if (!load_level(name) || (name != game->loaded_level.name)) {
        return false;
    }
    game_started = true;
    return true;
}

#if DEVELOPER
FUNCTION void save_map()
{
    Arena *a          = game->loaded_level_arena;
    Loaded_Level *lev = &game->loaded_level;
    arena_reset(a);
    
    lev->num_x  = NUM_X;
    lev->num_y  = NUM_Y;
    lev->size_x = SIZE_X;
    lev->size_y = SIZE_Y;
    Player player;
    player.x    = px;
    player.y    = py;
    player.dir  = pdir;
    lev->player = player;
    
    s32 num_rows = NUM_Y*SIZE_Y;
    s32 num_cols = NUM_X*SIZE_X;
    
    // Allocate memory for obj_map.
    lev->obj_map = PUSH_ARRAY(a, Obj*, num_rows);
    for (s32 i = 0; i < num_rows; i++) 
        lev->obj_map[i] = PUSH_ARRAY_ZERO(a, Obj, num_cols);
    // Copy.
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            lev->obj_map[y][x] = objmap[y][x];
        }
    }
    
    // Allocate memory for tile_map.
    lev->tile_map = PUSH_ARRAY(a, u8*, num_rows);
    for (s32 i = 0; i < num_rows; i++) 
        lev->tile_map[i] = PUSH_ARRAY_ZERO(a, u8, num_cols);
    // Copy.
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            lev->tile_map[y][x] = tilemap[y][x];
        }
    }
}

FUNCTION b32 save_level(String8 level_name)
{
    if (string_match(level_name, level_names[0])) {
        print("Can't save invalid level!\n");
        return false;
    }
    
    String_Builder sb = sb_init();
    defer(sb_free(&sb));
    
    Loaded_Level *lev = &game->loaded_level;
    s32 latest_version = LevelVersion_COUNT-1;
    
    save_map();
    sb_append(&sb, &latest_version, sizeof(s32));
    sb_append(&sb, &level_name.count, sizeof(u32));
    sb_append(&sb, level_name.data, level_name.count);
    sb_append(&sb, &lev->num_x);
    sb_append(&sb, &lev->num_y);
    sb_append(&sb, &lev->size_x);
    sb_append(&sb, &lev->size_y);
    
    sb_append(&sb, &lev->player);
    
    s32 num_rows = lev->num_y*lev->size_y;
    s32 num_cols = lev->num_x*lev->size_x;
    
    // append obj_map.
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            sb_append(&sb, &lev->obj_map[y][x]);
        }
    }
    
    // append tile_map.
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            sb_append(&sb, &lev->tile_map[y][x]);
        }
    }
    
    Arena_Temp scratch = get_scratch(0, 0);
    b32 result = os->write_entire_file(sprint(scratch.arena, "%Slevels/%S.nlf", os->data_folder, level_name), sb_to_string(&sb));
    free_scratch(scratch);
    
    return result;
}
#endif

FUNCTION b32 mouse_over_ui()
{
    b32 result = false;
    
#if DEVELOPER
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse) result = true;
#endif
    
    return result;
}

#if DEVELOPER
FUNCTION void resize_current_level(s32 num_x, s32 num_y, s32 size_x, s32 size_y)
{
    NUM_X  = num_x;
    NUM_Y  = num_y;
    SIZE_X = size_x;
    SIZE_Y = size_y;
    set_default_zoom();
    update_camera(true);
    
    arena_reset(current_level_arena);
	
    s32 num_rows = NUM_Y*SIZE_Y;
    s32 num_cols = NUM_X*SIZE_X;
    
    // Allocate memory for objmap.
    objmap = PUSH_ARRAY(current_level_arena, Obj*, num_rows);
    for (s32 i = 0; i < num_rows; i++) 
        objmap[i] = PUSH_ARRAY_ZERO(current_level_arena, Obj, num_cols);
    // Allocate memory for tilemap.
    tilemap = PUSH_ARRAY(current_level_arena, u8*, num_rows);
    for (s32 i = 0; i < num_rows; i++) 
        tilemap[i] = PUSH_ARRAY_ZERO(current_level_arena, u8, num_cols);
}

FUNCTION void make_empty_level()
{
    // Init map.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            tilemap[y][x] = Tile_FLOOR;
            objmap[y][x]  = {};
        }
    }
    // Initial player and room position.
    set_player_position(0, 0, Dir_E, true);
}

FUNCTION void expand_current_level(s32 num_x, s32 num_y, s32 size_x, s32 size_y)
{
    Arena_Temp scratch = get_scratch(0, 0);
    defer(free_scratch(scratch));
    
    u8  **old_tilemap;
    Obj **old_objmap;
    
    // Copy current level before resizing.
    s32 old_num_rows = NUM_Y*SIZE_Y;
    s32 old_num_cols = NUM_X*SIZE_X;
    old_objmap = PUSH_ARRAY(scratch.arena, Obj*, old_num_rows);
    for (s32 i = 0; i < old_num_rows; i++) 
        old_objmap[i] = PUSH_ARRAY_ZERO(scratch.arena, Obj, old_num_cols);
    old_tilemap = PUSH_ARRAY(scratch.arena, u8*, old_num_rows);
    for (s32 i = 0; i < old_num_rows; i++) 
        old_tilemap[i] = PUSH_ARRAY_ZERO(scratch.arena, u8, old_num_cols);
    
    for (s32 y = 0; y < old_num_rows; y++) {
        for (s32 x = 0; x < old_num_cols; x++) {
            old_tilemap[y][x] = tilemap[y][x];
            old_objmap[y][x]  = objmap[y][x];
        }
    }
    
    ////////////////////////////////
    ////////////////////////////////
    
    resize_current_level(num_x, num_y, size_x, size_y);
    
    // Restore old maps.
    s32 num_rows = MIN(old_num_rows, num_y*size_y);
    s32 num_cols = MIN(old_num_cols, num_x*size_x);
    for (s32 y = 0; y < num_rows; y++) {
        for (s32 x = 0; x < num_cols; x++) {
            tilemap[y][x] = old_tilemap[y][x];
            objmap[y][x]  = old_objmap[y][x];
        }
    }
}

FUNCTION void do_editor(b32 is_first_call)
{
    ImGuiIO& io      = ImGui::GetIO();
    b32 pressed_left = ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mouse_over_ui();
    
    ImGui::Begin("Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    
    //
    // Level meta.
    {
        LOCAL_PERSIST s32 num_x = NUM_X, num_y = NUM_Y, size_x = SIZE_X, size_y = SIZE_Y;
        
        // Maybe player restarted level in game mode, so ensure we update the level size.
        if (is_first_call) {
            num_x = NUM_X, num_y = NUM_Y, size_x = SIZE_X, size_y = SIZE_Y;
        }
        
        // Select level.
        LOCAL_PERSIST s32 selected_level = 0;
        const char* combo_preview_value = (const char*)level_names[selected_level].data;
        if(ImGui::BeginCombo("Choose level", combo_preview_value)) {
            for(int name_index = 0; name_index < IM_ARRAYSIZE(level_names); name_index++) {
                const bool is_selected = (selected_level == name_index);
                if(ImGui::Selectable((const char*)level_names[name_index].data, is_selected)) {
                    selected_level = name_index;
                    
                    if (!load_level(level_names[selected_level])) {
                        resize_current_level(1, 1, 8, 8);
                        make_empty_level();
                    }
                    
                    num_x = NUM_X, num_y = NUM_Y, size_x = SIZE_X, size_y = SIZE_Y;
                }
                
                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        
        
        ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
        
        
        // Expand level size.
        b32 do_expand = false;
        if (ImGui::InputInt("NUM_X", &num_x)) do_expand = true;
        if (ImGui::InputInt("NUM_Y", &num_y)) do_expand = true;;
        if (ImGui::InputInt("SIZE_X", &size_x)) do_expand = true;
        if (ImGui::InputInt("SIZE_Y", &size_y)) do_expand = true;
        num_x = CLAMP_LOWER(num_x, 0); 
        num_y = CLAMP_LOWER(num_y, 0); 
        size_x = CLAMP_LOWER(size_x, 0); 
        size_y = CLAMP_LOWER(size_y, 0); 
        if (do_expand)
            expand_current_level(num_x, num_y, size_x, size_y);
        
        
        ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
        
        
        // Save level.
        if (ImGui::Button("Save level")) {
            if (save_level(level_names[selected_level])) {
                ImGui::SameLine(); 
                ImGui::Text("Saved!");
            }
        }
    }
    
    ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::Checkbox("Draw grid", (bool*)&draw_grid);
    ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
    
    //
    // Edit tiles.
    {
        // Draw all tiles.
        //
        ImGui::Text("Tiles: ");
        ImTextureID tex_id = tex.view;
        ImVec2 image_size  = ImVec2(TILE_SIZE*4.0f, TILE_SIZE*4.0f);
        ImVec4 tint_col    = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
        ImVec4 border_col  = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
        for (s32 i = 0; i < ARRAY_COUNT(tile_sprite); i++) {
            if (i % 4 == 0)
                ImGui::NewLine();
            
            V2s sprite = tile_sprite[i];
            
            ImVec2 uv_min = {
                (((sprite.s + 0) * TILE_SIZE) + 0.05f) / tex.width,
                (((sprite.t + 0) * TILE_SIZE) + 0.05f) / tex.height
            };
            ImVec2 uv_max = {
                (((sprite.s + 1) * TILE_SIZE) - 0.05f) / tex.width,
                (((sprite.t + 1) * TILE_SIZE) - 0.05f) / tex.height
            };
            ImGui::Image(tex_id, image_size, uv_min, uv_max, tint_col, border_col);
            if (ImGui::IsItemHovered() && pressed_left) {
                game->selected_tile_or_obj = (u8)i;
                game->is_tile_selected = true;
            }
            
            ImGui::SameLine(0, 0.5f);
        }
        
        if (game->is_tile_selected) {
            // Draw selected tile.
            //
            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
            ImGui::Text("Selected Tile: ");
            V2s sprite = tile_sprite[game->selected_tile_or_obj];
            ImVec2 uv_min = {
                (((sprite.s + 0) * TILE_SIZE) + 0.05f) / tex.width,
                (((sprite.t + 0) * TILE_SIZE) + 0.05f) / tex.height
            };
            ImVec2 uv_max = {
                (((sprite.s + 1) * TILE_SIZE) - 0.05f) / tex.width,
                (((sprite.t + 1) * TILE_SIZE) - 0.05f) / tex.height
            };
            ImGui::Image(tex_id, image_size, uv_min, uv_max, tint_col, border_col);
        }
    }
    
    ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
    ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
    
    //
    // Edit objs.
    {
        // Draw all objs
        //
        ImGui::Text("Objs: ");
        ImTextureID tex_id = tex.view;
        ImVec2 image_size  = ImVec2(TILE_SIZE*4.0f, TILE_SIZE*4.0f);
        ImVec4 tint_col    = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // No tint
        ImVec4 border_col  = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
        for (s32 i = 0; i < ARRAY_COUNT(obj_sprite); i++) {
            if (i % 4 == 0)
                ImGui::NewLine();
            
            if (i == T_DOOR_OPEN)
                continue;
            
            V2s sprite = obj_sprite[i];
            
            ImVec2 uv_min = {
                (((sprite.s + 0) * TILE_SIZE) + 0.05f) / tex.width,
                (((sprite.t + 0) * TILE_SIZE) + 0.05f) / tex.height
            };
            ImVec2 uv_max = {
                (((sprite.s + 1) * TILE_SIZE) - 0.05f) / tex.width,
                (((sprite.t + 1) * TILE_SIZE) - 0.05f) / tex.height
            };
            ImGui::Image(tex_id, image_size, uv_min, uv_max, tint_col, border_col);
            if (ImGui::IsItemHovered() && pressed_left) {
                game->selected_tile_or_obj = (u8)i;
                game->is_tile_selected = false;
            }
            
            ImGui::SameLine(0, 0.5f);
        }
        
        if (!game->is_tile_selected) {
            // Draw selected obj.
            //
            ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
            ImGui::Text("Selected Obj: ");
            V2s sprite = obj_sprite[game->selected_tile_or_obj];
            ImVec2 uv_min = {
                (((sprite.s + 0) * TILE_SIZE) + 0.05f) / tex.width,
                (((sprite.t + 0) * TILE_SIZE) + 0.05f) / tex.height
            };
            ImVec2 uv_max = {
                (((sprite.s + 1) * TILE_SIZE) - 0.05f) / tex.width,
                (((sprite.t + 1) * TILE_SIZE) - 0.05f) / tex.height
            };
            ImGui::Image(tex_id, image_size, uv_min, uv_max, tint_col, border_col);
            
            // Choose color
            //
            if (game->selected_tile_or_obj == T_EMITTER  || 
                game->selected_tile_or_obj == T_DETECTOR ||
                game->selected_tile_or_obj == T_DOOR     ||
                game->selected_tile_or_obj == T_SPLITTER) {
                ImGui::SameLine(0, ImGui::GetFrameHeight());
                const char* cols[] = { "WHITE", "RED", "GREEN", "BLUE", "YELLOW", "MAGENTA", "CYAN", };
                const char* col = cols[game->selected_color];
                ImGui::ListBox("Color", &game->selected_color, cols, ARRAY_COUNT(cols));
            }
            
            if (game->selected_tile_or_obj == T_MIRROR || 
                game->selected_tile_or_obj == T_BENDER ||
                game->selected_tile_or_obj == T_SPLITTER) {
                ImGui::RadioButton("NONE",     &game->selected_flag, 0);      ImGui::SameLine();
                ImGui::RadioButton("LOCK CCW", &game->selected_flag, 1 << 0); ImGui::SameLine();
                ImGui::RadioButton("LOCK CW",  &game->selected_flag, 1 << 1); ImGui::SameLine();
                ImGui::RadioButton("LOCK OBJ", &game->selected_flag, 1 << 2);
            }
            
            if (game->selected_tile_or_obj == T_DOOR) {
                ImGui::SliderInt("Number of detectors required", &game->num_detectors_required, 1, 8, "%d", ImGuiSliderFlags_AlwaysClamp);
            }
            
            if (game->selected_tile_or_obj == T_TELEPORTER) {
                ImGui::SameLine(0, ImGui::GetFrameHeight());
                // Select level.
                const char* combo_preview_value = (const char*)level_names[game->level_teleport_to].data;
                if(ImGui::BeginListBox("Teleport to")) {
                    for(int name_index = 0; name_index < IM_ARRAYSIZE(level_names); name_index++) {
                        const bool is_selected = (game->level_teleport_to == name_index);
                        if(ImGui::Selectable((const char*)level_names[name_index].data, is_selected)) {
                            game->level_teleport_to = (u8)name_index;
                        }
                        
                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if(is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndListBox();
                }
            }
        }
    }
    
    ImGui::End();
}
#endif

FUNCTION b32 is_outside_map(s32 x, s32 y)
{
    b32 result = ((x < 0) || (x > NUM_X*SIZE_X-1) ||
                  (y < 0) || (y > NUM_Y*SIZE_Y-1));
    return result;
}

#define PRINT_ARR(a, message) {\
print(#message); \
print("\n");\
for (s32 i = 0; i < a.count; i++) \
print("%v3\n", a[i]);\
print("\n\n");\
}

#define NUM_ROTATION_PARTICLES 5
GLOBAL V2 random_rotation_particles[NUM_ROTATION_PARTICLES];
GLOBAL Array<u32> unique_draw_beams_calls;
FUNCTION void game_init()
{
#if 0
    //
    // @TESTING ARRAYS
    //
    Array<V3> arr1;
    array_init(&arr1, 10);
    
    Array<V3> arr2;
    array_init(&arr2, 2);
    
    array_add(&arr1, v3(1));
    array_add(&arr1, v3(2));
    array_add(&arr1, v3(3));
    array_add(&arr1, v3(4));
    
    PRINT_ARR(arr1, "arr1 first:");
    
    array_copy(&arr2, arr1);
    
    array_add(&arr2, v3(69));
    array_add(&arr2, v3(898));
    
    PRINT_ARR(arr2, "arr2 first:");
    
    array_copy(&arr1, arr2);
    
    PRINT_ARR(arr1, "arr1 AFTER22:");
    //
    // @TESTING
    //
#endif
    
    game = PUSH_STRUCT_ZERO(os->permanent_arena, Game_State);
    
    // Arenas init.
    // @Todo: If we notice that all levels are less than 1MB, we can simply decrease the max size
    // because we don't need to reserve that much.
    game->loaded_level_arena = arena_init(MEGABYTES(1));
    current_level_arena      = arena_init(MEGABYTES(1));
    
#if DEVELOPER
    resize_current_level(1, 1, 8, 8);
    make_empty_level();
    
    game->num_detectors_required = 1;
#endif
    
    game->rng = random_seed();
    for (s32 i = 0; i < NUM_ROTATION_PARTICLES; i++) {
        random_rotation_particles[i] = random_range_v2(&game->rng, v2(0), v2(1));
    }
    
    array_init(&unique_draw_beams_calls);
    
    {
        Arena_Temp scratch = get_scratch(0, 0);
        d3d11_load_texture(&tex, sprint(scratch.arena, "%Ssprites.png", os->data_folder));
        d3d11_load_texture(&font_tex, sprint(scratch.arena, "%Sfont_sprites.png", os->data_folder));
        free_scratch(scratch);
    }
    
    load_game();
    if (game_started)
        menu_selection = 0;
    
    set_default_zoom();
    update_camera(true);
    set_world_to_view(v3(camera_pos, 0));
    
    undo_handler_init(&undo_handler);
}

#if DEVELOPER
FUNCTION void move_camera(s32 dir_x, s32 dir_y)
{
    if (!dir_x && !dir_y) return;
    s32 temp_x = (s32)(camera.x + SIZE_X*dir_x);
    s32 temp_y = (s32)(camera.y + SIZE_Y*dir_y);
    if (is_outside_map(temp_x, temp_y))
        return;
    
    camera.x = (f32)temp_x;
    camera.y = (f32)temp_y;
    
    camera_pos = camera;
}

FUNCTION void swap_room(s32 dir_x, s32 dir_y)
{
    if (!dir_x && !dir_y) return;
    s32 room_x = SIZE_X*((s32)camera.x/SIZE_X);
    s32 room_y = SIZE_Y*((s32)camera.y/SIZE_Y);
    s32 dest_x = room_x + SIZE_X*dir_x;
    s32 dest_y = room_y + SIZE_Y*dir_y;
    if (is_outside_map(dest_x, dest_y))
        return;
    
    for (s32 y = room_y; y < room_y+SIZE_Y; y++) {
        for (s32 x = room_x; x < room_x+SIZE_X; x++) {
            SWAP(objmap[y][x], objmap[y + SIZE_Y*dir_y][x + SIZE_X*dir_x], Obj);
            SWAP(tilemap[y][x], tilemap[y + SIZE_Y*dir_y][x + SIZE_X*dir_x], u8);
        }
    }
}

FUNCTION void flip_room_horizontally()
{
    s32 room_x = SIZE_X*((s32)camera.x/SIZE_X);
    s32 room_y = SIZE_Y*((s32)camera.y/SIZE_Y);
    s32 room_end_x = room_x+SIZE_X-1;
    s32 room_end_y = room_y+SIZE_Y-1;
    
    for (s32 y = room_y; y <= room_end_y; y++) {
        for (s32 x = room_x; x <= room_end_x; x++) {
            // Flip directions.
            u8 d = objmap[y][x].dir;
            if (d == Dir_E || d == Dir_W)
                objmap[y][x].dir = WRAP_D(d + 4);
            else if (d == Dir_NE)
                objmap[y][x].dir = Dir_NW;
            else if (d == Dir_NW)
                objmap[y][x].dir = Dir_NE;
            else if (d == Dir_SW)
                objmap[y][x].dir = Dir_SE;
            else if (d == Dir_SE)
                objmap[y][x].dir = Dir_SW;
        }
    }
    for (s32 y = room_y; y <= room_end_y; y++) {
        for (s32 x = room_x; x <= (room_x + room_end_x)/2; x++) {
            // Flip objs and tiles.
            SWAP(objmap[y][x], objmap[y][room_end_x - (x - room_x)], Obj);
            SWAP(tilemap[y][x], tilemap[y][room_end_x - (x - room_x)], u8);
        }
    }
}
FUNCTION void flip_room_vertically()
{
    s32 room_x = SIZE_X*((s32)camera.x/SIZE_X);
    s32 room_y = SIZE_Y*((s32)camera.y/SIZE_Y);
    s32 room_end_x = room_x+SIZE_X-1;
    s32 room_end_y = room_y+SIZE_Y-1;
    
    for (s32 y = room_y; y <= room_end_y; y++) {
        for (s32 x = room_x; x <= room_end_x; x++) {
            // Flip directions.
            u8 d = objmap[y][x].dir;
            if (d == Dir_N || d == Dir_S)
                objmap[y][x].dir = WRAP_D(d + 4);
            else if (d == Dir_NE)
                objmap[y][x].dir = Dir_SE;
            else if (d == Dir_NW)
                objmap[y][x].dir = Dir_SW;
            else if (d == Dir_SW)
                objmap[y][x].dir = Dir_NW;
            else if (d == Dir_SE)
                objmap[y][x].dir = Dir_NE;
        }
    }
    for (s32 y = room_y; y <= (room_y + room_end_y)/2; y++) {
        for (s32 x = room_x; x <= room_end_x; x++) {
            // Flip objs and tiles.
            SWAP(objmap[y][x], objmap[room_end_y - (y - room_y)][x], Obj);
            SWAP(tilemap[y][x], tilemap[room_end_y - (y - room_y)][x], u8);
        }
    }
}
#endif

FUNCTION u8 mix_colors(u8 cur, u8 src)
{
    if (cur == Color_WHITE)
        return src;
    else if (src == Color_WHITE)
        return cur;
    else if (cur == src)
        return src;
    
    // primary-primary.
    else if ((cur == Color_RED && src == Color_GREEN) || (src == Color_RED && cur == Color_GREEN))
        return Color_YELLOW;
    else if ((cur == Color_RED && src == Color_BLUE) || (src == Color_RED && cur == Color_BLUE))
        return Color_MAGENTA;
    else if ((cur == Color_GREEN && src == Color_BLUE) || (src == Color_GREEN && cur == Color_BLUE))
        return Color_CYAN;
    
    // secondary-secondary.
    else if ((cur == Color_YELLOW && src == Color_MAGENTA) || (src == Color_YELLOW && cur == Color_MAGENTA))
        return Color_RED;
    else if ((cur == Color_YELLOW && src == Color_CYAN) || (src == Color_YELLOW && cur == Color_CYAN))
        return Color_GREEN;
    else if ((cur == Color_MAGENTA && src == Color_CYAN) || (src == Color_MAGENTA && cur == Color_CYAN))
        return Color_BLUE;
    
    // primary-secondary.
    else if ((cur == Color_YELLOW && ((src == Color_RED) || (src == Color_GREEN))) || (src == Color_YELLOW && ((cur == Color_RED) || (cur == Color_GREEN))))
        return Color_YELLOW;
    else if ((cur == Color_MAGENTA && ((src == Color_RED) || (src == Color_BLUE))) || (src == Color_MAGENTA && ((cur == Color_RED) || (cur == Color_BLUE))))
        return Color_MAGENTA;
    else if ((cur == Color_CYAN && ((src == Color_GREEN) || (src == Color_BLUE))) || (src == Color_CYAN && ((cur == Color_GREEN) || (cur == Color_BLUE))))
        return Color_CYAN;
    
    else
        return Color_WHITE;
}

// @Cleanup:
// @Todo: Keep for now, but eventually remove it if we settle on discrete pushing animation,
#define CONTINUOUS_PUSHING 0

FUNCTION void update_beams(s32 src_x, s32 src_y, u8 src_dir, u8 src_color)
{
    if (is_outside_map(src_x + dirs[src_dir].x, src_y + dirs[src_dir].y))
        return;
    
    s32 test_x = src_x + dirs[src_dir].x;
    s32 test_y = src_y + dirs[src_dir].y;
    Obj test_o = objmap[test_y][test_x];
    
    b32 is_obj_aligned = (((src_x == pushed_obj.x) && (src_x == pushed_obj_pos.x)) || 
                          ((src_y == pushed_obj.y) && (src_y == pushed_obj_pos.y)));
    
    // Advance until we hit a wall or an object.
    while ((test_o.type == T_EMPTY || test_o.type == T_DOOR_OPEN) && tilemap[test_y][test_x] != Tile_WALL) {
        
        // We hit the player.
        if (test_x == px && test_y == py) {
            pcolor = mix_colors(pcolor, src_color);
#if CONTINUOUS_PUSHING
            b32 is_player_aligned = ((src_x == px) || (src_y == py)) ;
            // @Note: Obj was hitting beam and we are now pushing it. 
            // Could be pushing it off OR pushing it along beam.
            // Either way stop updating.
            if (is_player_aligned && (length2(pushed_obj_pos - pushed_obj) > SQUARE(0.5f))) {
                return;
            }
#endif
        }
        
#if CONTINUOUS_PUSHING
        jump:
#endif
        if (is_outside_map(test_x + dirs[src_dir].x, test_y + dirs[src_dir].y))
            return;
        test_x += dirs[src_dir].x;
        test_y += dirs[src_dir].y;
        test_o  = objmap[test_y][test_x];
    }
    if (tilemap[test_y][test_x] == Tile_WALL) 
        return;
    
    // In case player is standing on some Obj like T_DETECTOR.
    if (test_x == px && test_y == py) {
        pcolor = mix_colors(pcolor, src_color);
    }
    
    b32 is_pushed_obj  = ((test_x == pushed_obj.x) && (test_y == pushed_obj.y));
    
    // We hit an object, so we should determine which color to reflect in which dir.  
    switch (test_o.type) {
        case T_DOOR:
        case T_EMITTER: {
        } break;
        case T_MIRROR:
        case T_BENDER:
        case T_SPLITTER: {
            u8 inv_d       = WRAP_D(src_dir + 4);
            u8 ninv_d      = WRAP_D(inv_d + 1);
            u8 pinv_d      = WRAP_D(inv_d - 1 );
            u8 p2inv_d     = WRAP_D(inv_d - 2);
            u8 reflected_d = U8_MAX;
            b32 penetrate  = true;
            
#if CONTINUOUS_PUSHING
            if (is_pushed_obj && !pushed_obj_is_at_rest()) {
                if (is_obj_aligned)
                    break;
                else if (length2(pushed_obj_pos - pushed_obj) <= SQUARE(0.5f))
                    break;
                else
                    goto jump;
            }
#endif
            
            if (src_color == test_o.color[reflected_d])
                break;
            
            if (test_o.type == T_MIRROR) {
                if (test_o.dir == ninv_d)      reflected_d = WRAP_D(ninv_d + 1);
                else if (test_o.dir == pinv_d) reflected_d = WRAP_D(pinv_d - 1);
                
                // Write the color.
                if (reflected_d != U8_MAX) {
                    objmap[test_y][test_x].color[reflected_d] = mix_colors(test_o.color[reflected_d], src_color);
                    update_beams(test_x, test_y, reflected_d, objmap[test_y][test_x].color[reflected_d]);
                }
            } else if (test_o.type == T_BENDER) {
                if (test_o.dir == inv_d)        reflected_d = ninv_d;
                else if (test_o.dir == ninv_d)  reflected_d = WRAP_D(ninv_d + 2);
                else if (test_o.dir == pinv_d)  reflected_d = pinv_d;
                else if (test_o.dir == p2inv_d) reflected_d = WRAP_D(p2inv_d - 1);
                
                // Write the color.
                if (reflected_d != U8_MAX) {
                    objmap[test_y][test_x].color[reflected_d] = mix_colors(test_o.color[reflected_d], src_color);
                    update_beams(test_x, test_y, reflected_d, objmap[test_y][test_x].color[reflected_d]);
                }
            } else {
                if (test_o.dir == inv_d || test_o.dir == src_dir)
                    penetrate = true;
                else if (test_o.dir == ninv_d || test_o.dir == WRAP_D(src_dir + 1))
                    reflected_d = WRAP_D(ninv_d + 1);
                else if (test_o.dir == pinv_d || test_o.dir == WRAP_D(src_dir - 1))
                    reflected_d = WRAP_D(pinv_d - 1);
                else 
                    penetrate = false;
                
                // Source direction.
                //
                if (penetrate) {
                    u8 c = test_o.c == Color_WHITE? mix_colors(test_o.color[src_dir], src_color) : test_o.c;
                    objmap[test_y][test_x].color[src_dir] = c;
                    update_beams(test_x, test_y, src_dir, c);
                }
                
                // Reflected direction.
                //
                if (reflected_d != U8_MAX) {
                    if (src_color == test_o.color[reflected_d])
                        break;
                    u8 c = test_o.c == Color_WHITE? mix_colors(test_o.color[reflected_d], src_color) : test_o.c;
                    objmap[test_y][test_x].color[reflected_d] = c;
                    update_beams(test_x, test_y, reflected_d, c);
                }
            }
        } break;
        case T_DETECTOR: {
            if (src_color == test_o.color[src_dir])
                break;
            objmap[test_y][test_x].color[src_dir] = mix_colors(test_o.color[src_dir], src_color);
            update_beams(test_x, test_y, src_dir, objmap[test_y][test_x].color[src_dir]);
        } break;
    }
}

FUNCTION b32 is_obj_collides(u8 type)
{
    switch (type) {
        case T_EMITTER:
        case T_MIRROR:
        case T_BENDER:
        case T_SPLITTER:
        case T_DETECTOR: 
        case T_DOOR:
        case T_DOOR_OPEN: return true;
        default: return false;
    }
}

FUNCTION void move_player(s32 dir_x, s32 dir_y)
{
    if (!dir_x && !dir_y) return;
    u8 old_dir = pdir;
    pdir       = dir_x? dir_x<0? (u8)Dir_W : (u8)Dir_E : dir_y<0? (u8)Dir_S : (u8)Dir_N;
    
    // @Cleanup:
    // @Cleanup:
    // @Cleanup:
    s32 tempx = px + dir_x; 
    s32 tempy = py + dir_y;
    if (is_outside_map(tempx, tempy))
        return;
    
    if (tilemap[tempy][tempx] == Tile_WALL) {
        return;
    }
    
    if (objmap[tempy][tempx].type == T_EMITTER) {
        return;
    }
    if (objmap[tempy][tempx].type == T_DOOR) {
        return;
    }
    
    if (objmap[tempy][tempx].type != T_DETECTOR &&
        objmap[tempy][tempx].type != T_DOOR_OPEN) {
        b32 collides = is_obj_collides(objmap[tempy][tempx].type);
        if (collides) {
            s32 nx = tempx + dirs[pdir].x;
            s32 ny = tempy + dirs[pdir].y;
            if (is_outside_map(nx, ny))
                return;
            if (tilemap[ny][nx] == Tile_WALL)
                return;
            
            b32 next_collides = is_obj_collides(objmap[ny][nx].type);
            if (next_collides) {
                return;
            }
            
#if 1
            pushed_obj     = v2((f32)nx, (f32)ny);
            pushed_obj_pos = v2((f32)tempx, (f32)tempy);
#else
            pushed_obj_pos = pushed_obj = v2((f32)nx, (f32)ny);
#endif
            undo_push_obj_move(&undo_handler, tempx, tempy, nx, ny);
            SWAP(objmap[tempy][tempx], objmap[ny][nx], Obj);
        }
    }
    
    undo_push_player_move(&undo_handler, px, py, old_dir);
    
#if 1
    set_player_position(tempx, tempy, pdir);
#else
    set_player_position(tempx, tempy, pdir, true);
#endif
}

#define MOVE_HOLD_DURATION 0.20f
GLOBAL f32 move_hold_timer;
GLOBAL s32 last_pressed;
GLOBAL s32 queued_moves_count;
GLOBAL V2s queued_moves[8]; // @Todo: Decrease max number of queued_moves.

#define UNDO_HOLD_DURATION 0.20f
GLOBAL f32 undo_hold_timer;
GLOBAL f32 undo_speed_scale;

FUNCTION void update_world()
{
    if (key_pressed(Key_G)) {
        draw_grid = !draw_grid;
    }
    
    ////////////////////////////////
    // Undo!
    //
    if (input_pressed(UNDO)) {
        undo_hold_timer  = 0.0f;
        undo_speed_scale = 1.0f;
    }
    
    undo_hold_timer = CLAMP_LOWER(undo_hold_timer - os->dt, 0);
    
    if (input_held(UNDO)) {
        if (undo_hold_timer <= 0) {
            undo_next(&undo_handler);
            undo_hold_timer  = UNDO_HOLD_DURATION * undo_speed_scale;
            undo_speed_scale = CLAMP_LOWER(undo_speed_scale - 3.5f*os->dt, 0.3f);
            
            // @Hardcoded:
            dead = false;
        }
    }
    
    // Load new level if we step on teleporter.
    if (player_is_at_rest()) {
        Obj o = objmap[py][px];
        if (o.type == T_TELEPORTER) {
            load_level(level_names[o.color[0]]);
        }
    }
    
    // Update player pos.
    f32 player_max_distance = PLAYER_ANIMATION_SPEED * os->dt;
    ppos = move_towards(ppos, v2((f32)px, (f32)py), player_max_distance);
    
    // Update pushed obj pos.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if (o.type == T_MIRROR ||
                o.type == T_BENDER ||
                o.type == T_SPLITTER) {
                if (pushed_obj.x == x && pushed_obj.y == y) {
                    pushed_obj_pos = move_towards(pushed_obj_pos, pushed_obj, player_max_distance);
                }
            }
        }
    }
    
    // Update player sprite.
    s32 base_s = pdir == Dir_N? 4 : 0;
    if (pdir == Dir_S)
        psprite.t = 6;
    else
        psprite.t = 5;
    if (player_is_at_rest()) {
        animation_timer = 0;
        psprite.s = base_s;
    } else if (animation_timer <= 0) {
        psprite.s = base_s + ((psprite.s + 1) % NUM_ANIMATION_FRAMES);
        animation_timer = ANIMATION_FRAME_DURATION;
    }  else {
        animation_timer = CLAMP_LOWER(animation_timer - os->dt, 0);
    }
    
    
    ////////////////////////////////
    // Player movement!
    //
    if (!dead) {
        if (input_pressed(MOVE_RIGHT)) {
            move_hold_timer = 0.0f;
            last_pressed = MOVE_RIGHT;
        } else if (input_pressed(MOVE_UP)) {
            move_hold_timer = 0.0f;
            last_pressed = MOVE_UP;
        } else if (input_pressed(MOVE_LEFT)) {
            move_hold_timer = 0.0f;
            last_pressed = MOVE_LEFT;
        } else if (input_pressed(MOVE_DOWN)) {
            move_hold_timer = 0.0f;
            last_pressed = MOVE_DOWN;
        }
        
        move_hold_timer = MAX(0, move_hold_timer - os->dt);
        
        V2s move_dir = {};
        b32 move_input_held = false; 
        if (input_held(MOVE_RIGHT) && last_pressed == MOVE_RIGHT) {
            move_input_held = true;
            if (move_hold_timer <= 0) {
                move_dir.x = 1;
                move_hold_timer = MOVE_HOLD_DURATION;
            }
        } else if (input_held(MOVE_UP) && last_pressed == MOVE_UP) {
            move_input_held = true;
            if (move_hold_timer <= 0) {
                move_dir.y = 1;
                move_hold_timer = MOVE_HOLD_DURATION;
            }
        } else if (input_held(MOVE_LEFT) && last_pressed == MOVE_LEFT) {
            move_input_held = true;
            if (move_hold_timer <= 0) {
                move_dir.x = -1;
                move_hold_timer = MOVE_HOLD_DURATION;
            }
        } else if (input_held(MOVE_DOWN) && last_pressed == MOVE_DOWN) {
            move_input_held = true;
            if (move_hold_timer <= 0) {
                move_dir.y = -1;
                move_hold_timer = MOVE_HOLD_DURATION;
            }
        }
        
        if (move_input_held) {
            undo_hold_timer  = UNDO_HOLD_DURATION;
            undo_speed_scale = 1.0f;
        }
        
        if (move_dir.x || move_dir.y) {
            if (queued_moves_count < ARRAY_COUNT(queued_moves)) {
                queued_moves[queued_moves_count++] = move_dir;
            }
        }
        
        if (player_is_at_rest() && queued_moves_count > 0) {
            // Pop the next move.
            V2s queued_move = queued_moves[0];
            // Slide other moves down the array.
            for (s32 i = 0; i < queued_moves_count-1; i++)
                queued_moves[i] = queued_moves[i+1];
            queued_moves_count--;
            
            move_player(queued_move.x, queued_move.y);
        }
        
        // Rotate objs around player.
        if (input_pressed(ROTATE_CCW) || input_pressed(ROTATE_CW)) {
            for (s32 dy = CLAMP_LOWER(py-1, 0); dy <= CLAMP_UPPER(NUM_Y*SIZE_Y-1, py+1); dy++) {
                for (s32 dx = CLAMP_LOWER(px-1, 0); dx <= CLAMP_UPPER(NUM_X*SIZE_X-1, px+1); dx++) {
                    if (dy == py && dx == px)
                        continue;
                    
                    switch (objmap[dy][dx].type) {
                        case T_MIRROR:
                        case T_BENDER:
                        case T_SPLITTER: {
                            if (is_set(objmap[dy][dx].flags, ObjFlags_NEVER_ROTATE)) 
                                continue;
                            
                            if (input_pressed(ROTATE_CCW)) {
                                undo_push_obj_rotate(&undo_handler, dx, dy, objmap[dy][dx].dir);
                                if (is_set(objmap[dy][dx].flags, ObjFlags_ONLY_ROTATE_CW))
                                    objmap[dy][dx].dir = WRAP_D(objmap[dy][dx].dir - 1);
                                else
                                    objmap[dy][dx].dir = WRAP_D(objmap[dy][dx].dir + 1);
                            } else if (input_pressed(ROTATE_CW)) {
                                undo_push_obj_rotate(&undo_handler, dx, dy, objmap[dy][dx].dir);
                                if (is_set(objmap[dy][dx].flags, ObjFlags_ONLY_ROTATE_CCW))
                                    objmap[dy][dx].dir = WRAP_D(objmap[dy][dx].dir + 1);
                                else
                                    objmap[dy][dx].dir = WRAP_D(objmap[dy][dx].dir - 1);
                            }
                        } break;
                    }
                }
            }
        }
        
    }
    
    ////////////////////////////////
    // Update map.
    //
    
    // Clear colors for all objs (except ones that use it specially).
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            if (objmap[y][x].type == T_TELEPORTER)
                continue;
            else if (objmap[y][x].type == T_DOOR || objmap[y][x].type == T_DOOR_OPEN)
                objmap[y][x].color[1] = 0;
            else
                MEMORY_ZERO_ARRAY(objmap[y][x].color);
        }
    }
    pcolor = Color_WHITE;
    
    // Update beams (do red emitters first).
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if ((o.type == T_EMITTER) && (o.c == Color_RED))
                update_beams(x, y, o.dir, o.c);
        }
    }
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if ((o.type == T_EMITTER) && (o.c != Color_RED))
                update_beams(x, y, o.dir, o.c);
        }
    }
    
    if (pcolor == Color_RED || pcolor == Color_MAGENTA || pcolor == Color_YELLOW)
        dead = true;
    else
        dead = false;
    
    // Update doors and detectors.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj detector = objmap[y][x];
            if (detector.type == T_DETECTOR) {
                s32 room_x = SIZE_X*(x/SIZE_X);
                s32 room_y = SIZE_Y*(y/SIZE_Y);
                
                u8 final_c = Color_WHITE;
                for (s32 i = 0; i < 8; i++)
                    final_c = mix_colors(final_c, detector.color[i]);
                
                for (s32 dy = room_y; dy < room_y+SIZE_Y; dy++) {
                    for (s32 dx = room_x; dx < room_x+SIZE_X; dx++) {
                        if (dx == x && dy == y)
                            continue;
                        if ((objmap[dy][dx].type == T_DOOR || objmap[dy][dx].type == T_DOOR_OPEN) && objmap[dy][dx].c == detector.c) {
                            if (final_c == detector.c) {
                                // Potentially open door.
                                objmap[dy][dx].color[1] += 1;
                                if (objmap[dy][dx].color[1] >= objmap[dy][dx].color[0])
                                    objmap[dy][dx].type = T_DOOR_OPEN;
                                else
                                    objmap[dy][dx].type = T_DOOR;
                            } else {
                                // Close door.
                                if (objmap[dy][dx].color[1] < objmap[dy][dx].color[0])
                                    objmap[dy][dx].type = T_DOOR;
                            }
                        }
                    }
                }
            }
        }
    }
    
    undo_end_frame(&undo_handler);
}

#if DEVELOPER
FUNCTION void update_editor()
{
    b32 shift_held = key_held(Key_SHIFT);
    if (input_pressed(MOVE_RIGHT)) {
        if (shift_held) swap_room(1, 0);
        move_camera(1, 0);
    } else if (input_pressed(MOVE_UP)) {
        if (shift_held) swap_room(0, 1);
        move_camera(0, 1);
    } else if (input_pressed(MOVE_LEFT)) {
        if (shift_held) swap_room(-1, 0);
        move_camera(-1, 0);
    } else if (input_pressed(MOVE_DOWN)) {
        if (shift_held) swap_room(0, -1);
        move_camera(0, -1);
    }
    
    if(shift_held) {
        zoom_level += -0.45f*os->mouse_scroll.y;
        zoom_level  = CLAMP_LOWER(zoom_level, 0.01f);
        set_view_to_proj(zoom_level);
        if (length2(os->mouse_scroll) != 0) {
            print("zoom_level: %f\n", zoom_level);
        }
    }
    if (key_held(Key_SHIFT) && key_pressed(Key_H)) {
        flip_room_horizontally();
    }
    if (key_held(Key_SHIFT) && key_pressed(Key_V)) {
        flip_room_vertically();
    }
    
    // Put stuff down.
    //
    if (!mouse_over_ui()) {
        if (key_held(Key_MLEFT)) {
            if (game->is_tile_selected) {
                if ((game->selected_tile_or_obj == Tile_WALL) && (objmap[my][mx].type != T_EMPTY));
                else
                    tilemap[my][mx] = game->selected_tile_or_obj;
            } else {
                if (tilemap[my][mx] != Tile_WALL) {
                    objmap[my][mx].type = game->selected_tile_or_obj;
                    
                    if (game->selected_tile_or_obj == T_EMITTER   || 
                        game->selected_tile_or_obj == T_DETECTOR  ||
                        game->selected_tile_or_obj == T_DOOR      ||
                        game->selected_tile_or_obj == T_DOOR_OPEN ||
                        game->selected_tile_or_obj == T_SPLITTER)
                        objmap[my][mx].c = (u8)game->selected_color;
                    
                    if (objmap[my][mx].flags == ObjFlags_NONE) {
                        objmap[my][mx].flags = (u8)game->selected_flag;
                        game->selected_flag = 0;
                    }
                    
                    if (game->selected_tile_or_obj == T_DOOR) {
                        objmap[my][mx].color[0] = (u8)game->num_detectors_required;
                    }
                    
                    if (game->selected_tile_or_obj == T_TELEPORTER) {
                        objmap[my][mx].color[0] = game->level_teleport_to;
                    }
                }
            }
        }
        if (key_held(Key_MRIGHT)) {
            if (game->is_tile_selected) {
                tilemap[my][mx] = Tile_FLOOR;
            } else {
                objmap[my][mx] = {};
            }
        }
        
        // Put player down.
        //
        if (key_held(Key_MMIDDLE)) {
            V2 cam = camera;
            set_player_position(mx, my, pdir, true);
            camera = cam;
            undo_handler_reset(&undo_handler);
        }
    }
    
    // Mouse scroll change dir.
    //
    if (!key_held(Key_SHIFT)) {
        if (os->mouse_scroll.y > 0) {
            objmap[my][mx].dir = WRAP_D(objmap[my][mx].dir + (s32)os->mouse_scroll.y);
        } else if (os->mouse_scroll.y < 0) {
            objmap[my][mx].dir = WRAP_D(objmap[my][mx].dir + (s32)os->mouse_scroll.y);
        }
    }
}
#endif

FUNCTION void get_enabled_choices(b32 is_enabled[MAX_CHOICES])
{
    for (s32 i = 0; i < MAX_CHOICES; i++)
        is_enabled[i] = true;
    if (!game_started)
        is_enabled[0] = false;
};

FUNCTION void move_selection(s32 dir)
{
    b32 is_enabled[MAX_CHOICES];
    get_enabled_choices(is_enabled);
    for (;;) {
        // Wrap move.
        menu_selection = (menu_selection + dir + ARRAY_COUNT(choices)) % ARRAY_COUNT(choices);
        if (is_enabled[menu_selection])
            break;
    }
    
}

FUNCTION void process_metagame()
{
    ////////////////////////////////
    // Main menu
    //
    if (input_pressed(MOVE_UP)) {
        move_hold_timer = 0.0f;
    } else if (input_pressed(MOVE_DOWN)) {
        move_hold_timer = 0.0f;
    }  
    
    move_hold_timer = MAX(0, move_hold_timer - os->dt);
    
    s32 dir = 0;
    if (input_held(MOVE_UP)) {
        if (move_hold_timer <= 0.0f) {
            dir = -1;
            move_hold_timer = MOVE_HOLD_DURATION;
        }
    } else if (input_held(MOVE_DOWN)) {
        if (move_hold_timer <= 0.0f) {
            dir = 1;
            move_hold_timer = MOVE_HOLD_DURATION;
        }
    }
    
    if (dir != 0) {
        move_selection(dir);
    }
    
    if (input_pressed(CONFIRM)) {
        switch (menu_selection) {
            case 0: {
                game_started = true;
                main_mode = M_GAME;
            } break;
            case 1: {
                if (load_level(S8LIT("intro"))) {
                    game_started = true;
                    main_mode = M_GAME;
                }
            } break;
            case 2: {
                os->fullscreen = !os->fullscreen;
            } break;
            case 3: {
                //if (dead)
                //undo_next(&undo_handler);
                save_game();
                os->exit = true;
            } break;
#if DEVELOPER
            case 4: {
                make_empty_level();
                main_mode = M_GAME;
            } break;
#endif
        }
    }
}

FUNCTION void game_update()
{
    game->delta_mouse   = os->mouse_ndc.xy - game->mouse_ndc_old;
    game->mouse_ndc_old = os->mouse_ndc.xy;
    game->mouse_world   = unproject(v3(camera, 0), 0.0f, 
                                    os->mouse_ndc,
                                    world_to_view_matrix,
                                    view_to_proj_matrix).xy;
    
    V2 m = round(game->mouse_world);
    m    = clamp(v2(0), m, v2((f32)(NUM_X*SIZE_X-1), (f32)(NUM_Y*SIZE_Y-1)));
    mx   = (s32)m.x;
    my   = (s32)m.y;
    
#if DEVELOPER
    if (key_pressed(Key_F1)) {
        if (main_mode == M_GAME)
            main_mode = M_EDITOR;
        else if (main_mode == M_EDITOR)
            main_mode = M_GAME;
        
        if (main_mode == M_GAME) {
            set_default_zoom();
            update_camera(true);
        }
    }
    if (key_pressed(Key_F5)) {
        PLAYER_ANIMATION_SPEED = CLAMP_LOWER(PLAYER_ANIMATION_SPEED - 1.0f, 1.0f);
        print("Animation speed: %f\n", PLAYER_ANIMATION_SPEED);
    }
    if (key_pressed(Key_F6)) {
        PLAYER_ANIMATION_SPEED = CLAMP_UPPER(20.0f, PLAYER_ANIMATION_SPEED + 1.0f);
        print("Animation speed: %f\n", PLAYER_ANIMATION_SPEED);
    }
#endif
    
    if (key_pressed(Key_ESCAPE)) {
        if (main_mode == M_GAME) {
            can_toggle_menu = true;
            if (game_started)
                menu_selection = 0;
        }
        
        if (can_toggle_menu)
            main_mode = main_mode == M_GAME? M_MENU : M_GAME;
        else
            main_mode = M_MENU;
    }
    
    if (input_pressed(RESTART_LEVEL)) {
        load_level(game->loaded_level.name);
        queued_moves_count = 0;
    }
    
    switch (main_mode) {
        case M_GAME: update_world(); break;
#if DEVELOPER
        case M_EDITOR: update_editor(); break; 
#endif
        default: process_metagame(); break;
    }
    
    camera_pos = move_towards(camera_pos, camera, os->dt*30.0f);
    set_world_to_view(v3(camera_pos, 0));
}

//
// @Todo: Combine draw_sprite and draw_spritef?
//
FUNCTION void draw_sprite(s32 x, s32 y, f32 w, f32 h, s32 s, s32 t, V4 *color, f32 a, Texture *texture = &tex)
{
    // Offsetting uv-coords with 0.05f to avoid texture bleeding.
    //
    V4 c   = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    f32 u0 = (((s + 0) * TILE_SIZE) + 0.05f) / texture->width;
    f32 v0 = (((t + 0) * TILE_SIZE) + 0.05f) / texture->height;
    f32 u1 = (((s + 1) * TILE_SIZE) - 0.05f) / texture->width;
    f32 v1 = (((t + 1) * TILE_SIZE) - 0.05f) / texture->height;
    
    immediate_rect(v2((f32)x, (f32)y), v2(w/2, h/2), v2(u0, v0), v2(u1, v1), c);
}
FUNCTION void draw_spritef(f32 x, f32 y, f32 w, f32 h, s32 s, s32 t, V4 *color, f32 a, b32 invert_u = false, Texture *texture = &tex)
{
    // Offsetting uv-coords with 0.05f to avoid texture bleeding.
    //
    V4 c   = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    f32 u0 = (((s + 0) * TILE_SIZE) + 0.05f) / texture->width;
    f32 v0 = (((t + 0) * TILE_SIZE) + 0.05f) / texture->height;
    f32 u1 = (((s + 1) * TILE_SIZE) - 0.05f) / texture->width;
    f32 v1 = (((t + 1) * TILE_SIZE) - 0.05f) / texture->height;
    
    if (invert_u) {
        SWAP(u0, u1, f32);
    }
    
    immediate_rect(v2(x, y), v2(w/2, h/2), v2(u0, v0), v2(u1, v1), c);
}
FUNCTION void draw_sprite_rotated(V2 position, V2 size, Quaternion q, s32 s, s32 t, V4 *color, f32 a, Texture *texture = &tex)
{
    // Offsetting uv-coords with 0.05f to avoid texture bleeding.
    //
    V4 c   = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    f32 u0 = (((s + 0) * TILE_SIZE) + 0.05f) / texture->width;
    f32 v0 = (((t + 0) * TILE_SIZE) + 0.05f) / texture->height;
    f32 u1 = (((s + 1) * TILE_SIZE) - 0.05f) / texture->width;
    f32 v1 = (((t + 1) * TILE_SIZE) - 0.05f) / texture->height;
    
    set_object_to_world(v3(position, 0.0f), q);
    immediate_rect(v2(0), size/2, v2(u0, v0), v2(u1, v1), c);
}

//
// @Cleanup:
//
FUNCTION void draw_sprite_text(f32 x, f32 y, f32 w, f32 h, s32 s, s32 t, V4 *color, f32 a)
{
    // @Note: We use this for text because we draw text with top_left position instead of center.
    // i.e. we call immediate_rect_tl().
    
    // Offsetting uv-coords with 0.05f to avoid texture bleeding.
    //
    V4 c   = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    f32 u0 = (((s + 0) * FONT_TILE_W) + 0.05f) / font_tex.width;
    f32 v0 = (((t + 0) * FONT_TILE_H) + 0.05f) / font_tex.height;
    f32 u1 = (((s + 1) * FONT_TILE_W) - 0.05f) / font_tex.width;
    f32 v1 = (((t + 1) * FONT_TILE_H) - 0.05f) / font_tex.height;
    
    immediate_rect_tl(v2(x, y), v2(w, h), v2(u0, v0), v2(u1, v1), c);
}
GLOBAL char *font = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/- ";
FUNCTION s32 draw_text(s32 top_left_x, s32 top_left_y, f32 scale, V4 color, u8 *text)
{
    while (*text) {
        char *p = strchr(font, *text);
        if (p) {
            s32 ch = (s32)(p - font);
            s32 s  = ch % (font_tex.width / FONT_TILE_W);
            s32 t  = ch / (font_tex.width / FONT_TILE_W);
            
            draw_sprite_text((f32)top_left_x, (f32)top_left_y, FONT_TILE_W*scale, FONT_TILE_H*scale, s, t, &color, 1.0f);
            top_left_x += (s32)(FONT_TILE_W * scale);
        }
        
        text++;
    }
    
    return top_left_x;
}

FUNCTION void draw_line(s32 src_x, s32 src_y, s32 dst_x, s32 dst_y, V4 *color, f32 a, f32 thickness = 0.06f)
{
    V4 c     = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    V2 start = v2((f32)src_x, (f32)src_y);
    V2 end   = v2((f32)dst_x, (f32)dst_y);
    
    immediate_line_2d(start, end, c, thickness);
}
FUNCTION void draw_line(V2 start, V2 end, V4 *color, f32 a, f32 thickness = 0.06f)
{
    V4 c = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    immediate_line_2d(start, end, c, thickness);
}

FUNCTION void draw_beams(s32 src_x, s32 src_y, u8 src_dir, u8 src_color)
{
    if (is_outside_map(src_x + dirs[src_dir].x, src_y + dirs[src_dir].y))
        return;
    
    V2 src_pos = {(f32)src_x, (f32)src_y};
    
    u32 func_id = 0;
    func_id |= (src_x     & 0xFF) << 24;
    func_id |= (src_y     & 0xFF) << 16;
    func_id |= (src_dir   & 0xFF) << 8; 
    func_id |= (src_color & 0xFF);
    if (array_find_index(&unique_draw_beams_calls, func_id) != -1) {
        // @Note: Started looping.
        return;
    } else {
        array_add(&unique_draw_beams_calls, func_id);
    }
    
    s32 test_x  = src_x + dirs[src_dir].x;
    s32 test_y  = src_y + dirs[src_dir].y;
    Obj test_o  = objmap[test_y][test_x];
    V2 test_pos = {(f32)test_x, (f32)test_y};
    
    b32 is_obj_aligned = (((src_x == pushed_obj.x) && (src_x == pushed_obj_pos.x)) || 
                          ((src_y == pushed_obj.y) && (src_y == pushed_obj_pos.y)));
    
    // Advance until we hit a wall or an object.
    while ((test_o.type == T_EMPTY || test_o.type == T_DOOR_OPEN) &&
           tilemap[test_y][test_x] != Tile_WALL) { 
        
        // We hit the player.
        if (test_x == px && test_y == py) {
#if CONTINUOUS_PUSHING
            b32 is_player_aligned = ((src_x == px) || (src_y == py)) ;
            // @Note: Obj was hitting beam and we are now pushing it off completely.
            if (!is_obj_aligned && is_player_aligned && (length2(pushed_obj_pos - pushed_obj) > SQUARE(0.5f))) {
                V2 dest = v2((f32)px, (f32)py) + 0.5f*fdirs[WRAP_D(src_dir + 4)];
                draw_line(src_pos, dest, &colors[src_color], 1.0f);
                return;
            }
#endif
        }
        
#if CONTINUOUS_PUSHING
        jump:
#endif
        if (is_outside_map(test_x + dirs[src_dir].x, test_y + dirs[src_dir].y)) {
            V2 edge = test_pos + 0.5f*fdirs[src_dir];
            draw_line(src_pos, edge, &colors[src_color], 1.0f);
            return;
        }
        
        test_x  += dirs[src_dir].x;
        test_y  += dirs[src_dir].y;
        test_o   = objmap[test_y][test_x];
        test_pos = {(f32)test_x, (f32)test_y};
    }
    if (tilemap[test_y][test_x] == Tile_WALL) {
        draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
        return;
    }
    
    b32 is_pushed_obj = ((test_x == pushed_obj.x) && (test_y == pushed_obj.y));
    
    // We hit an object, so we should determine which color to draw in which dir.  
    switch (test_o.type) {
        case T_DOOR: {
            draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
        } break;
        case T_EMITTER: {
            u8 c = (test_o.dir == WRAP_D(src_dir + 4))? mix_colors(test_o.c, src_color) : src_color;
            draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
        } break;
        case T_MIRROR: 
        case T_BENDER:
        case T_SPLITTER: {
            u8 inv_d       = WRAP_D(src_dir + 4);
            u8 ninv_d      = WRAP_D(inv_d + 1);
            u8 pinv_d      = WRAP_D(inv_d - 1);
            u8 p2inv_d     = WRAP_D(inv_d - 2);
            u8 reflected_d = U8_MAX;
            b32 penetrate  = true;
            
            u8 c = mix_colors(test_o.color[inv_d], src_color);
            
#if CONTINUOUS_PUSHING
            // Draw differently if this obj is being pushed.
            if (is_pushed_obj && !pushed_obj_is_at_rest()) {
                if (is_obj_aligned) {
                    // When we're in same col/row as source.
                    V2 dest = pushed_obj_pos + 0.5f*fdirs[inv_d];
                    draw_line(src_pos, dest, &colors[c], 1.0f);
                    break;
                }
                else if (length2(pushed_obj_pos - pushed_obj) <= SQUARE(0.5f)) {
                    V2 dest = test_pos + 0.5f*fdirs[inv_d];
                    draw_line(src_pos, dest, &colors[c], 1.0f);
                    break;
                } else
                    goto jump;
            }
#endif
            
            draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
            
            if (test_o.type == T_MIRROR) {
                if (test_o.dir == ninv_d)      reflected_d = WRAP_D(ninv_d + 1);
                else if (test_o.dir == pinv_d) reflected_d = WRAP_D(pinv_d - 1);
                
                if (reflected_d != U8_MAX)
                    draw_beams(test_x, test_y, reflected_d, test_o.color[reflected_d]);
            } else if (test_o.type == T_BENDER) {
                if (test_o.dir == inv_d)        reflected_d = ninv_d;
                else if (test_o.dir == ninv_d)  reflected_d = WRAP_D(ninv_d + 2);
                else if (test_o.dir == pinv_d)  reflected_d = pinv_d;
                else if (test_o.dir == p2inv_d) reflected_d = WRAP_D(p2inv_d - 1);
                
                if (reflected_d != U8_MAX)
                    draw_beams(test_x, test_y, reflected_d, test_o.color[reflected_d]);
            } else {
                if (test_o.dir == inv_d || test_o.dir == src_dir)
                    penetrate = true;
                else if (test_o.dir == ninv_d || test_o.dir == WRAP_D(src_dir + 1))
                    reflected_d = WRAP_D(ninv_d + 1);
                else if (test_o.dir == pinv_d || test_o.dir == WRAP_D(src_dir - 1))
                    reflected_d = WRAP_D(pinv_d - 1);
                else
                    penetrate = false;
                
                if (penetrate)
                    draw_beams(test_x, test_y, src_dir, test_o.color[src_dir]);
                if (reflected_d != U8_MAX)
                    draw_beams(test_x, test_y, reflected_d, test_o.color[reflected_d]);
            }
        } break;
        case T_DETECTOR: {
            u8 inv_d  = WRAP_D(src_dir + 4);
            u8 c = mix_colors(test_o.color[inv_d], src_color);
            draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
            draw_beams(test_x, test_y, src_dir, test_o.color[src_dir]);
        } break;
    }
}

FUNCTION void game_render()
{
    if (main_mode == M_MENU) {
        f32 w = os->drawing_rect.max.x - os->drawing_rect.min.x;
        f32 h = os->drawing_rect.max.y - os->drawing_rect.min.y;
        
        // @Note: Multiply these values with pixels, so that pixel sizes scale with drawing_rect size.
        f32 w_scale = w / os->render_size.width;
        f32 h_scale = h / os->render_size.height;
        
        // Gradient background.
        immediate_begin();
        set_texture(0);
        is_using_pixel_coords = true;
        V4 c0 = v4(0.275, 0.549, 0.820, 1.0f);
        V4 c1 = v4(0.298, 0.039, 0.710, 1.0f);
        V4 c2 = v4(0.629, 0.063, 0.678, 1.0f);
        V4 c3 = v4(0.451, 0.165, 0.831, 1.0f);
        immediate_vertex(v2(0, h), v2(0, 1), c0); // Bottom-left.
        immediate_vertex(v2(w, h), v2(1, 1), c1); // Bottom-right.
        immediate_vertex(v2(w, 0), v2(1, 0), c2); // Top-right.
        
        immediate_vertex(v2(0, h), v2(0, 1), c0); // Bottom-left.
        immediate_vertex(v2(w, 0), v2(1, 0), c2); // Top-right.
        immediate_vertex(v2(0, 0), v2(0, 0), c3); // Top-left.
        
        //immediate_line_2d(v2(w/2, 0), v2(w/2, h), v4(1, 0, 0, 1), 15);
        immediate_end();
        
        // Menu choices.
        immediate_begin();
        set_texture(&font_tex);
        is_using_pixel_coords = true;
        // Start slightly above center.
        s32 sx = (s32)(w/2);
        s32 sy = (s32)(h/3);
        b32 is_enabled[MAX_CHOICES];
        get_enabled_choices(is_enabled);
        for (s32 i = 0; i < MAX_CHOICES; i++) {
            if (is_enabled[i]) {
                b32 selected       = menu_selection == i;
                f32 selected_scale = selected? 5.0f : 4.0f;
                f32 final_scale    = selected_scale * w_scale;
                f32 text_width     = choices[i].count * FONT_TILE_W * final_scale;
                
                V4 c = v4(0.8f, 0.5f, 0.8f, 1.0f);
                if (selected)
                    c = v4(1);
                
                draw_text((s32)(sx - text_width/2), sy, final_scale, c, choices[i].data);
            }
            
            sy += (s32)(80 * h_scale);
        }
        immediate_end();
    } else {
        immediate_begin();
        set_texture(&tex);
        // Draw tiles.
        for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
            for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
                u8 t = tilemap[y][x];
                // Skip walls, we draw them later.
                if (t == Tile_WALL) continue; 
                
                draw_sprite(x, y, 1, 1, tile_sprite[t].s, tile_sprite[t].t, 0, 1.0f);
            }
        }
        immediate_end();
        
        if (draw_grid) {
            immediate_begin();
            set_texture(0);
            // Draw grid.
            immediate_grid(v2(-0.5f), NUM_X*SIZE_X, NUM_Y*SIZE_Y, 1, v4(1, 1, 1, 0.7f), 0.01f);
            immediate_end();
        }
        
        immediate_begin();
        set_texture(&tex);
        // Draw detectors.
        for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
            for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
                Obj o = objmap[y][x];
                V2s sprite = obj_sprite[o.type];
                if (o.type == T_DETECTOR) {
                    u8 final_c = Color_WHITE;
                    for (s32 i = 0; i < 8; i++)
                        final_c = mix_colors(final_c, o.color[i]);
                    
                    if (final_c == o.c)
                        draw_sprite(x, y, 1.1f, 1.1f, sprite.s, sprite.t, &colors[o.c], 1.0f);
                    else
                        draw_sprite(x, y, 0.9f, 0.9f, sprite.s, sprite.t, &colors[o.c], 0.6f);
                }
            }
        }
        immediate_end();
        
        immediate_begin();
        set_texture(0);
        // Draw laser beams.
        for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
            for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
                Obj o = objmap[y][x];
                if (o.type == T_EMITTER) {
                    array_reset(&unique_draw_beams_calls);
                    draw_beams(x, y, o.dir, o.c);
                }
            }
        }
        immediate_end();
        
        immediate_begin();
        set_texture(&tex);
        // Draw walls.
        for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
            for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
                u8 t = tilemap[y][x];
                if (t == Tile_WALL)
                    draw_sprite(x, y, 1, 1, tile_sprite[t].s, tile_sprite[t].t, 0, 1.0f);
            }
        }
        immediate_end();
        
#if 0
        immediate_begin();
        set_texture(&tex);
        for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
            for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
                Obj o = objmap[y][x];
                if (o.type == T_MIRROR) {
                    //
                    // @Todo: We can use quaternions to draw rotated objs instead of having separate
                    // sprite for each Dir. It's slower, but it looks much better.
                    //
                    // @Fix: When moving from Dir_SE to Dir_E, we move_towards in opposite direction.
                    //
                    V2 p = {};
                    if (pushed_obj.x == x && pushed_obj.y == y) {
                        pushed_obj_pos = move_towards(pushed_obj_pos, pushed_obj, player_max_distance);
                        p = pushed_obj_pos;
                    } else {
                        p = v2(x, y);
                    }
                    
                    V2s sprite = obj_sprite[o.type];
                    LOCAL_PERSIST f32 turns = o.dir / 8.0f;
                    turns = move_towards(turns, o.dir / 8.0f, os->dt);
                    print("turns: %f\n", turns);
                    Quaternion q = quaternion_from_axis_angle_turns(v3(0,0,1), turns);
                    draw_sprite_rotated(p, v2(1), q, sprite.s, sprite.t, &colors[o.c], 1.0f);
                }
            }
        }
        immediate_end();
#endif
        
        immediate_begin();
        set_texture(&tex);
        // Draw objs.
        for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
            for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
                Obj o = objmap[y][x];
                V2s sprite = obj_sprite[o.type];
                b32 is_pushed_obj = (pushed_obj.x == x && pushed_obj.y == y);
                switch (o.type) {
                    case T_EMPTY: continue;
                    case T_EMITTER: {
                        sprite.s += o.dir;
                        draw_sprite(x, y, 1, 1, sprite.s, sprite.t, &colors[o.c], 1.0f);
                    } break;
                    case T_MIRROR:
                    case T_BENDER:
                    case T_SPLITTER: {
                        V2s frame_closed = tile_sprite[Tile_OBJ_FRAME_CLOSED];
                        V2s frame_open   = tile_sprite[Tile_OBJ_FRAME_OPEN];
                        V4 c = v4(1);
                        if (o.type == T_SPLITTER) {
                            sprite.s = (sprite.s + o.dir) % 4;
                            c = colors[o.c];
                        } else {
                            sprite.s += o.dir;
                        }
                        
                        if (is_pushed_obj && !pushed_obj_is_at_rest())
                            draw_spritef(pushed_obj_pos.x, pushed_obj_pos.y, 1, 1, frame_closed.s, frame_closed.t, 0, 1.0f);
                        else
                            draw_sprite(x, y, 1, 1, frame_open.s, frame_open.t, 0, 1.0f);
                        
                        if (is_pushed_obj)
                            draw_spritef(pushed_obj_pos.x, pushed_obj_pos.y, 1, 1, sprite.s, sprite.t, &c, 1.0f);
                        else
                            draw_sprite(x, y, 1, 1, sprite.s, sprite.t, &c, 1.0f);
                    } break;
                    case T_DOOR:
                    case T_DOOR_OPEN: {
                        draw_sprite(x, y, 1, 1, sprite.s, sprite.t, &colors[o.c], 1.0f);
                    } break;
                    case T_TELEPORTER: {
                        draw_sprite(x, y, 1, 1, sprite.s, sprite.t, 0, 1.0f);
                    } break;
                }
            }
        }
        immediate_end();
        
        immediate_begin();
        set_texture(0);
        // Draw particles
        for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
            for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
                Obj o = objmap[y][x];
                if (o.type == T_EMPTY  &&
                    o.type != T_MIRROR &&
                    o.type != T_BENDER &&
                    o.type != T_SPLITTER) 
                    continue;
                
                // @Todo: These are all temporary and ugly particles.
                //
                if (is_set(o.flags, ObjFlags_NEVER_ROTATE)) {
                    V4 color   = v4(0.3f, 0.3f, 0.3f, 0.8f);
                    V2 size    = v2(0.06f, 0.06f);
                    V2 c       = v2((f32)x, (f32)y);
                    f32 offset = 0.35f;
                    immediate_rect(c + v2(-offset, -offset), size, color);
                    immediate_rect(c + v2( offset, -offset), size, color);
                    immediate_rect(c + v2( offset,  offset), size, color);
                    immediate_rect(c + v2(-offset,  offset), size, color);
                } else {
                    // Locked rotation particles.
                    V3 axis = {};
                    V4 color = {};
                    if (is_set(o.flags, ObjFlags_ONLY_ROTATE_CCW)) {
                        axis = v3(0,0,1);
                        color = v4(0.7f, 0.8f, 0.3f, 0.8f);
                    } else if (is_set(o.flags, ObjFlags_ONLY_ROTATE_CW)) {
                        axis = v3(0,0,-1);
                        color = v4(0.7f, 0.4f, 0.6f, 0.8f);
                    } 
                    if (is_set(o.flags, ObjFlags_ONLY_ROTATE_CCW) || is_set(o.flags, ObjFlags_ONLY_ROTATE_CW)) {
                        V2 pivot;
                        if (pushed_obj.x == x && pushed_obj.y == y)
                            pivot = v2(pushed_obj_pos.x, pushed_obj_pos.y);
                        else 
                            pivot = v2((f32)x, (f32)y);
                        
                        f32 duration = 3.0f;
                        V2 size      = v2(0.03f, 0.03f);
                        for (s32 i = 0; i < NUM_ROTATION_PARTICLES; i++) {
                            f32 radius = 0.4f;
                            V2 min     = pivot - v2(radius); 
                            V2 max     = pivot + v2(radius);
                            V2 p       = min + random_rotation_particles[i]*(max - min);
                            duration  *= length2(random_rotation_particles[i]);
                            f32 a      = repeat((f32)os->time/duration, 1.0f);
                            Quaternion q = quaternion_from_axis_angle_turns(axis, a);
                            V2 c = rotate_point_around_pivot(p, pivot, q);
                            immediate_rect(c, size, color);
                        }
                    }
                    
                }
            }
        }
        immediate_end();
        
        immediate_begin();
        set_texture(&tex);
        // Draw player.
        s32 c        = dead? Color_RED : Color_WHITE;
        f32 alpha    = (pcolor != Color_WHITE) && !dead? 0.8f : 1.0f;
        b32 invert_x = pdir == Dir_W;
        draw_spritef(ppos.x, ppos.y, 0.85f, 0.85f, psprite.s, psprite.t, &colors[c], alpha, invert_x);
        immediate_end();
        
#if DEVELOPER
        // Draw debugging stuff.
        if(main_mode == M_EDITOR)
        {
            bool show_demo_window = true;
            ImGui::ShowDemoWindow(&show_demo_window);
            do_editor(key_pressed(Key_F1));
        }
#endif
    }
}