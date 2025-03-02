
#include "sound.h"
#include "../../DFPSR/api/soundAPI.h"

#include <future>
#include <atomic>

namespace dsr {

static const int outputChannels = 2;
static const int outputSampleRate = 44100;
double outputSoundStep = 1.0 / (double)outputSampleRate;
double shortestTime = outputSoundStep * 0.01;

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
	void sampleMinMax(float &minimum, float &maximum, int startSample, int endSample, int channel) {
		if (startSample < 0) { startSample = 0; }
		if (endSample >= sound_getSamplesPerChannel(this->buffer)) { endSample = sound_getSamplesPerChannel(this->buffer) - 1; }
		if (channel < 0) { channel = 0; }
		if (channel >= sound_getChannelCount(this->buffer)) { channel = sound_getChannelCount(this->buffer) - 1; }
		int bufferIndex = startSample * sound_getChannelCount(this->buffer) + channel;
		SafePointer<float> source = sound_getSafePointer(this->buffer);
		for (int s = startSample; s <= endSample; s++) {
			minMax(minimum, maximum, source[bufferIndex]);
			bufferIndex += sound_getChannelCount(this->buffer);
		}
	}
};
List<Sound> sounds;
static int createEmptySoundBuffer(const ReadableString &name, bool fromFile, int samplesPerChannel, int sampleRate, int channelCount) {
	if (samplesPerChannel < 1) { throwError("Cannot create sound buffer without and length!\n");}
	if (channelCount < 1) { throwError("Cannot create sound buffer without any channels!\n");}
	if (sampleRate < 1) { throwError("Cannot create sound buffer without any sample rate!\n");}
	return sounds.pushConstructGetIndex(name, fromFile, samplesPerChannel, channelCount, sampleRate);
}
int generateMonoSoundBuffer(const ReadableString &name, int samplesPerChannel, int sampleRate, std::function<float(double time)> generator) {
	int result = createEmptySoundBuffer(name, false, samplesPerChannel, sampleRate, 1);
	double time = 0.0;
	double soundStep = 1.0 / (double)sampleRate;
	SafePointer<float> target = sound_getSafePointer(sounds.last().buffer);
	for (int s = 0; s < samplesPerChannel; s++) {
		target[s] = generator(time);
		time += soundStep;
	}
	return result;
}

int loadSoundFromFile(const ReadableString &filename, bool mustExist) {
	// Try to reuse any previously instance of the file before accessing the file system
	for (int s = 0; s < sounds.length(); s++) {
		if (sounds[s].fromFile && string_match(sounds[s].name, filename)) {
			return s;
		}
	}
	return sounds.pushConstructGetIndex(Sound(sound_decode_RiffWave(file_loadBuffer(filename, mustExist)), filename, true));
}

int getSoundBufferCount() {
	return sounds.length();
}

EnvelopeSettings::EnvelopeSettings()
: attack(0.0), decay(0.0), sustain(1.0), release(0.0), hold(0.0), rise(0.0), sustainedSmooth(0.0), releasedSmooth(0.0) {}

EnvelopeSettings::EnvelopeSettings(double attack, double decay, double sustain, double release, double hold, double rise, double sustainedSmooth, double releasedSmooth)
: attack(attack), decay(decay), sustain(sustain), release(release), hold(hold), rise(rise), sustainedSmooth(sustainedSmooth), releasedSmooth(releasedSmooth) {}

static double closerLinear(double &ref, double goal, double maxStep) {
	double difference;
	if (ref + maxStep < goal) {
		difference = maxStep;
		ref += maxStep;
	} else if (ref - maxStep > goal) {
		difference = -maxStep;
		ref -= maxStep;
	} else {
		difference = goal - ref;
		ref = goal;
	}
	return difference;
}

