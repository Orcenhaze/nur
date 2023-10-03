#ifndef WIN32_WASAPI_H
#define WIN32_WASAPI_H

// @Note: From Martins: 
// https://gist.github.com/mmozeiko/5a5b168e61aff4c1eaec0381da62808f#file-win32_wasapi-h
//

#define WIN32_LEAN_AND_MEAN
#include <initguid.h>
#include <windows.h>
#include <objbase.h>
#include <uuids.h>
#include <avrt.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#include <stddef.h>

// "count" means sample count (for example, 1 sample = 2 floats for stereo)
// "offset" or "size" means byte count

struct Wasapi_Audio
{
	// Public
    //
	
	// Describes sample_buffer format.
	WAVEFORMATEX* buffer_format;
    
	// Use these values only between lock_buffer/unlock_buffer calls.
	void  *sample_buffer;         // ringbuffer for interleaved samples, no need to handle wrapping.
	size_t sample_count;          // How big is ringbuffer in samples.
	size_t num_samples_submitted; // How many samples were actually used for playback since previous lock_buffer call
    
	// Private
    //
	IAudioClient *client;
	
    HANDLE event;
	HANDLE thread;
	LONG   stop;
	LONG   lock;
	
    BYTE  *buffer1;
	BYTE  *buffer2;
	UINT32 out_buffer_size;      // output buffer size in bytes
	UINT32 ringbuffer_size;      // ringbuffer size, always power of 2
	UINT32 buffer_used;          // how many samples are used from buffer
	BOOL   is_buffer_first_lock; // true when BufferLock is used at least once
	
    volatile LONG ringbuffer_read_offset;  // offset to read from buffer
	volatile LONG ringbuffer_lock_offset;  // offset up to what buffer is currently being used
	volatile LONG ringbuffer_write_offset; // offset up to what buffer is filled
};

//
// Interface.
//

// Pass 0 for rate/count/mask to get default format of output device (use audio->buffer_format)
// channelMask is bitmask of values from table here: https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatextensible#remarks
static void wasapi_start(Wasapi_Audio* audio, size_t sample_rate, size_t channel_count, DWORD channel_mask);

// stops the playback and releases resources
static void wasapi_stop(Wasapi_Audio* audio);

// once locked, then you're allowed to write samples into the ringbuffer
// use only sample_buffer, sample_count and num_samples_submitted members
static void wasapi_lock_buffer(Wasapi_Audio* audio);
static void wasapi_unlock_buffer(Wasapi_Audio* audio, size_t num_samples_written);

//
// Implementation.
//

// TODO: what's missing here:
// * proper error handling, like when no audio device is present (currently asserts)
// * automatically switch to new device when default audio device changes (IMMNotificationClient)

#pragma comment (lib, "avrt")
#pragma comment (lib, "ole32")
#pragma comment (lib, "onecore")

#include <assert.h>
#define HR(stmt) do { HRESULT _hr = stmt; assert(SUCCEEDED(_hr)); } while (0)

// These are missing from windows libs for some reason...
DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xbcde0395, 0xe52f, 0x467c, 0x8e, 0x3d, 0xc4, 0x57, 0x92, 0x91, 0x69, 0x2e);
DEFINE_GUID(IID_IMMDeviceEnumerator,  0xa95664d2, 0x9614, 0x4f35, 0xa7, 0x46, 0xde, 0x8d, 0xb6, 0x36, 0x17, 0xe6);
DEFINE_GUID(IID_IAudioClient,         0x1cb9ad4c, 0xdbfa, 0x4c32, 0xb1, 0x78, 0xc2, 0xf5, 0x68, 0xa7, 0x03, 0xb2);
DEFINE_GUID(IID_IAudioClient3,        0x7ed4ee07, 0x8e67, 0x4cd4, 0x8c, 0x1a, 0x2b, 0x7a, 0x59, 0x87, 0xad, 0x42);
DEFINE_GUID(IID_IAudioRenderClient,   0xf294acfc, 0x3146, 0x4483, 0xa7, 0xbf, 0xad, 0xdc, 0xa7, 0xc2, 0x60, 0xe2);

