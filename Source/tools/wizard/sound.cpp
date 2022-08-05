
#include "sound.h"
#include "../../soundManagers/soundManagers.h"

using namespace dsr;

#include <future>
#include <atomic>

static const int outputChannels = 2;
static const int outputSampleRate = 44100;
double outputSoundStep = 1.0 / (double)outputSampleRate;
double shortestTime = outputSoundStep * 0.01;

std::future<void> soundFuture;
static std::atomic<bool> soundRunning{true};
static std::mutex soundMutex;

static int soundFormatSize(int soundFormat) {
	if (soundFormat == soundFormat_I16) {
		return 2;
	} else if (soundFormat == soundFormat_F32) {
		return 4;
	} else {
		throwError("Cannot get size of unknown sound format!\n");
		return 0;
	}
}

static void minMax(float &minimum, float &maximum, float value) {
	if (value < minimum) { minimum = value; }
	if (value > maximum) { maximum = value; }
}

struct Sound {
	String name;
	bool fromFile;
	int sampleCount;
	int sampleRate;
	Buffer samples;
	int channelCount;
	int soundFormat;
	Sound(const ReadableString &name, bool fromFile, int sampleCount, int sampleRate, int channelCount, int soundFormat)
	: name(name), fromFile(fromFile), sampleCount(sampleCount), sampleRate(sampleRate), samples(buffer_create(sampleCount * channelCount * soundFormatSize(soundFormat))), channelCount(channelCount), soundFormat(soundFormat) {}
	float sampleLinear(int64_t floor, int64_t ceiling, double ratio, int channel) {
		int bufferIndexF = floor * this->channelCount + channel;
		int bufferIndexC = ceiling * this->channelCount + channel;
		float a = 0.0, b = 0.0;
		if (this->soundFormat == soundFormat_I16) {
			SafePointer<int16_t> source = buffer_getSafeData<int16_t>(this->samples, "I16 source sound buffer in sampleLinear");
			a = sound_convertI16ToF32(source[bufferIndexF]);
			b = sound_convertI16ToF32(source[bufferIndexC]);
		} else if (this->soundFormat == soundFormat_F32) {
			SafePointer<float> source = buffer_getSafeData<float>(this->samples, "F32 source sound buffer in sampleLinear");
			a = source[bufferIndexF];
			b = source[bufferIndexC];
		}
		return b * ratio + a * (1.0 - ratio);
	}
	float sampleLinear_cyclic(double location, int channel) {
		int64_t truncated = (int64_t)location;
		int64_t floor = truncated % this->sampleCount;
		int64_t ceiling = floor + 1; if (ceiling == sampleCount) { ceiling = 0; }
		double ratio = location - truncated;
		return this->sampleLinear(floor, ceiling, ratio, channel);
	}
	float sampleLinear_clamped(double location, int channel) {
		int64_t truncated = (int64_t)location;
		int64_t floor = truncated; if (floor >= sampleCount) { floor = sampleCount - 1; }
		int64_t ceiling = floor + 1; if (ceiling >= sampleCount) { ceiling = sampleCount - 1; }
		double ratio = location - truncated;
		return this->sampleLinear(floor, ceiling, ratio, channel);
	}
	void sampleMinMax(float &minimum, float &maximum, int startSample, int endSample, int channel) {
		if (startSample < 0) { startSample = 0; }
		if (endSample >= this->sampleCount) { endSample = this->sampleCount - 1; }
		if (channel < 0) { channel = 0; }
		if (channel >= this->channelCount) { channel = this->channelCount - 1; }
		int bufferIndex = startSample * this->channelCount + channel;
		if (this->soundFormat == soundFormat_I16) {
			SafePointer<int16_t> source = buffer_getSafeData<int16_t>(this->samples, "I16 source sound buffer in sampleMinMax");
			for (int s = startSample; s <= endSample; s++) {
				minMax(minimum, maximum, sound_convertI16ToF32(source[bufferIndex]));
				bufferIndex += this->channelCount;
			}
		} else if (this->soundFormat == soundFormat_F32) {
			SafePointer<float> source = buffer_getSafeData<float>(this->samples, "F32 source sound buffer in sampleMinMax");
			for (int s = startSample; s <= endSample; s++) {
				minMax(minimum, maximum, source[bufferIndex]);
				bufferIndex += this->channelCount;
			}
		}
	}
};
List<Sound> sounds;
static int createEmptySoundBuffer(const ReadableString &name, bool fromFile, int sampleCount, int sampleRate, int channelCount, int soundFormat) {
	if (sampleCount < 1) { throwError("Cannot create sound buffer without and length!\n");}
	if (channelCount < 1) { throwError("Cannot create sound buffer without any channels!\n");}
	if (sampleRate < 1) { throwError("Cannot create sound buffer without any sample rate!\n");}
	sounds.pushConstruct(name, fromFile, sampleCount, sampleRate, channelCount, soundFormat);
	return sounds.length() - 1;
}
int generateMonoSoundBuffer(const ReadableString &name, int sampleCount, int sampleRate, int soundFormat, std::function<double(double time)> generator) {
	int result = createEmptySoundBuffer(name, false, sampleCount, sampleRate, 1, soundFormat);
	double time = 0.0;
	double soundStep = 1.0 / (double)sampleRate;
	if (soundFormat == soundFormat_I16) {
		SafePointer<int16_t> target = buffer_getSafeData<int16_t>(sounds.last().samples, "I16 target sound buffer");
		for (int s = 0; s < sampleCount; s++) {
			target[s] = sound_convertF32ToI16(generator(time));
			time += soundStep;
		}
	} else if (soundFormat == soundFormat_F32) {
		SafePointer<float> target = buffer_getSafeData<float>(sounds.last().samples, "F32 target sound buffer");
		for (int s = 0; s < sampleCount; s++) {
			target[s] = generator(time);
			time += soundStep;
		}
	}
	return result;
}

