// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_IMAGE_PACK_ORDER
#define DFPSR_IMAGE_PACK_ORDER

#include <stdint.h>
#include "../api/types.h"
#include "../base/simd.h"
#include "../base/endian.h"
#include "../base/text.h"

namespace dsr {

// See types.h for the definition of PackOrderIndex

struct PackOrder {
public:
	// The index that it was constructed from
	PackOrderIndex packOrderIndex;
	// Byte array indices for each channel
	// Indices are the locations of each color, not which color that holds each location
	//   Example:
	//     The indices for ARGB are (1, 2, 3, 0)
	//     Because red is second at byte[1], green is third byte[2], blue is last in byte[3] and alpha is first in byte[0]
	int redIndex, greenIndex, blueIndex, alphaIndex;
	// Pre-multipled bit offsets
	int redOffset, greenOffset, blueOffset, alphaOffset;
	uint32_t redMask, greenMask, blueMask, alphaMask;
private:
	PackOrder(PackOrderIndex packOrderIndex, int redIndex, int greenIndex, int blueIndex, int alphaIndex) :
	  packOrderIndex(packOrderIndex),
	  redIndex(redIndex), greenIndex(greenIndex), blueIndex(blueIndex), alphaIndex(alphaIndex),
	  redOffset(redIndex * 8), greenOffset(greenIndex * 8), blueOffset(blueIndex * 8), alphaOffset(alphaIndex * 8),
	  redMask(ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, this->redOffset)),
	  greenMask(ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, this->greenOffset)),
	  blueMask(ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, this->blueOffset)),
	  alphaMask(ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, this->alphaOffset)) {}
public:
	// Constructors
	PackOrder() :
	  packOrderIndex(PackOrderIndex::RGBA),
	  redIndex(0), greenIndex(1), blueIndex(2), alphaIndex(3),
	  redOffset(0), greenOffset(8), blueOffset(16), alphaOffset(24),
	  redMask(ENDIAN32_BYTE_0), greenMask(ENDIAN32_BYTE_1), blueMask(ENDIAN32_BYTE_2), alphaMask(ENDIAN32_BYTE_3) {}
	static PackOrder getPackOrder(PackOrderIndex index) {
		if (index == PackOrderIndex::RGBA) {
			return PackOrder(index, 0, 1, 2, 3);
		} else if (index == PackOrderIndex::BGRA) {
			return PackOrder(index, 2, 1, 0, 3);
		} else if (index == PackOrderIndex::ARGB) {
			return PackOrder(index, 1, 2, 3, 0);
		} else if (index == PackOrderIndex::ABGR) {
			return PackOrder(index, 3, 2, 1, 0);
		} else {
			printText("Warning! Unknown packing order index ", index, ". Falling back on RGBA.");
			return PackOrder(index, 0, 1, 2, 3);
		}
	}
	uint32_t packRgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) const {
		uint32_t result;
		uint8_t *channels = (uint8_t*)(&result);
		channels[this->redIndex] = red;
		channels[this->greenIndex] = green;
		channels[this->blueIndex] = blue;
		channels[this->alphaIndex] = alpha;
		return result;
	}
};

inline bool operator==(const PackOrder &left, const PackOrder &right) {
	return left.packOrderIndex == right.packOrderIndex;
}

// Each input 32-bit element is from 0 to 255. Otherwise, the remainder will leak to other elements.
inline static U32x4 packBytes(const U32x4 &s0, const U32x4 &s1, const U32x4 &s2) {
	return s0 | ENDIAN_POS_ADDR(s1, 8) | ENDIAN_POS_ADDR(s2, 16);
}
// Using a specified packing order
inline U32x4 packBytes(const U32x4 &s0, const U32x4 &s1, const U32x4 &s2, const PackOrder &order) {
	return ENDIAN_POS_ADDR(s0, order.redOffset)
	     | ENDIAN_POS_ADDR(s1, order.greenOffset)
	     | ENDIAN_POS_ADDR(s2, order.blueOffset);
}