struct Envelope {
	// Settings
	EnvelopeSettings envelopeSettings;
	// TODO: Add different types of smoothing filters and interpolation methods
	// Dynamic
	int state = 0;
	double currentVolume = 0.0, currentGoal = 0.0, releaseVolume = 0.0, timeSinceChange = 0.0;
	bool lastSustained = true;
	Envelope(const EnvelopeSettings &envelopeSettings)
	: envelopeSettings(envelopeSettings) {
		// Avoiding division by zero using very short fades
		if (this->envelopeSettings.attack < shortestTime) { this->envelopeSettings.attack = shortestTime; }
		if (this->envelopeSettings.hold < shortestTime) { this->envelopeSettings.hold = shortestTime; }
		if (this->envelopeSettings.decay < shortestTime) { this->envelopeSettings.decay = shortestTime; }
		if (this->envelopeSettings.release < shortestTime) { this->envelopeSettings.release = shortestTime; }
	}
	double getVolume(bool sustained, double seconds) {
		if (sustained) {
			if (state == 0) {
				// Attack
				this->currentGoal += seconds / this->envelopeSettings.attack;
				if (this->currentGoal > 1.0) {
					this->currentGoal = 1.0;
					state = 1; this->timeSinceChange = 0.0;
				}
			} else if (state == 1) {
				// Hold
				if (this->timeSinceChange < this->envelopeSettings.hold) {
					this->currentGoal = 1.0;
				} else {
					state = 2; this->timeSinceChange = 0.0;
				}
			} else if (state == 2) {
				// Decay
				this->currentGoal += (this->envelopeSettings.sustain - 1.0) * seconds / this->envelopeSettings.decay;
				if (this->currentGoal < this->envelopeSettings.sustain) {
					this->currentGoal = this->envelopeSettings.sustain;
					state = 3; this->timeSinceChange = 0.0;
				}
			} else if (state == 3) {
				// Sustain / rise
				this->currentGoal += this->envelopeSettings.rise * seconds / this->envelopeSettings.decay;
				if (this->currentGoal < 0.0) {
					this->currentGoal = 0.0;
				} else if (this->currentGoal > 1.0) {
					this->currentGoal = 1.0;
				}
			}
		} else {
			// Release
			if (this->lastSustained) {
				this->releaseVolume = this->currentGoal;
			}
			// Linear release, using releaseVolume to calculate the slope needed for the current release time
			this->currentGoal -= this->releaseVolume * seconds / this->envelopeSettings.release;
			if (this->currentGoal < 0.0) {
				this->currentGoal = 0.0;
			}
			this->lastSustained = false;
		}
		double smooth = sustained ? this->envelopeSettings.sustainedSmooth : this->envelopeSettings.releasedSmooth;
		if (smooth > 0.0) {
			// Move faster to the goal the further away it is
			double change = seconds / smooth;
			if (change > 1.0) { change = 1.0; }
			double keep = 1.0 - change;
			this->currentVolume = this->currentVolume * keep + this->currentGoal * change;
			// Move slowly towards the goal with a fixed speed to finally reach zero and stop sampling the sound
			closerLinear(this->currentVolume, this->currentGoal, seconds * 0.01);
		} else {
			this->currentVolume = this->currentGoal;
		}
		this->timeSinceChange += seconds;
		return this->currentVolume;
	}
	bool done() {
		return this->currentVolume <= 0.0000000001 && !this->lastSustained;
	}
};

// Currently playing sounds
struct Player {
	// Unique identifier
	int64_t playerID;
	// Assigned from instrument
	int soundIndex;
	Envelope envelope;
	bool repeat;
	double leftVolume, rightVolume;
	double speed; // TODO: Use for playing with interpolation
	double location = 0; // Floating sample index
	bool sustained = true; // If the sound is still being generated
	Player(int64_t playerID, int soundIndex, bool repeat, double leftVolume, double rightVolume, double speed, const EnvelopeSettings &envelopeSettings)
	: playerID(playerID), soundIndex(soundIndex), envelope(envelopeSettings), repeat(repeat), leftVolume(leftVolume), rightVolume(rightVolume), speed(speed) {}
};
List<Player> players;
int64_t nextPlayerID = 0;
int playSound(int soundIndex, bool repeat, double leftVolume, double rightVolume, double speed, const EnvelopeSettings &envelopeSettings) {
	int result;
	soundMutex.lock();
		result = nextPlayerID;
		players.pushConstruct(nextPlayerID, soundIndex, repeat, leftVolume, rightVolume, speed, envelopeSettings);
		nextPlayerID++;
	soundMutex.unlock();
	return result;
}
int playSound(int soundIndex, bool repeat, double leftVolume, double rightVolume, double speed) {
	return playSound(soundIndex, repeat, leftVolume, rightVolume, speed, EnvelopeSettings());
}
static int findSound(int64_t playerID) {
	for (int p = 0; p < players.length(); p++) {
		if (players[p].playerID == playerID) {
			return p;
		}
	}
	return -1;
}
void releaseSound(int64_t playerID) {
	if (playerID != -1) {
		soundMutex.lock();
			int index = findSound(playerID);
			if (index > -1) {
				players[index].sustained = false;;
			}
		soundMutex.unlock();
	}
}
void stopSound(int64_t playerID) {
	if (playerID != -1) {
		soundMutex.lock();
			int index = findSound(playerID);
			if (index > -1) {
				players.remove(index);
			}
		soundMutex.unlock();
	}
}
void stopAllSounds() {
	soundMutex.lock();
		players.clear();
	soundMutex.unlock();
}

