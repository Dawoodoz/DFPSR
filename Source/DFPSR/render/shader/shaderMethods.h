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

#ifndef DFPSR_RENDER_SHADER_METHODS
#define DFPSR_RENDER_SHADER_METHODS

#include <stdint.h>
#include "../../math/FVector.h"
#include "../../base/simd3D.h"
#include "../../image/ImageRgbaU8.h"
#include "shaderTypes.h"
#include "../constants.h"

namespace dsr {

namespace shaderMethods {
	// Returns the linear interpolation of the values using corresponding weight ratios for A, B and C in 4 pixels at the same time.
	inline F32x4 interpolate(const FVector3D &vertexData, const F32x4x3 &vertexWeights) {
		ALIGN16 F32x4 vMA = vertexData.x * vertexWeights.v1;
		ALIGN16 F32x4 vMB = vertexData.y * vertexWeights.v2;
		ALIGN16 F32x4 vMC = vertexData.z * vertexWeights.v3;
		return vMA + vMB + vMC;
	}

	inline rgba_F32 interpolateVertexColor(const FVector3D &red, const FVector3D &green, const FVector3D &blue, const FVector3D &alpha, const F32x4x3 &vertexWeights) {
		return rgba_F32(
		  interpolate(red,   vertexWeights),
		  interpolate(green, vertexWeights),
		  interpolate(blue,  vertexWeights),
		  interpolate(alpha, vertexWeights)
		);
	}

	// Returns (colorA * weightA + colorB * weightB) / 256 as bytes
	// weightA and weightB should contain pairs of the same 16-bit weights for each of the 4 pixels in the corresponding A and B colors
	inline U32x4 weightColors(const U32x4 &colorA, const U16x8 &weightA, const U32x4 &colorB, const U16x8 &weightB) {
		ALIGN16 U32x4 lowMask(0x00FF00FFu);
		ALIGN16 U16x8 lowColorA = U16x8(colorA & lowMask);
		ALIGN16 U16x8 lowColorB = U16x8(colorB & lowMask);
		ALIGN16 U32x4 highMask(0xFF00FF00u);
		ALIGN16 U16x8 highColorA = U16x8((colorA & highMask) >> 8);
		ALIGN16 U16x8 highColorB = U16x8((colorB & highMask) >> 8);
		ALIGN16 U32x4 lowColor = (((lowColorA * weightA) + (lowColorB * weightB))).get_U32();
		ALIGN16 U32x4 highColor = (((highColorA * weightA) + (highColorB * weightB))).get_U32();
		return (((lowColor >> 8) & lowMask) | (highColor & highMask));
	}

	// The more significant bits must be zero so that the lower bits can fill the space.
	//   lowBits[x] < 2^16
	inline U16x8 repeatAs16Bits(const U32x4 &lowBits) {
		return U16x8(lowBits | (lowBits << 16));
	}

	// Returns 256 - weight
	inline U16x8 invertWeight(const U16x8 &weight) {
		return U16x8(0x01000100u) - weight;
	}

	inline U32x4 mix_L(const U32x4 &colorA, const U32x4 &colorB, const U32x4 &weight) {
		// Get inverse weights
		ALIGN16 U16x8 weightB = repeatAs16Bits(weight);
		ALIGN16 U16x8 weightA = invertWeight(weightB);
		// Multiply
		return weightColors(colorA, weightA, colorB, weightB);
	}

	inline U32x4 mix_BL(const U32x4 &colorA, const U32x4 &colorB, const U32x4 &colorC, const U32x4 &colorD, const U32x4 &weightX, const U32x4 &weightY) {
		// Get inverse weights
		ALIGN16 U16x8 weightXR = repeatAs16Bits(weightX);
		ALIGN16 U16x8 weightYB = repeatAs16Bits(weightY);
		ALIGN16 U16x8 weightXL = invertWeight(weightXR);
		ALIGN16 U16x8 weightYT = invertWeight(weightYB);
		// Multiply
		return weightColors(weightColors(colorA, weightXL, colorB, weightXR), weightYT, weightColors(colorC, weightXL, colorD, weightXR), weightYB);
	}

