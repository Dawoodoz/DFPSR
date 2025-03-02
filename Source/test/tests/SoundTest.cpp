
#include "../testTools.h"
#include "../../DFPSR/api/soundAPI.h"
#include "../../DFPSR/api/fileAPI.h"
#include "../../DFPSR/api/randomAPI.h"
#include <math.h>

void compareSoundBuffers(const SoundBuffer &given, const SoundBuffer &expected, float tolerance) {
	ASSERT_EQUAL(sound_getSamplesPerChannel(given), sound_getSamplesPerChannel(expected));
	ASSERT_EQUAL(sound_getChannelCount(given), sound_getChannelCount(expected));
	ASSERT_EQUAL(sound_getSampleRate(given), sound_getSampleRate(expected));
	SafePointer<float> givenSamples = sound_getSafePointer(given);
	SafePointer<float> expectedSamples = sound_getSafePointer(expected);
	float maxGiven = 0.0f;
	float minGiven = 0.0f;
	float maxExpected = 0.0f;
	float minExpected = 0.0f;
	float maxOffset = -1000000.0f;
	float minOffset =  1000000.0f;
	for (int t = 0; t < sound_getSamplesPerChannel(given); t++) {
		float givenValue = givenSamples[t];
		float expectedValue = expectedSamples[t];
		float offset = givenSamples[t] - expectedSamples[t];
		if (offset > maxOffset) maxOffset = offset;
		if (offset < minOffset) minOffset = offset;
		if (givenValue > maxGiven) maxGiven = givenValue;
		if (givenValue < minGiven) minGiven = givenValue;
		if (expectedValue > maxExpected) maxExpected = expectedValue;
		if (expectedValue < minExpected) minExpected = expectedValue;
	}
	ASSERT_LESSER(maxOffset, tolerance);
	ASSERT_GREATER(minOffset, -tolerance);
}

static const double pi = 3.1415926535897932384626433832795;
static const double cyclesToRadians = pi * 2.0;

// TODO: Implement basic sound generation functions and move them to the sound API.
//       Both in-place functions and allocating new buffers as needed to expand.
//       Generation functions, multiplying masks, fade masks, echo effects, frequency filters, equalization, resampling...

void sound_setNoise(SoundBuffer &sound, float minimum = -1.0f, float maximum = 1.0f) {
	RandomGenerator generator = random_createGenerator(917542);
	SafePointer<float> target = sound_getSafePointer(sound);
	intptr_t totalSamples = sound_getSamplesPerChannel(sound) * sound_getChannelCount(sound);
	for (intptr_t s = 0; s < totalSamples; s++) {
		target[s] = random_generate_range(generator, minimum, maximum);
	}
}

