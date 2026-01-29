
#include "../DFPSR/api/soundAPI.h"
#include "../DFPSR/api/timeAPI.h"
#include "../DFPSR/base/noSimd.h"
//#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>

namespace dsr {

static AudioUnit audioUnit;
static bool playing = false;
static void terminateSound() {
	AudioOutputUnitStop(audioUnit);
	AudioUnitUninitialize(audioUnit);
	AudioComponentInstanceDispose(audioUnit);
}

static uint32_t allocatedElements = 0u;
static Buffer floatBuffer;
static SafePointer<float> floatData;

static void allocateBuffers(int neededElements) {
	if (neededElements > allocatedElements) {
		floatBuffer = buffer_create(neededElements * sizeof(float));
		floatData = buffer_getSafeData<float>(floatBuffer, "Output data");
		allocatedElements = neededElements;
	}
}

int32_t engineChannels;
int32_t engineSampleRate;
static std::function<bool(SafePointer<float> data, int32_t length)> engineCallback;

OSStatus coreAudioCallback(void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, uint32_t, uint32_t samplesPerChannel, AudioBufferList *outputBuffers) {
	if (playing) {
		// Calculate the number of samples from all channels.
		uint32_t totalSamples = samplesPerChannel * engineChannels;
		// Make sure that we have enough memory in the float buffer to get sounds from the engine.
		allocateBuffers(totalSamples);
		// Set all elements to zero, so that the engine does not have to do it before accumulating sounds.
		safeMemorySet(floatData, 0, totalSamples * sizeof(float));
		// Get samples from the sound engine and check if we are done playing sounds.
		bool keepRunning = engineCallback(floatData, samplesPerChannel);
		// Convert from float to 16-bit signed PCM format.
		int16_t *outputData = (int16_t*)(outputBuffers->mBuffers[0].mData);
		for (uint32_t t = 0; t < samplesPerChannel * engineChannels; t++) {
			outputData[t] = truncateToI32(clamp(-32768.0f, floatData[t] * 32767.0f, 32767.0f));
		}	
		// If the engine is done taking requests, then this backend can terminate.
		if (!keepRunning) {
			playing = false;
		}
	}
	return noErr;
}

static void initializeSound() {
	OSErr errorCode;
	AudioComponentDescription audioComponentDescription;
	audioComponentDescription.componentType = kAudioUnitType_Output;
	audioComponentDescription.componentSubType = kAudioUnitSubType_DefaultOutput;
	audioComponentDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
	AudioComponent audioOutputDevice = AudioComponentFindNext(nullptr, &audioComponentDescription);
	if (audioOutputDevice == nullptr) {
		throwError(U"Failed to find any CoreAudio output device!\n");
		return;
	}
	errorCode = AudioComponentInstanceNew(audioOutputDevice, &audioUnit);
	if (errorCode != 0) {
		throwError(U"Failed to create the CoreAudio audio unit! Error code: ", errorCode, U"!\n");
		return;
	}
	AURenderCallbackStruct audioUnitCallbackStruct;
	audioUnitCallbackStruct.inputProc = coreAudioCallback;
	errorCode = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &audioUnitCallbackStruct, sizeof(audioUnitCallbackStruct));
	if (errorCode != 0) {
		throwError(U"Failed to assign the CoreAudio audio unit callback! Error code: ", errorCode, U"!\n");
		return;
	}
	AudioStreamBasicDescription audioStreamBasicDescription;
	audioStreamBasicDescription.mFormatID = kAudioFormatLinearPCM;
	audioStreamBasicDescription.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
	audioStreamBasicDescription.mSampleRate = engineSampleRate;
	audioStreamBasicDescription.mBitsPerChannel = 16;
	audioStreamBasicDescription.mChannelsPerFrame = engineChannels;
	audioStreamBasicDescription.mFramesPerPacket = 1;
	audioStreamBasicDescription.mBytesPerFrame = engineChannels * sizeof(int16_t);
	audioStreamBasicDescription.mBytesPerPacket = engineChannels * sizeof(int16_t);
	errorCode = AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &audioStreamBasicDescription, sizeof(audioStreamBasicDescription));
	if (errorCode != 0) {
		throwError(U"Failed to set the CoreAudio audio properties! Error code: ", errorCode, U"!\n");
		return;
	}
	errorCode = AudioUnitInitialize(audioUnit);
	if (errorCode != 0) {
		throwError(U"Failed to initialize the CoreAudio audio unit! Error code: ", errorCode, U"!\n");
		return;
	}
	errorCode = AudioOutputUnitStart(audioUnit);
	if (errorCode != 0) {
		throwError(U"Failed to start the CoreAudio audio unit! Error code: ", errorCode, U"!\n");
		return;
	}
	playing = true;
}

bool sound_streamToSpeakers(int32_t channels, int32_t sampleRate, std::function<bool(SafePointer<float> data, int32_t length)> soundOutput) {
	engineChannels = channels;
	engineSampleRate = sampleRate;
	engineCallback = soundOutput;
	initializeSound();
	// For consistent thread behavior between operating systems and letting callbacks finish before terminating CoreAudio's audio unit,
	//   a loop will check if it is time to terminate the sound engine's thread once in a while.
	// TODO: Make it faster and more efficient, by using a mutex that is locked while playing.
	while (playing) {
		time_sleepSeconds(0.2);
	}
	terminateSound();
	return true;
}

}
