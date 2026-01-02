
#ifndef MODULE_SOUND_ENVELOPE
#define MODULE_SOUND_ENVELOPE

#include "../../DFPSR/api/soundAPI.h"
#include "../../DFPSR/api/imageAPI.h"

namespace dsr {

struct EnvelopeSettings {
	// Basic ADSR
	double attack, decay, sustain, release;
	// Extended
	double hold, rise, sustainedSmooth, releasedSmooth;
	bool used;
	EnvelopeSettings();
	EnvelopeSettings(double attack, double decay, double sustain, double release, double hold = 0.0, double rise = 0.0, double sustainedSmooth = 0.0, double releasedSmooth = 0.0);
};

struct Envelope {
	// Settings
	EnvelopeSettings envelopeSettings;
	// Dynamic
	int32_t state = 0;
	double currentVolume = 0.0, currentGoal = 0.0, releaseVolume = 0.0, timeSinceChange = 0.0;
	bool lastSustained = true;
	Envelope(const EnvelopeSettings &envelopeSettings);
	double getVolume(bool sustained, double seconds);
	bool done();
};

// Visualization
void soundEngine_drawEnvelope(ImageRgbaU8 target, const IRect &region, const EnvelopeSettings &envelopeSettings, double releaseTime, double viewTime);

}

#endif