#define PREPARE_SAMPLE \
	double envelope = player->envelope.getVolume(player->sustained, outputSoundStep);
#define NEXT_SAMPLE_CYCLIC \
	player->location += sampleStep; \
	if (player->location >= sourceSampleCount) { \
		player->location -= sourceSampleCount; \
	} \
	if (player->envelope.done()) { \
		players.remove(p); \
		break; \
	}
#define NEXT_SAMPLE_ONCE \
	player->location += sampleStep; \
	if (player->location >= sourceSampleCount) { \
		players.remove(p); \
		break; \
	} \
	if (player->envelope.done()) { \
		players.remove(p); \
		break; \
	}

void sound_initialize() {
	// Start a worker thread mixing sounds in realtime
	std::function<void()> task = []() {
		sound_streamToSpeakers(outputChannels, outputSampleRate, [](SafePointer<float> target, int requestedSamples) -> bool {
			// Anyone wanting to change the played sounds from another thread will have to wait until this section has finished processing
			soundMutex.lock();
				// TODO: Create a graph of filters for different instruments
				// TODO: Let the output buffer be just another sound buffer, so that a reusable function can stream to sections of larger sound buffers
				for (int p = players.length() - 1; p >= 0; p--) {
					Player *player = &(players[p]);
					int soundIndex = player->soundIndex;
					Sound *sound = &(sounds[soundIndex]);
					int sourceSampleCount = sound_getSamplesPerChannel(sound->buffer);
					double sampleStep = player->speed * sound_getSampleRate(sound->buffer) * outputSoundStep;
					if (player->repeat) {
						if (sound_getChannelCount(sound->buffer) == 1) { // Mono source
							for (int t = 0; t < requestedSamples; t++) {
								PREPARE_SAMPLE
								float monoSource = sound->sampleLinear_cyclic(player->location, 0) * envelope;
								target[t * outputChannels + 0] += monoSource * player->leftVolume;
								target[t * outputChannels + 1] += monoSource * player->rightVolume;
								NEXT_SAMPLE_CYCLIC
							}
						} else if (sound_getChannelCount(sound->buffer) == 2) { // Stereo source
							for (int t = 0; t < requestedSamples; t++) {
								PREPARE_SAMPLE
								target[t * outputChannels + 0] += sound->sampleLinear_cyclic(player->location, 0) * envelope * player->leftVolume;
								target[t * outputChannels + 1] += sound->sampleLinear_cyclic(player->location, 1) * envelope * player->rightVolume;
								NEXT_SAMPLE_CYCLIC
							}
						}
					} else {
						if (sound_getChannelCount(sound->buffer) == 1) { // Mono source
							for (int t = 0; t < requestedSamples; t++) {
								PREPARE_SAMPLE
								float monoSource = sound->sampleLinear_clamped(player->location, 0) * envelope;
								target[t * outputChannels + 0] += monoSource * player->leftVolume;
								target[t * outputChannels + 1] += monoSource * player->rightVolume;
								NEXT_SAMPLE_ONCE
							}
						} else if (sound_getChannelCount(sound->buffer) == 2) { // Stereo source
							for (int t = 0; t < requestedSamples; t++) {
								PREPARE_SAMPLE
								target[t * outputChannels + 0] += sound->sampleLinear_clamped(player->location, 0) * envelope * player->leftVolume;
								target[t * outputChannels + 1] += sound->sampleLinear_clamped(player->location, 1) * envelope * player->rightVolume;
								NEXT_SAMPLE_ONCE
							}
						}
					}
				}
			soundMutex.unlock();
			return soundRunning;
		});
	};
	soundFuture = std::async(std::launch::async, task);
}

