#include "macros.h"
#include "audio_api.h"

#include <nanoshell/nanoshell.h>
#include <assert.h>

// NOTE: This is broken. The SB16 driver is kind of bad. While it mostly works,
// there's some terrible audio lag that I just can't seem to figure out.

// TODO: Call into the system to determine the preferred device.
static const char* audio_file = "/Device/Sb16";

static int audio_handle = -1;

static bool audio_nanoshell_init(void) {
	
	LogMsg("AudioInit");
	
	// try to open the file
	audio_handle = open(audio_file, O_WRONLY);
	
	if (audio_handle < 0)
	{
		LogMsg("Can't open audio handle: %s", strerror(errno));
		return false;
	}
	
	int length = 500;
	int16_t *sound_data = calloc(length, sizeof(int16_t));
	write(audio_handle, sound_data, length * sizeof(int16_t));
	free(sound_data);
	
    return true;
}

static int last_feed_tick_count = 0;

static int audio_nanoshell_get_desired_buffered(void) {
    return 1100;
}

static int audio_nanoshell_buffered(void) {
	// tick count is in milliseconds, by the way
	int ticks_since_last_feed = GetTickCount() - last_feed_tick_count;
	int samps_per_sec = 44100;
	
	int samps_consumed = samps_per_sec * ticks_since_last_feed / 1000;
	
	int result = audio_nanoshell_get_desired_buffered() - samps_consumed;
	if (result < 0)
		result = 0;
	
    return result;
}

// Function to upsample audio samples from 32kHz to 44.1kHz
void upsample(int16_t* input, int16_t* output, int inputLength) {
    int outputLength = (inputLength * 441) / 320;  // Calculate the length of the output buffer

    for (int i = 0; i < outputLength; i++) {
        int inIndex = i * 320 / 441;  // Calculate the corresponding index in the input buffer
        output[i] = input[inIndex];

        // Linear interpolation: Insert intermediate samples between the original samples
        if (inIndex < inputLength - 1) {
            int nextSample = input[inIndex + 1];
            int diff = nextSample - input[inIndex];
            int position = i * 320 % 441;
            output[i] += (diff * position) / 441;
        }
    }
}

static void audio_nanoshell_play(const uint8_t *buf, size_t len) {
	int num_samples     = (int)len / sizeof(int16_t) / 2; // 2 channels
	int num_samples_out = num_samples * 441 / 320;
	int16_t* sound_32000 = malloc(sizeof(int16_t) * num_samples);
	int16_t* sound_44100 = malloc(sizeof(int16_t) * num_samples_out);
	
	const int16_t* in_buf = (const int16_t*) buf;
	
	for (int i = 0, j = 0; i < num_samples; i++, j += 2) {
		sound_32000[i] = (int16_t)(((int)in_buf[j + 0] + (int)in_buf[j + 1]) / 2);
	}
	
	upsample(sound_32000, sound_44100, num_samples);
	
	write(audio_handle, sound_44100, sizeof(int16_t) * num_samples_out);
	
	LogMsg("Took %d ms to feed", GetTickCount() - last_feed_tick_count);
	
	last_feed_tick_count = GetTickCount();
}

static void audio_nanoshell_shutdown(void) {
	if (audio_handle == -1)
		return;
	
	close(audio_handle);
	audio_handle = -1;
}

struct AudioAPI audio_nanoshell = {
    audio_nanoshell_init,
    audio_nanoshell_buffered,
    audio_nanoshell_get_desired_buffered,
    audio_nanoshell_play,
	audio_nanoshell_shutdown,
};
