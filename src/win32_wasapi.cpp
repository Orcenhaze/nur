
// @Note: From Allen "mr4th" Webster.

// @Todo: Re-write this and learn from Martins' WASAPI example
// @Todo: Re-write this and learn from Martins' WASAPI example
// @Todo: Re-write this and learn from Martins' WASAPI example

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <audiopolicy.h>

struct Win32_Sound_Output
{
    b32 initialized;
    
    IMMDeviceEnumerator *mmdevice_enum;
    IMMDevice           *mmdevice;
    IAudioClient        *audio_client;
    IAudioRenderClient  *audio_render_client;
    
    REFERENCE_TIME sound_buffer_duration_100ns; // Each unit is 100ns.
    u32            sound_buffer_duration;       // In seconds.
    
    u32 buffer_frame_count;     // Takes into account requested duration.
    u32 samples_per_second;     // sample/frame count in one second regardless of requested duration.
    u32 channels;
    u32 latency_frame_count;
};

#pragma warning(push)
#pragma warning(disable:4211)
static const GUID IID_IAudioClient         = {0x1CB9AD4C, 0xDBFA, 0x4c32, 0xB1, 0x78, 0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2};
static const GUID IID_IAudioRenderClient   = {0xF294ACFC, 0x3146, 0x4483, 0xA7, 0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2};
static const GUID CLSID_MMDeviceEnumerator = {0xBCDE0395, 0xE52F, 0x467C, 0x8E, 0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E};
static const GUID IID_IMMDeviceEnumerator  = {0xA95664D2, 0x9614, 0x4F35, 0xA7, 0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6};
static const GUID PcmSubformatGuid         = {STATIC_KSDATAFORMAT_SUBTYPE_PCM};
#pragma warning(pop)

#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif

#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
#define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY 0x08000000
#endif

#define SOUND_LATENCY_FPS 10
#define REFTIMES_PER_SEC 10000000

//
// ole32.dll
//
typedef HRESULT CoCreateInstance_T(REFCLSID  rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv);
typedef HRESULT CoInitializeEx_T(LPVOID pvReserved, DWORD  dwCoInit);
FUNCTION HRESULT CoCreateInstanceStub(REFCLSID  rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
    return E_NOTIMPL;
}
FUNCTION HRESULT CoInitializeExStub(LPVOID pvReserved, DWORD  dwCoInit)
{
    return E_NOTIMPL;
}

GLOBAL CoCreateInstance_T *CoCreateInstanceProc = CoCreateInstanceStub;
GLOBAL CoInitializeEx_T *CoInitializeExProc     = CoInitializeExStub;

//
// Functions.
//
FUNCTION void win32_wasapi_load()
{
    HMODULE ole_lib = LoadLibraryA("ole32.dll");
    if (ole_lib) {
        CoCreateInstanceProc = (CoCreateInstance_T *) GetProcAddress(ole_lib, "CoCreateInstance");
        CoInitializeExProc   = (CoInitializeEx_T *) GetProcAddress(ole_lib, "CoInitializeEx");
    }
}