void sound_terminate() {
	if (soundRunning) {
		soundRunning = false;
		if (soundFuture.valid()) {
			soundFuture.wait();
		}
	}
}

void drawEnvelope(ImageRgbaU8 target, const IRect &region, const EnvelopeSettings &envelopeSettings, double releaseTime, double viewTime) {
	int top = region.top();
	int bottom = region.bottom() - 1;
	Envelope envelope = Envelope(envelopeSettings);
	double secondsPerPixel = viewTime / region.width();
	draw_rectangle(target, region, ColorRgbaI32(0, 0, 0, 255));
	draw_rectangle(target, IRect(region.left(), region.top(), region.width() * (releaseTime / viewTime), region.height() / 8), ColorRgbaI32(0, 128, 128, 255));
	int oldHardY = bottom;
	for (int s = 0; s < region.width(); s++) {
		int x = s + region.left();
		double time = s * secondsPerPixel;
		double smoothLevel = envelope.getVolume(time < releaseTime, secondsPerPixel);
		double hardLevel = envelope.currentGoal;
		if (envelope.done()) {
			draw_line(target, x, top, x, (top * 7 + bottom) / 8, ColorRgbaI32(128, 0, 0, 255));
		} else {
			draw_line(target, x, (top * smoothLevel) + (bottom * (1.0 - smoothLevel)), x, bottom, ColorRgbaI32(64, 64, 0, 255));
			int hardY = (top * hardLevel) + (bottom * (1.0 - hardLevel));
			draw_line(target, x, oldHardY, x, hardY, ColorRgbaI32(255, 255, 255, 255));
			oldHardY = hardY;
		}
	}
}

void drawSound(dsr::ImageRgbaU8 target, const dsr::IRect &region, int soundIndex, bool selected) {
	draw_rectangle(target, region, selected ? ColorRgbaI32(128, 255, 128, 255) : ColorRgbaI32(40, 40, 40, 255));
	Sound *sound = &(sounds[soundIndex]);
	int innerHeight = region.height() / sound_getChannelCount(sound->buffer);
	ColorRgbaI32 foreColor = selected ? ColorRgbaI32(200, 255, 200, 255) : ColorRgbaI32(200, 200, 200, 255);
	for (int c = 0; c < sound_getChannelCount(sound->buffer); c++) {
		IRect innerBound = IRect(region.left() + 1, region.top() + 1, region.width() - 2, innerHeight - 2);
		draw_rectangle(target, innerBound, selected ? ColorRgbaI32(0, 0, 0, 255) : ColorRgbaI32(20, 20, 20, 255));
		double strideX = ((double)sound_getSamplesPerChannel(sound->buffer) - 1.0) / (double)innerBound.width();
		double scale = innerBound.height() * 0.5;
		double center = innerBound.top() + scale;
		draw_line(target, innerBound.left(), center, innerBound.right() - 1, center, ColorRgbaI32(0, 0, 255, 255));
		if (strideX > 1.0) {
			double startSample = 0.0;
			double endSample = strideX;
			for (int x = innerBound.left(); x < innerBound.right(); x++) {
				float minimum = 1.0, maximum = -1.0;
				// TODO: Switch between min-max sampling (denser) and linear interpolation (sparser)
				sound->sampleMinMax(minimum, maximum, (int)startSample, (int)endSample, c);
				draw_line(target, x, center - (minimum * scale), x, center - (maximum * scale), foreColor);
				startSample = endSample;
				endSample = endSample + strideX;
			}
		} else {
			double sampleX = 0.0;
			for (int x = innerBound.left(); x < innerBound.right(); x++) {
				float valueLeft = sound->sampleLinear_clamped(sampleX, c);
				sampleX += strideX;
				float valueRight = sound->sampleLinear_clamped(sampleX, c);
				draw_line(target, x, center - (valueLeft * scale), x, center - (valueRight * scale), foreColor);
			}
		}
	}
	font_printLine(target, font_getDefault(), sound->name, IVector2D(region.left() + 5, region.top() + 5), foreColor);
}

}
