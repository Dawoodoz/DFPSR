
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

// Everything stored directly in the image types is immutable to allow value types to behave like reference types using the data that they point to.
// Image types can not be dynamically casted, because the inheritance is entirely static without any virtual functions.

#ifndef DFPSR_API_TEXTURE
#define DFPSR_API_TEXTURE

#include "../implementation/image/Texture.h"
#include "../implementation/image/Image.h"
#ifndef NDEBUG
	#include "../api/stringAPI.h"
#endif
#include "../base/DsrTraits.h"

namespace dsr {
	// Post-condition: Returns true iff texture exists.
	inline bool texture_exists(const Texture &texture) { return texture.impl_buffer.isNotNull(); }

	// Post-condition: Returns the width in pixels for the highest resolution at mip level 0.
	inline int32_t texture_getMaxWidth(const Texture &texture) { return int32_t(1) << texture.impl_log2width; }

	// Post-condition: Returns the width in pixels for the resolution at mipLevel.
	inline int32_t texture_getWidth(const Texture &texture, uint32_t mipLevel) { return int32_t(1) << (texture.impl_log2width - mipLevel); }

	// Post-condition: Returns the height in pixels for the highest resolution at mip level 0.
	inline int32_t texture_getMaxHeight(const Texture &texture) { return int32_t(1) << texture.impl_log2height; }

	// Post-condition: Returns the height in pixels for the resolution at mipLevel.
	inline int32_t texture_getHeight(const Texture &texture, uint32_t mipLevel) { return int32_t(1) << (texture.impl_log2height - mipLevel); }

	// Get the maximum mip level, with zero overhead.
	// Post-condition: Returns an index to the highest mip level.
	inline int32_t texture_getSmallestMipLevel(const TextureRgbaU8& texture) { return texture.impl_maxMipLevel; }

	// Get the number of mip levels, or zero if the texture does not exist.
	//   Useful for looping over all mip levels in a texture, by automatically skipping texture with no mip levels.
	// Post-condition: Returns the number of mip levels.
	inline int32_t texture_getMipLevelCount(const TextureRgbaU8& texture) { return texture_exists(texture) ? texture.impl_maxMipLevel + 1 : 0; }

	// Post-condition: Returns true iff texture has more than one mip level, so that updating the highest resolution needs to update lower layers.
	inline bool texture_hasPyramid(const Texture &texture) { return texture.impl_maxMipLevel != 0; }

	// Side-effect: Update all lower resolutions from the highest resolution using a basic linear average.
	void texture_generatePyramid(const TextureRgbaU8& texture);

