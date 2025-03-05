
#ifndef MODULE_SOUND_PLAYER
#define MODULE_SOUND_PLAYER

#include "../../DFPSR/api/soundAPI.h"

namespace dsr {

// The simplest type of sound player that requires that the given sound buffer has the same sampling rate as the target.
//   Writes all channels to the target region using memcpy.
struct FixedSoundPlayer {
	SoundBuffer soundBuffer;
	int64_t playerID;
	bool repeat = false;
	bool playing = true;
	uint32_t location = 0;
	FixedSoundPlayer(const SoundBuffer &soundBuffer, int64_t playerID, bool repeat = false, uint32_t startLocation = 0)
	: soundBuffer(soundBuffer), playerID(playerID), repeat(repeat), location(startLocation % sound_getSamplesPerChannel(soundBuffer)) {}
};

void player_getNextSamples(SafePointer<float> target, FixedSoundPlayer &source, int32_t playedSamplesPerChannel);

}

#endif
