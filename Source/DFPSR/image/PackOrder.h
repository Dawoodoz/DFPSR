// zlib open source license
//
// Copyright (c) 2017 to 2025 David Forsgren Piuva
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

#include <cstdint>
#include "Color.h"
#include "../base/endian.h"
#include "../base/DsrTraits.h"
#include "../api/stringAPI.h"
#include "../math/scalar.h"

namespace dsr {

// The pack order defines where each color channel should be when uint32_t is interpreted as an array of four bytes using local endianness.
// Packed into 2 bits in ImageDimensions, because one can assume that future pack orders at least have visible colors sorted by wavelength.
enum class PackOrderIndex : uint32_t {
	RGBA, // Red   Green Blue  Alpha
	BGRA, // Blue  Green Red   Alpha
	ARGB, // Alpha Red   Green Blue
	ABGR  // Alpha Blue  Green Red
};

inline String& string_toStreamIndented(String& target, const PackOrderIndex& index, const ReadableString& indentation) {
	ReadableString name;
	if (index == PackOrderIndex::RGBA) {
		name = U"RGBA";
	} else if (index == PackOrderIndex::BGRA) {
		name = U"BGRA";
	} else if (index == PackOrderIndex::ARGB) {
		name = U"ARGB";
	} else if (index == PackOrderIndex::ABGR) {
		name = U"ABGR";
	} else {
		name = U"?";
	}
	string_append(target, indentation, name);
	return target;
}

struct PackOrder {
public:
	// Byte array indices for each channel
	// Indices are the locations of each color, not which color that holds each location
	//   Example:
	//     The indices for ARGB are (1, 2, 3, 0), because RGB are placed at byte indices 1..3 and A is placed first at byte index 0.
	int32_t redIndex, greenIndex, blueIndex, alphaIndex;
	// Pre-multipled bit offsets
	int32_t redOffset, greenOffset, blueOffset, alphaOffset;
	uint32_t redMask, greenMask, blueMask, alphaMask;
private:
	constexpr PackOrder(int32_t redIndex, int32_t greenIndex, int32_t blueIndex, int32_t alphaIndex) :
	  redIndex(redIndex), greenIndex(greenIndex), blueIndex(blueIndex), alphaIndex(alphaIndex),
	  redOffset(redIndex << 3), greenOffset(greenIndex << 3), blueOffset(blueIndex << 3), alphaOffset(alphaIndex << 3),
	  redMask(ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, this->redOffset)),
	  greenMask(ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, this->greenOffset)),
	  blueMask(ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, this->blueOffset)),
	  alphaMask(ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, this->alphaOffset)) {}
public:
	// Constructors
	PackOrder() :
	  redIndex(0), greenIndex(1), blueIndex(2), alphaIndex(3),
	  redOffset(0), greenOffset(8), blueOffset(16), alphaOffset(24),
	  redMask(ENDIAN32_BYTE_0), greenMask(ENDIAN32_BYTE_1), blueMask(ENDIAN32_BYTE_2), alphaMask(ENDIAN32_BYTE_3) {}
	static PackOrder getPackOrder(PackOrderIndex index) {
		// Because the PackOrder constuctor is constexpr and all arguments are constant, these pack orders should be generated in compile time.
		if (index == PackOrderIndex::BGRA) {
			return PackOrder(2, 1, 0, 3); // PackOrderIndex::BGRA
		} else if (index == PackOrderIndex::ARGB) {
			return PackOrder(1, 2, 3, 0); // PackOrderIndex::ARGB
		} else if (index == PackOrderIndex::ABGR) {
			return PackOrder(3, 2, 1, 0); // PackOrderIndex::ABGR
		} else {
			return PackOrder(0, 1, 2, 3); // PackOrderIndex::RGBA
		}
	}
	// Pack the channels into a pixel color.
	uint32_t packRgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) const {
		uint32_t result;
		uint8_t *channels = (uint8_t*)(&result);
		channels[this->redIndex] = red;
		channels[this->greenIndex] = green;
		channels[this->blueIndex] = blue;
		channels[this->alphaIndex] = alpha;
		return result;
	}
	// Limit color to a 0..255 range and pack the channels into a pixel color.
	uint32_t saturateAndPackRgba(const ColorRgbaI32& color) {
		return this->packRgba(clamp(0, color.red, 255), clamp(0, color.green, 255), clamp(0, color.blue, 255), clamp(0, color.alpha, 255));
	}
	// A faster way of limiting input when you are sure that it won't overflow.
	uint32_t truncateAndPackRgba(const ColorRgbaI32& color) {
		return this->packRgba((uint8_t)color.red, (uint8_t)color.green, (uint8_t)color.blue, (uint8_t)color.alpha);
	}
	// The inverse of packRgba putting the channels back in order.
	ColorRgbaI32 unpackRgba(uint32_t packedColor) {
		uint8_t *channels = (uint8_t*)(&packedColor);
		return ColorRgbaI32(channels[this->redIndex], channels[this->greenIndex], channels[this->blueIndex], channels[this->alphaIndex]);
	}
};