START_TEST(Sound)
	String folderPath = file_combinePaths(U".", U"resources", U"sounds");
	// Check that we have a valid folder path to the resources.
	ASSERT_EQUAL(file_getEntryType(folderPath), EntryType::Folder);
	{ // Single channel wave files.
		// Generate the reference sine wave.
		static const int sampleRate = 44100;
		static const int samplesPerChannel = 441;
		static const int frequency = 100;
		SoundBuffer referenceSine = sound_create(samplesPerChannel, 1, sampleRate);
		SafePointer<float> target = sound_getSafePointer(referenceSine);
		static const double radiansPerElement = cyclesToRadians * (double)frequency / (double)sampleRate;
		for (int t = 0; t < samplesPerChannel; t++) {
			target[t] = (float)sin(double(t) * radiansPerElement);
		}
		// Load wave files that were exported from a 10 millisecond sine wave in the Audacity sound editor for reference.
		//   Because Audacity truncates towards zero instead of rounding to nearest, the worst case accuracy has twice the error.
		SoundBuffer sineU8 = sound_load(file_combinePaths(folderPath, U"SineU8.wav"));
		ASSERT(sound_exists(sineU8));
		compareSoundBuffers(sineU8, referenceSine, 0.02f);

		SoundBuffer sineI16 = sound_load(file_combinePaths(folderPath, U"SineI16.wav"));
		ASSERT(sound_exists(sineI16));
		compareSoundBuffers(sineI16, referenceSine, 0.0002f);

		SoundBuffer sineI24 = sound_load(file_combinePaths(folderPath, U"SineI24.wav"));
		ASSERT(sound_exists(sineI24));
		compareSoundBuffers(sineI24, referenceSine, 0.000002f);

		SoundBuffer sineI32 = sound_load(file_combinePaths(folderPath, U"SineI32.wav"));
		ASSERT(sound_exists(sineI32));
		compareSoundBuffers(sineI32, referenceSine, 0.00000001f);

		SoundBuffer sineF32 = sound_load(file_combinePaths(folderPath, U"SineF32.wav"));
		ASSERT(sound_exists(sineF32));
		compareSoundBuffers(sineF32, referenceSine, 0.00000001f);

		SoundBuffer sineF64 = sound_load(file_combinePaths(folderPath, U"SineF64.wav"));
		ASSERT(sound_exists(sineF64));
		compareSoundBuffers(sineF64, referenceSine, 0.00000001f);

		//sound_save_RiffWave(file_combinePaths(folderPath, U"SineI16_2.wav"), sineI16, RiffWaveFormat::RawI16);
		//sound_save_RiffWave(file_combinePaths(folderPath, U"SineI24_2.wav"), sineI24, RiffWaveFormat::RawI24);
		//sound_save_RiffWave(file_combinePaths(folderPath, U"SineI32_2.wav"), sineI32, RiffWaveFormat::RawI24);
	}
	{ // Brute-force encode and decode test with random noise.
		RandomGenerator generator = random_createGenerator(12345);
		for (uint32_t channelCount = 1; channelCount <= 16; channelCount++) {
			SoundBuffer original = sound_create(1024, channelCount, 44100);
			sound_setNoise(original);
			Buffer encodedU8_truncate  = sound_encode_RiffWave(original, RiffWaveFormat::RawU8 , RoundingMethod::Truncate);
			Buffer encodedU8_nearest   = sound_encode_RiffWave(original, RiffWaveFormat::RawU8 , RoundingMethod::Nearest);
			Buffer encodedI16_truncate = sound_encode_RiffWave(original, RiffWaveFormat::RawI16, RoundingMethod::Truncate);
			Buffer encodedI16_nearest  = sound_encode_RiffWave(original, RiffWaveFormat::RawI16, RoundingMethod::Nearest);
			Buffer encodedI24_nearest  = sound_encode_RiffWave(original, RiffWaveFormat::RawI24, RoundingMethod::Nearest);
			Buffer encodedI32_nearest  = sound_encode_RiffWave(original, RiffWaveFormat::RawI32, RoundingMethod::Nearest);
			SoundBuffer decodedU8_truncate  = sound_decode_RiffWave(encodedU8_truncate);
			SoundBuffer decodedU8_nearest   = sound_decode_RiffWave(encodedU8_nearest );
			SoundBuffer decodedI16_truncate = sound_decode_RiffWave(encodedI16_truncate);
			SoundBuffer decodedI16_nearest  = sound_decode_RiffWave(encodedI16_nearest);
			SoundBuffer decodedI24_nearest  = sound_decode_RiffWave(encodedI24_nearest);
			SoundBuffer decodedI32_nearest  = sound_decode_RiffWave(encodedI32_nearest);
			compareSoundBuffers(decodedU8_truncate , original, 2.02 / 256.0);
			compareSoundBuffers(decodedU8_nearest  , original, 1.01 / 256.0);
			compareSoundBuffers(decodedI16_truncate, original, 2.02 / 65536.0);
			compareSoundBuffers(decodedI16_nearest , original, 1.01 / 65536.0);
			compareSoundBuffers(decodedI24_nearest , original, 1.01 / 8388608.0);
			compareSoundBuffers(decodedI32_nearest , original, 1.01 / 2147483648.0);
			if (failed) break;
		}
	}
	// TODO: Test saving sounds to files.
END_TEST
