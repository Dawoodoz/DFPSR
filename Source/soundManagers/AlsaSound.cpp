﻿
// Use -lasound for linking to the winmm library in GCC/G++
//   Install on Arch: sudo pacman -S libasound-dev
//   Install on Debian: sudo apt-get install libasound-dev

#include "../DFPSR/api/soundAPI.h"
#include "../DFPSR/base/simd.h"
#include <alsa/asoundlib.h>

namespace dsr {

static snd_pcm_t *pcm = nullptr;
static Buffer outputBuffer, floatBuffer;
static SafePointer<int16_t> outputData;
static SafePointer<float> floatData;

static void allocateBuffers(int neededElements) {
	outputBuffer = buffer_create(neededElements * sizeof(int16_t));
	floatBuffer = buffer_create(neededElements * sizeof(float));
	outputData = buffer_getSafeData<int16_t>(outputBuffer, "Output data");
	floatData = buffer_getSafeData<float>(floatBuffer, "Output data");
}

static void terminateSound() {
	if (pcm) {
		snd_pcm_drop(pcm);
		snd_pcm_drain(pcm);
		snd_pcm_close(pcm);
		pcm = nullptr;
	}
}

bool sound_streamToSpeakers(int32_t channels, int32_t sampleRate, std::function<bool(SafePointer<float> data, int32_t length)> soundOutput) {
	int errorCode;
	if ((errorCode = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		terminateSound();
		sendWarning(U"Cannot open sound device. (", snd_strerror(errorCode), U")\n");
		return false;
	}
	snd_pcm_hw_params_t *hardwareParameters;
	snd_pcm_hw_params_alloca(&hardwareParameters);
	snd_pcm_hw_params_any(pcm, hardwareParameters);
	if ((errorCode = snd_pcm_hw_params_set_access(pcm, hardwareParameters, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		terminateSound();
		sendWarning(U"Failed to select interleaved sound. (", snd_strerror(errorCode), U")\n");
		return false;
	}
	if ((errorCode = snd_pcm_hw_params_set_format(pcm, hardwareParameters, SND_PCM_FORMAT_S16_LE)) < 0) {
		terminateSound();
		sendWarning(U"Failed to select sound format. (", snd_strerror(errorCode), U")\n");
		return false;
	}
	if ((errorCode = snd_pcm_hw_params_set_channels(pcm, hardwareParameters, channels)) < 0) {
		terminateSound();
		sendWarning(U"Failed to select channel count. (", snd_strerror(errorCode), U")\n");
		return false;
	}
	if ((errorCode = snd_pcm_hw_params_set_buffer_size(pcm, hardwareParameters, 2048)) < 0) {
		terminateSound();
		sendWarning(U"Failed to select buffer size. (", snd_strerror(errorCode), U")\n");
		return false;
	}
	uint rate = sampleRate;
	if ((errorCode = snd_pcm_hw_params_set_rate_near(pcm, hardwareParameters, &rate, 0)) < 0) {
		terminateSound();
		sendWarning(U"Failed to select approximate sample rate. (", snd_strerror(errorCode), U")\n");
		return false;
	}
	if ((errorCode = snd_pcm_hw_params(pcm, hardwareParameters)) < 0) {
		terminateSound();
		sendWarning(U"Failed to select hardware parameters. (", snd_strerror(errorCode), U")\n");
		return false;
	}
	// Allocate a buffer for sending data to speakers
	snd_pcm_uframes_t samplesPerChannel;
	snd_pcm_hw_params_get_period_size(hardwareParameters, &samplesPerChannel, 0);
	// Allocate target buffers
	int totalSamples = samplesPerChannel * channels;
	allocateBuffers(totalSamples);
	while (true) {
		safeMemorySet(floatData, 0, totalSamples * sizeof(float));
		bool keepRunning = soundOutput(floatData, samplesPerChannel);
		// Convert to target format so that the sound can be played
		for (uint32_t t = 0; t < samplesPerChannel * channels; t+=8) {
			// SIMD vectorized sound conversion with scaling and clamping to signed 16-bit integers.
			F32x4 lowerFloats = F32x4::readAligned(floatData + t, "sound_streamToSpeakers: Reading lower floats");
			F32x4 upperFloats = F32x4::readAligned(floatData + t + 4, "sound_streamToSpeakers: Reading upper floats");
			I32x4 lowerInts = truncateToI32(clamp(F32x4(-32768.0f), lowerFloats * 32767.0f, F32x4(32767.0f)));
			I32x4 upperInts = truncateToI32(clamp(F32x4(-32768.0f), upperFloats * 32767.0f, F32x4(32767.0f)));
			// TODO: Create I16x8 SIMD vectors for processing sound as 16-bit integers?
			//       Or just move unzip into simd.h with a fallback solution and remove simdExtra.h.
			//       Or just implement reading and writing of 16-bit signed integers using multiple SIMD registers or smaller memory regions.
			IVector4D lower = lowerInts.get();
			IVector4D upper = upperInts.get();
			outputData[t+0] = (int16_t)lower.x;
			outputData[t+1] = (int16_t)lower.y;
			outputData[t+2] = (int16_t)lower.z;
			outputData[t+3] = (int16_t)lower.w;
			outputData[t+4] = (int16_t)upper.x;
			outputData[t+5] = (int16_t)upper.y;
			outputData[t+6] = (int16_t)upper.z;
			outputData[t+7] = (int16_t)upper.w;
		}
		errorCode = snd_pcm_writei(pcm, outputData.getUnsafe(), samplesPerChannel);
		if (errorCode == -EPIPE) {
			// Came too late! Not enough written samples to play.
			snd_pcm_prepare(pcm);
		} else if (errorCode < 0) {
			terminateSound();
			throwError(U"Failed writing data to PCM. (", snd_strerror(errorCode), U")\n");
		}
		if (!keepRunning) {
			break;
		}
	}
	terminateSound();
	return true;
}

}