template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
inline U packOrder_getRed(U color) {
	return color & ENDIAN32_BYTE_0;
}
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
inline U packOrder_getRed(U color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.redMask, U(order.redOffset));
}
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
inline U packOrder_getGreen(U color) {
	return ENDIAN_NEG_ADDR_IMM(color & ENDIAN32_BYTE_1, 8);
}
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
inline U packOrder_getGreen(U color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.greenMask, U(order.greenOffset));
}
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
inline U packOrder_getBlue(U color) {
	return ENDIAN_NEG_ADDR_IMM(color & ENDIAN32_BYTE_2, 16);
}
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
inline U packOrder_getBlue(U color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.blueMask, U(order.blueOffset));
}
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
inline U packOrder_getAlpha(U color) {
	return ENDIAN_NEG_ADDR_IMM(color & ENDIAN32_BYTE_3, 24);
}
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
inline U packOrder_getAlpha(U color, const PackOrder &order) {
	return ENDIAN_NEG_ADDR(color & order.alphaMask, U(order.alphaOffset));
}

// Each input 32-bit element is from 0 to 255. Otherwise, the remainder will leak to other elements.
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
U packOrder_packBytes(const U &s0, const U &s1, const U &s2) {
	return s0 | ENDIAN_POS_ADDR_IMM(s1, 8) | ENDIAN_POS_ADDR_IMM(s2, 16);
}
// Using a specified packing order
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
U packOrder_packBytes(const U &s0, const U &s1, const U &s2, const PackOrder &order) {
	return ENDIAN_POS_ADDR(s0, U(order.redOffset))
		 | ENDIAN_POS_ADDR(s1, U(order.greenOffset))
		 | ENDIAN_POS_ADDR(s2, U(order.blueOffset));
}

// Each input 32-bit element is from 0 to 255. Otherwise, the remainder will leak to other elements.
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
U packOrder_packBytes(const U &s0, const U &s1, const U &s2, const U &s3) {
	return s0 | ENDIAN_POS_ADDR_IMM(s1, 8) | ENDIAN_POS_ADDR_IMM(s2, 16) | ENDIAN_POS_ADDR_IMM(s3, 24);
}
// Using a specified packing order
template<typename U, DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U))> // Accepting uint32_t, U32x4, U32x8 ... U32xX
U packOrder_packBytes(const U &s0, const U &s1, const U &s2, const U &s3, const PackOrder &order) {
	return ENDIAN_POS_ADDR(s0, U(order.redOffset))
		 | ENDIAN_POS_ADDR(s1, U(order.greenOffset))
		 | ENDIAN_POS_ADDR(s2, U(order.blueOffset))
		 | ENDIAN_POS_ADDR(s3, U(order.alphaOffset));
}

// Pack separate floats into saturated bytes
//   From float to uint32_t
//   From F32x4 to U32x4
//   From F32x8 to U32x8
//   From F32xX to U32xX
//   From F32xF to U32xF
template<typename U, typename F, DSR_ENABLE_IF(
	 DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U)
  && DSR_CHECK_PROPERTY(DsrTrait_Any_F32, F)
)>
inline U packOrder_floatToSaturatedByte(const F &s0, const F &s1, const F &s2, const F &s3) {
	return packOrder_packBytes(
	  truncateToU32(s0.clamp(0.1f, 255.1f)),
	  truncateToU32(s1.clamp(0.1f, 255.1f)),
	  truncateToU32(s2.clamp(0.1f, 255.1f)),
	  truncateToU32(s3.clamp(0.1f, 255.1f))
	);
}
// Using a specified pack order
template<typename U, typename F, DSR_ENABLE_IF(
	 DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U)
  && DSR_CHECK_PROPERTY(DsrTrait_Any_F32, F)
)>
inline U packOrder_floatToSaturatedByte(const F &s0, const F &s1, const F &s2, const F &s3, const PackOrder &order) {
	return packOrder_packBytes(
	  truncateToU32(s0.clamp(0.1f, 255.1f)),
	  truncateToU32(s1.clamp(0.1f, 255.1f)),
	  truncateToU32(s2.clamp(0.1f, 255.1f)),
	  truncateToU32(s3.clamp(0.1f, 255.1f)),
	  order
	);
}

}

#endif
