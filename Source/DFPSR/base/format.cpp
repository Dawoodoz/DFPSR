// zlib open source license
//
// Copyright (c) 2025 David Forsgren Piuva
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

#include "format.h"
#include <limits>
#include <cmath>

namespace dsr {

uint16_t format_readU16_LE(SafePointer<const uint8_t> source) {
	return ((uint16_t)source[0]     )
	     | ((uint16_t)source[1] << 8);
}

uint32_t format_readU24_LE(SafePointer<const uint8_t> source) {
	return ((int32_t)source[0]      )
	     | ((int32_t)source[1] << 8 )
	     | ((int32_t)source[2] << 16);
}

uint32_t format_readU32_LE(SafePointer<const uint8_t> source) {
	return ((uint32_t)source[0]      )
	     | ((uint32_t)source[1] << 8 )
	     | ((uint32_t)source[2] << 16)
	     | ((uint32_t)source[3] << 24);
}

uint64_t format_readU64_LE(SafePointer<const uint8_t> source) {
	return ((uint64_t)source[0]      )
	     | ((uint64_t)source[1] << 8 )
	     | ((uint64_t)source[2] << 16)
	     | ((uint64_t)source[3] << 24)
	     | ((uint64_t)source[4] << 32)
	     | ((uint64_t)source[5] << 40)
	     | ((uint64_t)source[6] << 48)
	     | ((uint64_t)source[7] << 56);
}

int16_t format_readI16_LE(SafePointer<const uint8_t> source) {
	return int16_t(format_readU16_LE(source));
}

int32_t format_readI24_LE(SafePointer<const uint8_t> source) {
	uint32_t result = format_readU24_LE(source);
	if (result & 0b00000000100000000000000000000000) result |= 0b11111111000000000000000000000000;
	return int32_t(result);
}

int32_t format_readI32_LE(SafePointer<const uint8_t> source) {
	return int32_t(format_readU32_LE(source));
}

int64_t format_readI64_LE(SafePointer<const uint8_t> source) {
	return int64_t(format_readU64_LE(source));
}

void format_writeU16_LE(SafePointer<uint8_t> target, uint16_t value) {
	target[0] = uint8_t((value & 0x00FF)     );
	target[1] = uint8_t((value & 0xFF00) >> 8);
}

void format_writeU24_LE(SafePointer<uint8_t> target, uint32_t value) {
	target[0] = uint8_t((value & 0x0000FF)      );
	target[1] = uint8_t((value & 0x00FF00) >>  8);
	target[2] = uint8_t((value & 0xFF0000) >> 16);
}

void format_writeU32_LE(SafePointer<uint8_t> target, uint32_t value) {
	target[0] = uint8_t((value & 0x000000FF)      );
	target[1] = uint8_t((value & 0x0000FF00) >>  8);
	target[2] = uint8_t((value & 0x00FF0000) >> 16);
	target[3] = uint8_t((value & 0xFF000000) >> 24);
}

void format_writeU64_LE(SafePointer<uint8_t> target, uint64_t value) {
	target[0] = uint8_t((value & 0x00000000000000FF)      );
	target[1] = uint8_t((value & 0x000000000000FF00) >>  8);
	target[2] = uint8_t((value & 0x0000000000FF0000) >> 16);
	target[3] = uint8_t((value & 0x00000000FF000000) >> 24);
	target[4] = uint8_t((value & 0x000000FF00000000) >> 32);
	target[5] = uint8_t((value & 0x0000FF0000000000) >> 40);
	target[6] = uint8_t((value & 0x00FF000000000000) >> 48);
	target[7] = uint8_t((value & 0xFF00000000000000) >> 56);
}

void format_writeI16_LE(SafePointer<uint8_t> target, int16_t value) {
	format_writeU16_LE(target, uint16_t(value));
}

void format_writeI24_LE(SafePointer<uint8_t> target, int32_t value) {
	format_writeU24_LE(target, uint32_t(value));
}

void format_writeI32_LE(SafePointer<uint8_t> target, int32_t value) {
	format_writeU32_LE(target, uint32_t(value));
}

void format_writeI64_LE(SafePointer<uint8_t> target, int64_t value) {
	format_writeU32_LE(target, uint64_t(value));
}

float format_bitsToF32_IEEE754(uint32_t bits) {
	bool     sign     =  bits & 0b10000000000000000000000000000000;
	uint32_t exponent = (bits & 0b01111111100000000000000000000000) >> 23;
	uint32_t mantissa =  bits & 0b00000000011111111111111111111111;
	if (exponent == 0b11111111) {
		if (mantissa == 0u) {
			return sign ? -std::numeric_limits<float>::infinity() : std::numeric_limits<float>::infinity();
		} else {
			return std::numeric_limits<float>::quiet_NaN();
		}
	} else if (exponent == 0 && mantissa == 0) {
		return 0.0f;
	}
	float value;
	if (exponent == 0) {
		value = std::ldexp(mantissa, -126 - 23);
	} else {
		value = std::ldexp((mantissa | 0b100000000000000000000000), exponent - 127 - 23);
	}
	return sign ? -value : value;
}

double format_bitsToF64_IEEE754(uint64_t bits) {
	bool     sign     =  bits & 0b1000000000000000000000000000000000000000000000000000000000000000;
	uint64_t exponent = (bits & 0b0111111111110000000000000000000000000000000000000000000000000000) >> 52;
	uint64_t mantissa =  bits & 0b0000000000001111111111111111111111111111111111111111111111111111;
	if (exponent == 0b11111111111) {
		if (mantissa == 0u) {
			return sign ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
		} else {
			return std::numeric_limits<double>::quiet_NaN();
		}
	} else if (exponent == 0 && mantissa == 0) {
		return 0.0;
	}
	double value;
	if (exponent == 0) {
		value = std::ldexp(mantissa, -1022 - 52);
	} else {
		value = std::ldexp((mantissa | 0b10000000000000000000000000000000000000000000000000000), exponent - 1023 - 52);
	}
	return sign ? -value : value;
}

}
