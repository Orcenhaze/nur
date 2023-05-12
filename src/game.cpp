
FUNCTION void update_camera()
{
    camera = v2(rx + SIZE_X/2, ry + SIZE_Y/2);
}

FUNCTION void set_player_position(s32 x, s32 y)
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
    px = x;
    py = y;
}

FUNCTION void save_map()
{
    Loaded_Game *g = &game->loaded_game;
    memory_copy(g->tile_map, tilemap, sizeof(tilemap));
    memory_copy(g->obj_map, objmap, sizeof(objmap));
    Player player;
    player.x = px;
    player.y = py;
    memory_copy(&g->player, &player, sizeof(player));
}

FUNCTION void save_game()
{
    USE_TEMP_ARENA_IN_THIS_SCOPE;
    String_Builder sb = sb_init();
    defer(sb_free(&sb));
    
    save_map();
    sb_append(&sb, &game->loaded_game, sizeof(game->loaded_game));
    os->write_entire_file(sprint("%Ssave.dat", os->data_folder), sb_to_string(&sb));
}

FUNCTION void reload_map()
{
    Loaded_Game *g = &game->loaded_game;
    memory_copy(tilemap, g->tile_map, sizeof(tilemap));
    memory_copy(objmap, g->obj_map, sizeof(objmap));
    Player player;
    memory_copy(&player, &g->player, sizeof(player));
    set_player_position(player.x, player.y);
}

FUNCTION void load_game()
{
    USE_TEMP_ARENA_IN_THIS_SCOPE;
    String8 file = os->read_entire_file(sprint("%Ssave.dat", os->data_folder));
    if (!file.data) {
        print("Couldn't load game!");
        return;
    }
    defer(os->free_file_memory(file.data));
    
    MEMORY_COPY_STRUCT(&game->loaded_game, file.data);
    reload_map();
    game_loaded = true;
}

FUNCTION b32 mouse_over_ui()
{
    b32 result = false;
    
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse) result = true;
    
    return result;
}

#if DEVELOPER
FUNCTION void do_editor()
{
    ImGuiIO& io      = ImGui::GetIO();
    Button_State *mb = os->mouse_buttons;
    b32 pressed_left  = was_pressed(mb[MouseButton_LEFT])  && mouse_over_ui();
    
    ImGui::Begin("Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    
    //
    // Save game.
    {
        if (ImGui::Button("Save Game")) {
            save_game();
            ImGui::SameLine(); ImGui::Text("Saved!");
        }
        
        if (was_pressed(os->keyboard_buttons[Key_F3])) {
            reload_map();
        }
    }
    
    ImGui::Dummy(ImVec2(0, ImGui::GetFrameHeight()));
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
            ImGui::SameLine(0, ImGui::GetFrameHeight());
            const char* cols[] = { "WHITE", "RED", "GREEN", "BLUE", "YELLOW", "MAGENTA", "CYAN", };
            const char* col = cols[game->selected_color];
            ImGui::SliderInt("Color", &game->selected_color, 0, ARRAY_COUNT(cols)-1, col);
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
    
    game = PUSH_STRUCT_ZERO(&os->permanent_arena, Game_State);
    
    {
        USE_TEMP_ARENA_IN_THIS_SCOPE;
        d3d11_load_texture(&tex, sprint("%Ssprites.png", os->data_folder));
    }
    
    load_game();
    
	// @Todo: Rename game_loaded to something that indicates whether a save.dat file is present or not. If not present: we don't load, we don't display "continue game". Instead, we only display "new game". 
    // If we "save and quit" and there's no save.dat, i.e. we never started the game, we simply return. 
	// "new game" calls init_map() and "continue game" just loads the map from read save.dat file.
    if (!game_loaded) {
        // Init map.
        for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
            for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
                tilemap[y][x] = Tile_FLOOR;
                objmap[y][x].type = T_EMPTY;
            }
        }
        // Initial player and room position.
        set_player_position(4, 4);
    }
    
    world_to_view_matrix = look_at(v3(camera, 0),
                                   v3(camera, 0) + v3(0, 0, -1),
                                   v3(0, 1, 0));
}

