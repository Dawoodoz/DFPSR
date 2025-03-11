
#ifndef MODULE_SOUND
#define MODULE_SOUND

#include "../../DFPSR/api/soundAPI.h"
#include "../../DFPSR/api/imageAPI.h"
#include "Envelope.h"

namespace dsr {

void soundEngine_initialize();
void soundEngine_terminate();

// Creates a single-channel sound using the generator function
// generator takes the time in seconds as input and returns a value from -1.0 to 1.0
int32_t soundEngine_getSoundBufferCount();

int32_t soundEngine_insertSoundBuffer(const SoundBuffer &buffer, const ReadableString &name, bool fromFile);

int32_t soundEngine_loadSoundFromFile(const ReadableString &filename, bool mustExist = true);

SoundBuffer soundEngine_getSound(int32_t soundIndex);

// TODO: Create getters for the ouput buffer settings for converting sounds with the wrong sample rate, merging too many channels, or repeating looped sounds that are shorter than the output buffer.
// Pre-conditions:
//   The sound at soundIndex must have the same sample rate as the engine's output buffer
//     and may not contain more channels than the engine's output buffer.
int32_t soundEngine_playSound(int32_t soundIndex, bool repeat, float leftVolume, float rightVolume, const EnvelopeSettings &envelopeSettings);
inline int32_t soundEngine_playSound(int32_t soundIndex, bool repeat = false, float leftVolume = 1.0f, float rightVolume = 1.0f) { return soundEngine_playSound(soundIndex, repeat, leftVolume, rightVolume, EnvelopeSettings()); }

//int32_t playSound(int32_t soundIndex, bool repeat, double volumeLeft, double volumeRight, double speed);
//int32_t playSound(int32_t soundIndex, bool repeat, double volumeLeft, double volumeRight, double speed, const EnvelopeSettings &envelopeSettings);
// Begin to fade out the sound and let it delete itself once done
void soundEngine_releaseSound(int64_t playerID);
// Stop the sound at once
void soundEngine_stopSound(int64_t playerID);
// Stop all sounds at once
void soundEngine_stopAllSounds();

// Visualization
void soundEngine_drawSound(ImageRgbaU8 target, const IRect &region, int32_t soundIndex, bool selected);

}

#endif
