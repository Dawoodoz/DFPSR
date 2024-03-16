// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

#include "timeAPI.h"
#include <chrono>
#include <thread>
#include <cstdint>

static bool started = false;
static std::chrono::time_point<std::chrono::steady_clock> startTime;

double dsr::time_getSeconds() {
	if (!started) {
		started = true;
		startTime = std::chrono::steady_clock::now();
		return 0.0;
	} else {
		auto currentTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> diff = currentTime - startTime;
		return diff.count();
	}
}

void dsr::time_sleepSeconds(double seconds) {
	// Skips instantly if there's no delay
	if (seconds > 0.0) {
		// Limits the time
		if (seconds > 9000000000000.0) {
			seconds = 9000000000000.0;
		}
		// The seconds are converted into integer microseconds without any risk of overflow
		std::this_thread::sleep_for(std::chrono::microseconds((int64_t)(seconds * 1000000.0)));
	}
}