#if DEVELOPER
FUNCTION void move_camera(s32 dir_x, s32 dir_y)
{
    if (!dir_x && !dir_y) return;
    s32 temp_x = camera.x + SIZE_X*dir_x;
    s32 temp_y = camera.y + SIZE_Y*dir_y;
    if (is_outside_map(temp_x, temp_y))
        return;
    
    camera.x = temp_x;
    camera.y = temp_y;
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
    
    // primary-secondary, choose secondary.
    else if ((cur >= Color_YELLOW && src <= Color_BLUE))
        return cur;
    else if ((cur <= Color_BLUE && src >= Color_YELLOW))
        return src;
    else
        return Color_WHITE;
}

FUNCTION void update_beams(s32 src_x, s32 src_y, u8 src_dir, u8 src_color)
{
    if (is_outside_map(src_x + dirs[src_dir].x, src_y + dirs[src_dir].y))
        return;
    
    s32 test_x = src_x + dirs[src_dir].x;
    s32 test_y = src_y + dirs[src_dir].y;
    Obj test_o = objmap[test_y][test_x];
    
    // Advance until we hit a wall or an object.
    while ((test_o.type == T_EMPTY || test_o.type == T_DOOR_OPEN) &&
           tilemap[test_y][test_x] != Tile_WALL) {
        if (is_outside_map(test_x + dirs[src_dir].x, test_y + dirs[src_dir].y))
            return;
        test_x += dirs[src_dir].x;
        test_y += dirs[src_dir].y;
        test_o  = objmap[test_y][test_x];
    }
    if (tilemap[test_y][test_x] == Tile_WALL) 
        return;
    
    // We hit an object, so we should determine which color to reflect in which dir.  
    switch (test_o.type) {
        case T_DOOR:
        case T_EMITTER: {
            return;
        } break;
        case T_MIRROR: {
            u8 inv_d  = WRAP_D(src_dir + 4);
            u8 ninv_d = WRAP_D(inv_d + 1);
            u8 pinv_d = WRAP_D(inv_d - 1 );
            u8 reflected_d;
            if (test_o.dir == ninv_d)
                reflected_d = WRAP_D(ninv_d + 1);
            else if (test_o.dir == pinv_d)
                reflected_d = WRAP_D(pinv_d - 1);
            else {
                // This mirror is not facing the light source OR directly facing the light source. 
                return;
            }
            
            // Write the color.
            objmap[test_y][test_x].color[reflected_d] = mix_colors(test_o.color[reflected_d], src_color);
            
            update_beams(test_x, test_y, reflected_d, objmap[test_y][test_x].color[reflected_d]);
        } break;
        case T_BENDER: {
            u8 inv_d   = WRAP_D(src_dir + 4);
            u8 ninv_d  = WRAP_D(inv_d + 1);
            u8 pinv_d  = WRAP_D(inv_d - 1);
            u8 p2inv_d = WRAP_D(inv_d - 2);
            u8 reflected_d;
            if (test_o.dir == inv_d)
                reflected_d = ninv_d;
            else if (test_o.dir == ninv_d)
                reflected_d = WRAP_D(ninv_d + 2);
            else if (test_o.dir == pinv_d)
                reflected_d = pinv_d;
            else if (test_o.dir == p2inv_d)
                reflected_d = WRAP_D(p2inv_d - 1);
            else {
                // This mirror is not facing the light source. 
                return;
            }
            
            // Write the color.
            objmap[test_y][test_x].color[reflected_d] = mix_colors(test_o.color[reflected_d], src_color);
            
            update_beams(test_x, test_y, reflected_d, objmap[test_y][test_x].color[reflected_d]);
        } break;
        case T_SPLITTER: {
            u8 inv_d  = WRAP_D(src_dir + 4);
            u8 ninv_d = WRAP_D(inv_d + 1);
            u8 pinv_d = WRAP_D(inv_d - 1);
            u8 reflected_d = 0;
            b32 do_reflect = true;
            if (test_o.dir == inv_d || test_o.dir == src_dir)
                do_reflect = false;
            else if (test_o.dir == ninv_d || test_o.dir == WRAP_D(src_dir + 1))
                reflected_d = WRAP_D(ninv_d + 1);
            else if (test_o.dir == pinv_d || test_o.dir == WRAP_D(src_dir - 1))
                reflected_d = WRAP_D(pinv_d - 1);
            else {
                // This mirror is not facing the light source. 
                return;
            }
            
            // Write the color.
            objmap[test_y][test_x].color[src_dir] = mix_colors(test_o.color[src_dir], src_color);
            update_beams(test_x, test_y, src_dir, objmap[test_y][test_x].color[src_dir]);
            
            if (do_reflect) {
                objmap[test_y][test_x].color[reflected_d] = mix_colors(test_o.color[reflected_d], src_color);
                update_beams(test_x, test_y, reflected_d, objmap[test_y][test_x].color[reflected_d]);
            }
        } break;
        case T_DETECTOR: {
            u8 inv_d  = WRAP_D(src_dir + 4);
            objmap[test_y][test_x].color[src_dir] = mix_colors(test_o.color[src_dir], src_color);
            
            update_beams(test_x, test_y, src_dir, objmap[test_y][test_x].color[src_dir]);
        } break;
    }
}

