
#include "Envelope.h"
#include "../../DFPSR/api/drawAPI.h"

namespace dsr {

EnvelopeSettings::EnvelopeSettings()
: attack(0.0), decay(0.0), sustain(1.0), release(0.0), hold(0.0), rise(0.0), sustainedSmooth(0.0), releasedSmooth(0.0), used(false) {}

EnvelopeSettings::EnvelopeSettings(double attack, double decay, double sustain, double release, double hold, double rise, double sustainedSmooth, double releasedSmooth)
: attack(attack), decay(decay), sustain(sustain), release(release), hold(hold), rise(rise), sustainedSmooth(sustainedSmooth), releasedSmooth(releasedSmooth), used(true) {}

Envelope::Envelope(const EnvelopeSettings &envelopeSettings)
: envelopeSettings(envelopeSettings) {
	// Avoiding division by zero using very short fades
	double shortestTime = 0.001;
	if (this->envelopeSettings.attack < shortestTime) { this->envelopeSettings.attack = shortestTime; }
	if (this->envelopeSettings.hold < shortestTime) { this->envelopeSettings.hold = shortestTime; }
	if (this->envelopeSettings.decay < shortestTime) { this->envelopeSettings.decay = shortestTime; }
	if (this->envelopeSettings.release < shortestTime) { this->envelopeSettings.release = shortestTime; }
}

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

// TODO: Pre-calculate divisions to save time.
double Envelope::getVolume(bool sustained, double seconds) {
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

bool Envelope::done() {
	return this->currentVolume <= 0.0000000001 && !this->lastSustained;
}

void soundEngine_drawEnvelope(ImageRgbaU8 target, const IRect &region, const EnvelopeSettings &envelopeSettings, double releaseTime, double viewTime) {
	int32_t top = region.top();
	int32_t bottom = region.bottom() - 1;
	Envelope envelope = Envelope(envelopeSettings);
	double secondsPerPixel = viewTime / region.width();
	draw_rectangle(target, region, ColorRgbaI32(0, 0, 0, 255));
	draw_rectangle(target, IRect(region.left(), region.top(), region.width() * (releaseTime / viewTime), region.height() / 8), ColorRgbaI32(0, 128, 128, 255));
	int32_t oldHardY = bottom;
	for (int32_t s = 0; s < region.width(); s++) {
		int32_t x = s + region.left();
		double time = s * secondsPerPixel;
		double smoothLevel = envelope.getVolume(time < releaseTime, secondsPerPixel);
		double hardLevel = envelope.currentGoal;
		if (envelope.done()) {
			draw_line(target, x, top, x, (top * 7 + bottom) / 8, ColorRgbaI32(128, 0, 0, 255));
		} else {
			draw_line(target, x, (top * smoothLevel) + (bottom * (1.0 - smoothLevel)), x, bottom, ColorRgbaI32(64, 64, 0, 255));
			int32_t hardY = (top * hardLevel) + (bottom * (1.0 - hardLevel));
			draw_line(target, x, oldHardY, x, hardY, ColorRgbaI32(255, 255, 255, 255));
			oldHardY = hardY;
		}
	}
}

}
