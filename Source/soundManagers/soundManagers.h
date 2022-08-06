
#ifndef DFPSR_SOUND_API
#define DFPSR_SOUND_API

#include <functional>
#include <stdint.h>

inline float sound_convertI16ToF32(int64_t input) {
	return input * (1.0f / 32767.0f);
}

inline int sound_convertF32ToI16(float input) {
	int64_t result = input * 32767.0f;
	if (result > 32767) { result = 32767; }
	if (result < -32768) { result = -32768; }
	return result;
}

// Call this function from a separate thread in a sound engine to initialize the sound system, call back with sound output requests and terminate when the callback returns false.
// The float array given to soundOutput should be filled with samples from 0 to totalSamples - 1.
// Channels from the same point in time are packed together without any padding in between.
// Returns false if the backend could not be created.
// Returns true iff the backend completed all work and terminated safely.
bool sound_streamToSpeakers(int channels, int sampleRate, std::function<bool(float*, int)> soundOutput);

#endif