FUNCTION b32 is_obj_collides(u8 type)
{
    // @Todo: Could make interesting puzzles if we allow pushing things over T_DOOR_OPEN.
    // But we can't have two objs occupying the same square!
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

FUNCTION void move(s32 dir_x, s32 dir_y)
{
    if (!dir_x && !dir_y) return;
    pdir = dir_x? dir_x<0? Dir_W : Dir_E : dir_y<0? Dir_S : Dir_N;
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
            
            SWAP(objmap[tempy][tempx], objmap[ny][nx], Obj);
        }
    }
    
    set_player_position(tempx, tempy);
}

FUNCTION void update_world()
{
    Button_State *kb   = os->keyboard_buttons;
    Button_State *mb   = os->mouse_buttons;
    b32 down_left      = is_down(mb[MouseButton_LEFT]);
    b32 released_left  = was_released(mb[MouseButton_LEFT]);
    b32 released_right = was_released(mb[MouseButton_RIGHT]);
    
    // @Important:
    // @Todo: Improve movement, must rework key processing in orh.h.
    //
    b32 right = was_pressed(kb[Key_D]) || was_pressed(kb[Key_RIGHT]);
    b32 up    = was_pressed(kb[Key_W]) || was_pressed(kb[Key_UP]);
    b32 left  = was_pressed(kb[Key_A]) || was_pressed(kb[Key_LEFT]);
    b32 down  = was_pressed(kb[Key_S]) || was_pressed(kb[Key_DOWN]);
    s32 accel_x = 0, accel_y = 0;
    if (right) {
        accel_x =  1;
        accel_y =  0;
    }
    else if (up) {
        accel_x =  0;
        accel_y =  1;
    }
    else if (left) {
        accel_x = -1;
        accel_y =  0;
    }
    else if (down) {
        accel_x =  0;
        accel_y = -1;
    }
    
    move(accel_x, accel_y);
    
    if (was_pressed(kb[Key_Q]) || was_pressed(kb[Key_E])) {
        // Rotate objs around player.
        for (s32 dy = CLAMP_LOWER(py-1, 0); dy <= CLAMP_UPPER(NUM_Y*SIZE_Y-1, py+1); dy++) {
            for (s32 dx = CLAMP_LOWER(px-1, 0); dx <= CLAMP_UPPER(NUM_X*SIZE_X-1, px+1); dx++) {
                if (dy == py && dx == px)
                    continue;
                
                switch (objmap[dy][dx].type) {
                    case T_MIRROR:
                    case T_BENDER:
                    case T_SPLITTER: {
                        if (was_pressed(kb[Key_Q])) // Rotate CCW
                            objmap[dy][dx].dir = WRAP_D(objmap[dy][dx].dir + 1);
                        else if (was_pressed(kb[Key_E])) // Rotate CW
                            objmap[dy][dx].dir = WRAP_D(objmap[dy][dx].dir - 1);
                    } break;
                }
            }
        }
    }
    
    // Clear colors for all objs.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            MEMORY_ZERO_ARRAY(objmap[y][x].color);
        }
    }
    
    // Update laser beams.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if (o.type == T_EMITTER) {
                update_beams(x, y, o.dir, o.c);
                
                for (s32 i = 0; i < 8; i++)
                    ASSERT(o.color[i] == 0);
            }
        }
    }
    
    // Update doors
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if (o.type == T_DETECTOR) {
                s32 room_x     = SIZE_X*(x/SIZE_X);
                s32 room_y     = SIZE_Y*(y/SIZE_Y);
                
                u8 final_c = Color_WHITE;
                for (s32 i = 0; i < 8; i++)
                    final_c = mix_colors(final_c, o.color[i]);
                
                if (final_c == o.c) {
                    // Open doors in room.
                    for (s32 dy = room_y; dy < room_y+SIZE_Y; dy++) {
                        for (s32 dx = room_x; dx < room_x+SIZE_X; dx++) {
                            if (dx == x && dy == y)
                                continue;
                            if ((objmap[dy][dx].type == T_DOOR || objmap[dy][dx].type == T_DOOR_OPEN) && objmap[dy][dx].c == o.c) 
                                objmap[dy][dx].type = T_DOOR_OPEN;
                        }}
                } else {
                    // Close doors in room.
                    for (s32 dy = room_y; dy < room_y+SIZE_Y; dy++) {
                        for (s32 dx = room_x; dx < room_x+SIZE_X; dx++) {
                            if (dx == x && dy == y)
                                continue;
                            if ((objmap[dy][dx].type == T_DOOR || objmap[dy][dx].type == T_DOOR_OPEN) && objmap[dy][dx].c == o.c)
                                objmap[dy][dx].type = T_DOOR;
                        }}
                }
            }
        }
    }
    
}

