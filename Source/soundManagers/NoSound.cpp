
#include "../DFPSR/api/soundAPI.h"

namespace dsr {

bool sound_streamToSpeakers(int32_t channels, int32_t sampleRate, std::function<bool(SafePointer<float>, int32_t)> soundOutput) {
	return false;
}

}
