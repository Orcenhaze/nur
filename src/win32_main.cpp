
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "win32_wasapi.h"
#include "win32_xinput.h"

// Tell AMD and Nvidia drivers to use the most powerful GPU instead of a lower-performance (such as integrated) GPU.
// App should use either DirectX or OpenGL for this to work.
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement                  = 0x00000001;
    __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001;
}

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

// For loading ogg vorbis audio files.
#pragma warning(push, 3)
#pragma warning(disable: 4701)
#include "stb/stb_vorbis.c"
#pragma warning(pop)

GLOBAL OS_State global_os;
GLOBAL s64      global_performance_frequency;
GLOBAL HANDLE   global_waitable_timer;
GLOBAL char     global_exe_full_path[256];
GLOBAL char     global_exe_parent_folder[256];
GLOBAL char     global_data_folder[256];

FUNCTION void win32_fatal_error(char *message)
{
    MessageBoxA(0, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

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

FUNCTION Sound win32_sound_load(String8 full_path, u32 sample_rate)
{
    String8 file = win32_read_entire_file(full_path);
    if (!file.data) {
        print("Couldn't load sound file %S\n", full_path);
        
        Sound dummy = {};
        return dummy;
    }
    
    s16 *out;
    s32 chan, samplerate;
    s32 count = stb_vorbis_decode_memory(file.data, (s32)file.count, &chan, &samplerate, &out);
    
    if (count == -1) {
        print("STB Error: Couldn't load sound file %S\n", full_path);
        
        Sound dummy = {};
        return dummy;
    }
    
    // @Note: Make sure we are loading mono audio with specified sample rate.
    ASSERT((chan == 1) && ((u32)samplerate == sample_rate));
    
    Sound result;
    result.samples = out;
    result.count   = result.pos = (u32)count;
    
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
FUNCTION b32  win32_commit(void *memory, u64 size)
{
    b32 result = (VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE) != 0);
    return result;
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
    MEMORY_COPY(global_exe_parent_folder, global_exe_full_path, one_past_slash - global_exe_full_path);
    
    // @Note: We will copy the data folder when building the game and put it next to .exe.
    string_format(global_data_folder, sizeof(global_data_folder), "%sdata/", global_exe_parent_folder);
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
                        if     ((vkcode >= VK_F1) && (vkcode <= VK_F12)) key = Key_F1 + (vkcode - VK_F1);
                        else if (vkcode == VK_ESCAPE)  key = Key_ESCAPE;
                        else if (vkcode == VK_BACK)    key = Key_BACKSPACE;
                        else if (vkcode == VK_TAB)     key = Key_TAB;
                        else if (vkcode == VK_RETURN)  key = Key_ENTER;
                        else if (vkcode == VK_SHIFT)   key = Key_SHIFT;
                        else if (vkcode == VK_CONTROL) key = Key_CONTROL;
                        else if (vkcode == VK_MENU)    key = Key_ALT;
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

FUNCTION LRESULT CALLBACK win32_wndproc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
#if DEVELOPER
    if (ImGui_ImplWin32_WndProcHandler(window, message, wparam, lparam))
        return true;
#endif
    
    LRESULT result = 0;
    
    switch (message) {
        case WM_ACTIVATE:
        case WM_ACTIVATEAPP: {
            // Clear all key states.
            MEMORY_ZERO_ARRAY(global_os.pressed);
            MEMORY_ZERO_ARRAY(global_os.held);
            MEMORY_ZERO_ARRAY(global_os.released);
        } break;
        
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
    // Mouse position.
    //
    POINT cursor; 
    GetCursorPos(&cursor);
    ScreenToClient(window, &cursor);
    V2 mouse_client = {
        (f32) cursor.x,
        ((f32)global_os.window_size.h - 1.0f) - cursor.y,
    };
    global_os.mouse_ndc = {
        2.0f*CLAMP01_RANGE(global_os.drawing_rect.min.x, mouse_client.x, global_os.drawing_rect.max.x) - 1.0f,
        2.0f*CLAMP01_RANGE(global_os.drawing_rect.min.y, mouse_client.y, global_os.drawing_rect.max.y) - 1.0f,
        0.0f
    };
    
    // Clear mouse-wheel scroll.
    //
    global_os.mouse_scroll = {};
    
    // Put input messages in queue + process mousewheel and such.
    //
    win32_process_pending_messages(window);
    
#if DEVELOPER
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.WantCaptureKeyboard || io.WantCaptureMouse)
        return;
#endif
    
    // Clear pressed and released states.
    //
    MEMORY_ZERO_ARRAY(global_os.pressed);
    MEMORY_ZERO_ARRAY(global_os.released);
    
    // Process queued inputs.
    //
    for (s32 i = 0; i < global_os.inputs_to_process.count; i++) {
        Queued_Input input = global_os.inputs_to_process[i];
        if (input.down) {
            // Defer DOWN event because UP was true, break to preserve order.
            if (global_os.released[input.key])
                break;
            // Defer DOWN event because DOWN is already true, break to preserve order.
            else if (global_os.pressed[input.key])
                break;
            else {
                if (!global_os.held[input.key])
                    global_os.pressed[input.key] = true;
                
                global_os.held[input.key] = true;
                array_ordered_remove_by_index(&global_os.inputs_to_process, i);
                i--;
            }
        } else {
            // Defer UP event because DOWN was true, break to preserve order.
            if (global_os.pressed[input.key])
                break;
            // Defer UP event because UP is already true, break to preserve order.
            else if (global_os.released[input.key])
                break;
            else {
                global_os.released[input.key] = true;
                global_os.held[input.key] = false;
                array_ordered_remove_by_index(&global_os.inputs_to_process, i);
                i--;
            }
        }
    }
    
    // Do same thing for gamepads... Get gamepad state, then encode button states how we want.
    //
    for (s32 i = 0; i < GAMEPADS_MAX; i++) {
        XInput_Gamepad xinput_pad = xinput_get_gamepad_state(i);
        if (!xinput_pad.connected)
            continue;
        
        Gamepad *pad       = &global_os.gamepads[i];
        pad->connected     = xinput_pad.connected;
        pad->left_stick    = {xinput_pad.stick_left_x, xinput_pad.stick_left_y};
        pad->right_stick   = {xinput_pad.stick_right_x, xinput_pad.stick_right_y};
        pad->left_trigger  = xinput_pad.trigger_left;
        pad->right_trigger = xinput_pad.trigger_right;
        
        // Clear pressed and released states.
        //
        MEMORY_ZERO_ARRAY(pad->pressed);
        MEMORY_ZERO_ARRAY(pad->released);
        
        // Process xinput button states and encode them the way we want.
        //
        for (s32 j = 1; j < XInputButton_COUNT; j++) {
            s32 button = j;
            b32 down   = xinput_pad.button_states[button];
            if (down) {
                if (!pad->held[button])
                    pad->pressed[button] = true;
                
                pad->held[button] = true;
            } else {
                if (pad->held[button])
                    pad->released[button] = true;
                
                pad->held[button] = false;
            }
            
#if 0
            // Test
            //
            //print("StickL(%v2)\n", pad->left_stick);
            //print("StickR(%v2)\n", pad->stick_right);
            //print("TriggerL(%f)\n", pad->trigger_left);
            //print("TriggerR(%f)\n", pad->trigger_right);
            
            if (pad->pressed[button])
                print("Pressed(%s)\n",  xinput_get_button_name((XInput_Button)button));
            if (pad->held[button])
                print("held(%s)\n",     xinput_get_button_name((XInput_Button)button));
            if (pad->released[button])
                print("released(%s)\n", xinput_get_button_name((XInput_Button)button));
#endif
        }
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance,
                   LPSTR command_line, int show_code)
{
    LARGE_INTEGER performance_frequency;
    QueryPerformanceFrequency(&performance_frequency);
    global_performance_frequency = performance_frequency.QuadPart;
    
    global_waitable_timer = CreateWaitableTimerExW(0, 0, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    ASSERT(global_waitable_timer != 0);
    
    win32_build_paths();
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
    //
    // Register window_class.
    WNDCLASS window_class = {};
    
    window_class.style         = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    window_class.lpfnWndProc   = win32_wndproc;
    window_class.hInstance     = instance;
    window_class.hCursor       = LoadCursor(0, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    //window_class.hIcon;
    window_class.lpszClassName = "MainWindowClass";
    
    if (!RegisterClassA(&window_class)) {
        win32_fatal_error("Could not register window class.");
    }
    
    //
    // Create window.
    HWND window = CreateWindowEx(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP, //WS_EX_TOPMOST,
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
        win32_fatal_error("Window creation failed.");
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
    // Initialize sound.
    Wasapi_Audio audio;
    wasapi_start(&audio, 48000, 2, SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT);
    u32 sample_rate      = audio.buffer_format->nSamplesPerSec;
    u32 bytes_per_sample = audio.buffer_format->nBlockAlign;
    
    //
    // Initialize XInput.
    xinput_init();
    
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
        win32_toggle_fullscreen(window);
#endif
        global_os.exit              = false;
        
        // @Note: I think this value is useless when using FLIP presentation model.
        // If user has vsync in control panel set to "use 3D application setting", we will always
        // limit fps to refresh rate, regardless of this value. Vsync is enforced on windowed and 
        // possibly also borderless fullscreen.
        // If the user has vsync in control panel set to "off", we will limit the FPS ourselves
        // using the fps_max variable.
        // For now, let's leave vsync here set to false.
        global_os.vsync             = false;
        global_os.fix_aspect_ratio  = true;
        global_os.render_size       = {1920, 1080};
        global_os.dt                = 1.0f/120.0f;
        global_os.fps_max           = 120;
        
        // @Note: Force fps_max to primary monitor refresh rate if possible.
        s32 refresh_hz = GetDeviceCaps(GetDC(window), VREFRESH);
        if (refresh_hz > 1)
            global_os.fps_max = refresh_hz;
        
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
        global_os.sound_load        = win32_sound_load;
        
        // Arenas.
        global_os.permanent_arena  = arena_init();
        
        // User Input.
        array_init_static(&global_os.inputs_to_process, 512);
        
        // Audio Output.
        global_os.sample_rate        = sample_rate;
        global_os.bytes_per_sample   = bytes_per_sample;
        global_os.samples_out        = 0;
        global_os.samples_to_write   = 0;
        global_os.samples_to_advance = 0;
    }
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
    //
    // Initialize D3D11 and game.
    
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
    
    b32 fullscreen_before = global_os.fullscreen;
    game_init();
    if (fullscreen_before != global_os.fullscreen) 
        win32_toggle_fullscreen(window);
    
    LARGE_INTEGER last_counter = win32_qpc();
    f64 accumulator = 0.0;
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
    //
    // Game loop.
    
    while (!global_os.exit) {
        //
        //
        // Update drawing region.
        //
        //
        RECT rect;
        GetClientRect(window, &rect);
        V2u window_size = {(u32)(rect.right - rect.left), (u32)(rect.bottom - rect.top)};
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
        // Process Inputs --> Update.
        //
        //
        //u32 num_updates_this_frame = 0;
        while (accumulator >= os->dt) {
            b32 last_fullscreen = global_os.fullscreen;
            win32_process_inputs(window);
            game_update();
            if (last_fullscreen != global_os.fullscreen)
                win32_toggle_fullscreen(window);
            
            accumulator -= os->dt;
            os->time    += os->dt;
            //num_updates_this_frame++;
        }
        
        //
        //
        // Update audio.
        //
        //
        // Write at least 100msec of samples into buffer (or whatever space available, whichever is smaller);
        // this is max amount of time you expect code will take until the next iteration of loop.
        // If code will take more time then you'll hear discontinuity as buffer will be filled with silence.
        // Alternatively, you can write as much as "audio.sample_count" to fully fill the buffer (~1 second),
        // then you can try to increase delay below to 900+ msec, it still should sound fine.
        //samples_to_write = audio.sample_count;
        wasapi_lock_buffer(&audio);
        global_os.samples_out        = (f32 *)audio.sample_buffer;
        global_os.samples_to_write   = (u32)MIN(sample_rate/10, audio.sample_count);
        global_os.samples_to_advance = (u32)audio.num_samples_submitted;
        MEMORY_ZERO(global_os.samples_out, global_os.samples_to_write * bytes_per_sample);
        game_fill_sound_buffer();
        wasapi_unlock_buffer(&audio, global_os.samples_to_write);
        
        //
        //
        // Render (only if window size is non-zero).
        //
        //
        if ((window_size.x != 0) && (window_size.y != 0)) {
            d3d11_wait_on_swapchain();
            d3d11_viewport(drawing_rect.min.x, 
                           drawing_rect.min.y, 
                           get_width(drawing_rect), 
                           get_height(drawing_rect));
            //d3d11_clear(0.20f, 0.20f, 0.20f, 1.0f);
            d3d11_clear(0.0097f, 0.043f, 0.082f, 1.0f);
            game_render();
#if DEVELOPER
            ImGui::Render();
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
            HRESULT hr = d3d11_present(global_os.vsync);
        }
        
        //
        //
        // Framerate limiter (If vsync is not working).
        //
        //
        if ((global_os.fps_max > 0)) {
            f64 target_seconds_per_frame = 1.0/(f64)global_os.fps_max;
            f64 seconds_elapsed_so_far = win32_get_seconds_elapsed(last_counter, win32_qpc());
            if (seconds_elapsed_so_far < target_seconds_per_frame) {
                if (global_waitable_timer) {
                    f64 us_to_sleep = (target_seconds_per_frame - seconds_elapsed_so_far) * 1000000.0;
                    us_to_sleep    -= 1000.0; // -= 1ms;
                    if (us_to_sleep > 1) {
                        LARGE_INTEGER due_time;
                        due_time.QuadPart = -(s64)us_to_sleep * 10; // *10 because 100 ns intervals.
                        b32 set_ok = SetWaitableTimerEx(global_waitable_timer, &due_time, 0, 0, 0, 0, 0);
                        ASSERT(set_ok != 0);
                        
                        WaitForSingleObjectEx(global_waitable_timer, INFINITE, 0);
                    }
                } else {
                    // @Todo: Must use other methods of sleeping for older versions of Windows that don't have high resolution waitable timer.
                }
                
                // Spin-lock the remaining amount.
                while (seconds_elapsed_so_far < target_seconds_per_frame)
                    seconds_elapsed_so_far = win32_get_seconds_elapsed(last_counter, win32_qpc());
            }
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
        //print("Num updates this frame: %d\n", num_updates_this_frame);
        //print("FPS: %d\n", (s32)(1.0/seconds_elapsed_for_frame));
    }
    
    /////////////////////////////////////////////////////
    /////////////////////////////////////////////////////
    
    //
    // Cleanup.
    
#if DEVELOPER
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
#endif
    
    wasapi_stop(&audio);
    
    return 0;
}