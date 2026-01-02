
#include "soundEngine.h"
#include "../../DFPSR/api/soundAPI.h"
#include "../../DFPSR/api/drawAPI.h"
#include "../../DFPSR/api/fileAPI.h"
#include "../../DFPSR/api/fontAPI.h"
#include "../../DFPSR/base/virtualStack.h"
#include "../../DFPSR/base/simd.h"
#include "SoundPlayer.h"

#include <future>
#include <atomic>

namespace dsr {

static const int32_t maxChannels = 2;
static const int32_t outputChannels = 2;
static const int32_t outputSampleRate = 44100;
double outputSoundStep = 1.0 / (double)outputSampleRate;

std::future<void> soundFuture;
static std::atomic<bool> soundRunning{true};
static std::mutex soundMutex;

static void minMax(float &minimum, float &maximum, float value) {
	if (value < minimum) { minimum = value; }
	if (value > maximum) { maximum = value; }
}

struct Sound {
	SoundBuffer buffer;
	String name;
	bool fromFile;
	Sound(const SoundBuffer &buffer, const ReadableString &name, bool fromFile)
	: buffer(buffer), name(name), fromFile(fromFile) {}
	Sound(const ReadableString &name, bool fromFile, int32_t samplesPerChannel, int32_t channelCount, int32_t sampleRate)
	: buffer(samplesPerChannel, channelCount, sampleRate), name(name), fromFile(fromFile) {}
	float sampleLinear(int32_t leftIndex, int32_t rightIndex, double ratio, int32_t channel) {
		int64_t leftOffset = leftIndex * sound_getChannelCount(this->buffer) + channel;
		int64_t rightOffset = rightIndex * sound_getChannelCount(this->buffer) + channel;
		float a = 0.0, b = 0.0;
		SafePointer<float> source = sound_getSafePointer(this->buffer);
		a = source[leftOffset];
		b = source[rightOffset];
		return b * ratio + a * (1.0 - ratio);
	}
	float sampleLinear_cyclic(double location, int32_t channel) {
		int32_t truncated = (int64_t)location;
		int32_t floor = truncated % sound_getSamplesPerChannel(this->buffer);
		int32_t ceiling = floor + 1; if (ceiling == sound_getSamplesPerChannel(this->buffer)) { ceiling = 0; }
		double ratio = location - truncated;
		return this->sampleLinear(floor, ceiling, ratio, channel);
	}
	float sampleLinear_clamped(double location, int32_t channel) {
		int32_t truncated = (int64_t)location;
		int32_t floor = truncated; if (floor >= sound_getSamplesPerChannel(this->buffer)) { floor = sound_getSamplesPerChannel(this->buffer) - 1; }
		int32_t ceiling = floor + 1; if (ceiling >= sound_getSamplesPerChannel(this->buffer)) { ceiling = sound_getSamplesPerChannel(this->buffer) - 1; }
		double ratio = location - truncated;
		return this->sampleLinear(floor, ceiling, ratio, channel);
	}
	void sampleMinMax(float &minimum, float &maximum, int32_t startSample, int32_t endSample, int32_t channel) {
		if (startSample < 0) { startSample = 0; }
		if (endSample >= sound_getSamplesPerChannel(this->buffer)) { endSample = sound_getSamplesPerChannel(this->buffer) - 1; }
		if (channel < 0) { channel = 0; }
		if (channel >= sound_getChannelCount(this->buffer)) { channel = sound_getChannelCount(this->buffer) - 1; }
		int32_t bufferIndex = startSample * sound_getChannelCount(this->buffer) + channel;
		SafePointer<float> source = sound_getSafePointer(this->buffer);
		for (int32_t s = startSample; s <= endSample; s++) {
			minMax(minimum, maximum, source[bufferIndex]);
			bufferIndex += sound_getChannelCount(this->buffer);
		}
	}
};
List<Sound> sounds;

int32_t soundEngine_loadSoundFromFile(const ReadableString &filename, bool mustExist) {
	// Try to reuse any previously instance of the file before accessing the file system
	for (int32_t s = 0; s < sounds.length(); s++) {
		if (sounds[s].fromFile && string_match(sounds[s].name, filename)) {
			return s;
		}
	}
	return soundEngine_insertSoundBuffer(sound_load(filename, mustExist), filename, true);
}

int32_t soundEngine_getSoundBufferCount() {
	return sounds.length();
}

List<SoundPlayer> fixedPlayers;
int64_t nextPlayerID = 0;
int32_t soundEngine_playSound(int32_t soundIndex, bool repeat, float leftVolume, float rightVolume, const EnvelopeSettings &envelopeSettings) {
	int32_t result;
	if (soundIndex < 0 || soundIndex >= sounds.length()) {
		sendWarning(U"playSound_simple: Sound index ", soundIndex, U" does not exist!\n");
		return -1;
	}
	Sound *sound = &(sounds[soundIndex]);
	if (!sound_exists(sound->buffer)) {
		// Nothing to play.
		return -1;
	}
	int32_t soundSampleRate = sound_getSampleRate(sound->buffer);
	if (soundSampleRate != outputSampleRate) {
		throwError(U"playSound_simple: The sound ", sound->name, U" has ", soundSampleRate, U" samples per second in each channel, but the sound engine samples output at ", outputSampleRate, U" samples per second!\n");
	}
	int32_t soundChannel = sound_getChannelCount(sound->buffer);
	if (soundChannel > maxChannels) {
		throwError(U"playSound_simple: The sound ", sound->name, U" has ", soundChannel, U" channels, but the sound engine can not play more than ", maxChannels, U"channels!\n");
	}
	soundMutex.lock();
		result = nextPlayerID;
		fixedPlayers.pushConstruct(sounds[soundIndex].buffer, soundIndex, result, repeat, 0u, leftVolume, rightVolume, envelopeSettings);
		nextPlayerID++;
	soundMutex.unlock();
	return result;
}

static int32_t findFixedPlayer(int64_t playerID) {
	for (int32_t p = 0; p < fixedPlayers.length(); p++) {
		if (fixedPlayers[p].playerID == playerID) {
			return p;
		}
	}
	return -1;
}
void soundEngine_releaseSound(int64_t playerID) {
	if (playerID != -1) {
		soundMutex.lock();
			int32_t index = findFixedPlayer(playerID);
			if (index > -1) {
				fixedPlayers[index].sustained = false;
			}
		soundMutex.unlock();
	}
}
void soundEngine_stopSound(int64_t playerID) {
	if (playerID != -1) {
		soundMutex.lock();
			int32_t index = findFixedPlayer(playerID);
			if (index > -1) {
				fixedPlayers.remove(index);
			}
		soundMutex.unlock();
	}
}
void soundEngine_stopAllSounds() {
	soundMutex.lock();
		fixedPlayers.clear();
	soundMutex.unlock();
}

// By using a fixed period size independently of the hardware's period size with sound_streamToSpeakers_fixed, we can reduce waste from SIMD padding and context switches.
static const int32_t periodSize = 1024;

void soundEngine_initialize() {
	// Start a worker thread mixing sounds in realtime
	std::function<void()> task = []() {
		//sound_streamToSpeakers(outputChannels, outputSampleRate, [](SafePointer<float> target, int32_t requestedSamplesPerChannel) -> bool {
		sound_streamToSpeakers_fixed(outputChannels, outputSampleRate, periodSize, [](SafePointer<float> target) -> bool {
			// TODO: Create a thread-safe swap chain or input queue, so that the main thread and sound thread can work at the same time.
			// Anyone wanting to change the played sounds from another thread will have to wait until this section has finished processing
			soundMutex.lock();
				VirtualStackAllocation<float> playerBuffer(periodSize * maxChannels, "Sound target buffer", memory_createAlignmentAndMask(sizeof(DSR_FLOAT_VECTOR_SIZE)));
				for (int32_t p = fixedPlayers.length() - 1; p >= 0; p--) {
					SoundPlayer *player = &(fixedPlayers[p]);
					SoundBuffer *sound = &(player->soundBuffer);
					// Get samples from the player.
					player_getNextSamples(*player, playerBuffer, periodSize, 1.0 / (double)outputSampleRate);
					// TODO: Use a swap chain to update volumes for sound players without stalling.
					// TODO: Fade volume transitions by assigning soft targets to slowly move towards.
					// Apply any volume scaling.
					//if (player->leftVolume < 0.999f || player->leftVolume > 1.001f || player->rightVolume < 0.999f || player->rightVolume > 1.001f) {
					//	
					//}
					//
					if (sound_getChannelCount(*sound) == 1) { // Mono source to stereo target
						// TODO: Use a zip/shuffle operation for duplicating channels when available in hardware.
						SafePointer<float> sourceBlock = playerBuffer;
						SafePointer<float> targetBlock = target;
						const bool multiplyLeft = player->fadeLeft;
						const bool multiplyRight = player->fadeRight;
						for (int32_t t = 0; t < periodSize; t++) {
							float value = *sourceBlock;
							if (multiplyLeft) {
								targetBlock[0] += value * player->leftVolume;
							} else {
								targetBlock[0] += value;
							}
							if (multiplyRight) {
								targetBlock[1] += value * player->rightVolume;
							} else {
								targetBlock[1] += value;
							}
							sourceBlock += 1;
							targetBlock += 2;
						}
					} else if (sound_getChannelCount(*sound) == 2) { // Stereo source to stereo target
						// Accumulating sound samples with the same number of channels in and out.
						SafePointer<const float> sourceBlock = playerBuffer;
						SafePointer<float> targetBlock = target;
						if (player->fadeLeft || player->fadeRight) {
							for (int32_t t = 0; t < periodSize; t++) {
								targetBlock[0] += sourceBlock[0] * player->leftVolume;
								targetBlock[1] += sourceBlock[1] * player->rightVolume;
								sourceBlock += 2;
								targetBlock += 2;
							}
						} else {
							for (int32_t t = 0; t < periodSize * outputChannels; t += laneCountF) {
								F32xF packedSamples = F32xF::readAligned(sourceBlock, "Reading stereo sound samples");
								F32xF oldTarget = F32xF::readAligned(targetBlock, "Reading stereo sound samples");
								F32xF result = oldTarget + packedSamples;
								result.writeAligned(targetBlock, "Incrementing stereo samples");
								// Move pointers to the next block of input and output data.
								sourceBlock.increaseBytes(DSR_FLOAT_VECTOR_SIZE);
								targetBlock.increaseBytes(DSR_FLOAT_VECTOR_SIZE);
							}
						}
					} else {
						// TODO: How should more input than output channels be handled?
					}
					// Remove players that are done.
					if (player->envelope.envelopeSettings.used) {
						// Remove after fading out when an envelope is used.
						if (player->envelope.done()) {
							fixedPlayers.remove(p);
						}
					} else {
						// Remove instantly on release if there is no envelope.
						if (!(player->sustained)) {
							fixedPlayers.remove(p);
						}
					}
				}
			soundMutex.unlock();
			return soundRunning;
		});
	};
	soundFuture = std::async(std::launch::async, task);
}

void soundEngine_terminate() {
	if (soundRunning) {
		soundRunning = false;
		if (soundFuture.valid()) {
			soundFuture.wait();
		}
	}
}

void soundEngine_drawSound(dsr::ImageRgbaU8 target, const dsr::IRect &region, int32_t soundIndex, bool selected) {
	uint32_t playerCount = 0u;
	for (int32_t p = 0; p < fixedPlayers.length(); p++) {
		if (fixedPlayers[p].soundIndex == soundIndex) {
			playerCount++;
		}
	}
	draw_rectangle(target, region, selected ? ColorRgbaI32(128, 255, 128, 255) : ColorRgbaI32(40, 40, 40, 255));
	Sound *sound = &(sounds[soundIndex]);
	int32_t innerHeight = region.height() / sound_getChannelCount(sound->buffer);
	ColorRgbaI32 foreColor = selected ? ColorRgbaI32(200, 255, 200, 255) : ColorRgbaI32(200, 200, 200, 255);
	for (int32_t c = 0; c < sound_getChannelCount(sound->buffer); c++) {
		IRect innerBound = IRect(region.left() + 1, region.top() + 1, region.width() - 2, innerHeight - 2);
		draw_rectangle(target, innerBound, playerCount ? ColorRgbaI32(40, 40, 0, 255) : selected ? ColorRgbaI32(0, 0, 0, 255) : ColorRgbaI32(20, 20, 20, 255));
		double strideX = ((double)sound_getSamplesPerChannel(sound->buffer) - 1.0) / (double)innerBound.width();
		double scale = innerBound.height() * 0.5;
		double center = innerBound.top() + scale;
		draw_line(target, innerBound.left(), center, innerBound.right() - 1, center, ColorRgbaI32(0, 0, 255, 255));
		if (strideX > 1.0) {
			double startSample = 0.0;
			double endSample = strideX;
			for (int32_t x = innerBound.left(); x < innerBound.right(); x++) {
				float minimum = 1.0, maximum = -1.0;
				// TODO: Switch between min-max sampling (denser) and linear interpolation (sparser)
				sound->sampleMinMax(minimum, maximum, (int32_t)startSample, (int32_t)endSample, c);
				draw_line(target, x, center - (minimum * scale), x, center - (maximum * scale), foreColor);
				startSample = endSample;
				endSample = endSample + strideX;
			}
		} else {
			double sampleX = 0.0;
			for (int32_t x = innerBound.left(); x < innerBound.right(); x++) {
				float valueLeft = sound->sampleLinear_clamped(sampleX, c);
				sampleX += strideX;
				float valueRight = sound->sampleLinear_clamped(sampleX, c);
				draw_line(target, x, center - (valueLeft * scale), x, center - (valueRight * scale), foreColor);
			}
		}
	}
	// Draw a location for each player using the sound.
	double pixelsPerSample = (double)(region.width()) / (double)sound_getSamplesPerChannel(sound->buffer);
	for (int32_t p = 0; p < fixedPlayers.length(); p++) {
		SoundPlayer *player = &(fixedPlayers[p]);
		if (player->soundIndex == soundIndex) {
			// TODO: Display a line along the sound for each player.
			int32_t pixelX = region.left() + int32_t(double(player->location) * pixelsPerSample);
			draw_line(target, pixelX, region.top(), pixelX, region.bottom(), foreColor);
			playerCount++;
		}
	}
	font_printLine(target, font_getDefault(), sound->name, IVector2D(region.left() + 5, region.top() + 5), foreColor);
}

int32_t soundEngine_insertSoundBuffer(const SoundBuffer &buffer, const ReadableString &name, bool fromFile) {
	return sounds.pushConstructGetIndex(buffer, name, fromFile);
}

SoundBuffer soundEngine_getSound(int32_t soundIndex) {
	if (soundIndex < 0 || soundIndex >= sounds.length()) {
		return SoundBuffer();
	} else {
		return sounds[soundIndex].buffer;
	}
}

}