FUNCTION void win32_wasapi_init(Win32_Sound_Output *output)
{
    CoInitializeExProc(0, COINIT_SPEED_OVER_MEMORY);
    
    HRESULT hr = CoCreateInstanceProc(CLSID_MMDeviceEnumerator, 
                                      0, 
                                      CLSCTX_ALL, 
                                      IID_IMMDeviceEnumerator, 
                                      (LPVOID *)(&output->mmdevice_enum));
    if (FAILED(hr)) {
        win32_messagebox("WASAPI Error", "Could not retrieve multi-media device enumerator.");
    }
    
    hr = output->mmdevice_enum->GetDefaultAudioEndpoint(eRender, eConsole, &output->mmdevice);
    if (FAILED(hr)) {
        win32_messagebox("WASAPI Error", "Default audio endpoint was not found.");
    }
    
    hr = output->mmdevice->Activate(IID_IAudioClient, 
                                    CLSCTX_ALL,
                                    0,
                                    (void **) &output->audio_client);
    if (FAILED(hr)) {
        win32_messagebox("WASAPI Error", "Could not activate audio device.");
    }
    
    // @Note: 
    // An audio frame in a PCM stream is a set of samples. The set contains one sample for each
    // channel in the stream.
    //
    // So the size of one audio frame in bytes is the size of one sample multiplied by number of 
    // channels.
    //
    // Number of frames per second is same as number of samples per second, only that each frame
    // will contain samples from all the channels. 
    //
    
    // @Todo: GetDevicePeriod()!!!
    REFERENCE_TIME requested_sound_duration_100ns = REFTIMES_PER_SEC * 1;
    
    output->samples_per_second     = 44100;
    output->latency_frame_count    = output->samples_per_second / SOUND_LATENCY_FPS;
    WORD bits_per_sample           = sizeof(s16)*8;
    WORD block_align               = (WORD)(output->channels * bits_per_sample/8);
    DWORD average_bytes_per_second = block_align * output->samples_per_second;
    WAVEFORMATEX new_wave_format   = {
        WAVE_FORMAT_PCM,
        (WORD)output->channels,     // Two for stereo.
        output->samples_per_second, // Same as frames per second.
        average_bytes_per_second,   // This is in frames (takes channels into account).
        block_align,                // This specifies the size of _one_ audio frame. 
        bits_per_sample,
        0,
    };
    hr = output->audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 
                                          AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
                                          requested_sound_duration_100ns,
                                          0,
                                          &new_wave_format,
                                          0);
    if (FAILED(hr)) {
        // @Todo: Simply print instead of doing message box, we don't need to crash or anything!
        win32_messagebox("WASAPI Error", "Audio client initialization failed.");
    }
    
    hr = output->audio_client->GetService(IID_IAudioRenderClient, 
                                          (void **) &output->audio_render_client);
    if (FAILED(hr)) {
        // @Todo: Simply print instead of doing message box, we don't need to crash or anything!
        win32_messagebox("WASAPI Error", "Could not retrieve audio render client.");
    }
    
    output->audio_client->GetBufferSize(&output->buffer_frame_count);
    
    output->sound_buffer_duration       = output->buffer_frame_count / output->samples_per_second;
    output->sound_buffer_duration_100ns = REFTIMES_PER_SEC * output->sound_buffer_duration;
    output->audio_client->Start();
    
    output->initialized = 1;
}

FUNCTION void win32_wasapi_release(Win32_Sound_Output *output)
{
    if (output->initialized) {
        output->audio_client->Stop();
        
        output->mmdevice_enum->Release();
        output->mmdevice->Release();
        output->audio_client->Release();
        output->audio_render_client->Release();
    }
}

FUNCTION s16 convert_f32_sample_to_s16(f32 sample)
{
    // Input sample is in range [-1.0f, 1.0f]; scale it to the range [0, 65535]. (65535 is U16_MAX).
    f32 scaled_sample = (sample + 1.0f) * 32767.0f; // (32767 is S16_MAX);
    
    u16 u16_sample;
    if (scaled_sample < 0.0f)
        u16_sample = 0;
    else if (scaled_sample > 65535.0f)
        u16_sample = U16_MAX;
    else
        u16_sample = (u16)scaled_sample;
    
    // Convert from range [0, 65535] to [-32768, 32767].
    s16 result = u16_sample - S16_MAX;
    return result;
}

FUNCTION void win32_fill_sound_buffer(Win32_Sound_Output *output, u32 frames_to_write, f32 *samples)
{
    if (frames_to_write) {
        BYTE *data = 0;
        HRESULT hr = output->audio_render_client->GetBuffer(frames_to_write, &data);
        
        if (data) {
            s16 *dst = (s16 *)data;
            f32 *src = samples;
            for (u32 i = 0; i < frames_to_write; i++) {
                s16 left  = convert_f32_sample_to_s16(*src++);
                s16 right = convert_f32_sample_to_s16(*src++);
                *dst++ = left;
                *dst++ = right;
            }
        }
        
        output->audio_render_client->ReleaseBuffer(frames_to_write, 0);
    }
}