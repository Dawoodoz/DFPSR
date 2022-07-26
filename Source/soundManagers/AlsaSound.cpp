
// Use -lasound for linking to the winmm library in GCC/G++
//   Install on Arch: sudo pacman -S libasound-dev
//   Install on Debian: sudo apt-get install libasound-dev

#include "../../dfpsr/Source/DFPSR/includeFramework.h"
#include "soundManagers.h"
#include <alsa/asoundlib.h>

using namespace dsr;

snd_pcm_t *pcm = nullptr;
static int bufferElements = 0;
static int16_t *outputBuffer = nullptr;
static float *floatBuffer = nullptr;
static void allocateBuffers(int neededElements) {
	// TODO: Use aligned memory with Buffer
	outputBuffer = (int16_t *)calloc(roundUp(neededElements, 8), sizeof(int16_t));
	floatBuffer = (float *)calloc(roundUp(neededElements, 8), sizeof(float));
	bufferElements = neededElements;
}

static void terminateSound() {
	if (pcm) {
		snd_pcm_drop(pcm);
		snd_pcm_drain(pcm);
		snd_pcm_close(pcm);
		pcm = nullptr;
	}
	if (outputBuffer) { free(outputBuffer); }
	if (floatBuffer) { free(floatBuffer); }
}

bool sound_streamToSpeakers(int channels, int sampleRate, std::function<bool(float*, int)> soundOutput) {
	int errorCode;
	if ((errorCode = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		terminateSound();
		throwError("Cannot open sound device. (", snd_strerror(errorCode), ")\n");
	}
	snd_pcm_hw_params_t *hardwareParameters;
	snd_pcm_hw_params_alloca(&hardwareParameters);
	snd_pcm_hw_params_any(pcm, hardwareParameters);
	if ((errorCode = snd_pcm_hw_params_set_access(pcm, hardwareParameters, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		terminateSound();
		throwError("Failed to select interleaved sound. (", snd_strerror(errorCode), ")\n");
	}
	if ((errorCode = snd_pcm_hw_params_set_format(pcm, hardwareParameters, SND_PCM_FORMAT_S16_LE)) < 0) {
		terminateSound();
		throwError("Failed to select sound format. (", snd_strerror(errorCode), ")\n");
	}
	if ((errorCode = snd_pcm_hw_params_set_channels(pcm, hardwareParameters, channels)) < 0) {
		terminateSound();
		throwError("Failed to select channel count. (", snd_strerror(errorCode), ")\n");
	}
	if ((errorCode = snd_pcm_hw_params_set_buffer_size(pcm, hardwareParameters, 2048)) < 0) {
		terminateSound();
		throwError("Failed to select buffer size. (", snd_strerror(errorCode), ")\n");
	}
	uint rate = sampleRate;
	if ((errorCode = snd_pcm_hw_params_set_rate_near(pcm, hardwareParameters, &rate, 0)) < 0) {
		terminateSound();
		throwError("Failed to select approximate sample rate. (", snd_strerror(errorCode), ")\n");
	}
	if ((errorCode = snd_pcm_hw_params(pcm, hardwareParameters)) < 0) {
		terminateSound();
		throwError("Failed to select hardware parameters. (", snd_strerror(errorCode), ")\n");
	}
	// Allocate a buffer for sending data to speakers
	snd_pcm_uframes_t samplesPerChannel;
	snd_pcm_hw_params_get_period_size(hardwareParameters, &samplesPerChannel, 0);
	// Allocate target buffers
	int totalSamples = samplesPerChannel * channels;
	allocateBuffers(totalSamples);
	while (true) {
		memset(floatBuffer, 0, totalSamples * sizeof(float));
		bool keepRunning = soundOutput(floatBuffer, samplesPerChannel);
		// Convert to target format so that the sound can be played
		// TODO: Use SIMD
		for (uint32_t t = 0; t < samplesPerChannel * channels; t++) {
			outputBuffer[t] = sound_convertF32ToI16(floatBuffer[t]);
		}
		errorCode = snd_pcm_writei(pcm, outputBuffer, samplesPerChannel);
		if (errorCode == -EPIPE) {
			// Came too late! Not enough written samples to play.
			snd_pcm_prepare(pcm);
		} else if (errorCode < 0) {
			terminateSound();
			throwError("Failed writing data to PCM. (", snd_strerror(errorCode), ")\n");
		}
		if (!keepRunning) {
			break;
		}
	}
	terminateSound();
	return true;
}