	// mipLevel starts from 0 at the highest resolution and ends with the lowest resolution.
	// Pre-condition:
	//   0 <= mipLevel <= 15
	// Post-condition:
	//   Returns the number of pixels from lower resolutions before the start of mipLevel.
	//   Always returns 0 when there is only one mip level available.
	template<
	  bool HIGHEST_RESOLUTION = false,
	  typename M, // uint32_t, U32x4, U32x8, U32xX
	  DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, M))>
	inline M texture_getPixelOffsetToLayer(const TextureRgbaU8 &texture, const M &mipLevel) {
		if (HIGHEST_RESOLUTION) {
			return M(texture.impl_startOffset);
		} else {
			return M(texture.impl_startOffset) & (M(texture.impl_maxLevelMask) >> bitShiftLeftImmediate<1>(mipLevel));
		}
	}

	// TODO: Use SQUARE AND SINGLE_LAYER to generate faster specialized shaders.
	// TODO: Generate an array of as many mip levels as the mip calculation generates ahead of time to accelerate texture sampling.

	// mipLevel starts from 0 at the highest resolution and ends with the lowest resolution.
	// Optimization arguments:
	//   * SQUARE can be set to true when you know in compile time that texture has the same width and height.
	//   * SINGLE_LAYER can be set to true when you know in compile time that there will only be a single resolution in the texture.
	//   * XY_INSIDE can be set to true if you know that the pixel coordinates will always be within texture bounds (0 <= x < width, 0 <= y < height) without tiling.
	//   * MIP_INSIDE can be set to true if you know that the mip level will always be within used indices (mipLevel <= texture_getSmallestMipLevel(texture)) without clamping.
	//     Either way, mipLevel must always be within the 0..15 range, because dynamic bit shifting might truncate offsets that are too big.
	//   * HIGHEST_RESOLUTION can be set to true if you want to ignore mipLevel and always sample the highest resolution at mipLevel 0.
	// Pre-condition:
	//   mipLevel <= 15
	// Post-condition:
	//   Returns the number of pixels before the pixel at (x, y) in mipLevel.
	template<
	  bool SQUARE = false,             // Width and height must be the same.
	  bool SINGLE_LAYER = false,       // Demanding that the texture only has a single layer.
	  bool XY_INSIDE = false,          // No pixels may be sampled outside.
	  bool MIP_INSIDE = false,         // Mip level may not go outside of existing layer indices.
	  bool HIGHEST_RESOLUTION = false, // Ignoring any lower layers.
	  typename U, // uint32_t, U32x4, U32x8, U32xX
	  typename M, // uint32_t or the same type as U (TODO: Constrain M to this condition)
	  DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U) && DSR_CHECK_PROPERTY(DsrTrait_Any_U32, M))>
	U texture_getPixelOffset(const TextureRgbaU8 &texture, const U &x, const U &y, const M &mipLevel) {
		// Clamp the mip-level using bitwise operations in a logarithmic scale, by masking out excess bits with zeroes and filling missing bits with ones.
		M tileMaskX = M(texture.impl_maxWidthAndMask );
		M tileMaskY = M(texture.impl_maxHeightAndMask);
		if (!SINGLE_LAYER && !HIGHEST_RESOLUTION) {
			tileMaskX = tileMaskX >> mipLevel;
			tileMaskY = tileMaskY >> mipLevel;
		}
		if (!MIP_INSIDE) {
			// If the mip level index might be higher than what is used in the texture, make sure that the tile masks have at least enough bits for the lowest texture resolution.
			tileMaskX = tileMaskX | texture.impl_minWidthOrMask;
			if (!SQUARE) {
				tileMaskY = tileMaskY | texture.impl_minHeightOrMask;
			}
		}
		M log2PixelStride = M(texture.impl_log2width);
		if (!SINGLE_LAYER && !HIGHEST_RESOLUTION) {
			log2PixelStride = log2PixelStride - mipLevel;
		}
		U tiledX = x;
		U tiledY = y;
		if (!XY_INSIDE) {
			tiledX = tiledX & tileMaskX;
			if (SQUARE) {
				// Apply the same mask to both for square images, so that the other mask can be optimized away.
				tiledY = tiledY & tileMaskX;
			} else {
				// Apply a separate mask for Y coordinates when the texture might not be square.
				tiledY = tiledY & tileMaskY;
			}
		}
		U coordinateOffset = ((tiledY << log2PixelStride) | tiledX);
		#ifndef NDEBUG
			// In debug mode, wrong use of optimization arguments will throw errors.
			if (SQUARE) {
				if (texture.impl_log2width != texture.impl_log2height) {
					throwError(U"texture_getPixelOffset was told that the texture would have square dimensions using SQUARE, but ", texture_getMaxWidth(texture), U"x", texture_getMaxHeight(texture), U" is not square!\n");
				}
			}
			if (SINGLE_LAYER) {
				if (texture_getSmallestMipLevel(texture) > 0) {
					throwError(U"texture_getPixelOffset was told that the texture would only have a single layer using SINGLE_LAYER, but it has ", texture_getSmallestMipLevel(texture) + 1, U" layers!\n");
				}
			}
			if (XY_INSIDE) {
				if (!(allLanesEqual(x & ~tileMaskX, U(0)) && allLanesEqual(y & ~tileMaskY, U(0)))) {
					throwError(U"texture_getPixelOffset was told that the pixel coordinates would stay inside using XY_INSIDE, but the coordinate (", x, U", ", y, U") is not within", texture_getMaxWidth(texture), U"x", texture_getMaxHeight(texture), U" pixels!\n");
				}
			}
			if (!SINGLE_LAYER && !HIGHEST_RESOLUTION) {
				if (!allLanesLesserOrEqual(mipLevel, M(15u))) {
					throwError(U"texture_getPixelOffset got mip level ", mipLevel, U", which is not within the fixed range of 0..15!\n");
				}
				if (MIP_INSIDE) {
					if (!allLanesLesserOrEqual(mipLevel, M(texture_getSmallestMipLevel(texture)))) {
						throwError(U"texture_getPixelOffset was told that the mip level would stay within valid indices using MIP_INSIDE, but mip level ", mipLevel, U" is not within 0..", texture_getSmallestMipLevel(texture), U"!\n");
					}
				}
			}
		#endif
		if (SINGLE_LAYER) {
			return coordinateOffset;
		} else {
			U startOffset = U(texture_getPixelOffsetToLayer<HIGHEST_RESOLUTION>(texture, mipLevel));
			return startOffset + coordinateOffset;
		}
	}

	template<
	  bool SQUARE = false,
	  bool SINGLE_LAYER = false,
	  bool XY_INSIDE = false,
	  bool MIP_INSIDE = false,
	  bool HIGHEST_RESOLUTION = false,
	  typename U, // uint32_t, U32x4, U32x8, U32xX
	  typename M, // uint32_t or the same type as U (TODO: Constrain M to this condition)
	  DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U) && DSR_CHECK_PROPERTY(DsrTrait_Any_U32, M))>
	inline U texture_readPixel(const TextureRgbaU8 &texture, const U &x, const U &y, const M &mipLevel) {
		#ifndef NDEBUG
			if (!texture_exists(texture)) {
				throwError(U"Tried to read pixels from a texture that does not exist!\n");
			}
			if (!HIGHEST_RESOLUTION) {
				if (!allLanesLesserOrEqual(mipLevel, M(15u))) {
					throwError(U"Tried to read pixels from mip level ", mipLevel, U", which is outside of the allowed 4-bit range 0..4!\n");
				}
			}
		#endif
		SafePointer<uint32_t> data = texture.impl_buffer.getSafe<uint32_t>("RgbaU8 pyramid pixel buffer for pixel reading");
		return gather_U32(data, texture_getPixelOffset<SQUARE, SINGLE_LAYER, XY_INSIDE, MIP_INSIDE, HIGHEST_RESOLUTION, U>(texture, x, y, mipLevel));
	}

	// Pre-condition:
	//   0 <= mipLevel <= 15
	inline void texture_writePixel(const TextureRgbaU8 &texture, uint32_t x, uint32_t y, uint32_t mipLevel, uint32_t packedColor) {
		#ifndef NDEBUG
			if (!texture_exists(texture)) {
				throwError(U"Tried to write a pixel to a texture that does not exist!\n");
			}
			if (mipLevel > 15u) {
				throwError(U"Tried to write a pixel to mip level ", mipLevel, U", which is outside of the allowed 4-bit range 0..4!\n");
			}
		#endif
		SafePointer<uint32_t> data = texture.impl_buffer.getSafe<uint32_t>("RgbaU8 pyramid pixel buffer for pixel writing");
		data[texture_getPixelOffset<false, false, false, false, false, uint32_t>(texture, x, y, mipLevel)] = packedColor;
	}

	// TODO: Use these template arguments in RgbaMultiply.h to improve performance for square textures with at least 4 mip levels and UV coordinates inside of the texture.
	// TODO: Can EXISTS be an argument to disable when non-existing images should be replaced with U(255u) for fast prototyping?
	// Sample the nearest pixel in a normalized UV scale where one unit equals one lap around the image.
	// Pre-condition:
	//   0.0f <= u, 0.0f <= v
	//   Negative texture coordinates are not allowed, because they are converted to unsigned integers for bitwise operations.
	template<
	  bool SQUARE = false,
	  bool SINGLE_LAYER = false,
	  bool MIP_INSIDE = false,
	  bool HIGHEST_RESOLUTION = false,
	  typename F, // float, F32x4, F32x8, F32xX, F32xF
	  typename M, // uint32_t or a SIMD vector of 32-bit unsigned integers with the same number of lanes of u and v
	  DSR_ENABLE_IF(DSR_CHECK_PROPERTY(DsrTrait_Any_F32, F) && DSR_CHECK_PROPERTY(DsrTrait_Any_U32, M))>
	inline auto texture_sample_nearest(const TextureRgbaU8 &texture, const F &u, const F &v, const M &mipLevel) {
		uint32_t scaleU = 1u << texture.impl_log2width;
		uint32_t scaleV = 1u << texture.impl_log2height;
		if (!HIGHEST_RESOLUTION) {
			scaleU = scaleU >> mipLevel;
			scaleV = scaleV >> mipLevel;
		}
		auto xPixel = truncateToU32(u * floatFromU32(scaleU));
		auto yPixel = truncateToU32(v * floatFromU32(scaleV));
		return texture_readPixel<SQUARE, SINGLE_LAYER, false, MIP_INSIDE, HIGHEST_RESOLUTION>(texture, xPixel, yPixel, mipLevel);
	}

	// Internal helper function, not a part of the API!
	// Returns (colorA * weightA + colorB * weightB) / 256 as bytes
	// weightA and weightB should contain pairs of the same 16-bit weights for each of the 4 pixels in the corresponding A and B colors
	template <typename U32, typename U16, DSR_ENABLE_IF(
	  DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U32) &&
	  DSR_CHECK_PROPERTY(DsrTrait_Any_U16, U16)
	)>
	inline U32 weightColors(const U32 &colorA, const U16 &weightA, const U32 &colorB, const U16 &weightB) {
		U32 lowMask(0x00FF00FFu);
		U16 lowColorA = reinterpret_U16FromU32(colorA & lowMask);
		U16 lowColorB = reinterpret_U16FromU32(colorB & lowMask);
		U32 highMask(0xFF00FF00u);
		U16 highColorA = reinterpret_U16FromU32(bitShiftRightImmediate<8>(colorA & highMask));
		U16 highColorB = reinterpret_U16FromU32(bitShiftRightImmediate<8>(colorB & highMask));
		U32 lowColor = reinterpret_U32FromU16(((lowColorA * weightA) + (lowColorB * weightB)));
		U32 highColor = reinterpret_U32FromU16(((highColorA * weightA) + (highColorB * weightB)));
		return ((bitShiftRightImmediate<8>(lowColor) & lowMask) | (highColor & highMask));
	}

	// Internal helper function, not a part of the API!
	// The more significant bits must be zero so that the lower bits can fill the space.
	//   lowBits[x] < 2¹⁶
	template <typename U32, DSR_ENABLE_IF(
	  DSR_CHECK_PROPERTY(DsrTrait_Any_U32, U32)
	)>
	inline auto repeatAs16Bits(const U32 &lowBits) {
		return reinterpret_U16FromU32(lowBits | bitShiftLeftImmediate<16>(lowBits));
	}

	// Internal helper function, not a part of the API!
	// Returns 256 - weight
	template <typename U16, DSR_ENABLE_IF(
	  DSR_CHECK_PROPERTY(DsrTrait_Any_U16, U16)
	)>
	inline U16 invertWeight(const U16 &weight) {
		return U16(0x0100u) - weight;
	}

	// TODO: Implement a scalar version for easy sampling of colors in reference implementations.
	// A X-->    B
	// Pre-condition:
	//   0 <= weight <= 256
	// Post-condition: Returns a bi-linear color mix of colors A and B.
	// texture_interpolate_color_linear(a, b, 0) = a
	// texture_interpolate_color_linear(a, b, 128) = floor((a + b) / 2)
	// texture_interpolate_color_linear(a, b, 256) = b
	template <typename U32>
	inline U32 texture_interpolate_color_linear(const U32 &colorA, const U32 &colorB, const U32 &weight) {
		// Get inverse weights
		auto weightB = repeatAs16Bits(weight);
		auto weightA = invertWeight(weightB);
		// Multiply
		return weightColors(colorA, weightA, colorB, weightB);
	}

	// TODO: Implement a scalar version for easy sampling of colors in reference implementations.
	// A X-->    B
	// Y
	// |
	// V
	//
	// C         D
	// Pre-condition:
	//   0 <= weightX <= 256
	//   0 <= weightY <= 256
	// Post-condition: Returns a bi-linear color mix of colors A, B, C, D using weights X and Y.
	// texture_interpolate_color_bilinear(a, b, c, d,   0,   0) = a
	// texture_interpolate_color_bilinear(a, b, c, d, 256,   0) = b
	// texture_interpolate_color_bilinear(a, b, c, d,   0, 256) = c
	// texture_interpolate_color_bilinear(a, b, c, d, 256, 256) = d
	// texture_interpolate_color_bilinear(a, b, c, d, 128, 128) = floor((a + b + c + d) / 4)
	template <typename U32>
	inline U32 texture_interpolate_color_bilinear(const U32 &colorA, const U32 &colorB, const U32 &colorC, const U32 &colorD, const U32 &weightX, const U32 &weightY) {
		// Get inverse weights
		auto weightXR = repeatAs16Bits(weightX);
		auto weightYB = repeatAs16Bits(weightY);
		auto weightXL = invertWeight(weightXR);
		auto weightYT = invertWeight(weightYB);
		// Multiply
		return weightColors(weightColors(colorA, weightXL, colorB, weightXR), weightYT, weightColors(colorC, weightXL, colorD, weightXR), weightYB);
	}

	template<
	  bool SQUARE = false,
	  bool SINGLE_LAYER = false,
	  bool MIP_INSIDE = false,
	  bool HIGHEST_RESOLUTION = false,
	  typename F32, // float, F32x4, F32x8, F32xX, F32xF
	  typename M, // uint32_t or the same type as U (TODO: Constrain M to this condition)
	  DSR_ENABLE_IF(
	    DSR_CHECK_PROPERTY(DsrTrait_Any_F32, F32) &&
	    DSR_CHECK_PROPERTY(DsrTrait_Any_U32, M)
	  )>
	inline auto texture_sample_bilinear(const TextureRgbaU8 &texture, const F32 &u, const F32 &v, const M &mipLevel) {
		M scaleU = M(256u << texture.impl_log2width);
		M scaleV = M(256u << texture.impl_log2height);
		if (!HIGHEST_RESOLUTION) {
			scaleU = scaleU >> mipLevel;
			scaleV = scaleV >> mipLevel;
		}
		// Convert from the normalized 0..1 scale to a 0..size*256 scale for 8 bits of sub-pixel precision.
		//   Half a pixel is subtracted so that the seam between bi-linear patches end up at the center of texels.
		auto subCenterX = truncateToU32(u * floatFromU32(scaleU)) - 128u;
		auto subCenterY = truncateToU32(v * floatFromU32(scaleV)) - 128u;
		// Get the remainders as interpolation weights.
		auto weightX = subCenterX & 0xFF;
		auto weightY = subCenterY & 0xFF;
		// Divide and truncate sub-pixel coordinates to get whole pixel coordinates.
		auto pixelLeft = bitShiftRightImmediate<8>(subCenterX);
		auto pixelTop = bitShiftRightImmediate<8>(subCenterY);
		auto pixelRight = pixelLeft + 1;
		auto pixelBottom = pixelTop + 1;
		// Generate pixel tiling masks.
		decltype(subCenterX) tileMaskX(texture.impl_maxWidthAndMask );
		decltype(subCenterY) tileMaskY(texture.impl_maxHeightAndMask);
		if (!HIGHEST_RESOLUTION) {
			tileMaskX = tileMaskX >> mipLevel;
			tileMaskY = tileMaskY >> mipLevel;
		}
		if (!MIP_INSIDE) {
			tileMaskX = tileMaskX | texture.impl_minWidthOrMask;
			if (!SQUARE) {
				tileMaskY = tileMaskY | texture.impl_minHeightOrMask;
			}
		}
		// Get the stride.
		M log2PixelStride = M(texture.impl_log2width);
		if (!HIGHEST_RESOLUTION) {
			log2PixelStride = log2PixelStride - mipLevel;
		}
		// Apply tiling masks
		pixelLeft = pixelLeft & tileMaskX;
		pixelRight = pixelRight & tileMaskX;
		if (SQUARE) {
			// Apply the same mask to both for square images, so that the other mask can be optimized away.
			pixelTop = pixelTop & tileMaskX;
			pixelBottom = pixelBottom & tileMaskX;
		} else {
			// Apply a separate mask for Y coordinates when the texture might not be square.
			pixelTop = pixelTop & tileMaskY;
			pixelBottom = pixelBottom & tileMaskY;
		}
		#ifndef NDEBUG
			// In debug mode, wrong use of optimization arguments will throw errors.
			if (SQUARE && (texture.impl_log2width != texture.impl_log2height)) {
				throwError(U"texture_getPixelOffset was told that the texture would have square dimensions using SQUARE, but ", texture_getMaxWidth(texture), U"x", texture_getMaxHeight(texture), U" is not square!\n");
			}
			if (SINGLE_LAYER && (texture_getSmallestMipLevel(texture) > 0)) {
				throwError(U"texture_getPixelOffset was told that the texture would only have a single layer using SINGLE_LAYER, but it has ", texture_getSmallestMipLevel(texture) + 1, U" layers!\n");
			}
			if (!HIGHEST_RESOLUTION) {
				if (!allLanesLesserOrEqual(mipLevel, M(15u))) {
					throwError(U"texture_getPixelOffset got mip level ", mipLevel, U", which is not within the fixed range of 0..15!\n");
				}
				if (MIP_INSIDE) {
					if (!allLanesLesserOrEqual(mipLevel, M(texture_getSmallestMipLevel(texture)))) {
						throwError(U"texture_getPixelOffset was told that the mip level would stay within valid indices using MIP_INSIDE, but mip level ", mipLevel, U" is not within 0..", texture_getSmallestMipLevel(texture), U"!\n");
					}
				}
			}
		#endif
		auto upperOffset       = pixelTop    << log2PixelStride;
		auto bottomOffset      = pixelBottom << log2PixelStride;
		auto upperLeftOffset   = upperOffset  | pixelLeft;
		auto upperRightOffset  = upperOffset  | pixelRight;
		auto bottomLeftOffset  = bottomOffset | pixelLeft;
		auto bottomRightOffset = bottomOffset | pixelRight;
		if (!SINGLE_LAYER) {
			auto layerStartOffset(texture_getPixelOffsetToLayer<HIGHEST_RESOLUTION>(texture, mipLevel));
			upperLeftOffset  = upperLeftOffset  + layerStartOffset;
			upperRightOffset = upperRightOffset + layerStartOffset;
			bottomLeftOffset  = bottomLeftOffset  + layerStartOffset;
			bottomRightOffset = bottomRightOffset + layerStartOffset;
		}
		SafePointer<uint32_t> data = texture.impl_buffer.getSafe<uint32_t>("RgbaU8 pyramid pixel buffer for bi-linear pixel sampling");
		auto upperLeftColor   = gather_U32(data, upperLeftOffset  );
		auto upperRightColor  = gather_U32(data, upperRightOffset );
		auto bottomLeftColor  = gather_U32(data, bottomLeftOffset );
		auto bottomRightColor = gather_U32(data, bottomRightOffset);
		return texture_interpolate_color_bilinear(upperLeftColor, upperRightColor, bottomLeftColor, bottomRightColor, weightX, weightY);
	}

	// resolutions is the maximum number of resolutions to create.
	//   The actual number of layers in the texture is limited by the most narrow dimension.
	//   A texture of 16x4 pixels can have up to three resolutions, 4x1, 8x2 and 16x4.
	//   A texture of 8x8 pixels can have up to four resolutions, 1x1, 2x2, 4x4 and 8x8.
	// Pre-condition:
	//   1 <= width <= 32768
	//   1 <= height <= 32768
	//   0 <= resolutions <= 16
	// Post-condition:
	//   Returns a pyramid image of the smallest power of two size capable of storing width x height pixels, by scaling up the resolution with interpolation if needed.
	TextureRgbaU8 texture_create_RgbaU8(int32_t width, int32_t height, int32_t resolutions);
	// Pre-condition:
	//   1 <= width <= 32768
	//   1 <= height <= 32768
	//   1 <= resolutions
	// Post-condition:
	//   Returns a pyramid image created from image, or an empty pyramid if the image is empty.
	TextureRgbaU8 texture_create_RgbaU8(const ImageRgbaU8& image, int32_t resolutions);

	// Get a layer from the texture as an image.
	// Pre-condition:
	//   texture_exists(texture)
	//   0 <= mipLevel <= texture_getSmallestMipLevel(texture)
	// Post-condition:
	//   Returns an unaligned RGBA image sharing pixel data with the requested texture layer.
	ImageRgbaU8 texture_getMipLevelImage(const TextureRgbaU8& texture, int32_t mipLevel);

	// TODO: Pre-calculate the pixel offset, float scales and tile masks and merge into a reusable multi-layer sampling method.
	//       Because dynamic bit shifts can not be vectorized on Intel processors and would be the same for 2x2 pixels anyway.
	//       The hard part will be to implement it for ARM SVE with variable width vectors, so maybe calculate the
	//         2x2 derivation sparsely, interpolate a floating mip level and do comparisons vectorized with blend instructions to select masks.
	template <typename F>
	inline uint32_t texture_getMipLevelIndex(const TextureRgbaU8 &source, const F &u, const F &v) {
		// TODO: Support reading elements from SIMD vectors of any size somehow. Can use SVE's maximum size of 2048 bits as the space to allocate in advance to aligned stack memory.
		// Assume that U is at least 128 bits wide and reuse the result for additional pixels if there is more.
		auto ua = u.get();
		auto va = v.get();
		float offsetUX = fabs(ua.x - ua.y); // Left U - Right  U
		float offsetUY = fabs(ua.x - ua.z); // Top  U - Bottom U
		float offsetVX = fabs(va.x - va.y); // Left V - Right  V
		float offsetVY = fabs(va.x - va.z); // Top  V - Bottom V
		float offsetU = max(offsetUX, offsetUY) * source.impl_floatMaxWidth;
		float offsetV = max(offsetVX, offsetVY) * source.impl_floatMaxHeight;
		float offset = max(offsetU, offsetV);
		int result = 0;
		// TODO: Can count leading zeroes be used with integers to use all available mip levels?
		//       It would make MIP_INSIDE useless for optimization.
		if (offset >  2.0f) { result = 1; }
		if (offset >  4.0f) { result = 2; }
		if (offset >  8.0f) { result = 3; }
		if (offset > 16.0f) { result = 4; }
		// TODO: Should it be possible to configure the number of mip levels?
		return result;
	}

	// TODO: Optimize using template arguments.
	// Pre-conditions:
	//   0 <= mipLevel <= texture_getSmallestMipLevel(texture)
	// Post-condition:
	//   Returns a safe pointer to the first pixel at mipLevel in texture.
	template <typename U = uint32_t>
	inline SafePointer<U> texture_getSafePointer(const TextureRgbaU8& texture, uint32_t mipLevel) {
		// Get a pointer to the start of the image.
		return texture.impl_buffer.getSafe<U>("RgbaU8 pyramid pixel buffer").increaseBytes(texture_getPixelOffsetToLayer(texture, mipLevel) * sizeof(uint32_t));
	}

	// TODO: Optimize using template arguments.
	// Pre-conditions:
	//   0 <= mipLevel <= texture_getSmallestMipLevel(texture)
	//   0 <= rowIndex < (1 << mipLevel)
	// Post-condition:
	//   Returns a safe pointer to the first pixel at rowIndex in mipLevel in texture.
	template <typename U = uint32_t>
	inline SafePointer<U> texture_getSafePointer(const TextureRgbaU8& texture, int32_t mipLevel, int32_t rowIndex) {
		return texture_getSafePointer<U>(texture, mipLevel).increaseBytes(texture_getWidth(texture, mipLevel) * sizeof(uint32_t) * rowIndex);
	}
}

#endif