static void wasapi__lock(Wasapi_Audio* audio)
{
	// loop while audio->lock != FALSE
	while (InterlockedCompareExchange(&audio->lock, TRUE, FALSE) != FALSE) {
		// wait while audio->lock == locked
		LONG locked = FALSE;
		WaitOnAddress(&audio->lock, &locked, sizeof(locked), INFINITE);
	}
	// now audio->lock == TRUE
}

static void wasapi__unlock(Wasapi_Audio* audio)
{
	// audio->lock = FALSE
	InterlockedExchange(&audio->lock, FALSE);
	WakeByAddressSingle(&audio->lock);
}

static DWORD CALLBACK wasapi__audio_thread(LPVOID arg)
{
	Wasapi_Audio *audio = (Wasapi_Audio *)arg;
    
	DWORD task;
	HANDLE handle = AvSetMmThreadCharacteristicsW(L"Pro Audio", &task);
	assert(handle);
    
	IAudioClient *client = audio->client;
    
	IAudioRenderClient *render_client;
	HR(client->GetService(IID_IAudioRenderClient, (LPVOID*)&render_client));
    
	// get audio buffer size in samples
	UINT32 num_buffer_samples;
	HR(client->GetBufferSize(&num_buffer_samples));
    
	// start the render_client
	HR(client->Start());
    
	UINT32 bytes_per_sample = audio->buffer_format->nBlockAlign;
	BYTE *input             = audio->buffer1;
    
	while (WaitForSingleObject(audio->event, INFINITE) == WAIT_OBJECT_0) {
		if (InterlockedExchange(&audio->stop, FALSE))
			break;
        
		UINT32 num_padding_samples;
		HR(client->GetCurrentPadding(&num_padding_samples));
        
		// get output buffer from WASAPI
		BYTE* output;
		UINT32 max_output_samples = num_buffer_samples - num_padding_samples;
		HR(render_client->GetBuffer(max_output_samples, &output));
        
		wasapi__lock(audio);
        
		UINT32 read_offset  = audio->ringbuffer_read_offset;
		UINT32 write_offset = audio->ringbuffer_write_offset;
        
		// how many bytes available to read from ringbuffer
		UINT32 available_size = write_offset - read_offset;
        
		// how many samples available
		UINT32 available_samples = available_size / bytes_per_sample;
        
		// will use up to max that's possible to output
		UINT32 use_samples = min(available_samples, max_output_samples);
        
		// how many bytes to use
		UINT32 use_size = use_samples * bytes_per_sample;
        
		// lock range [read, lock) that memcpy will read from below
		audio->ringbuffer_lock_offset = read_offset + use_size;
        
		// will always submit required amount of samples, but if there's not enough to use, then submit silence
		UINT32 submit_count = use_samples? use_samples : max_output_samples;
		DWORD flags         = use_samples? 0           : AUDCLNT_BUFFERFLAGS_SILENT;
        
		// remember how many samples are submitted
		audio->buffer_used += submit_count;
        
		wasapi__unlock(audio);
        
		// copy bytes to output
		// safe to do it outside wasapi__lock/Unlock, because nobody will overwrite [read, lock) interval
        // since ringbuffer size is always pow2, we can use mask trick.
		memcpy(output, input + (read_offset & (audio->ringbuffer_size - 1)), use_size);
        
		// advance read offset up to lock position, allows writing to [read, lock) interval
		InterlockedAdd(&audio->ringbuffer_read_offset, use_size);
        
		// submit output buffer to WASAPI
		HR(render_client->ReleaseBuffer(submit_count, flags));
	}
    
	// stop the playback
	HR(client->Stop());
	render_client->Release();
    
	AvRevertMmThreadCharacteristics(handle);
	return 0;
}

static DWORD round_up_pow2(DWORD value)
{
	unsigned long index;
	_BitScanReverse(&index, value - 1);
	assert(index < 31);
	return 1U << (index + 1);
}