#if DEVELOPER
FUNCTION void update_editor()
{
    Button_State *kb = os->keyboard_buttons;
    Button_State *mb = os->mouse_buttons;
    b32 shift_down = is_down(kb[Key_SHIFT]);
    b32 right = was_pressed(kb[Key_D]) || was_pressed(kb[Key_RIGHT]);
    b32 up    = was_pressed(kb[Key_W]) || was_pressed(kb[Key_UP]);
    b32 left  = was_pressed(kb[Key_A]) || was_pressed(kb[Key_LEFT]);
    b32 down  = was_pressed(kb[Key_S]) || was_pressed(kb[Key_DOWN]);
    if (right) {
        if (shift_down) swap_room(1, 0);
        move_camera(1, 0);
    } else if (up) {
        if (shift_down) swap_room(0, 1);
        move_camera(0, 1);
    } else if (left) {
        if (shift_down) swap_room(-1, 0);
        move_camera(-1, 0);
    } else if (down) {
        if (shift_down) swap_room(0, -1);
        move_camera(0, -1);
    }
    
    if(is_down(kb[Key_SHIFT])) {
        ortho_zoom += -0.45f*os->mouse_scroll.y;
        ortho_zoom  = CLAMP_LOWER(ortho_zoom, 0.01f);
        if (length2(os->mouse_scroll) != 0) {
            print("Zoom %f\n", ortho_zoom);
        }
    }
    if (is_down(kb[Key_CONTROL]) && was_pressed(kb[Key_H])) {
        flip_room_horizontally();
    }
    if (is_down(kb[Key_CONTROL]) && was_pressed(kb[Key_V])) {
        flip_room_vertically();
    }
    
    
    // Put stuff down.
    //
    if (!mouse_over_ui()) {
        if (is_down(mb[MouseButton_LEFT])) {
            if (game->is_tile_selected) {
                if ((game->selected_tile_or_obj == Tile_WALL) && (objmap[my][mx].type != T_EMPTY));
                else
                    tilemap[my][mx] = game->selected_tile_or_obj;
            } else {
                if (tilemap[my][mx] != Tile_WALL) {
                    objmap[my][mx].type = game->selected_tile_or_obj;
                    
                    if (game->selected_tile_or_obj == T_EMITTER || 
                        game->selected_tile_or_obj == T_DETECTOR ||
                        game->selected_tile_or_obj == T_DOOR ||
                        game->selected_tile_or_obj == T_DOOR_OPEN)
                        objmap[my][mx].c = (u8)game->selected_color;
                }
            }
        }
        if (is_down(mb[MouseButton_RIGHT])) {
            if (game->is_tile_selected) {
                tilemap[my][mx] = Tile_FLOOR;
            } else {
                objmap[my][mx] = {};
            }
        }
    }
    
    // Put player down.
    //
    if (is_down(mb[MouseButton_MIDDLE])) {
        V2 cam = camera;
        set_player_position(mx, my);
        camera = cam;
    }
    
    // Mouse scroll change dir.
    //
    if (os->mouse_scroll.y > 0) {
        objmap[my][mx].dir = WRAP_D(objmap[my][mx].dir + (s32)os->mouse_scroll.y);
    } else if (os->mouse_scroll.y < 0) {
        objmap[my][mx].dir = WRAP_D(objmap[my][mx].dir + (s32)os->mouse_scroll.y);
    }
}
#endif

