
FUNCTION b32 mouse_over_ui()
{
    b32 result = false;
    
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse) result = true;
    
    return result;
}

FUNCTION void move_camera(u8 dir)
{
    V2 *cam = &game->camera_position;
    switch (dir) {
        case Dir_E: {
            if (cam->x + SIZE_X < WORLD_EDGE_X)
                cam->x += SIZE_X;
        } break;
        case Dir_N: {
            if (cam->y + SIZE_Y < WORLD_EDGE_Y)
                cam->y += SIZE_Y;
        } break;
        case Dir_W: {
            if (cam->x - SIZE_X > 0)
                cam->x -= SIZE_X;
        } break;
        case Dir_S: {
            if (cam->y - SIZE_Y > 0)
                cam->y -= SIZE_Y;
        } break;
    }
    
    // @Sanity:
    game->camera_position.x = CLAMP(0, game->camera_position.x, WORLD_EDGE_X-1);
    game->camera_position.y = CLAMP(0, game->camera_position.y, WORLD_EDGE_Y-1);
}

#if DEVELOPER
FUNCTION void do_editor()
{
    ImGuiIO& io      = ImGui::GetIO();
    Button_State *mb = os->mouse_buttons;
    b32 pressed_left  = was_pressed(mb[MouseButton_LEFT])  && mouse_over_ui();
    
    ImGui::Begin("Editor", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    
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
            ImGui::Combo("Color", &game->selected_color, cols, IM_ARRAYSIZE(cols));
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
    
    // Init map.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            tilemap[y][x] = Tile_FLOOR;
            objmap[y][x].type = T_EMPTY;
        }
    }
    
    // Initial camera position.
    game->camera_position = WORLD_ORIGIN + 0.5f*v2(SIZE_X, SIZE_Y);
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
    else if ((cur == Color_RED && src == Color_GREEN) || (cur == Color_GREEN && src == Color_RED))
        return Color_YELLOW;
    else if ((cur == Color_RED && src == Color_BLUE) || (cur == Color_BLUE && src == Color_RED))
        return Color_MAGENTA;
    else if ((cur == Color_GREEN && src == Color_BLUE) || (cur == Color_BLUE && src == Color_GREEN))
        return Color_CYAN;
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
    while (test_o.type == T_EMPTY && tilemap[test_y][test_x] != Tile_WALL) {
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
        case T_EMITTER: {
            return;
        } break;
        case T_MIRROR: {
            u8 inv_d  = (src_dir + 4) % 8;
            u8 ninv_d = (inv_d + 1) % 8;
            u8 pinv_d = (inv_d - 1 + 8) % 8;
            u8 reflected_d;
            if (test_o.dir == inv_d)
                reflected_d = inv_d;
            else if (test_o.dir == ninv_d)
                reflected_d = (ninv_d + 1) % 8;
            else if (test_o.dir == pinv_d)
                reflected_d = (pinv_d - 1 + 8) % 8;
            else {
                // This mirror is not facing the light source. 
                return;
            }
            
            // Write the color.
            objmap[test_y][test_x].color[inv_d] = mix_colors(test_o.color[inv_d], src_color);
            objmap[test_y][test_x].color[reflected_d] = mix_colors(test_o.color[reflected_d], src_color);
            
            update_beams(test_x, test_y, reflected_d, objmap[test_y][test_x].color[reflected_d]);
        }
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
        if (objmap[my][mx].type != T_EMITTER) {
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
            case T_MIRROR: {
                if (released_left)
                    objmap[my][mx].dir = (objmap[my][mx].dir + 1) % 8;
                else if (released_right)
                    objmap[my][mx].dir = (objmap[my][mx].dir - 1 + 8) % 8;
            } break;
        }
    }
    
    // Clear colors for all objs except lasers.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if (o.type == T_EMPTY) continue;
            else if (o.type != T_EMITTER) {
                MEMORY_ZERO_ARRAY(objmap[y][x].color);
            }
        }
    }
    
    // Update laser beams.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if (o.type == T_EMITTER) {
                update_beams(x, y, o.dir, o.color[o.dir]);
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
                    u8 d = objmap[my][mx].dir;
                    objmap[my][mx].type = game->selected_tile_or_obj;
                    
                    if (game->selected_tile_or_obj == T_EMITTER)
                        objmap[my][mx].color[d] = (u8)game->selected_color;
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
    
    // Mouse scroll change dir and update color.
    //
    if (os->mouse_scroll.y > 0) {
        u8 old_dir_color = objmap[my][mx].color[objmap[my][mx].dir];
        objmap[my][mx].dir = (objmap[my][mx].dir + (s32)os->mouse_scroll.y) % 8;
        
        if (objmap[my][mx].type == T_EMITTER) {
            MEMORY_ZERO_ARRAY(objmap[my][mx].color);
            objmap[my][mx].color[objmap[my][mx].dir] = old_dir_color;
        }
    } else if (os->mouse_scroll.y < 0) {
        u8 old_dir_color = objmap[my][mx].color[objmap[my][mx].dir];
        objmap[my][mx].dir = (objmap[my][mx].dir + (s32)os->mouse_scroll.y + 8) % 8;
        
        if (objmap[my][mx].type == T_EMITTER) {
            MEMORY_ZERO_ARRAY(objmap[my][mx].color);
            objmap[my][mx].color[objmap[my][mx].dir] = old_dir_color;
        }
    }
}
#endif

