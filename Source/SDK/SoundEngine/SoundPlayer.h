
#ifndef MODULE_SOUND_PLAYER
#define MODULE_SOUND_PLAYER

#include "../../DFPSR/api/soundAPI.h"
#include "Envelope.h"

namespace dsr {

struct SoundPlayer {
	SoundBuffer soundBuffer;
	int32_t soundIndex;
	int64_t playerID;
	bool repeat = false;
	bool sustained = true;
	// TODO: Use a union for the location to allow switching between fixed and interpolating implementations.
	// Fixed
	uint32_t location = 0;
	// Optional volume to be applied externally, because the player does not duplicate channels.
	bool fadeLeft, fadeRight; // True iff the corresponding volume is not 1.0.
	float leftVolume, rightVolume;
	// Optional Envelope
	Envelope envelope;
	SoundPlayer(const SoundBuffer &soundBuffer, int32_t soundIndex, int64_t playerID, bool repeat, uint32_t startLocation, float leftVolume, float rightVolume, const EnvelopeSettings &envelopeSettings);
};

void player_getNextSamples(SoundPlayer &source, SafePointer<float> target, int32_t playedSamplesPerChannel, double secondsPerSample);

}

#endif