static void wasapi_start(Wasapi_Audio* audio, size_t sample_rate, size_t channel_count, DWORD channel_mask)
{
	// initialize COM
	HR(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
    
	// create enumerator to get audio device
	IMMDeviceEnumerator *enumerator;
	HR(CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (LPVOID*)&enumerator));
    
	// get default playback device
	IMMDevice *mmdevice;
	HR(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &mmdevice));
	enumerator->Release();
    
	// create audio client for mmdevice
	HR(mmdevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (LPVOID*)&audio->client));
	mmdevice->Release();
    
	WAVEFORMATEXTENSIBLE formatEx = {};
    formatEx.Format.wFormatTag           = WAVE_FORMAT_EXTENSIBLE;
    formatEx.Format.nChannels            = (WORD)channel_count;
    formatEx.Format.nSamplesPerSec       = (WORD)sample_rate;
    formatEx.Format.nAvgBytesPerSec      = (DWORD)(sample_rate * channel_count * sizeof(float));
    formatEx.Format.nBlockAlign          = (WORD)(channel_count * sizeof(float));
    formatEx.Format.wBitsPerSample       = (WORD)(8 * sizeof(float));
    formatEx.Format.cbSize               = sizeof(formatEx) - sizeof(formatEx.Format);
    formatEx.Samples.wValidBitsPerSample = 8 * sizeof(float);
    formatEx.dwChannelMask               = channel_mask;
    formatEx.SubFormat                   = MEDIASUBTYPE_IEEE_FLOAT;
    
	WAVEFORMATEX *wfx;
	if (sample_rate == 0 || channel_count == 0 || channel_mask == 0) {
		// use native mixing format
		HR(audio->client->GetMixFormat(&wfx));
		audio->buffer_format = wfx;
	} else {
		// will use our format
		wfx = &formatEx.Format;
	}
	
	BOOL client_initialized = FALSE;
    
	// try to initialize client with newer functionality in Windows 10, no AUTOCONVERTPCM allowed
	IAudioClient3 *client3;
	if (SUCCEEDED(audio->client->QueryInterface(IID_IAudioClient3, (LPVOID*)&client3))) {
		// minimum buffer size will typically be 480 samples (10msec @ 48khz)
		// but it can be 128 samples (2.66 msec @ 48khz) if driver is properly installed
		// see bullet-point instructions here: https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/low-latency-audio#measurement-tools
		UINT32 default_period_samples, fundamental_period_samples, min_period_samples, max_period_samples;
		client3->GetSharedModeEnginePeriod(wfx, &default_period_samples, &fundamental_period_samples, &min_period_samples, &max_period_samples);
        
		const DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		if (SUCCEEDED(client3->InitializeSharedAudioStream(flags, min_period_samples, wfx, NULL)))
			client_initialized = TRUE;
        
		client3->Release();
	}
    
	if (!client_initialized) {
		// get duration for shared-mode streams, this will typically be 480 samples (10msec @ 48khz)
		REFERENCE_TIME duration;
		HR(audio->client->GetDevicePeriod(&duration, NULL));
        
		// initialize audio playback
		const DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
		HR(audio->client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, duration, 0, wfx, NULL));
	}
    
	if (wfx == &formatEx.Format) {
		HR(audio->client->GetMixFormat(&wfx));
		audio->buffer_format = wfx;
	}
    
	UINT32 num_buffer_samples;
	HR(audio->client->GetBufferSize(&num_buffer_samples));
	audio->out_buffer_size = num_buffer_samples * audio->buffer_format->nBlockAlign;
    
	// setup event handle to wait on
	audio->event = CreateEventW(NULL, FALSE, FALSE, NULL);
	HR(audio->client->SetEventHandle(audio->event));
    
	// use at least 64KB or 1 second whichever is larger, and round upwards to pow2 for ringbuffer
	DWORD ringbuffer_size = round_up_pow2(max(64 * 1024, audio->buffer_format->nAvgBytesPerSec));
    
	// reserve virtual address placeholder for 2x size for magic ringbuffer
	char *placeholder1 = (char*) VirtualAlloc2(NULL, NULL, 2 * ringbuffer_size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
	char *placeholder2 = placeholder1 + ringbuffer_size;
	assert(placeholder1);
    
	// split allocated address space in half
	BOOL ok = VirtualFree(placeholder1, ringbuffer_size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
	assert(ok);
    
	// create page-file backed section for buffer
	HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, ringbuffer_size, NULL);
	assert(section);
    
	// map same section into both addresses
	void *view1 = MapViewOfFile3(section, NULL, placeholder1, 0, ringbuffer_size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
	void *view2 = MapViewOfFile3(section, NULL, placeholder2, 0, ringbuffer_size, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
	assert(view1 && view2);
    
	audio->sample_buffer           = NULL;
	audio->sample_count            = 0;
	audio->num_samples_submitted   = 0;
	audio->buffer1                 = (BYTE *)view1;
	audio->buffer2                 = (BYTE *)view2;
	audio->ringbuffer_size         = ringbuffer_size;
	audio->buffer_used             = 0;
	audio->is_buffer_first_lock    = TRUE;
	audio->ringbuffer_read_offset  = 0;
	audio->ringbuffer_lock_offset  = 0;
	audio->ringbuffer_write_offset = 0;
	InterlockedExchange(&audio->stop, FALSE);
	InterlockedExchange(&audio->lock, FALSE);
	audio->thread = CreateThread(NULL, 0, &wasapi__audio_thread, audio, 0, NULL);
    
	// this is ok, actual memory will be freed only when it is unmapped
	VirtualFree(placeholder1, 0, MEM_RELEASE);
	VirtualFree(placeholder2, 0, MEM_RELEASE);
	CloseHandle(section);
}

static void wasapi_stop(Wasapi_Audio* audio)
{
	// notify thread to stop
	InterlockedExchange(&audio->stop, TRUE);
	SetEvent(audio->event);
    
	// wait for thread to finish
	WaitForSingleObject(audio->thread, INFINITE);
	CloseHandle(audio->thread);
	CloseHandle(audio->event);
    
	// release ringbuffer
	UnmapViewOfFileEx(audio->buffer1, 0);
	UnmapViewOfFileEx(audio->buffer2, 0);
    
	// release audio client
	CoTaskMemFree(audio->buffer_format);
	audio->client->Release();
    
	// done with COM
	CoUninitialize();
}

static void wasapi_lock_buffer(Wasapi_Audio* audio)
{
	UINT32 bytes_per_sample = audio->buffer_format->nBlockAlign;
	UINT32 ringbuffer_size  = audio->ringbuffer_size;
	UINT32 out_buffer_size  = audio->out_buffer_size;
    
	wasapi__lock(audio);
    
	UINT32 read_offset  = audio->ringbuffer_read_offset;
	UINT32 lock_offset  = audio->ringbuffer_lock_offset;
	UINT32 write_offset = audio->ringbuffer_write_offset;
    
	// how many bytes are used in buffer by reader = [read, lock) range
	UINT32 used_size = lock_offset - read_offset;
    
	// make sure there are samples available for one wasapi buffer submission
	// so in case audio thread needs samples before unlock_buffer is called, it can get some 
	if (used_size < out_buffer_size)
	{
		// how many bytes available in current buffer = [read, write) range
		UINT32 available_size = write_offset - read_offset;
        
		// if [read, lock) is smaller than out_buffer_size buffer, then increase lock to [read, read+out_buffer_size) range
		used_size                     = min(out_buffer_size, available_size);
		audio->ringbuffer_lock_offset = lock_offset = read_offset + used_size;
	}
    
	// how many bytes can be written to buffer
	UINT32 write_size = ringbuffer_size - used_size;
    
	// reset write marker to beginning of lock offset (can start writing there)
	audio->ringbuffer_write_offset = lock_offset;
    
	// reset play sample count, use 0 for num_samples_submitted when lock_buffer is called first time
	audio->num_samples_submitted = audio->is_buffer_first_lock ? 0 : audio->buffer_used;
	audio->is_buffer_first_lock  = FALSE;
	audio->buffer_used           = 0;
    
	wasapi__unlock(audio);
    
	// buffer offset/size where to write
	// safe to write in [write, read) range, because reading happen in [read, lock) range (lock==write)
	audio->sample_buffer = audio->buffer1 + (lock_offset & (ringbuffer_size - 1));
	audio->sample_count  = write_size / bytes_per_sample;
}

static void wasapi_unlock_buffer(Wasapi_Audio* audio, size_t num_samples_written)
{
	UINT32 bytes_per_sample = audio->buffer_format->nBlockAlign;
	size_t write_size       = num_samples_written * bytes_per_sample;
    
	// advance write offset to allow reading new samples
	InterlockedAdd(&audio->ringbuffer_write_offset, (LONG)write_size);
}

#endif //WIN32_WASAPI_H