FUNCTION void game_update()
{
    Button_State *kb    = os->keyboard_buttons;
    game->delta_mouse   = os->mouse_ndc.xy - game->mouse_ndc_old;
    game->mouse_ndc_old = os->mouse_ndc.xy;
    game->mouse_world   = unproject(v3(camera, 0), 0.0f, 
                                    os->mouse_ndc,
                                    world_to_view_matrix,
                                    view_to_proj_matrix).xy;
    
    V2 m = round(game->mouse_world);
    m    = clamp(v2(0), m, v2(NUM_X*SIZE_X-1, NUM_X*SIZE_X-1));
    mx   = (s32)m.x;
    my   = (s32)m.y;
    
#if DEVELOPER
    if (was_pressed(kb[Key_F1]))
        main_mode = (main_mode == M_GAME)? M_EDITOR : M_GAME;
    if (main_mode == M_GAME) {
        // @Hardcoded:
        ortho_zoom = 4.5f;
        update_camera();
    }
#endif
    
    switch (main_mode) {
        case M_GAME: update_world(); break;
#if DEVELOPER
        case M_EDITOR: update_editor(); break; 
#endif
    }
    
    world_to_view_matrix = look_at(v3(camera, 0),
                                   v3(camera, 0) + v3(0, 0, -1),
                                   v3(0, 1, 0));
}

FUNCTION void draw_sprite(s32 x, s32 y, f32 w_scale, f32 h_scale, s32 s, s32 t, V4 *color, f32 a)
{
    // Offsetting uv-coords with 0.05f to avoid texture bleeding.
    //
    V4 c   = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    V2 hs  = v2(0.5f * w_scale, 0.5f * h_scale);
    f32 u0 = (((s + 0) * TILE_SIZE) + 0.05f) / tex.width;
    f32 v0 = (((t + 0) * TILE_SIZE) + 0.05f) / tex.height;
    f32 u1 = (((s + 1) * TILE_SIZE) - 0.05f) / tex.width;
    f32 v1 = (((t + 1) * TILE_SIZE) - 0.05f) / tex.height;
    
    immediate_rect(v2((f32)x, (f32)y), hs, v2(u0, v0), v2(u1, v1), c);
}

