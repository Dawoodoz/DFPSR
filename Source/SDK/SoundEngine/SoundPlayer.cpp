
#include "SoundPlayer.h"

namespace dsr {

SoundPlayer::SoundPlayer(const SoundBuffer &soundBuffer, int32_t soundIndex, int64_t playerID, bool repeat, uint32_t startLocation, float leftVolume, float rightVolume, const EnvelopeSettings &envelopeSettings)
: soundBuffer(soundBuffer),
  soundIndex(soundIndex),
  playerID(playerID),
  repeat(repeat),
  location(startLocation % sound_getSamplesPerChannel(soundBuffer)),
  fadeLeft(leftVolume < 0.9999f || leftVolume > 1.0001f),
  fadeRight(rightVolume < 0.9999f || rightVolume > 1.0001f),
  leftVolume(leftVolume),
  rightVolume(rightVolume),
  envelope(envelopeSettings) {}

void player_getNextSamples(SoundPlayer &player, SafePointer<float> target, int32_t playedSamplesPerChannel, double secondsPerSample) {
	uint32_t totalSamplesPerChannel = sound_getSamplesPerChannel(player.soundBuffer);
	uint32_t channelCount = sound_getChannelCount(player.soundBuffer);
	SafePointer<const float> source = sound_getSafePointer(player.soundBuffer);
	uint32_t startIndex = player.location;
	uint32_t blockBytes = channelCount * sizeof(float);
	if (startIndex + playedSamplesPerChannel > totalSamplesPerChannel) {
		// TODO: Properly test this part of the code using regression tests.
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
			player.sustained = false;
		}
	} else {
		// Only a single copy is needed to fill the output buffer with samples.
		safeMemoryCopy(target, source + (player.location * channelCount), playedSamplesPerChannel * blockBytes);
	}
	player.location += playedSamplesPerChannel;
	if (player.repeat) {
		while (player.location >= totalSamplesPerChannel) {
			player.location -= totalSamplesPerChannel;
		}
	}
	// Apply optional envelope
	if (player.envelope.envelopeSettings.used) {
		SafePointer<float> currentTarget = target;
		for (int32_t s = 0; s < playedSamplesPerChannel; s++) {
			float volume = float(player.envelope.getVolume(player.sustained, secondsPerSample));
			for (uint32_t c = 0; c < channelCount; c++) {
				*currentTarget *= volume;
				currentTarget += 1;
			}
		}
	}
}

}
