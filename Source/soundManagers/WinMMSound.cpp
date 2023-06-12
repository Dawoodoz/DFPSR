
// Use -lwinmm for linking to the winmm library in GCC/G++

#include "soundManagers.h"
#include <windows.h>
#include <mmsystem.h>

using namespace dsr;

static const int samplesPerChannel = 2048;

static int bufferElements = 0;
static Buffer outputBuffer, floatBuffer;
static SafePointer<int16_t> outputData[2];
static SafePointer<float> floatData;

// Aligning memory to allow using the widest available floating-point SIMD vector.
static const int soundBufferAlignment = DSR_FLOAT_ALIGNMENT;

static void allocateBuffers(int neededElements) {
	int64_t roundedElements = roundUp(neededElements, soundBufferAlignment / 2);
	int64_t outputSize = roundedElements * sizeof(int16_t);
	outputBuffer = buffer_create(outputSize * 2);
	floatBuffer = buffer_create(roundedElements * sizeof(float), soundBufferAlignment);
	SafePointer<int16_t> allOutputData = buffer_getSafeData<int16_t>(outputBuffer, "Output data");
	outputData[0] = allOutputData.slice("Output data 0", 0, outputSize);
	outputData[1] = allOutputData.slice("Output data 1", outputSize, outputSize);
	floatData = buffer_getSafeData<float>(floatBuffer, "Output data");
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
}

bool sound_streamToSpeakers(int channels, int sampleRate, std::function<bool(SafePointer<float> data, int length)> soundOutput) {
	bufferEndEvent = CreateEvent(0, FALSE, FALSE, 0);
	if (bufferEndEvent == 0) {
		terminateSound();
		sendWarning(U"Failed to create buffer end event!");
		return false;
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
		sendWarning(U"Failed to open wave output!");
		return false;
	}
	if(waveOutSetVolume(waveOutput, 0xFFFFFFFF) != MMSYSERR_NOERROR) {
		terminateSound();
		sendWarning(U"Failed to set volume!");
		return false;
	}
	for (int b = 0; b < 2; b++) {
		ZeroMemory(&header[b], sizeof(WAVEHDR));
		header[b].dwBufferLength = totalSamples * sizeof(int16_t);
		header[b].lpData = (LPSTR)(outputData[b].getUnsafe());
		if (waveOutPrepareHeader(waveOutput, &header[b], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			terminateSound();
			sendWarning(U"Failed to prepare buffer for streaming!");
			return false;
		}
	}
	running = true;
	while (running) {
		for (int b = 0; b < 2; b++) {
			if ((header[b].dwFlags & WHDR_INQUEUE) == 0) {
				// When one of the buffers is done playing, generate new sound and write more data to the output.
				safeMemorySet(floatData, 0, totalSamples * sizeof(float));
				running = soundOutput(floatData, samplesPerChannel);
				//for (int i = 0; i < totalSamples; i++) {
				//	outputData[b][i] = sound_convertF32ToI16(floatBuffer[i]);
				//}
				SafePointer<int16_t> target = outputData[b];
				// Convert to target format so that the sound can be played
				for (uint32_t t = 0; t < totalSamples; t+=8) {
					// SIMD vectorized sound conversion with scaling and clamping to signed 16-bit integers.
					F32x4 lowerFloats = F32x4::readAligned(floatData + t, "sound_streamToSpeakers: Reading lower floats");
					F32x4 upperFloats = F32x4::readAligned(floatData + t + 4, "sound_streamToSpeakers: Reading upper floats");
					I32x4 lowerInts = truncateToI32((lowerFloats * 32767.0f).clamp(-32768.0f, 32767.0f));
					I32x4 upperInts = truncateToI32((upperFloats * 32767.0f).clamp(-32768.0f, 32767.0f));
					// TODO: Create I16x8 SIMD vectors for processing sound as 16-bit integers?
					//       Or just move unzip into simd.h with a fallback solution and remove simdExtra.h.
					//       Or just implement reading and writing of 16-bit signed integers using multiple SIMD registers or smaller memory regions.
					IVector4D lower = lowerInts.get();
					IVector4D upper = upperInts.get();
					target[t+0] = (int16_t)lower.x;
					target[t+1] = (int16_t)lower.y;
					target[t+2] = (int16_t)lower.z;
					target[t+3] = (int16_t)lower.w;
					target[t+4] = (int16_t)upper.x;
					target[t+5] = (int16_t)upper.y;
					target[t+6] = (int16_t)upper.z;
					target[t+7] = (int16_t)upper.w;
				}
				if (waveOutWrite(waveOutput, &header[b], sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
					terminateSound();
					sendWarning(U"Failed to write wave output!");
					return false;
				}
				if (!running) { break; }
			}
		}
		WaitForSingleObject(bufferEndEvent, INFINITE);
	}
	terminateSound();
	return true;
}
