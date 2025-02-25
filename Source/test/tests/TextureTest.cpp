
#include "../testTools.h"
#include "../../DFPSR/base/simd.h"
#include "../../DFPSR/implementation/image/PackOrder.h"
#include "../../DFPSR/api/textureAPI.h"

#define ASSERT_EQUAL_SIMD(A, B) ASSERT_COMP(A, B, allLanesEqual, "==")
#define ASSERT_NOTEQUAL_SIMD(A, B) ASSERT_COMP(A, B, !allLanesEqual, "!=")

START_TEST(Texture)
	{
		// Linear blending of colors using unsigned integers.
		U32x4 mixedColor = texture_interpolate_color_linear<U32x4>(
		    packOrder_packBytes(U32x4(255, 175, 253,  95), U32x4(255,  84, 255, 210), U32x4(  0, 253, 172, 100), U32x4(  0, 150, 241,  61)),
		    packOrder_packBytes(U32x4(  0, 215,  62, 127), U32x4(255, 162, 152,  93), U32x4(255,  71,  62, 200), U32x4(  0, 139, 180, 124)),
		    U32x4(  0, 128, 256, 256)
		  );
		U32x4 expectedColor = packOrder_packBytes(U32x4(255, 195,  62, 127), U32x4(255, 123, 152,  93), U32x4(  0, 162,  62, 200), U32x4(  0, 144, 180, 124));
		ASSERT_EQUAL_SIMD(mixedColor, expectedColor);
	}
	{
		// 1x1, 2x2, 4x4, 8x8, 16x16
		TextureRgbaU8 texture = TextureRgbaU8(4, 4);
		ASSERT(texture_hasPyramid(texture));
		ASSERT_EQUAL(texture_getMaxWidth(texture), 16);
		ASSERT_EQUAL(texture_getMaxHeight(texture), 16);
		ASSERT_EQUAL(texture_getSmallestMipLevel(texture), 4);
		ASSERT_EQUAL(texture.impl_startOffset , 0b00000000000000000000000001010101);
		ASSERT_EQUAL(texture.impl_maxLevelMask, 0b00000000000000000000000011111111);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 15u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 14u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 13u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 12u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 11u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 10u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  9u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  8u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  7u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  6u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  5u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  4u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  3u), 0b00000000000000000000000000000001);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  2u), 0b00000000000000000000000000000101);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  1u), 0b00000000000000000000000000010101);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  0u), 0b00000000000000000000000001010101);
		ASSERT_EQUAL(texture_getPixelOffset(texture,   7534u,    424u, 15u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,  75624u,   6217u, 14u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,   8562u,  91287u, 13u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,     66u,   3578u, 12u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,  13593u,  14375u, 11u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,   2586u,   1547u, 10u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,  34589u,   2358u,  9u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture, 835206u,  23817u,  8u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,    265u,   1365u,  7u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,   8520u,   4895u,  6u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,    574u,  86316u,  5u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  4u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      0u,  4u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      1u,  4u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,     25u,     85u,  4u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture, 246753u, 837624u,  4u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  3u),  1u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      0u,  3u),  2u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      1u,  3u),  3u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      1u,  3u),  4u);
		ASSERT_EQUAL(texture_getPixelOffset(texture, 246753u, 837624u,  3u),  2u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      9u,  3u),  3u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,     13u,     79u,  3u),  4u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  2u),  5u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      0u,  2u),  6u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      0u,  2u),  7u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      0u,  2u),  8u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      1u,  2u),  9u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      1u,  2u), 10u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      1u,  2u), 11u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      1u,  2u), 12u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      2u,  2u), 13u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      2u,  2u), 14u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      2u,  2u), 15u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      2u,  2u), 16u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      3u,  2u), 17u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      3u,  2u), 18u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      3u,  2u), 19u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      3u,  2u), 20u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,  65536u,   2050u,  2u), 13u);
		ASSERT_EQUAL(texture_getPixelOffset(texture, 991366u,      5u,  2u), 11u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  1u), 21u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      0u,  1u), 22u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      0u,  1u), 23u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      0u,  1u), 24u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      4u,      0u,  1u), 25u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      5u,      0u,  1u), 26u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      0u,  1u), 27u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      7u,      0u,  1u), 28u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      1u,  1u), 29u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      1u,  1u), 30u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      1u,  1u), 31u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      1u,  1u), 32u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      4u,      1u,  1u), 33u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      5u,      1u,  1u), 34u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      1u,  1u), 35u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      7u,      1u,  1u), 36u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      2u,  1u), 37u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      2u,  1u), 38u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      2u,  1u), 39u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      2u,  1u), 40u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      4u,      2u,  1u), 41u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      5u,      2u,  1u), 42u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      2u,  1u), 43u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      7u,      2u,  1u), 44u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      3u,  1u), 45u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      3u,  1u), 46u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      3u,  1u), 47u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      3u,  1u), 48u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      4u,      3u,  1u), 49u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      5u,      3u,  1u), 50u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      3u,  1u), 51u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      7u,      3u,  1u), 52u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      4u,  1u), 53u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      4u,  1u), 54u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      4u,  1u), 55u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      4u,  1u), 56u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      4u,      4u,  1u), 57u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      5u,      4u,  1u), 58u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      4u,  1u), 59u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      7u,      4u,  1u), 60u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      5u,  1u), 61u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      5u,  1u), 62u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      5u,  1u), 63u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      5u,  1u), 64u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      4u,      5u,  1u), 65u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      5u,      5u,  1u), 66u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      5u,  1u), 67u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      7u,      5u,  1u), 68u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      6u,  1u), 69u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      6u,  1u), 70u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      6u,  1u), 71u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      6u,  1u), 72u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      4u,      6u,  1u), 73u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      5u,      6u,  1u), 74u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      6u,  1u), 75u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      7u,      6u,  1u), 76u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      7u,  1u), 77u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      7u,  1u), 78u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      7u,  1u), 79u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      7u,  1u), 80u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      4u,      7u,  1u), 81u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      5u,      7u,  1u), 82u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      6u,      7u,  1u), 83u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      7u,      7u,  1u), 84u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,     37u,    132u,  1u), 58u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,    518u,    260u,  1u), 59u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,     15u,     15u,  1u), 84u);
		ASSERT_EQUAL(texture_getPixelOffset(texture, 0u, 0u, 0u), 85u);

		// The four first template arguments to texture_getPixelOffset are SQUARE, SINGLE_LAYER, XY_INSIDE and MIP_INSIDE, which can be used to simplify the calculations with any information known in compile time.

		// Optimized by saying that the image is a square, with multiple levels, and mip level within used bounds.
		uint32_t result = texture_getPixelOffset<true, false, true, true>(texture, 0u, 0u, 0u);
		ASSERT_EQUAL(result, 85u);
		#ifndef NDEBUG
			// Should crash with an error when making a false claim that the texture only has a single layer.
			BEGIN_CRASH(U"texture_getPixelOffset was told that the texture would only have a single layer");
				result = texture_getPixelOffset<false, true, false, false>(texture, 0u, 0u, 0u);
			END_CRASH
		#endif
		ASSERT_EQUAL_SIMD(texture_getPixelOffset(texture, U32x4(0u, 0u, 0u, 0u), U32x4(0u, 0u, 0u, 0u), U32x4(0u, 1u, 2u, 3u)), U32x4(85u, 21u, 5u, 1u));
		ASSERT_EQUAL_SIMD(texture_getPixelOffset(texture, U32x4(0u, 1u, 0u, 1u), U32x4(0u, 0u, 1u, 1u), U32x4(3u, 3u, 3u, 3u)), U32x4(1u, 2u, 3u, 4u));
		ASSERT_EQUAL_SIMD(texture_getPixelOffset(texture, U32x4(2u, 3u, 0u, 1u), U32x4(0u, 0u, 1u, 1u), U32x4(0u)), U32x4(87u, 88u, 101u, 102u));
		ASSERT_EQUAL_SIMD(texture_getPixelOffset(texture, U32x4(2u, 3u, 0u, 1u), U32x4(0u, 0u, 1u, 1u), U32x4(1u)), U32x4(23u, 24u, 29u, 30u));
		ASSERT_EQUAL_SIMD(texture_getPixelOffset(texture, U32x4(2u, 3u, 0u, 1u), U32x4(0u, 0u, 1u, 1u), U32x4(2u)), U32x4(7u, 8u, 9u, 10u));
		ASSERT_EQUAL_SIMD(texture_getPixelOffset(texture, U32x8(0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u), U32x8(0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u), U32x8(0u)), U32x8(85u, 86u, 87u, 88u, 101u, 102u, 103u, 104u));
		ASSERT_EQUAL_SIMD(texture_getPixelOffset(texture, U32x8(0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u), U32x8(0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u), U32x8(1u)), U32x8(21u, 22u, 23u, 24u, 29u, 30u, 31u, 32u));
		ASSERT_EQUAL_SIMD(texture_getPixelOffset(texture, U32x8(0u, 1u, 2u, 3u, 0u, 1u, 2u, 3u), U32x8(0u, 0u, 0u, 0u, 1u, 1u, 1u, 1u), U32x8(2u)), U32x8(5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u));
	}
	{
		// 1x2, 2x4, 4x8
		TextureRgbaU8 texture = TextureRgbaU8(2, 3);
		ASSERT(texture_hasPyramid(texture));
		ASSERT_EQUAL(texture_getMaxWidth(texture), 4);
		ASSERT_EQUAL(texture_getMaxHeight(texture), 8);
		ASSERT_EQUAL(texture_getSmallestMipLevel(texture), 2);
		ASSERT_EQUAL(texture.impl_startOffset , 0b00000000000000000000000000001010);
		ASSERT_EQUAL(texture.impl_maxLevelMask, 0b00000000000000000000000000011111);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 15u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 14u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 13u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 12u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 11u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 10u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  9u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  8u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  7u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  6u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  5u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  4u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  3u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  2u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  1u), 0b00000000000000000000000000000010);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  0u), 0b00000000000000000000000000001010);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  15u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  14u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  13u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  12u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  11u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,  10u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   9u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   8u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   7u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   6u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   5u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   4u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   3u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   2u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,     63u,      0u,   2u),  0u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      1u,   2u),  1u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,     94u,      7u,   2u),  1u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   1u),  2u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      0u,   1u),  3u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      1u,   1u),  4u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      1u,   1u),  5u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      2u,   1u),  6u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      2u,   1u),  7u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      3u,   1u),  8u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      3u,   1u),  9u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      0u,   0u), 10u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      0u,   0u), 11u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      0u,   0u), 12u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      0u,   0u), 13u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      1u,   0u), 14u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      1u,   0u), 15u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      1u,   0u), 16u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      1u,   0u), 17u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      2u,   0u), 18u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      2u,   0u), 19u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      2u,   0u), 20u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      2u,   0u), 21u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      3u,   0u), 22u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      3u,   0u), 23u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      3u,   0u), 24u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      3u,   0u), 25u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      4u,   0u), 26u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      4u,   0u), 27u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      4u,   0u), 28u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      4u,   0u), 29u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      5u,   0u), 30u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,     32u,     29u,   0u), 30u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      5u,   0u), 31u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      5u,   0u), 32u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      5u,   0u), 33u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      6u,   0u), 34u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      6u,   0u), 35u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      6u,   0u), 36u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      6u,   0u), 37u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      0u,      7u,   0u), 38u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      1u,      7u,   0u), 39u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      2u,      7u,   0u), 40u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,      3u,      7u,   0u), 41u);
	}
	{
		// 2x1, 4x2, 8x4, 16x8
		TextureRgbaU8 texture = TextureRgbaU8(4, 3);
		ASSERT(texture_hasPyramid(texture));
		ASSERT_EQUAL(texture_getMaxWidth(texture), 16);
		ASSERT_EQUAL(texture_getMaxHeight(texture), 8);
		ASSERT_EQUAL(texture_getSmallestMipLevel(texture), 3);
		ASSERT_EQUAL(texture.impl_startOffset , 0b00000000000000000000000000101010);
		ASSERT_EQUAL(texture.impl_maxLevelMask, 0b00000000000000000000000001111111);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 15u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 14u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 13u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 12u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 11u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 10u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  9u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  8u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  7u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  6u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  5u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  4u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  3u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  2u), 0b00000000000000000000000000000010);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  1u), 0b00000000000000000000000000001010);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  0u), 0b00000000000000000000000000101010);
	}
	{
		// 4x4, 8x8, 16x16, 32x32
		TextureRgbaU8 texture = TextureRgbaU8(5, 5, 3);
		ASSERT(texture_hasPyramid(texture));
		ASSERT_EQUAL(texture_getMaxWidth(texture), 32);
		ASSERT_EQUAL(texture_getMaxHeight(texture), 32);
		ASSERT_EQUAL(texture_getSmallestMipLevel(texture), 3);
		ASSERT_EQUAL(texture.impl_startOffset , 0b00000000000000000000000101010000);
		ASSERT_EQUAL(texture.impl_maxLevelMask, 0b00000000000000000000001111111111);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 15u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 14u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 13u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 12u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 11u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 10u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  9u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  8u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  7u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  6u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  5u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  4u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  3u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  2u), 0b00000000000000000000000000010000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  1u), 0b00000000000000000000000001010000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  0u), 0b00000000000000000000000101010000);
		ASSERT_EQUAL(texture_getPixelOffset(texture,    0u,    0u, 0u),  336u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,   32u,   32u, 0u),  336u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,   64u,   64u, 0u),  336u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,  128u,  128u, 0u),  336u);
		ASSERT_EQUAL(texture_getPixelOffset(texture, 8192u, 8192u, 0u),  336u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,   31u,    0u, 0u),  367u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,    0u,    1u, 0u),  368u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,    0u,   31u, 0u), 1328u);
		ASSERT_EQUAL(texture_getPixelOffset(texture,   31u,   31u, 0u), 1359u);
	}
	{
		// 16x8, 32x16
		TextureRgbaU8 texture = TextureRgbaU8(5, 4, 1);
		ASSERT(texture_hasPyramid(texture));
		ASSERT_EQUAL(texture_getMaxWidth(texture), 32);
		ASSERT_EQUAL(texture_getMaxHeight(texture), 16);
		ASSERT_EQUAL(texture_getSmallestMipLevel(texture), 1);
		ASSERT_EQUAL(texture.impl_startOffset , 0b00000000000000000000000010000000);
		ASSERT_EQUAL(texture.impl_maxLevelMask, 0b00000000000000000000000111111111);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 15u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 14u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 13u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 12u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 11u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 10u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  9u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  8u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  7u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  6u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  5u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  4u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  3u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  2u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  1u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  0u), 0b00000000000000000000000010000000);
	}
	{
		// 16x32
		TextureRgbaU8 texture = TextureRgbaU8(4, 5, 0);
		ASSERT(!texture_hasPyramid(texture));
		ASSERT_EQUAL(texture_getMaxWidth(texture), 16);
		ASSERT_EQUAL(texture_getMaxHeight(texture), 32);
		ASSERT_EQUAL(texture_getSmallestMipLevel(texture), 0);
		ASSERT_EQUAL(texture.impl_startOffset , 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture.impl_maxLevelMask, 0b00000000000000000000000111111111);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 15u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 14u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 13u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 12u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 11u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture, 10u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  9u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  8u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  7u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  6u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  5u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  4u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  3u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  2u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  1u), 0b00000000000000000000000000000000);
		ASSERT_EQUAL(texture_getPixelOffsetToLayer(texture,  0u), 0b00000000000000000000000000000000);
	}
	{
		// 1x1, 2x2, 4x4
		TextureRgbaU8 texture = TextureRgbaU8(2, 2);
		texture_writePixel(texture, 0u, 0u, 2u, 1000u);
		texture_writePixel(texture, 0u, 0u, 1u, 1001u);
		texture_writePixel(texture, 1u, 0u, 1u, 1101u);
		texture_writePixel(texture, 0u, 1u, 1u, 1011u);
		texture_writePixel(texture, 1u, 1u, 1u, 1111u);
		texture_writePixel(texture, 0u, 0u, 0u, 1002u);
		texture_writePixel(texture, 1u, 0u, 0u, 1102u);
		texture_writePixel(texture, 2u, 0u, 0u, 1202u);
		texture_writePixel(texture, 3u, 0u, 0u, 1302u);
		texture_writePixel(texture, 0u, 1u, 0u, 1012u);
		texture_writePixel(texture, 1u, 1u, 0u, 1112u);
		texture_writePixel(texture, 2u, 1u, 0u, 1212u);
		texture_writePixel(texture, 3u, 1u, 0u, 1312u);
		texture_writePixel(texture, 0u, 2u, 0u, 1022u);
		texture_writePixel(texture, 1u, 2u, 0u, 1122u);
		texture_writePixel(texture, 2u, 2u, 0u, 1222u);
		texture_writePixel(texture, 3u, 2u, 0u, 1322u);
		texture_writePixel(texture, 0u, 3u, 0u, 1032u);
		texture_writePixel(texture, 1u, 3u, 0u, 1132u);
		texture_writePixel(texture, 2u, 3u, 0u, 1232u);
		texture_writePixel(texture, 3u, 3u, 0u, 1332u);
		ASSERT_EQUAL(texture_readPixel(texture, 0u, 0u, 2u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture, 0u, 0u, 1u), 1001u);
		ASSERT_EQUAL(texture_readPixel(texture, 1u, 0u, 1u), 1101u);
		ASSERT_EQUAL(texture_readPixel(texture, 0u, 1u, 1u), 1011u);
		ASSERT_EQUAL(texture_readPixel(texture, 1u, 1u, 1u), 1111u);
		ASSERT_EQUAL(texture_readPixel(texture, 0u, 0u, 0u), 1002u);
		ASSERT_EQUAL(texture_readPixel(texture, 1u, 0u, 0u), 1102u);
		ASSERT_EQUAL(texture_readPixel(texture, 2u, 0u, 0u), 1202u);
		ASSERT_EQUAL(texture_readPixel(texture, 3u, 0u, 0u), 1302u);
		ASSERT_EQUAL(texture_readPixel(texture, 0u, 1u, 0u), 1012u);
		ASSERT_EQUAL(texture_readPixel(texture, 1u, 1u, 0u), 1112u);
		ASSERT_EQUAL(texture_readPixel(texture, 2u, 1u, 0u), 1212u);
		ASSERT_EQUAL(texture_readPixel(texture, 3u, 1u, 0u), 1312u);
		ASSERT_EQUAL(texture_readPixel(texture, 0u, 2u, 0u), 1022u);
		ASSERT_EQUAL(texture_readPixel(texture, 1u, 2u, 0u), 1122u);
		ASSERT_EQUAL(texture_readPixel(texture, 2u, 2u, 0u), 1222u);
		ASSERT_EQUAL(texture_readPixel(texture, 3u, 2u, 0u), 1322u);
		ASSERT_EQUAL(texture_readPixel(texture, 0u, 3u, 0u), 1032u);
		ASSERT_EQUAL(texture_readPixel(texture, 1u, 3u, 0u), 1132u);
		ASSERT_EQUAL(texture_readPixel(texture, 2u, 3u, 0u), 1232u);
		ASSERT_EQUAL(texture_readPixel(texture, 3u, 3u, 0u), 1332u);
		ASSERT_EQUAL(texture_readPixel(texture, 7u, 3u, 0u), 1332u);
		ASSERT_EQUAL(texture_readPixel(texture, 3u, 11u, 0u), 1332u);
		ASSERT_EQUAL(texture_readPixel(texture, 1u, 0u, 2u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture, 0u, 1u, 2u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture, 1u, 1u, 2u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,   426462u, 1257535u,  2u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u,  3u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u,  3u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u,  4u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u,  4u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u,  5u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u,  5u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u,  6u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u,  6u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u,  7u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u,  7u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u,  8u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u,  8u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u,  9u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u,  9u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u, 10u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u, 10u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u, 11u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u, 11u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u, 12u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u, 12u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u, 13u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u, 13u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u, 14u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u, 14u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,        0u,       0u, 15u), 1000u);
		ASSERT_EQUAL(texture_readPixel(texture,    34698u,    7456u, 15u), 1000u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.0f, 0.0f, 2u), 1000u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.7f, 0.1f, 2u), 1000u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.5f, 0.2f, 2u), 1000u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 4.2f, 7.2f, 2u), 1000u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.25f, 0.25f, 1u), 1001u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.75f, 0.25f, 1u), 1101u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.25f, 0.75f, 1u), 1011u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.75f, 0.75f, 1u), 1111u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.5f / 4.0f, 0.5f / 4.0f, 0u), 1002u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 1.5f / 4.0f, 0.5f / 4.0f, 0u), 1102u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 2.5f / 4.0f, 0.5f / 4.0f, 0u), 1202u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 3.5f / 4.0f, 0.5f / 4.0f, 0u), 1302u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.5f / 4.0f, 1.5f / 4.0f, 0u), 1012u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 1.5f / 4.0f, 1.5f / 4.0f, 0u), 1112u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 2.5f / 4.0f, 1.5f / 4.0f, 0u), 1212u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 3.5f / 4.0f, 1.5f / 4.0f, 0u), 1312u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.5f / 4.0f, 2.5f / 4.0f, 0u), 1022u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 1.5f / 4.0f, 2.5f / 4.0f, 0u), 1122u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 2.5f / 4.0f, 2.5f / 4.0f, 0u), 1222u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 3.5f / 4.0f, 2.5f / 4.0f, 0u), 1322u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 0.5f / 4.0f, 3.5f / 4.0f, 0u), 1032u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 1.5f / 4.0f, 3.5f / 4.0f, 0u), 1132u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 2.5f / 4.0f, 3.5f / 4.0f, 0u), 1232u);
		ASSERT_EQUAL(texture_sample_nearest(texture, 3.5f / 4.0f, 3.5f / 4.0f, 0u), 1332u);
		ASSERT_EQUAL(texture_sample_nearest(texture, -53.0f, -17.0f,  2u), 1000u);
		ASSERT_EQUAL(texture_sample_nearest(texture, -53.0f, -17.0f,  3u), 1000u);
		ASSERT_EQUAL(texture_sample_nearest(texture, -53.0f, -17.0f, 15u), 1000u);
		// TODO: Test the optimization template flags.
	}
		// TODO: Test reading pixels from SafePointer with and without a specified row index.
	{
		/*
		OrderedImageRgbaU8 originalImage = filter_generateRgbaU8(64, 64, [](int x, int y) -> ColorRgbaI32 {
			return ColorRgbaI32(x * 4, y * 4, 0, 255);
		});
		TextureRgbaU8 texture = texture_create_RgbaU8(originalImage);
		*/
		// TODO: Do some kind of test with the texture.
		// TODO: Allow creating an unaligned image pointing directly to a specific mip level's pixel data, so that it can easily be drawn for debugging.
	}
	// TODO: Create equivalent functionality that can easily replace the old interface.
	{ // RGBA Texture
		/*
		ImageRgbaU8 image;
		image = image_create_RgbaU8(256, 256);
		ASSERT_EQUAL(image_hasPyramid(image), false);
		image_generatePyramid(image);
		ASSERT_EQUAL(image_hasPyramid(image), true);
		image_removePyramid(image);
		ASSERT_EQUAL(image_hasPyramid(image), false);
		image_generatePyramid(image);
		ASSERT_EQUAL(image_hasPyramid(image), true);
		*/
	}
	{ // Texture criterias
		/*
		ImageRgbaU8 image, subImage;
		image = image_create_RgbaU8(16, 16);
		ASSERT_EQUAL(image_isTexture(image), false); // Too small
		image = image_create_RgbaU8(47, 64);
		ASSERT_EQUAL(image_isTexture(image), false); // Not power-of-two width
		image = image_create_RgbaU8(32, 35);
		ASSERT_EQUAL(image_isTexture(image), false); // Not power-of-two height
		image = image_create_RgbaU8(32, 32);
		ASSERT_EQUAL(image_isTexture(image), true); // Okay
		image = image_create_RgbaU8(32, 16384);
		subImage = image_getSubImage(image, IRect(0, 0, 32, 128));
		ASSERT_EQUAL(image_isTexture(image), true); // Okay
		ASSERT_EQUAL(image_isTexture(subImage), true); // Okay to use full-width vertical sub-images
		image = image_create_RgbaU8(16384, 32);
		subImage = image_getSubImage(image, IRect(0, 0, 128, 32));
		ASSERT_EQUAL(image_isTexture(image), true); // Okay
		ASSERT_EQUAL(image_isTexture(subImage), false); // Not okay to use partial width leading to partial stride
		image = image_create_RgbaU8(16384 + 1, 32);
		ASSERT_EQUAL(image_isTexture(image), false); // Too wide and not power-of-two width
		image = image_create_RgbaU8(32768, 32);
		ASSERT_EQUAL(image_isTexture(image), false); // Too wide
		image = image_create_RgbaU8(32, 16384 + 1);
		ASSERT_EQUAL(image_isTexture(image), false); // Too high and not power-of-two height
		image = image_create_RgbaU8(32, 32768);
		ASSERT_EQUAL(image_isTexture(image), false); // Too high
		*/
	}
END_TEST
