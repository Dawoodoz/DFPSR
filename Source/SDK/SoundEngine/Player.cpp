
#include "Player.h"

namespace dsr {

void player_getNextSamples(SafePointer<float> target, FixedSoundPlayer &player, int32_t playedSamplesPerChannel) {
	if (player.playing) {
		int32_t totalSamplesPerChannel = sound_getSamplesPerChannel(player.soundBuffer);
		int32_t channelCount = sound_getChannelCount(player.soundBuffer);
		SafePointer<const float> source = sound_getSafePointer(player.soundBuffer);
		int32_t startIndex = player.location;
		uint32_t blockBytes = channelCount * sizeof(float);
		if (startIndex + playedSamplesPerChannel > totalSamplesPerChannel) {
			SafePointer<float> writer = target;
			// Calculate how many samples that are inside of the buffer.
			uint32_t insideSamples = totalSamplesPerChannel - startIndex;
			if (insideSamples > 0) {
				uintptr_t insideBytes = insideSamples * blockBytes;
				safeMemoryCopy(writer, source + (player.location * channelCount), insideBytes);
				writer.increaseBytes(insideBytes);
			}
			// Calculate how many samples that are outside of the buffer.
			uint32_t outsideSamples = playedSamplesPerChannel - insideSamples;
			if (player.repeat) {
				// Copy whole laps of the sound buffer.
				uintptr_t wholeLapBytes = totalSamplesPerChannel * blockBytes;
				while (outsideSamples >= totalSamplesPerChannel) {
					safeMemoryCopy(writer, source, wholeLapBytes);
					writer.increaseBytes(wholeLapBytes);
					outsideSamples -= totalSamplesPerChannel;
				}
				// Copy a partial lap at the end if there are samples remaining.
				if (outsideSamples > 0) {
					uintptr_t lastLapBytes = outsideSamples * blockBytes;
					safeMemoryCopy(writer, source, lastLapBytes);
				}
			} else {
				// Padd the outside with zeroes.
				uint32_t outsideBytes = outsideSamples * blockBytes;
				safeMemorySet(target, 0, outsideBytes);
				player.playing = false;
			}
		} else {
			// Only a single copy is needed to fill the output buffer with samples.
			safeMemoryCopy(target, source + (player.location * channelCount), playedSamplesPerChannel * blockBytes);
		}
		player.location += playedSamplesPerChannel;
	}
}

}
