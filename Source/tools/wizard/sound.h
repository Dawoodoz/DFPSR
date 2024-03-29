﻿
#ifndef MODULE_SOUND
#define MODULE_SOUND

#include "../../DFPSR/includeFramework.h"

static const int soundFormat_I16 = 0; // Hi-fi bit-depth
static const int soundFormat_F32 = 1; // Studio bit-depth

void sound_initialize();
void sound_terminate();

// Creates a single-channel sound using the generator function
// generator takes the time in seconds as input and returns a value from -1.0 to 1.0
int generateMonoSoundBuffer(const dsr::ReadableString &name, int sampleCount, int sampleRate, int soundFormat, std::function<double(double time)> generator);
int getSoundBufferCount();

int loadWaveSoundFromBuffer(const dsr::ReadableString &name, dsr::Buffer bufferconst);
int loadSoundFromFile(const dsr::ReadableString &filename, bool mustExist = true);

struct EnvelopeSettings {
	// Basic ADSR
	double attack, decay, sustain, release;
	// Extended
	double hold, rise, sustainedSmooth, releasedSmooth;
	EnvelopeSettings();
	EnvelopeSettings(double attack, double decay, double sustain, double release, double hold = 0.0, double rise = 0.0, double sustainedSmooth = 0.0, double releasedSmooth = 0.0);
};

int playSound(int soundIndex, bool repeat, double volumeLeft, double volumeRight, double speed);
int playSound(int soundIndex, bool repeat, double volumeLeft, double volumeRight, double speed, const EnvelopeSettings &envelopeSettings);
// Begin to fade out the sound and let it delete itself once done
void releaseSound(int64_t playerID);
// Stop the sound at once
void stopSound(int64_t playerID);
// Stop all sounds at once
void stopAllSounds();

#endif
