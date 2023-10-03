#ifndef WIN32_XINPUT_H
#define WIN32_XINPUT_H

#include <xinput.h>
#include <stdint.h>

enum XInput_Button
{
    XInputButton_NONE,
    
    XInputButton_DPAD_UP,
    XInputButton_DPAD_DOWN,
    XInputButton_DPAD_LEFT,
    XInputButton_DPAD_RIGHT,
    XInputButton_START,
    XInputButton_BACK,
    XInputButton_LEFT_THUMB,
    XInputButton_RIGHT_THUMB,
    XInputButton_LEFT_SHOULDER,
    XInputButton_RIGHT_SHOULDER,
    XInputButton_A,
    XInputButton_B,
    XInputButton_X,
    XInputButton_Y,
    
    XInputButton_COUNT,
};

struct XInput_Gamepad
{
    int32_t connected;
    
    // Analog direction vectors in range [-1, 1]
    float   stick_left_x;
    float   stick_left_y;
    float   stick_right_x;
    float   stick_right_y;
    
    // Analog in range [0, 1]
    float   trigger_left;
    float   trigger_right;
    
    // Down or not.
    int32_t button_states[XInputButton_COUNT];
};

//
// @Todo: Better error handling.
//
typedef DWORD XInputGetState_T(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef DWORD XInputSetState_T(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);

static DWORD XInputGetState__stub(DWORD dwUserIndex, XINPUT_STATE *pState)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
static DWORD XInputSetState__stub(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
static XInputGetState_T *XInputGetStateProc;
static XInputSetState_T *XInputSetStateProc;

static float xinput_normalize_stick_value(SHORT value, SHORT deadzone)
{
    float result = 0.0f;
    
    if (value < -deadzone)
        result = (float)((value + deadzone) / (32768.f - deadzone));
    else if (value > deadzone)
        result = (float)((value - deadzone) / (32768.f - deadzone));
    
    return result;
}

static float xinput_normalize_trigger_value(BYTE value, BYTE threshold)
{
    float result = 0.0f;
    
    if (value > threshold)
        result = (float)((value - threshold) / (255.f - threshold));
    
    return result;
}

static void xinput_init()
{
    XInputGetStateProc = XInputGetState__stub;
    XInputSetStateProc = XInputSetState__stub;
    
    HMODULE xinput_dll = LoadLibraryA("xinput1_4.dll");
    if (!xinput_dll)
        xinput_dll = LoadLibraryA("xinput9_1_0.dll");
    if (!xinput_dll)
        xinput_dll = LoadLibraryA("xinput1_3.dll");
    if (!xinput_dll) {
        OutputDebugStringA("XInput Error: Could not load xinput.dll\n");
        return;
    }
    
    XInputGetStateProc = (XInputGetState_T *) GetProcAddress(xinput_dll, "XInputGetState");
    XInputSetStateProc = (XInputSetState_T *) GetProcAddress(xinput_dll, "XInputSetState");
}

static XInput_Gamepad xinput_get_gamepad_state(DWORD gamepad_index)
{
    XInput_Gamepad result = {};
    
    if (gamepad_index >= XUSER_MAX_COUNT)
        return result;
    
    XINPUT_STATE controller_state = {};
    XINPUT_GAMEPAD *pad           = &controller_state.Gamepad;
    if(XInputGetStateProc(gamepad_index, &controller_state) != ERROR_SUCCESS)
        return result;
    
    result.connected = TRUE;
    
    result.button_states[XInputButton_DPAD_UP]        |= !!(pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
    result.button_states[XInputButton_DPAD_DOWN]      |= !!(pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
    result.button_states[XInputButton_DPAD_LEFT]      |= !!(pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
    result.button_states[XInputButton_DPAD_RIGHT]     |= !!(pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
    result.button_states[XInputButton_START]          |= !!(pad->wButtons & XINPUT_GAMEPAD_START);
    result.button_states[XInputButton_BACK]           |= !!(pad->wButtons & XINPUT_GAMEPAD_BACK);
    result.button_states[XInputButton_LEFT_THUMB]     |= !!(pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
    result.button_states[XInputButton_RIGHT_THUMB]    |= !!(pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
    result.button_states[XInputButton_LEFT_SHOULDER]  |= !!(pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
    result.button_states[XInputButton_RIGHT_SHOULDER] |= !!(pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
    result.button_states[XInputButton_A]              |= !!(pad->wButtons & XINPUT_GAMEPAD_A);
    result.button_states[XInputButton_B]              |= !!(pad->wButtons & XINPUT_GAMEPAD_B);
    result.button_states[XInputButton_X]              |= !!(pad->wButtons & XINPUT_GAMEPAD_X);
    result.button_states[XInputButton_Y]              |= !!(pad->wButtons & XINPUT_GAMEPAD_Y);
    
    
    SHORT deadzone       = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
    result.stick_left_x  = xinput_normalize_stick_value(pad->sThumbLX, deadzone);
    result.stick_left_y  = xinput_normalize_stick_value(pad->sThumbLY, deadzone);
    deadzone             = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
    result.stick_right_x = xinput_normalize_stick_value(pad->sThumbRX, deadzone);
    result.stick_right_y = xinput_normalize_stick_value(pad->sThumbRY, deadzone);
    
    BYTE threshold       = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    result.trigger_left  = xinput_normalize_trigger_value(pad->bLeftTrigger, threshold);
    result.trigger_right = xinput_normalize_trigger_value(pad->bRightTrigger, threshold);
    
    return result;
}

static char* xinput_get_button_name(XInput_Button button)
{
    char *result = 0;
    
    switch (button) {
        case XInputButton_DPAD_UP:        { result = "DPAD_UP"; } break;
        case XInputButton_DPAD_DOWN:      { result = "DPAD_DOWN"; } break;
        case XInputButton_DPAD_LEFT:      { result = "DPAD_LEFT"; } break;
        case XInputButton_DPAD_RIGHT:     { result = "DPAD_RIGHT"; } break;
        case XInputButton_START:          { result = "START"; } break;
        case XInputButton_BACK:           { result = "BACK"; } break;
        case XInputButton_LEFT_THUMB:     { result = "THUMB_L"; } break;
        case XInputButton_RIGHT_THUMB:    { result = "THUMB_R"; } break;
        case XInputButton_LEFT_SHOULDER:  { result = "SHOULDER_L"; } break;
        case XInputButton_RIGHT_SHOULDER: { result = "SHOULDER_R"; } break;
        case XInputButton_A:              { result = "A"; } break;
        case XInputButton_B:              { result = "B"; } break;
        case XInputButton_X:              { result = "X"; } break;
        case XInputButton_Y:              { result = "Y"; } break;
        
        default: { result = "UNKNOWN_BUTTON"; } break;
    }
    
    return result;
}

#endif //WIN32_XINPUT_H