	// Single layer sampling methods
	inline U32x4 sample_U32(const TextureRgbaLayer *source, const U32x4 &col, const U32x4 &row) {
		#ifdef USE_AVX2
			ALIGN16 U32x4 pixelOffset((col + (row << (source->strideShift - 2)))); // PixelOffset = Column + Row * PixelStride
			return U32x4(GATHER_U32_AVX2(source->data, pixelOffset.v, 4));
		#else
			UVector4D byteOffset = ((col << 2) + (row << source->strideShift)).get(); // ByteOffset = Column * 4 + Row * ByteStride
			return U32x4(
			  *((uint32_t*)(source->data + byteOffset.x)),
			  *((uint32_t*)(source->data + byteOffset.y)),
			  *((uint32_t*)(source->data + byteOffset.z)),
			  *((uint32_t*)(source->data + byteOffset.w))
			);
		#endif
	}

	// How many mip levels down from here should be sampled for the given texture coordinates
	template<int maxOffset>
	inline int getMipLevelOffset(const TextureRgbaLayer *source, const F32x4 &u, const F32x4 &v) {
		FVector4D ua = u.get();
		FVector4D va = v.get();
		float offsetUX = fabs(ua.x - ua.y);
		float offsetUY = fabs(ua.x - ua.z);
		float offsetVX = fabs(va.x - va.y);
		float offsetVY = fabs(va.x - va.z);
		float offsetU = std::max(offsetUX, offsetUY) * source->width;
		float offsetV = std::max(offsetVX, offsetVY) * source->height;
		float offset = std::max(offsetU, offsetV);

		// This log2 approximation has to be adapted if the number of mip levels changes.
		static_assert(MIP_BIN_COUNT == 5, "Changing MIP_BIN_COUNT must also adapt shaderMethods::getMipLevelOffset");
		int result = 0;
		if (offset > 2.0f) { result = 1; }
		if (offset > 4.0f) { result = 2; }
		if (offset > 8.0f) { result = 3; }
		if (offset > 16.0f) { result = 4; }
		return result;
	}

	inline int getMipLevel(const TextureRgba *source, const F32x4 &u, const F32x4 &v) {
		return getMipLevelOffset<MIP_BIN_COUNT - 1>(source->mips, u, v);
	}

	// Single layer sampling method
	// Precondition: u, v > -0.875f = 1 - (0.5 / minimumMipSize)
	template<Interpolation INTERPOLATION>
	inline U32x4 sample_U32(const TextureRgbaLayer *source, const F32x4 &u, const F32x4 &v) {
		if (INTERPOLATION == Interpolation::BL) {
			ALIGN16 F32x4 uLow(u + source->halfPixelOffsetU);
			ALIGN16 F32x4 vLow(v + source->halfPixelOffsetV);
			ALIGN16 U32x4 subPixLowX(truncateToU32(uLow * source->subWidth)); // SubPixelLowX = ULow * (Width * 256)
			ALIGN16 U32x4 subPixLowY(truncateToU32(vLow * source->subHeight)); // SubPixelLowY = VLow * (Height * 256)
			ALIGN16 U32x4 weightX = subPixLowX & 255; // WeightX = SubPixelLowX % 256
			ALIGN16 U32x4 weightY = subPixLowY & 255; // WeightY = SubPixelLowY % 256
			ALIGN16 U32x4 pixLowX(subPixLowX >> 8); // PixelLowX = SubPixelLowX / 256
			ALIGN16 U32x4 pixLowY(subPixLowY >> 8); // PixelLowY = SubPixelLowY / 256
			ALIGN16 U32x4 wMask(source->widthMask);
			ALIGN16 U32x4 hMask(source->heightMask);
			ALIGN16 U32x4 colLow(pixLowX & wMask); // ColumnLow = PixelLowX % Width
			ALIGN16 U32x4 rowLow(pixLowY & hMask); // RowLow = PixelLowY % Height
			ALIGN16 U32x4 colHigh(((colLow + 1) & wMask)); // ColumnHigh = (ColumnLow + 1) % Width
			ALIGN16 U32x4 rowHigh(((rowLow + 1) & hMask)); // RowHigh = (RowLow + 1) % Height
			// Sample colors in the 4 closest pixels
			ALIGN16 U32x4 colorA(sample_U32(source, colLow, rowLow));
			ALIGN16 U32x4 colorB(sample_U32(source, colHigh, rowLow));
			ALIGN16 U32x4 colorC(sample_U32(source, colLow, rowHigh));
			ALIGN16 U32x4 colorD(sample_U32(source, colHigh, rowHigh));
			// Take a weighted average
			return shaderMethods::mix_BL(colorA, colorB, colorC, colorD, weightX, weightY);
		} else { // Interpolation::NN or unhandled
			ALIGN16 U32x4 pixX(truncateToU32(u * source->width)); // PixelX = U * Width
			ALIGN16 U32x4 pixY(truncateToU32(v * source->height)); // PixelY = V * Height
			ALIGN16 U32x4 col(pixX & source->widthMask); // Column = PixelX % Width
			ALIGN16 U32x4 row(pixY & source->heightMask); // Row = PixelY % Height
			return sample_U32(source, col, row);
		}
	}

