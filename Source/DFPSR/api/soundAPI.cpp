
#include "soundAPI.h"
#include "fileAPI.h"
#include "../settings.h"
#include "../base/format.h"
#include "../base/noSimd.h"

namespace dsr {

// See the Source/soundManagers folder for implementations of sound_streamToSpeakers for different operating systems.

bool sound_streamToSpeakers_fixed(int32_t channels, int32_t sampleRate, int32_t periodSamplesPerChannel, std::function<bool(SafePointer<float> fixedTarget)> soundOutput) {
	int32_t bufferSamplesPerChannel = 0;
	int32_t blockBytes = channels * sizeof(float);
	Buffer fixedBuffer;
	SafePointer<float> bufferPointer;
	int32_t writeLocation = 0;
	int32_t readLocation = 0;
	return sound_streamToSpeakers(channels, sampleRate, [&](SafePointer<float> dynamicTarget, int32_t requestedSamplesPerChannel) -> bool {
		// When running for the first time, a buffer large enough for both input and output will be allocated.
		if (bufferSamplesPerChannel == 0) {
			// Calculate how much space we need as a minimum.
			int32_t minimumBufferSize = max(requestedSamplesPerChannel, periodSamplesPerChannel) * 2;
			// Find a large enough power of two buffer size.
			bufferSamplesPerChannel = 8192;
			while (bufferSamplesPerChannel < minimumBufferSize) bufferSamplesPerChannel *= 2;
			// Allocate the buffer and point to it.
			fixedBuffer = buffer_create(bufferSamplesPerChannel * blockBytes);
			bufferPointer = buffer_getSafeData<float>(fixedBuffer, "Fixed size output sound buffer");
		}
		int32_t availableSamplesPerChannel = writeLocation - readLocation;
		if (availableSamplesPerChannel < 0) availableSamplesPerChannel += bufferSamplesPerChannel;
		while (availableSamplesPerChannel < requestedSamplesPerChannel) {
			safeMemorySet(bufferPointer + (writeLocation * channels), 0, periodSamplesPerChannel * blockBytes);
			if (!soundOutput(bufferPointer + (writeLocation * channels))) {
				return false;
			}
			availableSamplesPerChannel += periodSamplesPerChannel;
			writeLocation += periodSamplesPerChannel;
			while (writeLocation >= bufferSamplesPerChannel) writeLocation -= bufferSamplesPerChannel;
		}
		int32_t readEndLocation = readLocation + requestedSamplesPerChannel;
		if (readEndLocation <= bufferSamplesPerChannel) {
			// Continuous memory.
			safeMemoryCopy(dynamicTarget, bufferPointer + (readLocation * channels), requestedSamplesPerChannel * blockBytes);
		} else {
			// Wraps around the fixed buffer's end.
			int32_t firstLength = bufferSamplesPerChannel - readLocation;
			int32_t secondLength = requestedSamplesPerChannel - firstLength;
			int32_t firstSize = firstLength * blockBytes;
			int32_t secondSize = secondLength * blockBytes;
			safeMemoryCopy(dynamicTarget, bufferPointer + (readLocation * channels), firstSize);
			safeMemoryCopy(dynamicTarget + (firstLength * channels), bufferPointer, secondSize);
		}
		readLocation = readEndLocation;
		while (readLocation >= bufferSamplesPerChannel) readLocation -= bufferSamplesPerChannel;
		return true;
	});
}

SoundBuffer::SoundBuffer(uint32_t samplesPerChannel, uint32_t channelCount, uint32_t sampleRate) {
	this->impl_samplesPerChannel = samplesPerChannel;
	if (this->impl_samplesPerChannel < 1) this->impl_samplesPerChannel = 1;
	this->impl_channelCount = channelCount;
	if (this->impl_channelCount < 1) this->impl_channelCount = 1;
	this->impl_sampleRate = sampleRate;
	if (this->impl_sampleRate < 1) this->impl_sampleRate = 1;	
	this->impl_samples = buffer_create(this->impl_samplesPerChannel * this->impl_channelCount * sizeof(float));
}

// scaleOffset 0.0 preserves the mantissa better using power of two multiplications.
// scaleOffset 1.0 allows using the full -1.0 to +1.0 range to prevent hard clipping of high values.
static double scaleOffset = 1.0f;

static double toIntegerScaleU8 = 128.0 - scaleOffset;
static double toIntegerScaleI16 = 32768.0 - scaleOffset;
static double toIntegerScaleI24 = 8388608.0 - scaleOffset;
static double toIntegerScaleI32 = 2147483648.0 - scaleOffset;
static double fromIntegerScaleU8 = 1.0 / toIntegerScaleU8;
static double fromIntegerScaleI16 = 1.0 / toIntegerScaleI16;
static double fromIntegerScaleI24 = 1.0 / toIntegerScaleI24;
static double fromIntegerScaleI32 = 1.0 / toIntegerScaleI32;

// TODO: Create a folder for implementations of sound formats.

static const int fmtOffset_audioFormat = 0;
static const int fmtOffset_channelCount = 2;
static const int fmtOffset_sampleRate = 4;
static const int fmtOffset_bytesPerSecond = 8;
static const int fmtOffset_blockAlign = 12;
static const int fmtOffset_bitsPerSample = 14;

static uint32_t getSampleBits(RiffWaveFormat format) {
	if (format == RiffWaveFormat::RawU8) {
		return 8;
	} else if (format == RiffWaveFormat::RawI16) {
		return 16;
	} else if (format == RiffWaveFormat::RawI24) {
		return 24;
	} else {
		return 32;
	}
}

static inline int64_t roundTo(double value, RoundingMethod roundingMethod) {
	if (roundingMethod == RoundingMethod::Nearest){
		return int64_t(value + (value > 0.0 ? 0.5 : -0.5));
	} else { // RoundingMethod::Truncate
		return int64_t(value);
	}
}

static inline uint8_t floatToNormalizedU8(float value, RoundingMethod roundingMethod) {
	int64_t closest = roundTo((double(value) * toIntegerScaleU8) + 128.0, roundingMethod);
	if (closest <   0) closest =   0;
	if (closest > 255) closest = 255;
	return (uint8_t)closest;
}

static inline int16_t floatToNormalizedI16(float value, RoundingMethod roundingMethod) {
	int64_t closest = roundTo(double(value) * toIntegerScaleI16, roundingMethod);
	if (closest < -32768) closest = -32768;
	if (closest >  32767) closest =  32767;
	return (int16_t)closest;
}

static inline int32_t floatToNormalizedI24(float value, RoundingMethod roundingMethod) {
	int64_t closest = roundTo(double(value) * toIntegerScaleI24, roundingMethod);
	if (closest < -8388608) closest = -8388608;
	if (closest >  8388607) closest =  8388607;
	return (int32_t)closest;
}

static inline int32_t floatToNormalizedI32(float value, RoundingMethod roundingMethod) {
	int64_t closest = roundTo(double(value) * toIntegerScaleI32, roundingMethod);
	if (closest < -2147483648) closest = -2147483648;
	if (closest >  2147483647) closest =  2147483647;
	return (int32_t)closest;
}

static inline float floatFromNormalizedU8(uint8_t value) {
	return float((double(value) - 128.0) * fromIntegerScaleU8);
}

static inline float floatFromNormalizedI16(int16_t value) {
	return float(double(value) * fromIntegerScaleI16);
}

static inline float floatFromNormalizedI24(int32_t value) {
	return float(double(value) * fromIntegerScaleI24);
}

static inline float floatFromNormalizedI32(int32_t value) {
	return float(double(value) * fromIntegerScaleI32);
}

struct Chunk {
	String name;
	SafePointer<const uint8_t> chunkStart;
	intptr_t chunkSize = 0;
	Chunk(const ReadableString &name, const Buffer &buffer)
	: name(name), chunkStart(buffer_getSafeData<uint8_t>(buffer, "Chunk buffer")), chunkSize(buffer_getSize(buffer)) {}
	Chunk(const ReadableString &name, SafePointer<const uint8_t> chunkStart, intptr_t chunkSize)
	: name(name), chunkStart(chunkStart), chunkSize(chunkSize) {}
	Chunk() {}
};

static Buffer combineRiffChunks(List<Chunk> subChunks) {
	uintptr_t payloadSize = 4u; // "WAVE"
	for (intptr_t s = 0; s < subChunks.length(); s++) {
		payloadSize += 8 + subChunks[s].chunkSize;
	}
	uintptr_t totalSize = payloadSize + 8u; // RIFF size
	Buffer result = buffer_create(totalSize);
	SafePointer<uint8_t> targetBytes = buffer_getSafeData<uint8_t>(result, "RIFF encoding target buffer");
	targetBytes[0] = 'R';
	targetBytes[1] = 'I';
	targetBytes[2] = 'F';
	targetBytes[3] = 'F';
	targetBytes += 4;
	format_writeU32_LE(targetBytes, payloadSize);
	targetBytes += 4;
	targetBytes[0] = 'W';
	targetBytes[1] = 'A';
	targetBytes[2] = 'V';
	targetBytes[3] = 'E';
	targetBytes += 4;
	for (intptr_t s = 0; s < subChunks.length(); s++) {
		uintptr_t subChunkSize = subChunks[s].chunkSize;
		targetBytes[0] = char(subChunks[s].name[0]);
		targetBytes[1] = char(subChunks[s].name[1]);
		targetBytes[2] = char(subChunks[s].name[2]);
		targetBytes[3] = char(subChunks[s].name[3]);
		targetBytes += 4;
		format_writeU32_LE(targetBytes, subChunkSize);
		targetBytes += 4;
		safeMemoryCopy(targetBytes, subChunks[s].chunkStart, subChunkSize);
		targetBytes += subChunkSize;
	}
	return result;
}

Buffer sound_encode_RiffWave(const SoundBuffer &sound, RiffWaveFormat format, RoundingMethod roundingMethod) {
	uint32_t bitsPerSample = getSampleBits(format);
	uint32_t bytesPerSample = bitsPerSample / 8;
	uint32_t channelCount = sound_getChannelCount(sound);
	uint32_t samplesPerChannel = sound_getSamplesPerChannel(sound);
	uint32_t blockAlign = channelCount * bytesPerSample;
	uint32_t dataBytes = blockAlign * samplesPerChannel;
	uint32_t sampleRate = sound_getSampleRate(sound);
	uint32_t bytesPerSecond = blockAlign * sampleRate;

	Buffer fmt = buffer_create(16);
	SafePointer<uint8_t> formatBytes = buffer_getSafeData<uint8_t>(fmt, "RIFF encoding format buffer");
	format_writeU16_LE(formatBytes + fmtOffset_audioFormat, 1); // PCM
	format_writeU16_LE(formatBytes + fmtOffset_channelCount, channelCount);
	format_writeU32_LE(formatBytes + fmtOffset_sampleRate, sampleRate);
	format_writeU32_LE(formatBytes + fmtOffset_bytesPerSecond, bytesPerSecond);
	format_writeU16_LE(formatBytes + fmtOffset_blockAlign, blockAlign);
	format_writeU16_LE(formatBytes + fmtOffset_bitsPerSample, bitsPerSample);

	Buffer data = buffer_create(dataBytes);
	SafePointer<uint8_t> target = buffer_getSafeData<uint8_t>(data, "RIFF encoding data buffer");
	SafePointer<const float> source = sound_getSafePointer(sound);
	uintptr_t totalSamples = channelCount * samplesPerChannel;
	if (format == RiffWaveFormat::RawU8) {
		for (uintptr_t s = 0; s < totalSamples; s++) {
			target[s] = floatToNormalizedU8(source[s], roundingMethod);
		}
	} else if (format == RiffWaveFormat::RawI16) {
		for (uintptr_t s = 0; s < totalSamples; s++) {
			format_writeI16_LE(target + s * bytesPerSample, floatToNormalizedI16(source[s], roundingMethod));
		}
	} else if (format == RiffWaveFormat::RawI24) {
		for (uintptr_t s = 0; s < totalSamples; s++) {
			format_writeI24_LE(target + s * bytesPerSample, floatToNormalizedI24(source[s], roundingMethod));
		}
	} else if (format == RiffWaveFormat::RawI32) {
		for (uintptr_t s = 0; s < totalSamples; s++) {
			format_writeI32_LE(target + s * bytesPerSample, floatToNormalizedI32(source[s], roundingMethod));
		}
	}
	return combineRiffChunks(List<Chunk>(Chunk(U"fmt ", fmt), Chunk(U"data", data)));
}

static String readChar4(SafePointer<const uint8_t> nameStart) {
	String name;
	for (uintptr_t b = 0; b < 4; b++) {
		string_appendChar(name, DsrChar(nameStart[b]));
	}
	return name;
}

static void getRiffChunks(const Chunk &parentChunk, std::function<void(const ReadableString &name, const Chunk &chunk)> returnChunk) {
	SafePointer<const uint8_t> chunkStart = parentChunk.chunkStart;
	SafePointer<const uint8_t> chunkEnd = chunkStart + parentChunk.chunkSize;
	while (chunkStart.getUnchecked() + 8 <= chunkEnd.getUnchecked()) {
		String name = readChar4(chunkStart);
		uint32_t chunkSize = format_readU32_LE(chunkStart + 4);
		SafePointer<const uint8_t> chunkPayload = chunkStart + 8;
		if (chunkPayload.getUnchecked() + chunkSize > chunkEnd.getUnchecked()) {
			sendWarning(U"Not enough space remaining (", uint64_t((uintptr_t)chunkEnd.getUnchecked() - (uintptr_t)chunkPayload.getUnchecked()), U" bytes) in the RIFF wave file to read the ", name, U" chunk of ", chunkSize, U" bytes!\n");
			return;
		}
		returnChunk(name, Chunk(name, chunkPayload, chunkSize));
		chunkStart = chunkStart + 8 + chunkSize;
	}
}

static void getRiffChunks(const Buffer &fileBuffer, std::function<void(const ReadableString &name, const Chunk &chunk)> returnChunk) {
	Chunk rootChunk = Chunk(U"RIFF", fileBuffer);
	getRiffChunks(rootChunk, [&returnChunk](const ReadableString &name, const Chunk &chunk) {
		if (string_match(name, U"RIFF")) {
			if (!string_match(readChar4(chunk.chunkStart), U"WAVE")) {
				throwError(U"WAVE format expected in RIFF file!\n");
			}
			getRiffChunks(Chunk(name, chunk.chunkStart + 4, chunk.chunkSize - 4), returnChunk);
		}
	});
}

SoundBuffer sound_decode_RiffWave(const Buffer &fileBuffer) {
	Chunk fmtChunk;
	Chunk dataChunk;
	bool hasFmt = false;
	bool hasData = false;
	SafePointer<uint8_t> bufferStart = buffer_getSafeData<uint8_t>(fileBuffer, "File buffer");
	getRiffChunks(fileBuffer, [&bufferStart, &fmtChunk, &hasFmt, &dataChunk, &hasData](const ReadableString &name, const Chunk &chunk) {
		if (string_match(name, U"fmt ")) {
			fmtChunk = chunk;
			hasFmt = true;
		} else if (string_match(name, U"data")) {
			dataChunk = chunk;
			hasData = true;
		}
	});
	if (!hasFmt || !hasData) {
		if (!hasFmt) {
			sendWarning(U"Failed to find any fmt chunk in the RIFF wave file!\n");
		}
		if (!hasData) {
			sendWarning(U"Failed to find any data chunk in the RIFF wave file!\n");
		}
		return SoundBuffer();
	}
	if (fmtChunk.chunkSize < 16) {
		sendWarning(U"The fmt chunk of ", fmtChunk.chunkSize, U" bytes is not large enough in the RIFF wave file!\n");
		return SoundBuffer();
	}
	uintptr_t audioFormat      = format_readU16_LE(fmtChunk.chunkStart + fmtOffset_audioFormat);
	uintptr_t channelCount     = format_readU16_LE(fmtChunk.chunkStart + fmtOffset_channelCount);
	uintptr_t sampleRate       = format_readU32_LE(fmtChunk.chunkStart + fmtOffset_sampleRate);
	//uintptr_t bytesPerSecond = format_readU32_LE(fmtChunk.chunkStart + fmtOffset_bytesPerSecond);
	uintptr_t blockAlign       = format_readU16_LE(fmtChunk.chunkStart + fmtOffset_blockAlign);
	uintptr_t bitsPerSample    = format_readU16_LE(fmtChunk.chunkStart + fmtOffset_bitsPerSample);
	uintptr_t bytesPerSample   = bitsPerSample / 8;
	uintptr_t dataSize         = dataChunk.chunkSize;
	uintptr_t blockCount = dataSize / blockAlign;
	SoundBuffer result = SoundBuffer(blockCount, channelCount, sampleRate);
	SafePointer<float> target = sound_getSafePointer(result);
	SafePointer<const uint8_t> waveContent = dataChunk.chunkStart;
	if (audioFormat == 1 && bitsPerSample == 8) {
		for (uintptr_t b = 0; b < blockCount; b++) {
			for (uintptr_t c = 0; c < channelCount; c++) {
				*target = floatFromNormalizedU8(waveContent[c]);
				target += 1;
			}
			waveContent += blockAlign;
		}
		return result;
	} else if (audioFormat == 1 && bitsPerSample == 16) {
		for (uintptr_t b = 0; b < blockCount; b++) {
			for (uintptr_t c = 0; c < channelCount; c++) {
				*target = floatFromNormalizedI16(format_readI16_LE(waveContent + c * bytesPerSample));
				target += 1;
			}
			waveContent += blockAlign;
		}
		return result;
	} else if (audioFormat == 1 && bitsPerSample == 24) {
		for (uintptr_t b = 0; b < blockCount; b++) {
			for (uintptr_t c = 0; c < channelCount; c++) {
				*target = floatFromNormalizedI24(format_readI24_LE(waveContent + c * bytesPerSample));
				target += 1;
			}
			waveContent += blockAlign;
		}
		return result;
	} else if (audioFormat == 1 && bitsPerSample == 32) {
		for (uintptr_t b = 0; b < blockCount; b++) {
			for (uintptr_t c = 0; c < channelCount; c++) {
				*target = floatFromNormalizedI32(format_readI32_LE(waveContent + c * bytesPerSample));
				target += 1;
			}
			waveContent += blockAlign;
		}
		return result;
	} else if (audioFormat == 3 && bitsPerSample == 32) {
		for (uintptr_t b = 0; b < blockCount; b++) {
			for (uintptr_t c = 0; c < channelCount; c++) {
				*target = format_bitsToF32_IEEE754(format_readU32_LE(waveContent + c * bytesPerSample));
				target += 1;
			}
			waveContent += blockAlign;
		}
		return result;
	} else if (audioFormat == 3 && bitsPerSample == 64) {
		for (uintptr_t b = 0; b < blockCount; b++) {
			for (uintptr_t c = 0; c < channelCount; c++) {
				*target = format_bitsToF64_IEEE754(format_readU64_LE(waveContent + c * bytesPerSample));
				target += 1;
			}
			waveContent += blockAlign;
		}
		return result;
	} else {
		sendWarning(U"Unsupported sound format ", audioFormat, U" of ", bitsPerSample, U" bits in RIFF wave file.\n");
		// Returning an empty buffer because of the failure.
		return SoundBuffer();
	}
	return SoundBuffer();
}

enum class SoundFileFormat {
	Unknown,
	WAV
};

static SoundFileFormat detectSoundFileExtension(const ReadableString& filename) {
	SoundFileFormat result = SoundFileFormat::Unknown;
	intptr_t lastDotIndex = string_findLast(filename, U'.');
	if (lastDotIndex != -1) {
		String extension = string_upperCase(file_getExtension(filename));
		if (string_match(extension, U"WAV")) {
			result = SoundFileFormat::WAV;
		}
	}
	return result;
}

SoundBuffer sound_load(const ReadableString& filename, bool mustExist) {
	SoundFileFormat extension = detectSoundFileExtension(filename);
	Buffer fileContent = file_loadBuffer(filename, mustExist);
	SoundBuffer result;
	if (buffer_exists(fileContent)) {
		if (extension == SoundFileFormat::WAV) {
			result = sound_decode_RiffWave(fileContent);
			if (mustExist && !sound_exists(result)) {
				throwError(U"sound_load: Failed to load the sound at ", filename, U".\n");
			}
		}
	}
	return result;
}

bool sound_save(const ReadableString& filename, const SoundBuffer &sound, bool mustWork) {
	SoundFileFormat extension = detectSoundFileExtension(filename);
	if (extension == SoundFileFormat::WAV) {
		Buffer fileContent = sound_encode_RiffWave(sound, RiffWaveFormat::RawI16);
		return file_saveBuffer(filename, fileContent, mustWork);
	// TODO: Add more sound formats.
	} else {
		if (mustWork) {
			throwError(U"The extension of \"", filename, U"\" did not match any supported sound format!\n");
		}
		return false;
	}
}

bool sound_save_RiffWave(const ReadableString& filename, const SoundBuffer &sound, RiffWaveFormat format, RoundingMethod roundingMethod, bool mustWork) {
	SoundFileFormat extension = detectSoundFileExtension(filename);
	if (extension == SoundFileFormat::WAV) {
		Buffer fileContent = sound_encode_RiffWave(sound, format, roundingMethod);
		return file_saveBuffer(filename, fileContent, mustWork);
	} else {
		if (mustWork) {
			throwError(U"The extension of \"", filename, U"\" did not match RIFF wave's extension of *.wav!\n");
		}
		return false;
	}
}

SoundBuffer sound_generate_function(uint32_t samplesPerChannel, uint32_t channelCount, uint32_t sampleRate, std::function<float(double time, uint32_t channelIndex)> generator) {
	SoundBuffer result = sound_create(samplesPerChannel, channelCount, sampleRate);
	SafePointer<float> target = sound_getSafePointer(result);
	double time = 0.0;
	double step = 1.0 / sampleRate;
	for (uintptr_t b = 0u; b < samplesPerChannel; b++) {
		for (uintptr_t c = 0u; c < channelCount; c++) {
			*target = generator(time, c);
			target += 1;
		}
		time += step;
	}
	return result;

}

}