// Each input 32-bit element is from 0 to 255. Otherwise, the remainder will leak to other elements.
inline static U32x4 packBytes(const U32x4 &s0, const U32x4 &s1, const U32x4 &s2, const U32x4 &s3) {
	return s0 | ENDIAN_POS_ADDR(s1, 8) | ENDIAN_POS_ADDR(s2, 16) | ENDIAN_POS_ADDR(s3, 24);
}
// Using a specified packing order
inline U32x4 packBytes(const U32x4 &s0, const U32x4 &s1, const U32x4 &s2, const U32x4 &s3, const PackOrder &order) {
	return ENDIAN_POS_ADDR(s0, order.redOffset)
	     | ENDIAN_POS_ADDR(s1, order.greenOffset)
	     | ENDIAN_POS_ADDR(s2, order.blueOffset)
	     | ENDIAN_POS_ADDR(s3, order.alphaOffset);
}

// Pack separate floats into saturated bytes
inline static U32x4 floatToSaturatedByte(const F32x4 &s0, const F32x4 &s1, const F32x4 &s2, const F32x4 &s3) {
	return packBytes(
	  truncateToU32(s0.clamp(0.1f, 255.1f)),
	  truncateToU32(s1.clamp(0.1f, 255.1f)),
	  truncateToU32(s2.clamp(0.1f, 255.1f)),
	  truncateToU32(s3.clamp(0.1f, 255.1f))
	);
}
// Using a specified packing order
inline U32x4 floatToSaturatedByte(const F32x4 &s0, const F32x4 &s1, const F32x4 &s2, const F32x4 &s3, const PackOrder &order) {
	return packBytes(
	  truncateToU32(s0.clamp(0.1f, 255.1f)),
	  truncateToU32(s1.clamp(0.1f, 255.1f)),
	  truncateToU32(s2.clamp(0.1f, 255.1f)),
	  truncateToU32(s3.clamp(0.1f, 255.1f)),
	  order
	);
}

inline uint32_t getRed(uint32_t color) {
	return color & ENDIAN32_BYTE_0;
}
inline uint32_t getRed(uint32_t color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.redMask, order.redOffset);
}
inline uint32_t getGreen(uint32_t color) {
	return ENDIAN_NEG_ADDR(color & ENDIAN32_BYTE_1, 8);
}
inline uint32_t getGreen(uint32_t color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.greenMask, order.greenOffset);
}
inline uint32_t getBlue(uint32_t color) {
	return ENDIAN_NEG_ADDR(color & ENDIAN32_BYTE_2, 16);
}
inline uint32_t getBlue(uint32_t color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.blueMask, order.blueOffset);
}
inline uint32_t getAlpha(uint32_t color) {
	return ENDIAN_NEG_ADDR(color & ENDIAN32_BYTE_3, 24);
}
inline uint32_t getAlpha(uint32_t color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.alphaMask, order.alphaOffset);
}

inline U32x4 getRed(const U32x4 &color) {
	return color & ENDIAN32_BYTE_0;
}
inline U32x4 getRed(const U32x4 &color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.redMask, order.redOffset);
}
inline U32x4 getGreen(const U32x4 &color) {
	return ENDIAN_NEG_ADDR(color & ENDIAN32_BYTE_1, 8);
}
inline U32x4 getGreen(const U32x4 &color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.greenMask, order.greenOffset);
}
inline U32x4 getBlue(const U32x4 &color) {
	return ENDIAN_NEG_ADDR(color & ENDIAN32_BYTE_2, 16);
}
inline U32x4 getBlue(const U32x4 &color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.blueMask, order.blueOffset);
}
inline U32x4 getAlpha(const U32x4 &color) {
	return ENDIAN_NEG_ADDR(color & ENDIAN32_BYTE_3, 24);
}
inline U32x4 getAlpha(const U32x4 &color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.alphaMask, order.alphaOffset);
}

inline String getName(PackOrderIndex index) {
	if (index == PackOrderIndex::RGBA) {
		return U"RGBA";
	} else if (index == PackOrderIndex::BGRA) {
		return U"BGRA";
	} else if (index == PackOrderIndex::ARGB) {
		return U"ARGB";
	} else if (index == PackOrderIndex::ABGR) {
		return U"ABGR";
	} else {
		return U"?";
	}
}
inline String& string_toStreamIndented(String& target, const PackOrderIndex& source, const ReadableString& indentation) {
	string_append(target, indentation, getName(source));
	return target;
}
inline String& string_toStreamIndented(String& target, const PackOrder& source, const ReadableString& indentation) {
	string_append(target, indentation, getName(source.packOrderIndex));
	return target;
}

}

#endif

