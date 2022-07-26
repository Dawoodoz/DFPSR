
// Use -lwinmm for linking to the winmm library in GCC/G++

#include "../../dfpsr/Source/DFPSR/includeFramework.h"
#include "soundManagers.h"
#include <windows.h>
#include <mmsystem.h>

using namespace dsr;

static const int samplesPerChannel = 2048;

static int bufferElements = 0;
static int16_t *outputBuffer[2] = {nullptr, nullptr};
static float *floatBuffer = nullptr;
static void allocateBuffers(int neededElements) {
	// TODO: Use aligned memory with Buffer
	outputBuffer[0] = (int16_t *)calloc(roundUp(neededElements, 8), sizeof(int16_t));
	outputBuffer[1] = (int16_t *)calloc(roundUp(neededElements, 8), sizeof(int16_t));
	floatBuffer = (float *)calloc(roundUp(neededElements, 8), sizeof(float));
	bufferElements = neededElements;
}

static HWAVEOUT waveOutput;
static WAVEHDR header[2] = {0};
static HANDLE bufferEndEvent = 0;

static bool running = true;
static void terminateSound() {
	printText("Terminating sound.\n");
	running = false;
	if (waveOutput) {
		waveOutReset(waveOutput);
		for (int b = 0; b < 2; b++) {
			waveOutUnprepareHeader(waveOutput, &header[b], sizeof(WAVEHDR));
		}
		waveOutClose(waveOutput);
		waveOutput = 0;
	}
	if (bufferEndEvent) {
		CloseHandle(bufferEndEvent);
		bufferEndEvent = 0;
	}
	for (int b = 0; b < 2; b++) {
		if (outputBuffer[b]) { free(outputBuffer[b]); }
	}
	if (floatBuffer) { free(floatBuffer); }
}

bool sound_streamToSpeakers(int channels, int sampleRate, std::function<bool(float*, int)> soundOutput) {
	bufferEndEvent = CreateEvent(0, FALSE, FALSE, 0);
	if (bufferEndEvent == 0) {
		terminateSound();
		throwError(U"Failed to create buffer end event!");
	}
	int totalSamples = samplesPerChannel * channels;
	allocateBuffers(totalSamples);
	WAVEFORMATEX format;
	ZeroMemory(&format, sizeof(WAVEFORMATEX));
	format.nChannels = (WORD)channels;
	format.nSamplesPerSec = (DWORD)sampleRate;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.wBitsPerSample = 16;
	format.nBlockAlign = format.nChannels * sizeof(int16_t);
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
	format.cbSize = 0;
	if(waveOutOpen(&waveOutput, WAVE_MAPPER, &format, (DWORD_PTR)bufferEndEvent, (DWORD_PTR)NULL, CALLBACK_EVENT) != MMSYSERR_NOERROR) {
		terminateSound();
		throwError(U"Failed to open wave output!");
	}
	if(waveOutSetVolume(waveOutput, 0xFFFFFFFF) != MMSYSERR_NOERROR) {
		terminateSound();
		throwError(U"Failed to set volume!");
	}
	for (int b = 0; b < 2; b++) {
		ZeroMemory(&header[b], sizeof(WAVEHDR));
		header[b].dwBufferLength = totalSamples * sizeof(int16_t);
		header[b].lpData = (LPSTR)(outputBuffer[b]);
		if (waveOutPrepareHeader(waveOutput, &header[b], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			terminateSound();
			throwError(U"Failed to prepare buffer for streaming!");
		}
	}
	running = true;
	while (running) {
		for (int b = 0; b < 2; b++) {
			if ((header[b].dwFlags & WHDR_INQUEUE) == 0) {
				// When one of the buffers is done playing, generate new sound and write more data to the output.
				memset(floatBuffer, 0, totalSamples * sizeof(float));
				// TODO: Use 128-bit aligned memory
				running = soundOutput(floatBuffer, samplesPerChannel);
				// TODO: Use SIMD
				for (int i = 0; i < totalSamples; i++) {
					outputBuffer[b][i] = sound_convertF32ToI16(floatBuffer[i]);
				}
				if (waveOutWrite(waveOutput, &header[b], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
					terminateSound(); throwError(U"Failed to write wave output!");
				}
				if (!running) { break; }
			}
		}
		WaitForSingleObject(bufferEndEvent, INFINITE);
	}
	terminateSound();
	return true;
}