uint16_t readU16LE(const SafePointer<uint8_t> source, int firstByteIndex) {
	return ((uint16_t)source[firstByteIndex])
	     | ((uint16_t)source[firstByteIndex + 1] << 8);
}

uint32_t readU32LE(const SafePointer<uint8_t> source, int firstByteIndex) {
	return ((uint32_t)source[firstByteIndex])
	     | ((uint32_t)source[firstByteIndex + 1] << 8)
	     | ((uint32_t)source[firstByteIndex + 2] << 16)
	     | ((uint32_t)source[firstByteIndex + 3] << 24);
}

/*struct WaveHeader {
	char chunkId[4]; // @0 RIFF
	uint32_t chunkSize; //@ 4
	char format[4]; // @ 8 WAVE
	char subChunkId[4]; // @ 12 fmt
	uint32_t subChunkSize; // @ 16
	uint16_t audioFormat; // @ 20
	uint16_t numChannels; // @ 22
	uint32_t sampleRate; // @ 24
	uint32_t bytesPerSecond; // @ 28
	uint16_t blockAlign; // @ 32
	uint16_t bitsPerSample; // @ 34
	char dataChunkId[4]; // @ 36
	uint32_t dataSize; // @ 40
};*/
static const int waveFileHeaderOffset_chunkId = 0;
static const int waveFileHeaderOffset_chunkSize = 4;
static const int waveFileHeaderOffset_format = 8;
static const int waveFileHeaderOffset_subChunkId = 12;
static const int waveFileHeaderOffset_subChunkSize = 16;
static const int waveFileHeaderOffset_audioFormat = 20;
static const int waveFileHeaderOffset_numChannels = 22;
static const int waveFileHeaderOffset_sampleRate = 24;
static const int waveFileHeaderOffset_bytesPerSecond = 28;
static const int waveFileHeaderOffset_blockAlign = 32;
static const int waveFileHeaderOffset_bitsPerSample = 34;
static const int waveFileHeaderOffset_dataChunkId = 36;
static const int waveFileHeaderOffset_dataSize = 40;
static const int waveFileDataOffset = 44;
int loadWaveSoundFromBuffer(const ReadableString &name, Buffer buffer) {
	SafePointer<uint8_t> fileContent = buffer_getSafeData<uint8_t>(buffer, "Wave file buffer");
	//uint32_t chunkSize = readU32LE(fileContent, waveFileHeaderOffset_chunkSize);
	uint32_t subChunkSize = readU32LE(fileContent, waveFileHeaderOffset_subChunkSize);
	uint16_t audioFormat = readU16LE(fileContent, waveFileHeaderOffset_audioFormat);
	uint16_t numChannels = readU16LE(fileContent, waveFileHeaderOffset_numChannels);
	uint32_t sampleRate = readU32LE(fileContent, waveFileHeaderOffset_sampleRate);
	//uint32_t bytesPerSecond = readU32LE(fileContent, waveFileHeaderOffset_bytesPerSecond);
	//uint16_t blockAlign = readU16LE(fileContent, waveFileHeaderOffset_blockAlign);
	//uint16_t bitsPerSample = readU16LE(fileContent, waveFileHeaderOffset_bitsPerSample);
	uint32_t dataSize = readU32LE(fileContent, waveFileHeaderOffset_dataSize);
	if (audioFormat != 1) { // Only PCM format supported
		throwError(U"Unhandled audio format ", audioFormat, " in wave file.\n"); return -1;
	}
	int result = -1;
	if (subChunkSize == 16) {
		if (dataSize > (buffer_getSize(buffer) - waveFileDataOffset)) {
			throwError(U"Data size out of bound in wave file.\n"); return -1;
		}
		int totalSamples = dataSize / 2; // Safer to calculate length from the file's size
		result = createEmptySoundBuffer(name, true, totalSamples, sampleRate, numChannels, soundFormat_I16);
		SafePointer<int16_t> target = buffer_getSafeData<int16_t>(sounds.last().samples, "I16 target sound buffer");
		SafePointer<int16_t> waveContent = buffer_getSafeData<int16_t>(buffer, "Wave file buffer");
		waveContent.increaseBytes(waveFileDataOffset);
		for (int s = 0; s < totalSamples; s ++) {
			target[s] = waveContent[s]; // This part has to assume little endian because the value is signed. :(
		}
	} else {
		throwError(U"Unsupported bit depth ", audioFormat, " in wave file.\n"); return -1;
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
	// Assuming the wave format until more are supported.
	return loadWaveSoundFromBuffer(filename, file_loadBuffer(filename, mustExist));
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

void drawSound(dsr::ImageRgbaU8 target, const dsr::IRect &region, int soundIndex) {
	draw_rectangle(target, region, ColorRgbaI32(128, 128, 128, 255));
	Sound *sound = &(sounds[soundIndex]);
	int innerHeight = region.height() / sound->channelCount;
	for (int c = 0; c < sound->channelCount; c++) {
		IRect innerBound = IRect(region.left() + 1, region.top() + 1, region.width() - 2, innerHeight - 2);
		draw_rectangle(target, innerBound, ColorRgbaI32(0, 0, 0, 255));
		double strideX = ((double)sound->sampleCount - 1.0) / (double)innerBound.width();	
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
				draw_line(target, x, center - (minimum * scale), x, center - (maximum * scale), ColorRgbaI32(255, 255, 255, 255));
				startSample = endSample;
				endSample = endSample + strideX;
			}
		} else {
			double sampleX = 0.0;
			for (int x = innerBound.left(); x < innerBound.right(); x++) {
				float valueLeft = sound->sampleLinear_clamped(sampleX, c);
				sampleX += strideX;
				float valueRight = sound->sampleLinear_clamped(sampleX, c);
				draw_line(target, x, center - (valueLeft * scale), x, center - (valueRight * scale), ColorRgbaI32(255, 255, 255, 255));
			}
		}
	}
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
		sound_streamToSpeakers(outputChannels, outputSampleRate, [](float *target, int requestedSamples) -> bool {
			// Anyone wanting to change the played sounds from another thread will have to wait until this section has finished processing
			soundMutex.lock();
				// TODO: Create a graph of filters for different instruments
				// TODO: Let the output buffer be just another sound buffer, so that a reusable function can stream to sections of larger sound buffers
				for (int p = players.length() - 1; p >= 0; p--) {
					Player *player = &(players[p]);
					int soundIndex = player->soundIndex;
					Sound *sound = &(sounds[soundIndex]);
					int sourceSampleCount = sound->sampleCount;
					double sampleStep = player->speed * sound->sampleRate * outputSoundStep;
					if (player->repeat) {
						if (sound->channelCount == 1) { // Mono source
							for (int t = 0; t < requestedSamples; t++) {
								PREPARE_SAMPLE
								float monoSource = sound->sampleLinear_cyclic(player->location, 0) * envelope;
								target[t * outputChannels + 0] += monoSource * player->leftVolume;
								target[t * outputChannels + 1] += monoSource * player->rightVolume;
								NEXT_SAMPLE_CYCLIC
							}
						} else if (sound->channelCount == 2) { // Stereo source
							for (int t = 0; t < requestedSamples; t++) {
								PREPARE_SAMPLE
								target[t * outputChannels + 0] += sound->sampleLinear_cyclic(player->location, 0) * envelope * player->leftVolume;
								target[t * outputChannels + 1] += sound->sampleLinear_cyclic(player->location, 1) * envelope * player->rightVolume;
								NEXT_SAMPLE_CYCLIC
							}
						}
					} else {
						if (sound->channelCount == 1) { // Mono source
							for (int t = 0; t < requestedSamples; t++) {
								PREPARE_SAMPLE
								float monoSource = sound->sampleLinear_clamped(player->location, 0) * envelope;
								target[t * outputChannels + 0] += monoSource * player->leftVolume;
								target[t * outputChannels + 1] += monoSource * player->rightVolume;
								NEXT_SAMPLE_ONCE
							}
						} else if (sound->channelCount == 2) { // Stereo source
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
