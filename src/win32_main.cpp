
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#if DEVELOPER
#include "imgui/imgui.cpp"
#include "imgui/imgui_impl_win32.cpp"
#include "imgui/imgui_impl_dx11.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#include "imgui/imgui_demo.cpp"
#endif

#define ORH_STATIC
#define ORH_IMPLEMENTATION
#include "orh.h"
#include "orh_d3d11.cpp"

#include "game.h"
#include "game.cpp"

GLOBAL OS_State global_os;
GLOBAL s64      global_performance_frequency;
GLOBAL char     global_exe_full_path[256];
GLOBAL char     global_exe_parent_folder[256];
GLOBAL char     global_data_folder[256];

inline LARGE_INTEGER win32_qpc()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline f64 win32_get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    f64 result = (f64)(end.QuadPart - start.QuadPart) / (f64)global_performance_frequency;
    
    return result;
}

FUNCTION void win32_toggle_fullscreen(HWND window)
{
    LOCAL_PERSIST WINDOWPLACEMENT last_window_placement = {
        sizeof(last_window_placement)
    };
    
    DWORD window_style = GetWindowLong(window, GWL_STYLE);
    if(window_style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitor_info = { sizeof(monitor_info) };
        if(GetWindowPlacement(window, &last_window_placement) &&
           GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY),
                          &monitor_info))
        {
            
            SetWindowLong(window, GWL_STYLE,
                          window_style & ~WS_OVERLAPPEDWINDOW);
            
            SetWindowPos(window, HWND_TOP,
                         monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right -
                         monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom -
                         monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(window, GWL_STYLE,
                      window_style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &last_window_placement);
        SetWindowPos(window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

FUNCTION void win32_free_file_memory(void *memory)
{
    if (memory) {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

FUNCTION String8 win32_read_entire_file(String8 full_path)
{
    String8 result = {};
    
    HANDLE file_handle = CreateFile((char*)full_path.data, GENERIC_READ, FILE_SHARE_READ, 0, 
                                    OPEN_EXISTING, 0, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        print("OS Error: read_entire_file() INVALID_HANDLE_VALUE!\n");
        return result;
    }
    
    LARGE_INTEGER file_size64;
    if (GetFileSizeEx(file_handle, &file_size64) == 0) {
        print("OS Error: read_entire_file() GetFileSizeEx() failed!\n");
    }
    
    u32 file_size32 = (u32)file_size64.QuadPart;
    result.data = (u8 *) VirtualAlloc(0, file_size32, MEM_RESERVE|MEM_COMMIT, 
                                      PAGE_READWRITE);
    
    if (!result.data) {
        print("OS Error: read_entire_file() VirtualAlloc() returned 0!\n");
    }
    
    DWORD bytes_read;
    if (ReadFile(file_handle, result.data, file_size32, &bytes_read, 0) && (file_size32 == bytes_read)) {
        result.count = file_size32;
    } else {
        print("OS Error: read_entire_file() ReadFile() failed!\n");
        
        win32_free_file_memory(result.data);
        result.data = 0;
    }
    
    CloseHandle(file_handle);
    
    return result;
}

FUNCTION b32 win32_write_entire_file(String8 full_path, String8 data)
{
    b32 result = false;
    
    HANDLE file_handle = CreateFile((char*)full_path.data, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (file_handle == INVALID_HANDLE_VALUE) {
        print("OS Error: write_entire_file() INVALID_HANDLE_VALUE!\n");
    }
    
    DWORD bytes_written;
    if (WriteFile(file_handle, data.data, (DWORD)data.count, &bytes_written, 0) && (bytes_written == data.count)) {
        result = true;
    } else {
        print("OS Error: write_entire_file() WriteFile() failed!\n");
    }
    
    CloseHandle(file_handle);
    
    return result;
}

FUNCTION void* win32_reserve(u64 size)
{
    void *memory = VirtualAlloc(0, size, MEM_RESERVE, PAGE_NOACCESS);
    return memory;
}
FUNCTION void  win32_release(void *memory)
{
    VirtualFree(memory, 0, MEM_RELEASE);
}
FUNCTION void  win32_commit(void *memory, u64 size)
{
    VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);
}
FUNCTION void  win32_decommit(void *memory, u64 size)
{
    VirtualFree(memory, size, MEM_DECOMMIT);
}
FUNCTION void win32_print_to_console(String8 text)
{
    OutputDebugStringA((LPCSTR)text.data);
}

FUNCTION void win32_build_paths()
{
    GetModuleFileNameA(0, global_exe_full_path, sizeof(global_exe_full_path));
    
    char *one_past_slash = global_exe_full_path;
    char *exe            = global_exe_full_path;
    while (*exe)  {
        if (*exe++ == '\\') one_past_slash = exe;
    }
    memory_copy(global_exe_parent_folder, global_exe_full_path, one_past_slash - global_exe_full_path);
    
    // @Todo: On release we may need to change what to append.
    //
    string_format(global_data_folder, sizeof(global_data_folder), "%s..\\data\\", global_exe_parent_folder);
}

FUNCTION void win32_process_pending_messages(HWND window)
{
#if DEVELOPER
    ImGuiIO& io = ImGui::GetIO(); (void)io;
#endif
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
#if DEVELOPER
        if (io.WantCaptureKeyboard || io.WantCaptureMouse) {
            TranslateMessage(&message);
            DispatchMessage(&message);
            
            // @Hack: When we hold mouse button off ImGui and transition to ImGui window then release it.
            // Our game will think it's still down. So we'll just copy the mouse state from imgui.
            //
            global_os.pressed[Key_MLEFT]    = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
            global_os.held[Key_MLEFT]       = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            global_os.released[Key_MLEFT]   = ImGui::IsMouseReleased(ImGuiMouseButton_Left);
            
            global_os.pressed[Key_MRIGHT]   = ImGui::IsMouseClicked(ImGuiMouseButton_Right);
            global_os.held[Key_MRIGHT]      = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            global_os.released[Key_MRIGHT]  = ImGui::IsMouseReleased(ImGuiMouseButton_Right);
            
            global_os.pressed[Key_MMIDDLE]  = ImGui::IsMouseClicked(ImGuiMouseButton_Middle);
            global_os.held[Key_MMIDDLE]     = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
            global_os.released[Key_MMIDDLE] = ImGui::IsMouseReleased(ImGuiMouseButton_Middle);
        } else {
#endif
            switch (message.message) {
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP: {
                    s32 vkcode   = (s32) message.wParam;
                    b32 alt_down = (message.lParam & (1 << 29));
                    b32 was_down = (message.lParam & (1 << 30)) == 1;
                    b32 is_down  = (message.lParam & (1 << 31)) == 0;
                    
                    if((vkcode == VK_F4) && alt_down)
                    {
                        global_os.exit = true;
                        return;
                    }
                    
                    s32 key = Key_NONE;
                    if ((vkcode >= 'A') && (vkcode <= 'Z')) {
                        key = Key_A + (vkcode - 'A');
                    } else if ((vkcode >= '0') && (vkcode <= '9')) {
                        key = Key_0 + (vkcode - '0');
                    } else {
                        if      ((vkcode >= VK_F1) && (vkcode <= VK_F12)) key = Key_F1 + (vkcode - VK_F1);
                        else if (vkcode == VK_RETURN)  key = Key_ENTER;
                        else if (vkcode == VK_SHIFT)   key = Key_SHIFT;
                        else if (vkcode == VK_CONTROL) key = Key_CONTROL;
                        else if (vkcode == VK_MENU)    key = Key_ALT;
                        else if (vkcode == VK_ESCAPE)  key = Key_ESCAPE;
                        else if (vkcode == VK_SPACE)   key = Key_SPACE;
                        else if (vkcode == VK_LEFT)    key = Key_LEFT;
                        else if (vkcode == VK_UP)      key = Key_UP;
                        else if (vkcode == VK_RIGHT)   key = Key_RIGHT;
                        else if (vkcode == VK_DOWN)    key = Key_DOWN;
                    }
                    
                    if (key != Key_NONE) {
                        Queued_Input input = {key, is_down};
                        array_add(&global_os.inputs_to_process, input);
                    }
                } break;
                
                case WM_LBUTTONDOWN: {
                    SetCapture(window);
                    Queued_Input input = {Key_MLEFT, (message.wParam & MK_LBUTTON)};
                    array_add(&global_os.inputs_to_process, input);
                } break;
                case WM_LBUTTONUP: {
                    ReleaseCapture();
                    Queued_Input input = {Key_MLEFT, (message.wParam & MK_LBUTTON)};
                    array_add(&global_os.inputs_to_process, input);
                } break;
                
                case WM_RBUTTONDOWN: {
                    SetCapture(window);
                    Queued_Input input = {Key_MRIGHT, (message.wParam & MK_RBUTTON)};
                    array_add(&global_os.inputs_to_process, input);
                } break;
                case WM_RBUTTONUP: {
                    ReleaseCapture();
                    Queued_Input input = {Key_MRIGHT, (message.wParam & MK_RBUTTON)};
                    array_add(&global_os.inputs_to_process, input);
                } break;
                
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP: {
                    Queued_Input input = {Key_MMIDDLE, (message.wParam & MK_MBUTTON)};
                    array_add(&global_os.inputs_to_process, input);
                } break;
                
                case WM_MOUSEWHEEL: {
                    f32 wheel_delta        = (f32) GET_WHEEL_DELTA_WPARAM(message.wParam) / WHEEL_DELTA;
                    global_os.mouse_scroll = v2(0, wheel_delta);
                } break;
                case WM_MOUSEHWHEEL: {
                    f32 wheel_delta        = (f32) GET_WHEEL_DELTA_WPARAM(message.wParam) / WHEEL_DELTA;
                    global_os.mouse_scroll = v2(wheel_delta, 0);
                } break;
                
                default:
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                } break;
            }
#if DEVELOPER
        }
#endif
    }
}

LRESULT CALLBACK win32_main_window_callback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
#if DEVELOPER
    if (ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam))
        return true;
#endif
    
    LRESULT result = 0;
    
    switch (message) {
        case WM_CLOSE: 
        case WM_DESTROY:
        case WM_QUIT: {
            global_os.exit = true;
        } break;
        
        default: {
            result = DefWindowProcA(window, message, wparam, lparam);
        } break;
    }
    
    return result;
}

FUNCTION void win32_process_inputs(HWND window)
{
    // Clear pressed and released states.
    MEMORY_ZERO_ARRAY(global_os.pressed);
    MEMORY_ZERO_ARRAY(global_os.released);
    
    // Mouse position.
    POINT cursor; 
    GetCursorPos(&cursor);
    ScreenToClient(window, &cursor);
    V2 mouse_client = {
        (f32) cursor.x,
        ((f32)global_os.window_size.height - 1.0f) - cursor.y,
    };
    global_os.mouse_ndc = {
        2.0f*CLAMP01_RANGE(global_os.drawing_rect.min.x, mouse_client.x, global_os.drawing_rect.max.x) - 1.0f,
        2.0f*CLAMP01_RANGE(global_os.drawing_rect.min.y, mouse_client.y, global_os.drawing_rect.max.y) - 1.0f,
        0.0f
    };
    
    // Clear mouse-wheel scroll.
    global_os.mouse_scroll = {};
    
    // Put input messages in queue.
    //
    win32_process_pending_messages(window);
    
#if DEVELOPER
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.WantCaptureKeyboard || io.WantCaptureMouse)
        return;
#endif
    
    // Process queued inputs.
    //
    for (s32 i = 0; i < global_os.inputs_to_process.count; i++) {
        Queued_Input input = global_os.inputs_to_process[i];
        if (input.down) {
            
            // Key already pressed, defer this press for later to preserve input order.
            if (global_os.pressed[input.key])
                break;
            // Key was marked as released before we got here, so defer for later.
            else if (global_os.released[input.key])
                break;
            else {
                if (!global_os.held[input.key])
                    global_os.pressed[input.key] = true;
                
                global_os.held[input.key] = true;
                array_ordered_remove_by_index(&global_os.inputs_to_process, i);
                i--;
            }
        } else {
            global_os.released[input.key] = true;
            global_os.held[input.key] = false;
            array_ordered_remove_by_index(&global_os.inputs_to_process, i);
            i--;
        }
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance,
                   LPSTR command_line, int show_code)
{
    LARGE_INTEGER performance_frequency;
    QueryPerformanceFrequency(&performance_frequency);
    global_performance_frequency = performance_frequency.QuadPart;
    win32_build_paths();
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
    //
    // Register window_class.
    WNDCLASS window_class = {};
    
    window_class.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc   = win32_main_window_callback;
    window_class.hInstance     = instance;
    window_class.hCursor       = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    //window_class.hIcon;
    window_class.lpszClassName = "MainWindowClass";
    
    if (!RegisterClassA(&window_class)) {
        OutputDebugStringA("OS Error: WinMain() RegisterClassA() failed!\n");
        MessageBox(0, "Couldn't register window class.\n", 0, 0);
        return 0;
    }
    
    //
    // Create window.
    HWND window = CreateWindowEx(0, //WS_EX_TOPMOST,
                                 window_class.lpszClassName,
                                 "NUR",
                                 WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 1600,
                                 900,
                                 0,
                                 0,
                                 instance,
                                 0);
    
    if (!window) {
        OutputDebugStringA("OS Error: WinMain() CreateWindowEx() failed!\n");
        MessageBox(0, "Couldn't create window.\n", 0, 0);
        return 0;
    }
    
    ShowWindow(window, show_code);
#if DEVELOPER
    ShowCursor(true);
#else
    // @Todo: Show cursor when we hover on window borders.
    ShowCursor(false);
#endif
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
    //
    // Initialize os.
    {
        os = &global_os;
        
        // Meta-data.
        global_os.exe_full_path     = string(global_exe_full_path);
        global_os.exe_parent_folder = string(global_exe_parent_folder);
        global_os.data_folder       = string(global_data_folder);
        
        // Options.
#if DEVELOPER
        global_os.fullscreen        = false;
#else
        global_os.fullscreen        = true;
#endif
        global_os.exit              = false;
        global_os.vsync             = false;
        global_os.fix_aspect_ratio  = true;
        global_os.render_size       = {1920, 1080};
        global_os.dt                = 1.0f/144.0f;
        global_os.time              = 0.0f;
        
        // Functions.
        global_os.reserve           = win32_reserve;
        global_os.release           = win32_release;
        global_os.commit            = win32_commit;
        global_os.decommit          = win32_decommit;
        global_os.print_to_console  = win32_print_to_console;
        global_os.read_entire_file  = win32_read_entire_file;
        global_os.write_entire_file = win32_write_entire_file;
        global_os.free_file_memory  = win32_free_file_memory;
        
        // Arenas.
        global_os.permanent_arena  = arena_init();
        
        // User Input.
        array_init(&global_os.inputs_to_process);
    }
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
    d3d11_init(window);
    
#if DEVELOPER
    //
    // Setup Dear ImGui context and renderer.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 1.0f;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device, device_context);
#endif
    
    game_init();
    
    if (global_os.fullscreen) 
        win32_toggle_fullscreen(window);
    
    LARGE_INTEGER last_counter = win32_qpc();
    f64 accumulator = 0.0;
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
    while (!global_os.exit) {
        //
        //
        // Update drawing region.
        //
        //
        RECT rect;
        GetClientRect(window, &rect);
        V2u window_size    = {(u32)(rect.right - rect.left), (u32)(rect.bottom - rect.top)};
        Rect2 drawing_rect;
        if (global_os.fix_aspect_ratio) {
            drawing_rect = aspect_ratio_fit(global_os.render_size, window_size);
        } else {
            drawing_rect.min = {0, 0};
            drawing_rect.max = {(f32)window_size.x, (f32)window_size.y};
        }
        
        global_os.window_size  = window_size;
        global_os.drawing_rect = drawing_rect;
        
#if DEVELOPER
        // Start the Dear ImGui frame only if window size is non-zero.
        if ((window_size.x != 0) && (window_size.y != 0)) {
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
        }
#endif
        //
        //
        // Process Inputs --> Update --> Render.
        //
        //
        u32 num_updates_this_frame = 0;
        while (accumulator >= os->dt) {
            b32 last_fullscreen = global_os.fullscreen;
            win32_process_inputs(window);
            game_update();
            accumulator -= os->dt;
            os->time    += os->dt;
            if (last_fullscreen != global_os.fullscreen)
                win32_toggle_fullscreen(window);
            
            num_updates_this_frame++;
        }
        
        // Render only if window size is non-zero.
        if ((window_size.x != 0) && (window_size.y != 0)) {
            d3d11_viewport(drawing_rect.min.x, drawing_rect.min.y, 
                           drawing_rect.max.x - drawing_rect.min.x, 
                           drawing_rect.max.y - drawing_rect.min.y);
            d3d11_clear(0.20f, 0.20f, 0.20f, 1.0f);
            game_render();
            
#if DEVELOPER
            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
            
            HRESULT hr = d3d11_present(global_os.vsync);
        }
        
        //
        //
        // Frame end.
        //
        //
        LARGE_INTEGER end_counter     = win32_qpc();
        f64 seconds_elapsed_for_frame = win32_get_seconds_elapsed(last_counter, end_counter);
        last_counter                  = end_counter;
        accumulator                  += seconds_elapsed_for_frame;
        print("Num updates this frame: %d\n", num_updates_this_frame);
    }
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
#if DEVELOPER
    //
    // Cleanup.
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif
    
    return 0;
}