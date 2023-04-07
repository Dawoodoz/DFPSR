
#ifndef DFPSR_SOUND_API
#define DFPSR_SOUND_API

#include "../DFPSR/includeFramework.h"

// TODO: The float array should be padded to at least 16 bytes for 128-bit SIMD.

// Call this function from a separate thread in a sound engine to initialize the sound system, call back with sound output requests and terminate when the callback returns false.
// The float array given to soundOutput should be filled with samples from 0 to totalSamples - 1.
// Channels from the same point in time are packed together without any padding in between.
// Returns false if the backend could not be created.
// Returns true iff the backend completed all work and terminated safely.
bool sound_streamToSpeakers(int channels, int sampleRate, std::function<bool(dsr::SafePointer<float> data, int length)> soundOutput);

#endif
