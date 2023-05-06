
FUNCTION void save_map()
{
    Loaded_Game *g = &game->loaded_game;
    memory_copy(g->tile_map, tilemap, sizeof(tilemap));
    memory_copy(g->obj_map, objmap, sizeof(objmap));
    memory_copy(&g->cam, &camera, sizeof(camera));
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
    memory_copy(&camera, &g->cam, sizeof(camera));
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

FUNCTION void game_init()
{
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
        // Initial camera position.
        camera = 0.5f*v2(SIZE_X, SIZE_Y);
    }
}

FUNCTION void move_camera(u8 dir)
{
    switch (dir) {
        case Dir_E: {
            if (camera.x + SIZE_X < WORLD_EDGE_X)
                camera.x += SIZE_X;
        } break;
        case Dir_N: {
            if (camera.y + SIZE_Y < WORLD_EDGE_Y)
                camera.y += SIZE_Y;
        } break;
        case Dir_W: {
            if (camera.x - SIZE_X > 0)
                camera.x -= SIZE_X;
        } break;
        case Dir_S: {
            if (camera.y - SIZE_Y > 0)
                camera.y -= SIZE_Y;
        } break;
    }
    
    // @Sanity:
    camera.x = CLAMP(0, camera.x, WORLD_EDGE_X-1);
    camera.y = CLAMP(0, camera.y, WORLD_EDGE_Y-1);
}

FUNCTION b32 is_outside_map(s32 x, s32 y)
{
    b32 result = ((x < 0) || (x > WORLD_EDGE_X-1) ||
                  (y < 0) || (y > WORLD_EDGE_Y-1));
    return result;
}

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

// @Note: Square coords under the cursor in the current frame.
// src_m is used as fall back if we drop picked obj on a wrong square.
//
GLOBAL s32 mx, my;
GLOBAL s32 src_mx, src_my;
GLOBAL Obj picked_obj;

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