	// Precondition: u, v > -0.875f = 1 - (0.5 / minimumMipSize)
	template<Interpolation INTERPOLATION, bool HIGH_QUALITY>
	inline rgba_F32 sample_F32(const TextureRgbaLayer *source, const F32x4 &u, const F32x4 &v) {
		if (INTERPOLATION == Interpolation::BL) {
			if (HIGH_QUALITY) { // High quality interpolation
				ALIGN16 F32x4 uLow(u + source->halfPixelOffsetU);
				ALIGN16 F32x4 vLow(v + source->halfPixelOffsetV);
				ALIGN16 F32x4 pixX = uLow * source->width; // PixelX = ULow * Width
				ALIGN16 F32x4 pixY = vLow * source->height; // PixelY = VLow * Height
				// Truncation can be used as floor for positive input
				ALIGN16 U32x4 pixLowX(truncateToU32(pixX)); // PixelLowX = floor(PixelX)
				ALIGN16 U32x4 pixLowY(truncateToU32(pixY)); // PixelLowY = floor(PixelY)
				ALIGN16 U32x4 wMask(source->widthMask);
				ALIGN16 U32x4 hMask(source->heightMask);
				ALIGN16 U32x4 colLow(pixLowX & wMask); // ColumnLow = PixelLowX % Width
				ALIGN16 U32x4 rowLow(pixLowY & hMask); // RowLow = PixelLowY % Height
				ALIGN16 U32x4 colHigh(((colLow + 1) & wMask)); // ColumnHigh = (ColumnLow + 1) % Width
				ALIGN16 U32x4 rowHigh(((rowLow + 1) & hMask)); // RowHigh = (RowLow + 1) % Height
				// Sample colors in the 4 closest pixels
				ALIGN16 rgba_F32 colorA(rgba_F32(sample_U32(source, colLow, rowLow)));
				ALIGN16 rgba_F32 colorB(rgba_F32(sample_U32(source, colHigh, rowLow)));
				ALIGN16 rgba_F32 colorC(rgba_F32(sample_U32(source, colLow, rowHigh)));
				ALIGN16 rgba_F32 colorD(rgba_F32(sample_U32(source, colHigh, rowHigh)));

				ALIGN16 F32x4 weightX = pixX - floatFromU32(pixLowX);
				ALIGN16 F32x4 weightY = pixY - floatFromU32(pixLowY);
				ALIGN16 F32x4 invWeightX = 1.0f - weightX;
				ALIGN16 F32x4 invWeightY = 1.0f - weightY;
				return (colorA * invWeightX + colorB * weightX) * invWeightY + (colorC * invWeightX + colorD * weightX) * weightY;
			} else { // Fast interpolation
				return rgba_F32(sample_U32<Interpolation::BL>(source, u, v));
			}
		} else { // Interpolation::NN or unhandled
			return rgba_F32(sample_U32<Interpolation::NN>(source, u, v));
		}
	}

	// Multi layer sampling method
	// Precondition: u, v > -0.875f = 1 - (0.5 / minimumMipSize)
	template<Interpolation INTERPOLATION>
	inline U32x4 sample_U32(const TextureRgba *source, const F32x4 &u, const F32x4 &v) {
		int mipLevel = getMipLevel(source, u, v);
		return sample_U32<INTERPOLATION>(&(source->mips[mipLevel]), u, v);
	}

	// Precondition: u, v > -0.875f = 1 - (0.5 / minimumMipSize)
	template<Interpolation INTERPOLATION, bool HIGH_QUALITY>
	inline rgba_F32 sample_F32(const TextureRgba *source, const F32x4 &u, const F32x4 &v) {
		int mipLevel = getMipLevel(source, u, v);
		return sample_F32<INTERPOLATION, HIGH_QUALITY>(&(source->mips[mipLevel]), u, v);
	}
}

}

#endif