FUNCTION void draw_spritef(f32 x, f32 y, f32 w_scale, f32 h_scale, s32 s, s32 t, V4 *color, f32 a)
{
    // Offsetting uv-coords with 0.05f to avoid texture bleeding.
    //
    V4 c   = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    V2 hs  = v2(0.5f * w_scale, 0.5f * h_scale);
    f32 u0 = (((s + 0) * TILE_SIZE) + 0.05f) / tex.width;
    f32 v0 = (((t + 0) * TILE_SIZE) + 0.05f) / tex.height;
    f32 u1 = (((s + 1) * TILE_SIZE) - 0.05f) / tex.width;
    f32 v1 = (((t + 1) * TILE_SIZE) - 0.05f) / tex.height;
    
    immediate_rect(v2(x, y), hs, v2(u0, v0), v2(u1, v1), c);
}

FUNCTION void draw_line(s32 src_x, s32 src_y, s32 dst_x, s32 dst_y, V4 *color, f32 a, f32 thickness = 0.06f)
{
    V4 c     = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    V2 start = v2((f32)src_x, (f32)src_y);
    V2 end   = v2((f32)dst_x, (f32)dst_y);
    
    immediate_line_2d(start, end, c, thickness);
}

FUNCTION void draw_beams(s32 src_x, s32 src_y, u8 src_dir, u8 src_color)
{
    if (is_outside_map(src_x + dirs[src_dir].x, src_y + dirs[src_dir].y))
        return;
    
    s32 test_x  = src_x + dirs[src_dir].x;
    s32 test_y  = src_y + dirs[src_dir].y;
    Obj test_o  = objmap[test_y][test_x];
    
    // Advance until we hit a wall or an object.
    while ((test_o.type == T_EMPTY || test_o.type == T_DOOR_OPEN) &&
           tilemap[test_y][test_x] != Tile_WALL) { 
        if (is_outside_map(test_x + dirs[src_dir].x, test_y + dirs[src_dir].y)) {
            draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
            return;
        }
        test_x += dirs[src_dir].x;
        test_y += dirs[src_dir].y;
        test_o  = objmap[test_y][test_x];
    }
    if (tilemap[test_y][test_x] == Tile_WALL) {
        draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
        return;
    }
    
    // We hit an object, so we should determine which color to reflect in which dir.  
    switch (test_o.type) {
        case T_DOOR: {
            draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
            return;
        } break;
        case T_EMITTER: {
            u8 inv_d  = WRAP_D(src_dir + 4);
            if (test_o.dir == inv_d) {
                u8 c = mix_colors(test_o.c, src_color);
                draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
            } else {
                draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
            }
            return;
        } break;
        case T_MIRROR: {
            u8 inv_d  = WRAP_D(src_dir + 4);
            u8 ninv_d = WRAP_D(inv_d + 1);
            u8 pinv_d = WRAP_D(inv_d - 1);
            u8 reflected_d;
            if (test_o.dir == ninv_d)
                reflected_d = WRAP_D(ninv_d + 1);
            else if (test_o.dir == pinv_d)
                reflected_d = WRAP_D(pinv_d - 1);
            else {
                // This mirror is not facing the light source OR directly facing the light source. 
                u8 c = mix_colors(test_o.color[inv_d], src_color);
                draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
                return;
            }
            
            u8 c = mix_colors(test_o.color[inv_d], src_color);
            draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
            draw_beams(test_x, test_y, reflected_d, test_o.color[reflected_d]);
        } break;
        case T_BENDER: {
            u8 inv_d   = WRAP_D(src_dir + 4);
            u8 ninv_d  = WRAP_D(inv_d + 1);
            u8 pinv_d  = WRAP_D(inv_d - 1);
            u8 p2inv_d = WRAP_D(inv_d - 2);
            u8 reflected_d;
            if (test_o.dir == inv_d)
                reflected_d = ninv_d;
            else if (test_o.dir == ninv_d)
                reflected_d = WRAP_D(ninv_d + 2);
            else if (test_o.dir == pinv_d)
                reflected_d = pinv_d;
            else if (test_o.dir == p2inv_d)
                reflected_d = WRAP_D(p2inv_d - 1);
            else {
                // This mirror is not facing the light source. 
                u8 c = mix_colors(test_o.color[inv_d], src_color);
                draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
                return;
            }
            
            u8 c = mix_colors(test_o.color[inv_d], src_color);
            draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
            draw_beams(test_x, test_y, reflected_d, test_o.color[reflected_d]);
        } break;
        case T_SPLITTER: {
            u8 inv_d  = WRAP_D(src_dir + 4);
            u8 ninv_d = WRAP_D(inv_d + 1);
            u8 pinv_d = WRAP_D(inv_d - 1);
            u8 reflected_d = 0;
            b32 do_reflect = true;
            if (test_o.dir == inv_d || test_o.dir == src_dir)
                do_reflect = false;
            else if (test_o.dir == ninv_d || test_o.dir == WRAP_D(src_dir + 1))
                reflected_d = WRAP_D(ninv_d + 1);
            else if (test_o.dir == pinv_d || test_o.dir == WRAP_D(src_dir - 1))
                reflected_d = WRAP_D(pinv_d - 1);
            else {
                // This mirror is not facing the light source.
                u8 c = mix_colors(test_o.color[inv_d], src_color);
                draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
                return;
            }
            
            u8 c = mix_colors(test_o.color[inv_d], src_color);
            draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
            draw_beams(test_x, test_y, src_dir, test_o.color[src_dir]);
            
            if (do_reflect) {
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
    
    immediate_begin();
    set_texture(0);
    // Draw grid.
    immediate_grid(v2(-0.5f), NUM_X*SIZE_X, NUM_Y*SIZE_Y, 1, v4(1));
    immediate_end();
    
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
    
    immediate_begin();
    set_texture(&tex);
    // Draw objs.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            V2s sprite = obj_sprite[o.type];
            switch (o.type) {
                case T_EMPTY: continue;
                case T_EMITTER: {
                    sprite.s += o.dir;
                    draw_sprite(x, y, 1, 1, sprite.s, sprite.t, &colors[o.c], 1.0f);
                } break;
                case T_MIRROR: {
                    sprite.s += o.dir;
                    draw_sprite(x, y, 1, 1, sprite.s, sprite.t, 0, 1.0f);
                } break;
                case T_BENDER: {
                    sprite.s += o.dir;
                    draw_sprite(x, y, 1, 1, sprite.s, sprite.t, 0, 1.0f);
                } break;
                case T_SPLITTER: {
                    sprite.s = (sprite.s + o.dir) % 4;
                    draw_sprite(x, y, 1, 1, sprite.s, sprite.t, 0, 1.0f);
                } break;
                case T_DOOR:
                case T_DOOR_OPEN: {
                    draw_sprite(x, y, 1, 1, sprite.s, sprite.t, &colors[o.c], 1.0f);
                } break;
            }
        }
    }
    
    // Draw player.
    draw_sprite(px, py, 0.8f, 0.8f, 0 + pdir, 7, 0, 1.0f);
    immediate_end();
    
#if DEVELOPER
    // Draw debugging stuff.
    immediate_begin();
    set_texture(0);
    //immediate_rect(v2(0), v2(0.2f, 0.2f), v4(1,0,1,1));
    //immediate_rect(v2(NUM_X*SIZE_X-1, 0), v2(0.2f, 0.2f), v4(1,0,1,1));
    immediate_end();
    
    if(main_mode == M_EDITOR)
    {
        bool show_demo_window = true;
        ImGui::ShowDemoWindow(&show_demo_window);
        do_editor();
    }
#endif
}