FUNCTION void game_update()
{
    Button_State *kb    = os->keyboard_buttons;
    game->delta_mouse   = os->mouse_ndc.xy - game->mouse_ndc_old;
    game->mouse_ndc_old = os->mouse_ndc.xy;
    game->mouse_world   = unproject(v3(game->camera_position, 0), 0.0f, 
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
    }
    world_to_view_matrix = look_at(v3(game->camera_position, 0),
                                   v3(game->camera_position, 0) + v3(0, 0, -1),
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

FUNCTION void draw_sprite(s32 x, s32 y, s32 s, s32 t, V4 *color, f32 a)
{
    // Offsetting uv-coords with 0.05f to avoid texture bleeding.
    //
    V4 c   = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    f32 u0 = (((s + 0) * TILE_SIZE) + 0.05f) / tex.width;
    f32 v0 = (((t + 0) * TILE_SIZE) + 0.05f) / tex.height;
    f32 u1 = (((s + 1) * TILE_SIZE) - 0.05f) / tex.width;
    f32 v1 = (((t + 1) * TILE_SIZE) - 0.05f) / tex.height;
    
    immediate_rect(v2((f32)x, (f32)y), v2(0.5f), v2(u0, v0), v2(u1, v1), c);
}

FUNCTION void draw_spritef(f32 x, f32 y, s32 s, s32 t, V4 *color, f32 a)
{
    // Offsetting uv-coords with 0.05f to avoid texture bleeding.
    //
    V4 c   = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    f32 u0 = (((s + 0) * TILE_SIZE) + 0.05f) / tex.width;
    f32 v0 = (((t + 0) * TILE_SIZE) + 0.05f) / tex.height;
    f32 u1 = (((s + 1) * TILE_SIZE) - 0.05f) / tex.width;
    f32 v1 = (((t + 1) * TILE_SIZE) - 0.05f) / tex.height;
    
    immediate_rect(v2(x, y), v2(0.5f), v2(u0, v0), v2(u1, v1), c);
}

FUNCTION void draw_line(s32 src_x, s32 src_y, s32 dst_x, s32 dst_y, V4 *color, f32 a, f32 thickness = 0.06f)
{
    V4 c     = color? v4(color->rgb, color->a * a) : v4(1, 1, 1, a);
    V2 start = v2((f32)src_x, (f32)src_y);
    V2 end   = v2((f32)dst_x, (f32)dst_y);
    
    immediate_line_2d(start, end, c, thickness);
}

FUNCTION void draw_beams(s32 src_x, s32 src_y, u8 src_dir, u8 src_color, u8 src_is_emitter)
{
    if (is_outside_map(src_x + dirs[src_dir].x, src_y + dirs[src_dir].y))
        return;
    
    s32 test_x  = src_x + dirs[src_dir].x;
    s32 test_y  = src_y + dirs[src_dir].y;
    Obj test_o  = objmap[test_y][test_x];
    
    // Advance until we hit a wall or an object.
    while (test_o.type == T_EMPTY && tilemap[test_y][test_x] != Tile_WALL) {
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
        case T_EMITTER: {
            draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
            return;
        } break;
        case T_MIRROR: {
            u8 inv_d  = (src_dir + 4) % 8;
            u8 ninv_d = (inv_d + 1) % 8;
            u8 pinv_d = (inv_d - 1 + 8) % 8;
            u8 reflected_d;
            if (test_o.dir == inv_d)
                reflected_d = inv_d;
            else if (test_o.dir == ninv_d)
                reflected_d = (ninv_d + 1) % 8;
            else if (test_o.dir == pinv_d)
                reflected_d = (pinv_d - 1 + 8) % 8;
            else {
                // This mirror is not facing the light source. 
                draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
                return;
            }
            
            if (src_is_emitter) {
                u8 c = mix_colors(test_o.color[inv_d], src_color);
                draw_line(src_x, src_y, test_x, test_y, &colors[c], 1.0f);
            } else {
                draw_line(src_x, src_y, test_x, test_y, &colors[src_color], 1.0f);
            }
            draw_beams(test_x, test_y, reflected_d, test_o.color[reflected_d], 0);
        }
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
            if (t == Tile_WALL) continue;
            
            draw_sprite(x, y, tile_sprite[t].s, tile_sprite[t].t, 0, 1.0f);
        }
    }
    immediate_end();
    
    immediate_begin();
    set_texture(0);
    // Draw grid.
    immediate_grid(WORLD_ORIGIN, NUM_X*SIZE_X, NUM_Y*SIZE_Y, 1, v4(1));
    immediate_end();
    
    immediate_begin();
    set_texture(0);
    // Draw laser beams.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            if (o.type == T_EMITTER) {
                draw_beams(x, y, o.dir, o.color[o.dir], 1);
            }
        }
    }
    immediate_end();
    
    immediate_begin();
    set_texture(&tex);
    // Draw tiles.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            u8 t = tilemap[y][x];
            if (t == Tile_WALL)
                draw_sprite(x, y, tile_sprite[t].s, tile_sprite[t].t, 0, 1.0f);
        }
    }
    immediate_end();
    
    
    immediate_begin();
    set_texture(&tex);
    // Draw objs.
    for (s32 y = 0; y < NUM_Y*SIZE_Y; y++) {
        for (s32 x = 0; x < NUM_X*SIZE_X; x++) {
            Obj o = objmap[y][x];
            switch (o.type) {
                case T_EMPTY: continue; break;
                case T_EMITTER: {
                    V2s sprite = obj_sprite[o.type];
                    sprite.s  += o.dir;
                    draw_sprite(x, y, sprite.s, sprite.t, &colors[o.color[o.dir]], 1.0f);
                } break;
                case T_MIRROR: {
                    V2s sprite = obj_sprite[o.type];
                    sprite.s  += o.dir;
                    draw_sprite(x, y, sprite.s, sprite.t, 0, 1.0f);
                } break;
            }
        }
    }
    
    // Draw picked obj.
    if (picked_obj.type != T_EMPTY) {
        V2 mf   = game->mouse_world;
        mf      = clamp(v2(0), mf, v2(WORLD_EDGE_X-1, WORLD_EDGE_X-1));
        V2s sprite = obj_sprite[picked_obj.type];
        draw_spritef(mf.x, mf.y, sprite.s + picked_obj.dir, sprite.t, 0, 1.0f);
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