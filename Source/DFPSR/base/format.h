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

#ifndef DFPSR_FORMAT
#define DFPSR_FORMAT

#include "SafePointer.h"
#include <cstdint>

namespace dsr {

// Helper functions for encoding and decoding binary file formats in buffers.

// Read an unaligned unsigned integer with the first byte at source using little-endian byte order.
uint16_t format_readU16_LE(SafePointer<const uint8_t> source);
uint32_t format_readU24_LE(SafePointer<const uint8_t> source);
uint32_t format_readU32_LE(SafePointer<const uint8_t> source);
uint64_t format_readU64_LE(SafePointer<const uint8_t> source);

// Read an unaligned signed integer in two's complement with the first byte at source using little-endian byte order.
int16_t format_readI16_LE(SafePointer<const uint8_t> source);
int32_t format_readI24_LE(SafePointer<const uint8_t> source);
int32_t format_readI32_LE(SafePointer<const uint8_t> source);
int64_t format_readI64_LE(SafePointer<const uint8_t> source);

// Write an unaligned unsigned integer with the first byte at target using little-endian byte order.
void format_writeU16_LE(SafePointer<uint8_t> target, uint16_t value);
void format_writeU24_LE(SafePointer<uint8_t> target, uint32_t value);
void format_writeU32_LE(SafePointer<uint8_t> target, uint32_t value);
void format_writeU64_LE(SafePointer<uint8_t> target, uint64_t value);

// Write an unaligned signed integer in two's complement with the first byte at target using little-endian byte order.
void format_writeI16_LE(SafePointer<uint8_t> target, int16_t value);
void format_writeI24_LE(SafePointer<uint8_t> target, int32_t value);
void format_writeI32_LE(SafePointer<uint8_t> target, int32_t value);
void format_writeI64_LE(SafePointer<uint8_t> target, int64_t value);

// Convert bits interpreted as a 32-bit IEEE754 floating-point value into the native float representation.
float format_bitsToF32_IEEE754(uint32_t bits);

// Convert bits interpreted as a 65-bit IEEE754 floating-point value into the native double representation.
double format_bitsToF64_IEEE754(uint64_t bits);
	
}

#endif