FUNCTION void update_world()
{
    Button_State *mb   = os->mouse_buttons;
    b32 down_left      = is_down(mb[MouseButton_LEFT]);
    b32 released_left  = was_released(mb[MouseButton_LEFT]);
    b32 released_right = was_released(mb[MouseButton_RIGHT]);
    
    if (down_left && picked_obj.type == T_EMPTY && (length2(game->delta_mouse) > SQUARE(0.000002f))) {
        // Pick obj.
        if (objmap[my][mx].type != T_EMITTER && 
            objmap[my][mx].type != T_DETECTOR && 
            objmap[my][mx].type != T_DOOR &&
            objmap[my][mx].type != T_DOOR_OPEN) {
            src_mx = mx;
            src_my = my;
            picked_obj = objmap[my][mx];
            objmap[my][mx] = {};
        }
    } else if (released_left && picked_obj.type != T_EMPTY) {
        // Drop obj.
        if (objmap[my][mx].type != T_EMPTY || tilemap[my][mx] == Tile_WALL) 
            objmap[src_my][src_mx] = picked_obj;
        else
            objmap[my][mx] = picked_obj;
        picked_obj = {};
    } else {
        // Rotate obj.
        switch (objmap[my][mx].type) {
            case T_MIRROR:
            case T_BENDER:
            case T_SPLITTER: {
                if (released_left)
                    objmap[my][mx].dir = WRAP_D(objmap[my][mx].dir + 1);
                else if (released_right)
                    objmap[my][mx].dir = WRAP_D(objmap[my][mx].dir - 1);
            } break;
        }
    }
    
    // Clear colors for all objs except lasers.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if (o.type != T_EMITTER) {
                MEMORY_ZERO_ARRAY(objmap[y][x].color);
            }
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
    
    // @Debug: We have an edge case for this when a detector and a beam are on opposite sides of the door,
    // the doors and the detector would flicker.
    // Update doors
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if (o.type == T_DETECTOR) {
                u8 final_c = Color_WHITE;
                for (s32 i = 0; i < 8; i++)
                    final_c = mix_colors(final_c, o.color[i]);
                
                if (final_c == o.c) {
                    // Look for compatible doors around detector and "open" them.
                    for (s32 dy = CLAMP_LOWER(y-1, 0); dy <= CLAMP_UPPER(NUM_Y*SIZE_Y, y+1); dy++) {
                        for (s32 dx = CLAMP_LOWER(x-1, 0); dx <= CLAMP_UPPER(NUM_X*SIZE_X, x+1); dx++) {
                            if ((objmap[dy][dx].type == T_DOOR || objmap[dy][dx].type == T_DOOR_OPEN) && objmap[dy][dx].c == o.c) 
                                objmap[dy][dx].type = T_DOOR_OPEN;
                        }}
                } else {
                    // Look for compatible doors around detector and "close" them.
                    for (s32 dy = CLAMP_LOWER(y-1, 0); dy <= CLAMP_UPPER(NUM_Y*SIZE_Y, y+1); dy++) {
                        for (s32 dx = CLAMP_LOWER(x-1, 0); dx <= CLAMP_UPPER(NUM_X*SIZE_X, x+1); dx++) {
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
    Button_State *mb = os->mouse_buttons;
    
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
    //
    // Camera movement.
    //
    if (was_pressed(kb[Key_D]))
        move_camera(Dir_E);
    if (was_pressed(kb[Key_W]))
        move_camera(Dir_N);
    if (was_pressed(kb[Key_A]))
        move_camera(Dir_W);
    if (was_pressed(kb[Key_S]))
        move_camera(Dir_S);
    if(is_down(kb[Key_SHIFT])) {
        ortho_zoom += -0.45f*os->mouse_scroll.y;
        ortho_zoom  = CLAMP_LOWER(ortho_zoom, 0.01f);
        if (length2(os->mouse_scroll) != 0) {
            print("Zoom %f\n", ortho_zoom);
        }
    }
    world_to_view_matrix = look_at(v3(camera, 0),
                                   v3(camera, 0) + v3(0, 0, -1),
                                   v3(0, 1, 0));
    
    V2 m = round(game->mouse_world);
    m    = clamp(v2(0), m, v2(WORLD_EDGE_X-1, WORLD_EDGE_X-1));
    mx   = (s32)m.x;
    my   = (s32)m.y;
    
#if DEVELOPER
    if (was_pressed(kb[Key_F1]))
        main_mode = (main_mode == M_GAME)? M_EDITOR : M_GAME;
#endif
    
    switch (main_mode) {
        case M_GAME: update_world(); break;
#if DEVELOPER
        case M_EDITOR: update_editor(); break; 
#endif
    }
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
    
    // Draw picked obj.
    if (picked_obj.type != T_EMPTY) {
        V2 mf   = game->mouse_world;
        mf      = clamp(v2(0), mf, v2(WORLD_EDGE_X-1, WORLD_EDGE_X-1));
        V2s sprite = obj_sprite[picked_obj.type];
        s32 t = sprite.t;
        s32 s = sprite.s + picked_obj.dir;
        if (picked_obj.type == T_SPLITTER)
            s = s % 4;
        
        draw_spritef(mf.x, mf.y, 1, 1, s, t, 0, 1.0f);
    }
    immediate_end();
    
#if DEVELOPER
    // Draw debugging stuff.
    immediate_begin();
    set_texture(0);
    //immediate_rect(v2(0), v2(0.2f, 0.2f), v4(1,0,1,1));
    //immediate_rect(v2(WORLD_EDGE_X-1, 0), v2(0.2f, 0.2f), v4(1,0,1,1));
    immediate_end();
    
    if(main_mode == M_EDITOR)
    {
        bool show_demo_window = true;
        ImGui::ShowDemoWindow(&show_demo_window);
        do_editor();
    }
#endif
}