// zlib open source license
//
// Copyright (c) 2017 to 2023 David Forsgren Piuva
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

// Hardware abstraction layer for portable SIMD math.
//    Covers a small intersection of SSE2 and NEON in order to reduce the number
//    of bugs from multiple implementations when nothing advanced is required.

#ifndef DFPSR_SIMD
#define DFPSR_SIMD
	#include <stdint.h>
	#include <cassert>
	#include "SafePointer.h"
	#include "../math/FVector.h"
	#include "../math/IVector.h"
	#include "../math/UVector.h"

	// To allow turning off SIMD intrinsics for testing
	#ifdef __SSE2__
		#define USE_SSE2 // Comment out this line to test without SSE2
		#ifdef USE_SSE2
			#ifdef __SSSE3__
				#define USE_SSSE3 // Comment out this line to test without SSSE3
			#endif
			#ifdef __AVX2__
				#define USE_AVX2 // Comment out this line to test without AVX2
			#endif
		#endif
	#elif __ARM_NEON
		#define USE_NEON // Comment out this line to test without NEON
	#endif

	// Enable the EMULATE_256BIT_SIMD macro to force use of 256-bit vectors even when there is no hardware instructions supporting it.
	//   F32xX will then be an alias for F32x8, laneCountX_32Bit will be 8 for iterating your algorithms in larger steps, buffers and image rows will padd and align for 256 bits, et cetera...
	//   This will be slower if only compiling with 128-bit SIMD instructions enabled, but can show if the algorithm is using variable lane count correctly before profiling on a processor that has the extension.
	//   Useful for testing algorithms when the computer used for programming does not have the hardware instructions.
	//   To get real 256-bit SIMD on an Intel processor with AVX2 hardware instructions, enable the AVX2 compiler flag for the library and your project (which is -mavx2 in GCC).
	//#define EMULATE_256BIT_SIMD

	// Portable SIMD flags
	//   USE_BASIC_SIMD is defined when most of the operations have intrinsic functions for 128-bit SIMD.
	//   USE_256BIT_SIMD is defined when most of the operations have intrinsic functions for 256-bit SIMD.
	#ifdef USE_SSE2
		#define USE_BASIC_SIMD
		#ifdef USE_AVX2 // Needs both floats and integers to benefit from using 256-bit vectors.
			#define USE_256BIT_SIMD // Comment out to avoid using 256-bit SIMD vectors, even if the same extensions are still used.
		#endif
	#endif
	#ifdef USE_NEON
		#define USE_BASIC_SIMD
	#endif

	// Alignment in bytes
	#define ALIGN_BYTES(SIZE)  __attribute__((aligned(SIZE)))
	#define ALIGN16 ALIGN_BYTES(16) // 128-bit alignment
	#define ALIGN32 ALIGN_BYTES(32) // 256-bit alignment

	// Everything declared in here handles things specific for SSE.
	// Direct use of the macros will not provide portability to all hardware.
	#ifdef USE_SSE2
		#define USE_DIRECT_SIMD_MEMORY_ACCESS
		#include <emmintrin.h> // SSE2
		#ifdef USE_SSSE3
			#include <tmmintrin.h> // SSSE3
		#endif
		#ifdef USE_AVX2
			#include <immintrin.h> // AVX / AVX2
		#endif

		// Vector types
		#define SIMD_F32x4 __m128
		#define SIMD_U8x16 __m128i
		#define SIMD_U16x8 __m128i
		#define SIMD_U32x4 __m128i
		#define SIMD_I32x4 __m128i

		// Vector uploads in address order
		#define LOAD_VECTOR_F32_SIMD(A, B, C, D) _mm_set_ps(D, C, B, A)
		#define LOAD_SCALAR_F32_SIMD(A) _mm_set1_ps(A)
		#define LOAD_VECTOR_U8_SIMD(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) _mm_set_epi8(P, O, N, M, L, K, J, I, H, G, F, E, D, C, B, A)
		#define LOAD_SCALAR_U8_SIMD(A) _mm_set1_epi8(A)
		#define LOAD_VECTOR_U16_SIMD(A, B, C, D, E, F, G, H) _mm_set_epi16(H, G, F, E, D, C, B, A)
		#define LOAD_SCALAR_U16_SIMD(A) _mm_set1_epi16(A)
		#define LOAD_VECTOR_U32_SIMD(A, B, C, D) _mm_set_epi32(D, C, B, A)
		#define LOAD_SCALAR_U32_SIMD(A) _mm_set1_epi32(A)
		#define LOAD_VECTOR_I32_SIMD(A, B, C, D) _mm_set_epi32(D, C, B, A)
		#define LOAD_SCALAR_I32_SIMD(A) _mm_set1_epi32(A)

		// Conversions
		#define F32_TO_I32_SIMD(A) _mm_cvttps_epi32(A)
		#define F32_TO_U32_SIMD(A) _mm_cvttps_epi32(A)
		#define I32_TO_F32_SIMD(A) _mm_cvtepi32_ps(A)
		#define U32_TO_F32_SIMD(A) _mm_cvtepi32_ps(A)

		// Unpacking conversions
		#define U8_LOW_TO_U16_SIMD(A) _mm_unpacklo_epi8(A, _mm_set1_epi8(0))
		#define U8_HIGH_TO_U16_SIMD(A) _mm_unpackhi_epi8(A, _mm_set1_epi8(0))
		#define U16_LOW_TO_U32_SIMD(A) _mm_unpacklo_epi16(A, _mm_set1_epi16(0))
		#define U16_HIGH_TO_U32_SIMD(A) _mm_unpackhi_epi16(A, _mm_set1_epi16(0))

		// Saturated packing
		//   Credit: Using ideas from Victoria Zhislina's NEON_2_SSE.h header from the Intel corporation, but not trying to emulate NEON
		inline SIMD_U8x16 PACK_SAT_U16_TO_U8_SIMD(const SIMD_U16x8& a, const SIMD_U16x8& b) {
			SIMD_U16x8 mask, a2, b2;
			mask = _mm_set1_epi16(0b0111111111111111);
			a2 = _mm_and_si128(a, mask);
			a2 = _mm_or_si128(a2, _mm_and_si128(_mm_cmpgt_epi16(a2, a), mask));
			b2 = _mm_and_si128(b, mask);
			b2 = _mm_or_si128(b2, _mm_and_si128(_mm_cmpgt_epi16(b2, b), mask));
			return _mm_packus_epi16(a2, b2);
		}

		// Reinterpret casting
		#define REINTERPRET_U32_TO_U8_SIMD(A) (A)
		#define REINTERPRET_U32_TO_U16_SIMD(A) (A)
		#define REINTERPRET_U8_TO_U32_SIMD(A) (A)
		#define REINTERPRET_U16_TO_U32_SIMD(A) (A)
		#define REINTERPRET_U32_TO_I32_SIMD(A) (A)
		#define REINTERPRET_I32_TO_U32_SIMD(A) (A)

		// Vector float operations returning SIMD_F32x4
		#define ADD_F32_SIMD(A, B) _mm_add_ps(A, B)
		#define SUB_F32_SIMD(A, B) _mm_sub_ps(A, B)
		#define MUL_F32_SIMD(A, B) _mm_mul_ps(A, B)

		// Vector integer operations returning SIMD_I32x4
		#define ADD_I32_SIMD(A, B) _mm_add_epi32(A, B)
		#define SUB_I32_SIMD(A, B) _mm_sub_epi32(A, B)
		// 32-bit integer multiplications are not available on SSE2.

		// Vector integer operations returning SIMD_U32x4
		#define ADD_U32_SIMD(A, B) _mm_add_epi32(A, B)
		#define SUB_U32_SIMD(A, B) _mm_sub_epi32(A, B)
		// 32-bit integer multiplications are not available on SSE2.

		// Vector integer operations returning SIMD_U16x8
		#define ADD_U16_SIMD(A, B) _mm_add_epi16(A, B)
		#define SUB_U16_SIMD(A, B) _mm_sub_epi16(A, B)
		#define MUL_U16_SIMD(A, B) _mm_mullo_epi16(A, B)

		// Vector integer operations returning SIMD_U8x16
		#define ADD_U8_SIMD(A, B) _mm_add_epi8(A, B)
		#define ADD_SAT_U8_SIMD(A, B) _mm_adds_epu8(A, B) // Saturated addition
		#define SUB_U8_SIMD(A, B) _mm_sub_epi8(A, B)
		#define SUB_SAT_U8_SIMD(A, B) _mm_subs_epu8(A, B) // Saturated subtraction
		// No 8-bit multiplications

		// Statistics
		#define MIN_F32_SIMD(A, B) _mm_min_ps(A, B)
		#define MAX_F32_SIMD(A, B) _mm_max_ps(A, B)
		// TODO: Implement minimum and maximum for integer vectors, so that all operations exist for all applicable types:
		//       Using _mm256_max_epu16... in AVX2 for 256-bit versions
		//       Using comparisons and masking in SSE2 when _mm_max_epu16... in SSE4.1 is not available

		// Bitwise
		#define BITWISE_AND_U32_SIMD(A, B) _mm_and_si128(A, B)
		#define BITWISE_OR_U32_SIMD(A, B) _mm_or_si128(A, B)
		#define BITWISE_XOR_U32_SIMD(A, B) _mm_xor_si128(A, B)

		#ifdef USE_AVX2
			// 256-bit vector types
			#define SIMD_F32x8 __m256
			#define SIMD_U8x32 __m256i
			#define SIMD_U16x16 __m256i
			#define SIMD_U32x8 __m256i
			#define SIMD_I32x8 __m256i

			// Vector uploads in address order
			#define LOAD_VECTOR_F32_SIMD256(A, B, C, D, E, F, G, H) _mm256_set_ps(H, G, F, E, D, C, B, A)
			#define LOAD_SCALAR_F32_SIMD256(A) _mm256_set1_ps(A)
			#define LOAD_VECTOR_U8_SIMD256(A1, B1, C1, D1, E1, F1, G1, H1, I1, J1, K1, L1, M1, N1, O1, P1, Q1, R1, S1, T1, U1, V1, W1, X1, Y1, Z1, A2, B2, C2, D2, E2, F2) _mm256_set_epi8(F2, E2, D2, C2, B2, A2, Z1, Y1, X1, W1, V1, U1, T1, S1, R1, Q1, P1, O1, N1, M1, L1, K1, J1, I1, H1, G1, F1, E1, D1, C1, B1, A1)
			#define LOAD_SCALAR_U8_SIMD256(A) _mm256_set1_epi8(A)
			#define LOAD_VECTOR_U16_SIMD256(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) _mm256_set_epi16(P, O, N, M, L, K, J, I, H, G, F, E, D, C, B, A)
			#define LOAD_SCALAR_U16_SIMD256(A) _mm256_set1_epi16(A)
			#define LOAD_VECTOR_U32_SIMD256(A, B, C, D, E, F, G, H) _mm256_set_epi32(H, G, F, E, D, C, B, A)
			#define LOAD_SCALAR_U32_SIMD256(A) _mm256_set1_epi32(A)
			#define LOAD_VECTOR_I32_SIMD256(A, B, C, D, E, F, G, H) _mm256_set_epi32(H, G, F, E, D, C, B, A)
			#define LOAD_SCALAR_I32_SIMD256(A) _mm256_set1_epi32(A)

			// Conversions
			#define F32_TO_I32_SIMD256(A) _mm256_cvttps_epi32(A)
			#define F32_TO_U32_SIMD256(A) _mm256_cvttps_epi32(A)
			#define I32_TO_F32_SIMD256(A) _mm256_cvtepi32_ps(A)
			#define U32_TO_F32_SIMD256(A) _mm256_cvtepi32_ps(A)

			// Unpacking conversions
			#define U8_LOW_TO_U16_SIMD256(A) _mm256_unpacklo_epi8(_mm256_permute4x64_epi64(A, 0b11011000), _mm256_set1_epi8(0))
			#define U8_HIGH_TO_U16_SIMD256(A) _mm256_unpackhi_epi8(_mm256_permute4x64_epi64(A, 0b11011000), _mm256_set1_epi8(0))
			#define U16_LOW_TO_U32_SIMD256(A) _mm256_unpacklo_epi16(_mm256_permute4x64_epi64(A, 0b11011000), _mm256_set1_epi16(0))
			#define U16_HIGH_TO_U32_SIMD256(A) _mm256_unpackhi_epi16(_mm256_permute4x64_epi64(A, 0b11011000), _mm256_set1_epi16(0))

			// Saturated packing
			//   Credit: Using ideas from Victoria Zhislina's NEON_2_SSE.h header from the Intel corporation, but not trying to emulate NEON
			inline SIMD_U8x32 PACK_SAT_U16_TO_U8_SIMD256(const SIMD_U16x16& a, const SIMD_U16x16& b) {
				SIMD_U16x16 mask, a2, b2;
				mask = _mm256_set1_epi16(0b0111111111111111);
				a2 = _mm256_and_si256(a, mask);
				a2 = _mm256_or_si256(a2, _mm256_and_si256(_mm256_cmpgt_epi16(a2, a), mask));
				b2 = _mm256_and_si256(b, mask);
				b2 = _mm256_or_si256(b2, _mm256_and_si256(_mm256_cmpgt_epi16(b2, b), mask));
				// The 256-bit pack instruction _mm256_packus_epi16 is not serial, so the result has to be permutated into the correct order.
				//   0 2 1 3
				//   |  X  |
				//   0 1 2 3
				return _mm256_permute4x64_epi64(_mm256_packus_epi16(a2, b2), 0b11011000);
			}

			// Reinterpret casting
			#define REINTERPRET_U32_TO_U8_SIMD256(A) (A)
			#define REINTERPRET_U32_TO_U16_SIMD256(A) (A)
			#define REINTERPRET_U8_TO_U32_SIMD256(A) (A)
			#define REINTERPRET_U16_TO_U32_SIMD256(A) (A)
			#define REINTERPRET_U32_TO_I32_SIMD256(A) (A)
			#define REINTERPRET_I32_TO_U32_SIMD256(A) (A)

			// Vector float operations returning SIMD_F32x4
			#define ADD_F32_SIMD256(A, B) _mm256_add_ps(A, B)
			#define SUB_F32_SIMD256(A, B) _mm256_sub_ps(A, B)
			#define MUL_F32_SIMD256(A, B) _mm256_mul_ps(A, B)

			// Vector integer operations returning SIMD_I32x4
			#define ADD_I32_SIMD256(A, B) _mm256_add_epi32(A, B)
			#define SUB_I32_SIMD256(A, B) _mm256_sub_epi32(A, B)
			#define MUL_I32_SIMD256(A, B) _mm256_mullo_epi32(A, B)

			// Vector integer operations returning SIMD_U32x4
			#define ADD_U32_SIMD256(A, B) _mm256_add_epi32(A, B)
			#define SUB_U32_SIMD256(A, B) _mm256_sub_epi32(A, B)
			#define MUL_U32_SIMD256(A, B) _mm256_mullo_epi32(A, B)

			// Vector integer operations returning SIMD_U16x8
			#define ADD_U16_SIMD256(A, B) _mm256_add_epi16(A, B)
			#define SUB_U16_SIMD256(A, B) _mm256_sub_epi16(A, B)
			#define MUL_U16_SIMD256(A, B) _mm256_mullo_epi16(A, B)

			// Vector integer operations returning SIMD_U8x16
			#define ADD_U8_SIMD256(A, B) _mm256_add_epi8(A, B)
			#define ADD_SAT_U8_SIMD256(A, B) _mm256_adds_epu8(A, B) // Saturated addition
			#define SUB_U8_SIMD256(A, B) _mm256_sub_epi8(A, B)
			#define SUB_SAT_U8_SIMD256(A, B) _mm256_subs_epu8(A, B) // Saturated subtraction
			// No 8-bit multiplications

			// Statistics
			#define MIN_F32_SIMD256(A, B) _mm256_min_ps(A, B)
			#define MAX_F32_SIMD256(A, B) _mm256_max_ps(A, B)

			// Bitwise
			#define BITWISE_AND_U32_SIMD256(A, B) _mm256_and_si256(A, B)
			#define BITWISE_OR_U32_SIMD256(A, B) _mm256_or_si256(A, B)
			#define BITWISE_XOR_U32_SIMD256(A, B) _mm256_xor_si256(A, B)
		#endif
	#endif

	// Everything declared in here handles things specific for NEON.
	// Direct use of the macros will not provide portability to all hardware.
	#ifdef USE_NEON
		#include <arm_neon.h> // NEON

		// Vector types
		#define SIMD_F32x4 float32x4_t
		#define SIMD_U8x16 uint8x16_t
		#define SIMD_U16x8 uint16x8_t
		#define SIMD_U32x4 uint32x4_t
		#define SIMD_I32x4 int32x4_t

		// Vector uploads in address order
		inline SIMD_F32x4 LOAD_VECTOR_F32_SIMD(float a, float b, float c, float d) {
			float data[4] ALIGN16 = {a, b, c, d};
			return vld1q_f32(data);
		}
		inline SIMD_F32x4 LOAD_SCALAR_F32_SIMD(float a) {
			return vdupq_n_f32(a);
		}
		inline SIMD_U8x16 LOAD_VECTOR_U8_SIMD(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h,
		                                      uint8_t i, uint8_t j, uint8_t k, uint8_t l, uint8_t m, uint8_t n, uint8_t o, uint8_t p) {
			uint8_t data[16] ALIGN16 = {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p};
			return vld1q_u8(data);
		}
		inline SIMD_U16x8 LOAD_SCALAR_U8_SIMD(uint16_t a) {
			return vdupq_n_u8(a);
		}
		inline SIMD_U16x8 LOAD_VECTOR_U16_SIMD(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h) {
			uint16_t data[8] ALIGN16 = {a, b, c, d, e, f, g, h};
			return vld1q_u16(data);
		}
		inline SIMD_U16x8 LOAD_SCALAR_U16_SIMD(uint16_t a) {
			return vdupq_n_u16(a);
		}
		inline SIMD_U32x4 LOAD_VECTOR_U32_SIMD(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
			uint32_t data[4] ALIGN16 = {a, b, c, d};
			return vld1q_u32(data);
		}
		inline SIMD_U32x4 LOAD_SCALAR_U32_SIMD(uint32_t a) {
			return vdupq_n_u32(a);
		}
		inline SIMD_I32x4 LOAD_VECTOR_I32_SIMD(int32_t a, int32_t b, int32_t c, int32_t d) {
			int32_t data[4] ALIGN16 = {a, b, c, d};
			return vld1q_s32(data);
		}
		inline SIMD_I32x4 LOAD_SCALAR_I32_SIMD(int32_t a) {
			return vdupq_n_s32(a);
		}

		// Conversions
		#define F32_TO_I32_SIMD(A) vcvtq_s32_f32(A)
		#define F32_TO_U32_SIMD(A) vcvtq_u32_f32(A)
		#define I32_TO_F32_SIMD(A) vcvtq_f32_s32(A)
		#define U32_TO_F32_SIMD(A) vcvtq_f32_u32(A)

		// Unpacking conversions
		#define U8_LOW_TO_U16_SIMD(A) vmovl_u8(vget_low_u8(A))
		#define U8_HIGH_TO_U16_SIMD(A) vmovl_u8(vget_high_u8(A))
		#define U16_LOW_TO_U32_SIMD(A) vmovl_u16(vget_low_u16(A))
		#define U16_HIGH_TO_U32_SIMD(A) vmovl_u16(vget_high_u16(A))

		// Saturated packing
		#define PACK_SAT_U16_TO_U8_SIMD(A, B) vcombine_u8(vqmovn_u16(A), vqmovn_u16(B))

		// Reinterpret casting
		#define REINTERPRET_U32_TO_U8_SIMD(A) vreinterpretq_u8_u32(A)
		#define REINTERPRET_U32_TO_U16_SIMD(A) vreinterpretq_u16_u32(A)
		#define REINTERPRET_U8_TO_U32_SIMD(A) vreinterpretq_u32_u8(A)
		#define REINTERPRET_U16_TO_U32_SIMD(A) vreinterpretq_u32_u16(A)
		#define REINTERPRET_U32_TO_I32_SIMD(A) vreinterpretq_s32_u32(A)
		#define REINTERPRET_I32_TO_U32_SIMD(A) vreinterpretq_u32_s32(A)

		// Vector float operations returning SIMD_F32x4
		#define ADD_F32_SIMD(A, B) vaddq_f32(A, B)
		#define SUB_F32_SIMD(A, B) vsubq_f32(A, B)
		#define MUL_F32_SIMD(A, B) vmulq_f32(A, B)

		// Vector integer operations returning SIMD_I32x4
		#define ADD_I32_SIMD(A, B) vaddq_s32(A, B)
		#define SUB_I32_SIMD(A, B) vsubq_s32(A, B)
		#define MUL_I32_NEON(A, B) vmulq_s32(A, B)

		// Vector integer operations returning SIMD_U32x4
		#define ADD_U32_SIMD(A, B) vaddq_u32(A, B)
		#define SUB_U32_SIMD(A, B) vsubq_u32(A, B)
		#define MUL_U32_NEON(A, B) vmulq_u32(A, B)

		// Vector integer operations returning SIMD_U16x8
		#define ADD_U16_SIMD(A, B) vaddq_u16(A, B)
		#define SUB_U16_SIMD(A, B) vsubq_u16(A, B)
		#define MUL_U16_SIMD(A, B) vmulq_u16(A, B)

		// Vector integer operations returning SIMD_U8x16
		#define ADD_U8_SIMD(A, B) vaddq_u8(A, B)
		#define ADD_SAT_U8_SIMD(A, B) vqaddq_u8(A, B) // Saturated addition
		#define SUB_U8_SIMD(A, B) vsubq_u8(A, B)
		#define SUB_SAT_U8_SIMD(A, B) vqaddq_u8(A, B) // Saturated subtraction
		// No 8-bit multiplications

		// Statistics
		#define MIN_F32_SIMD(A, B) vminq_f32(A, B)
		#define MAX_F32_SIMD(A, B) vmaxq_f32(A, B)

		// Bitwise
		#define BITWISE_AND_U32_SIMD(A, B) vandq_u32(A, B)
		#define BITWISE_OR_U32_SIMD(A, B) vorrq_u32(A, B)
		#define BITWISE_XOR_U32_SIMD(A, B) veorq_u32(A, B)
	#endif

	/*
	The vector types below are supposed to be portable across different CPU architectures.
		When mixed with handwritten SIMD intrinsics:
			Use "USE_SSE2" instead of "__SSE2__"
			Use "USE_AVX2" instead of "__AVX2__"
			Use "USE_NEON" instead of "__ARM_NEON"
	Portability exceptions:
		* The "v" variable is the native backend, which is only defined when SIMD is supported by hardware.
			Only use when USE_BASIC_SIMD is defined.
			Will not work on scalar emulation.
		* The "shared_memory" array is only defined for targets with direct access to SIMD registers. (SSE)
			Only use when USE_DIRECT_SIMD_MEMORY_ACCESS is defined.
			Will not work on NEON or scalar emulation.
		* The "emulated" array is ony defined when SIMD is turned off.
			Cannot be used when USE_BASIC_SIMD is defined.
	*/

	union F32x4 {
		#ifdef USE_BASIC_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				// Direct access cannot be done on NEON!
				float shared_memory[4];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_F32x4 v;
			// Construct a portable vector from a native SIMD vector
			explicit F32x4(const SIMD_F32x4& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			F32x4(float a1, float a2, float a3, float a4) : v(LOAD_VECTOR_F32_SIMD(a1, a2, a3, a4)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit F32x4(float scalar) : v(LOAD_SCALAR_F32_SIMD(scalar)) {}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			float emulated[4];
			// Construct a portable vector from a set of scalars
			F32x4(float a1, float a2, float a3, float a4) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit F32x4(float scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
			}
		#endif
		// Construct a portable SIMD vector from a pointer to aligned data
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		static inline F32x4 readAlignedUnsafe(const float* data) {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					return F32x4(_mm_load_ps(data));
				#elif USE_NEON
					return F32x4(vld1q_f32(data));
				#endif
			#else
				return F32x4(data[0], data[1], data[2], data[3]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(float* data) const {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					_mm_store_ps(data, this->v);
				#elif USE_NEON
					vst1q_f32(data, this->v);
				#endif
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
			#endif
		}
		#ifdef DFPSR_GEOMETRY_FVECTOR
			dsr::FVector4D get() const {
				float data[4] ALIGN16;
				this->writeAlignedUnsafe(data);
				return dsr::FVector4D(data[0], data[1], data[2], data[3]);
			}
		#endif
		// Bound and alignment checked reading
		static inline F32x4 readAligned(const dsr::SafePointer<float> data, const char* methodName) {
			const float* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			return F32x4::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<float> data, const char* methodName) const {
			float* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
		// 1 / x
		//   Useful for multiple divisions with the same denominator
		//   Useless if the denominator is a constant
		F32x4 reciprocal() const {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					// Approximate
					SIMD_F32x4 lowQ = _mm_rcp_ps(this->v);
					// Refine
					return F32x4(SUB_F32_SIMD(ADD_F32_SIMD(lowQ, lowQ), MUL_F32_SIMD(this->v, MUL_F32_SIMD(lowQ, lowQ))));
				#elif USE_NEON
					// Approximate
					SIMD_F32x4 result = vrecpeq_f32(this->v);
					// Refine
					result = MUL_F32_SIMD(vrecpsq_f32(this->v, result), result);
					return F32x4(MUL_F32_SIMD(vrecpsq_f32(this->v, result), result));
				#else
					assert(false);
					return F32x4(0);
				#endif
			#else
				return F32x4(1.0f / this->emulated[0], 1.0f / this->emulated[1], 1.0f / this->emulated[2], 1.0f / this->emulated[3]);
			#endif
		}
		// 1 / sqrt(x)
		//   Useful for normalizing vectors
		F32x4 reciprocalSquareRoot() const {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					SIMD_F32x4 reciRoot = _mm_rsqrt_ps(this->v);
					SIMD_F32x4 mul = MUL_F32_SIMD(MUL_F32_SIMD(this->v, reciRoot), reciRoot);
					reciRoot = MUL_F32_SIMD(MUL_F32_SIMD(LOAD_SCALAR_F32_SIMD(0.5f), reciRoot), SUB_F32_SIMD(LOAD_SCALAR_F32_SIMD(3.0f), mul));
					return F32x4(reciRoot);
				#elif USE_NEON
					// Approximate
					SIMD_F32x4 reciRoot = vrsqrteq_f32(this->v);
					// Refine
					reciRoot = MUL_F32_SIMD(vrsqrtsq_f32(MUL_F32_SIMD(this->v, reciRoot), reciRoot), reciRoot);
					return reciRoot;
				#else
					assert(false);
					return F32x4(0);
				#endif
			#else
				return F32x4(1.0f / sqrt(this->emulated[0]), 1.0f / sqrt(this->emulated[1]), 1.0f / sqrt(this->emulated[2]), 1.0f / sqrt(this->emulated[3]));
			#endif
		}
		// sqrt(x)
		//   Useful for getting lengths of vectors
		F32x4 squareRoot() const {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					SIMD_F32x4 half = LOAD_SCALAR_F32_SIMD(0.5f);
					// Approximate
					SIMD_F32x4 root = _mm_sqrt_ps(this->v);
					// Refine
					root = _mm_mul_ps(_mm_add_ps(root, _mm_div_ps(this->v, root)), half);
					return F32x4(root);
				#elif USE_NEON
					return F32x4(MUL_F32_SIMD(this->v, this->reciprocalSquareRoot().v));
				#else
					assert(false);
					return F32x4(0);
				#endif
			#else
				return F32x4(sqrt(this->emulated[0]), sqrt(this->emulated[1]), sqrt(this->emulated[2]), sqrt(this->emulated[3]));
			#endif
		}
		F32x4 clamp(float minimum, float maximum) const {
			#ifdef USE_BASIC_SIMD
				return F32x4(MIN_F32_SIMD(MAX_F32_SIMD(this->v, LOAD_SCALAR_F32_SIMD(minimum)), LOAD_SCALAR_F32_SIMD(maximum)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				if (minimum > val0) { val0 = minimum; }
				if (maximum < val0) { val0 = maximum; }
				if (minimum > val1) { val1 = minimum; }
				if (maximum < val1) { val1 = maximum; }
				if (minimum > val2) { val2 = minimum; }
				if (maximum < val2) { val2 = maximum; }
				if (minimum > val3) { val3 = minimum; }
				if (maximum < val3) { val3 = maximum; }
				return F32x4(val0, val1, val2, val3);
			#endif
		}
		F32x4 clampLower(float minimum) const {
			#ifdef USE_BASIC_SIMD
				return F32x4(MAX_F32_SIMD(this->v, LOAD_SCALAR_F32_SIMD(minimum)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				if (minimum > val0) { val0 = minimum; }
				if (minimum > val1) { val1 = minimum; }
				if (minimum > val2) { val2 = minimum; }
				if (minimum > val3) { val3 = minimum; }
				return F32x4(val0, val1, val2, val3);
			#endif
		}
		F32x4 clampUpper(float maximum) const {
			#ifdef USE_BASIC_SIMD
				return F32x4(MIN_F32_SIMD(this->v, LOAD_SCALAR_F32_SIMD(maximum)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				if (maximum < val0) { val0 = maximum; }
				if (maximum < val1) { val1 = maximum; }
				if (maximum < val2) { val2 = maximum; }
				if (maximum < val3) { val3 = maximum; }
				return F32x4(val0, val1, val2, val3);
			#endif
		}
	};

	union I32x4 {
		#ifdef USE_BASIC_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				// Direct access cannot be done on NEON!
				int32_t shared_memory[4];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_I32x4 v;
			// Construct a portable vector from a native SIMD vector
			explicit I32x4(const SIMD_I32x4& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			I32x4(int32_t a1, int32_t a2, int32_t a3, int32_t a4) : v(LOAD_VECTOR_I32_SIMD(a1, a2, a3, a4)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit I32x4(int32_t scalar) : v(LOAD_SCALAR_I32_SIMD(scalar)) {}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			int32_t emulated[4];
			// Construct a portable vector from a set of scalars
			I32x4(int32_t a1, int32_t a2, int32_t a3, int32_t a4) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit I32x4(int32_t scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
			}
		#endif
		// Construct a portable SIMD vector from a pointer to aligned data
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		static inline I32x4 readAlignedUnsafe(const int32_t* data) {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					return I32x4(_mm_load_si128((const __m128i*)data));
				#elif USE_NEON
					return I32x4(vld1q_s32(data));
				#endif
			#else
				return I32x4(data[0], data[1], data[2], data[3]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(int32_t* data) const {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					_mm_store_si128((__m128i*)data, this->v);
				#elif USE_NEON
					vst1q_s32(data, this->v);
				#endif
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
			#endif
		}
		#ifdef DFPSR_GEOMETRY_IVECTOR
			dsr::IVector4D get() const {
				int32_t data[4] ALIGN16;
				this->writeAlignedUnsafe(data);
				return dsr::IVector4D(data[0], data[1], data[2], data[3]);
			}
		#endif
		// Bound and alignment checked reading
		static inline I32x4 readAligned(const dsr::SafePointer<int32_t> data, const char* methodName) {
			const int32_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			return I32x4::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<int32_t> data, const char* methodName) const {
			int32_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	union U32x4 {
		#ifdef USE_BASIC_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				// Direct access cannot be done on NEON!
				uint32_t shared_memory[4];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U32x4 v;
			// Construct a portable vector from a native SIMD vector
			explicit U32x4(const SIMD_U32x4& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			U32x4(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) : v(LOAD_VECTOR_U32_SIMD(a1, a2, a3, a4)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit U32x4(uint32_t scalar) : v(LOAD_SCALAR_U32_SIMD(scalar)) {}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			uint32_t emulated[4];
			// Construct a portable vector from a set of scalars
			U32x4(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U32x4(uint32_t scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
			}
		#endif
		// Construct a portable SIMD vector from a pointer to aligned data
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		static inline U32x4 readAlignedUnsafe(const uint32_t* data) {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					return U32x4(_mm_load_si128((const __m128i*)data));
				#elif USE_NEON
					return U32x4(vld1q_u32(data));
				#endif
			#else
				return U32x4(data[0], data[1], data[2], data[3]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(uint32_t* data) const {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					_mm_store_si128((__m128i*)data, this->v);
				#elif USE_NEON
					vst1q_u32(data, this->v);
				#endif
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
			#endif
		}
		#ifdef DFPSR_GEOMETRY_UVECTOR
			dsr::UVector4D get() const {
				uint32_t data[4] ALIGN16;
				this->writeAlignedUnsafe(data);
				return dsr::UVector4D(data[0], data[1], data[2], data[3]);
			}
		#endif
		// Bound and alignment checked reading
		static inline U32x4 readAligned(const dsr::SafePointer<uint32_t> data, const char* methodName) {
			const uint32_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			return U32x4::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint32_t> data, const char* methodName) const {
			uint32_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	union U16x8 {
		#ifdef USE_BASIC_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				// Direct access cannot be done on NEON!
				uint16_t shared_memory[8];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U16x8 v;
			// Construct a portable vector from a native SIMD vector
			explicit U16x8(const SIMD_U16x8& v) : v(v) {}
			// Construct a vector of 8 x 16-bit unsigned integers from a vector of 4 x 32-bit unsigned integers
			//   Reinterpret casting is used
			explicit U16x8(const U32x4& vector) : v(REINTERPRET_U32_TO_U16_SIMD(vector.v)) {}
			// Construct a portable vector from a set of scalars
			U16x8(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8) : v(LOAD_VECTOR_U16_SIMD(a1, a2, a3, a4, a5, a6, a7, a8)) {}
			// Construct a vector of 8 x 16-bit unsigned integers from a single duplicated 32-bit unsigned integer
			//   Reinterpret casting is used
			// TODO: Remove all reintreprets from constructors to improve readability
			explicit U16x8(uint32_t scalar) : v(REINTERPRET_U32_TO_U16_SIMD(LOAD_SCALAR_U32_SIMD(scalar))) {}
			// Construct a portable vector from a single duplicated scalar
			explicit U16x8(uint16_t scalar) : v(LOAD_SCALAR_U16_SIMD(scalar)) {}
			// Reinterpret cast to a vector of 4 x 32-bit unsigned integers
			U32x4 get_U32() const {
				return U32x4(REINTERPRET_U16_TO_U32_SIMD(this->v));
			}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			uint16_t emulated[8];
			// Construct a vector of 8 x 16-bit unsigned integers from a vector of 4 x 32-bit unsigned integers
			//   Reinterpret casting is used
			explicit U16x8(const U32x4& vector) {
				uint64_t *target = (uint64_t*)this->emulated;
				uint64_t *source = (uint64_t*)vector.emulated;
				target[0] = source[0];
				target[1] = source[1];
			}
			// Construct a portable vector from a set of scalars
			U16x8(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
				this->emulated[4] = a5;
				this->emulated[5] = a6;
				this->emulated[6] = a7;
				this->emulated[7] = a8;
			}
			// Construct a vector of 8 x 16-bit unsigned integers from a single duplicated 32-bit unsigned integer
			//   Reinterpret casting is used
			explicit U16x8(uint32_t scalar) {
				uint32_t *target = (uint32_t*)this->emulated;
				target[0] = scalar;
				target[1] = scalar;
				target[2] = scalar;
				target[3] = scalar;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U16x8(uint16_t scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
				this->emulated[4] = scalar;
				this->emulated[5] = scalar;
				this->emulated[6] = scalar;
				this->emulated[7] = scalar;
			}
			// Reinterpret cast to a vector of 4 x 32-bit unsigned integers
			U32x4 get_U32() const {
				U32x4 result(0);
				uint64_t *target = (uint64_t*)result.emulated;
				uint64_t *source = (uint64_t*)this->emulated;
				target[0] = source[0];
				target[1] = source[1];
				return result;
			}
		#endif
		static inline U16x8 readAlignedUnsafe(const uint16_t* data) {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					return U16x8(_mm_load_si128((const __m128i*)data));
				#elif USE_NEON
					return U16x8(vld1q_u16(data));
				#endif
			#else
				return U16x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			#endif
		}
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(uint16_t* data) const {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					_mm_store_si128((__m128i*)data, this->v);
				#elif USE_NEON
					vst1q_u16(data, this->v);
				#endif
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
				data[4] = this->emulated[4];
				data[5] = this->emulated[5];
				data[6] = this->emulated[6];
				data[7] = this->emulated[7];
			#endif
		}
		// Bound and alignment checked reading
		static inline U16x8 readAligned(const dsr::SafePointer<uint16_t> data, const char* methodName) {
			const uint16_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			return U16x8::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint16_t> data, const char* methodName) const {
			uint16_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	union U8x16 {
		#ifdef USE_BASIC_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				// Direct access cannot be done on NEON!
				uint8_t shared_memory[16];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U8x16 v;
			// Construct a portable vector from a native SIMD vector
			explicit U8x16(const SIMD_U8x16& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			U8x16(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8,
			      uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12, uint8_t a13, uint8_t a14, uint8_t a15, uint8_t a16)
			: v(LOAD_VECTOR_U8_SIMD(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit U8x16(uint8_t scalar) : v(LOAD_SCALAR_U8_SIMD(scalar)) {}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			uint8_t emulated[16];
			// Construct a portable vector from a set of scalars
			U8x16(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8,
			      uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12, uint8_t a13, uint8_t a14, uint8_t a15, uint8_t a16) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
				this->emulated[4] = a5;
				this->emulated[5] = a6;
				this->emulated[6] = a7;
				this->emulated[7] = a8;
				this->emulated[8] = a9;
				this->emulated[9] = a10;
				this->emulated[10] = a11;
				this->emulated[11] = a12;
				this->emulated[12] = a13;
				this->emulated[13] = a14;
				this->emulated[14] = a15;
				this->emulated[15] = a16;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U8x16(uint8_t scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
				this->emulated[4] = scalar;
				this->emulated[5] = scalar;
				this->emulated[6] = scalar;
				this->emulated[7] = scalar;
				this->emulated[8] = scalar;
				this->emulated[9] = scalar;
				this->emulated[10] = scalar;
				this->emulated[11] = scalar;
				this->emulated[12] = scalar;
				this->emulated[13] = scalar;
				this->emulated[14] = scalar;
				this->emulated[15] = scalar;
			}
		#endif
		static inline U8x16 readAlignedUnsafe(const uint8_t* data) {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					return U8x16(_mm_load_si128((const __m128i*)data));
				#elif USE_NEON
					return U8x16(vld1q_u8(data));
				#endif
			#else
				return U8x16(
				  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
				  data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]
				);
			#endif
		}
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(uint8_t* data) const {
			#ifdef USE_BASIC_SIMD
				#ifdef USE_SSE2
					_mm_store_si128((__m128i*)data, this->v);
				#elif USE_NEON
					vst1q_u8(data, this->v);
				#endif
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
				data[4] = this->emulated[4];
				data[5] = this->emulated[5];
				data[6] = this->emulated[6];
				data[7] = this->emulated[7];
				data[8] = this->emulated[8];
				data[9] = this->emulated[9];
				data[10] = this->emulated[10];
				data[11] = this->emulated[11];
				data[12] = this->emulated[12];
				data[13] = this->emulated[13];
				data[14] = this->emulated[14];
				data[15] = this->emulated[15];
			#endif
		}
		// Bound and alignment checked reading
		static inline U8x16 readAligned(const dsr::SafePointer<uint8_t> data, const char* methodName) {
			const uint8_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			return U8x16::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint8_t> data, const char* methodName) const {
			uint8_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 15) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	union F32x8 {
		#ifdef USE_256BIT_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				float shared_memory[8];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_F32x8 v;
			// Construct a portable vector from a native SIMD vector
			explicit F32x8(const SIMD_F32x8& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			F32x8(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8)
			: v(LOAD_VECTOR_F32_SIMD256(a1, a2, a3, a4, a5, a6, a7, a8)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit F32x8(float scalar) : v(LOAD_SCALAR_F32_SIMD256(scalar)) {}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			float emulated[8];
			// Uninitialized default constructor for looping over elemets during emulation.
			F32x8() {}
			// Construct a portable vector from a set of scalars
			F32x8(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
				this->emulated[4] = a5;
				this->emulated[5] = a6;
				this->emulated[6] = a7;
				this->emulated[7] = a8;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit F32x8(float scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
				this->emulated[4] = scalar;
				this->emulated[5] = scalar;
				this->emulated[6] = scalar;
				this->emulated[7] = scalar;
			}
		#endif
		// Construct a portable SIMD vector from a pointer to aligned data
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		static inline F32x8 readAlignedUnsafe(const float* data) {
			#ifdef USE_AVX2
				return F32x8(_mm256_load_ps(data));
			#else
				return F32x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(float* data) const {
			#ifdef USE_AVX2
				_mm256_store_ps(data, this->v);
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
				data[4] = this->emulated[4];
				data[5] = this->emulated[5];
				data[6] = this->emulated[6];
				data[7] = this->emulated[7];
			#endif
		}
		// Bound and alignment checked reading
		static inline F32x8 readAligned(const dsr::SafePointer<float> data, const char* methodName) {
			const float* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			return F32x8::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<float> data, const char* methodName) const {
			float* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
		// 1 / x
		//   Useful for multiple divisions with the same denominator
		//   Useless if the denominator is a constant
		F32x8 reciprocal() const {
			#ifdef USE_AVX2
				// Approximate
				SIMD_F32x8 lowQ = _mm256_rcp_ps(this->v);
				// Refine
				return F32x8(SUB_F32_SIMD256(ADD_F32_SIMD256(lowQ, lowQ), MUL_F32_SIMD256(this->v, MUL_F32_SIMD256(lowQ, lowQ))));
			#else
				return F32x8(
				  1.0f / this->emulated[0],
				  1.0f / this->emulated[1],
				  1.0f / this->emulated[2],
				  1.0f / this->emulated[3],
				  1.0f / this->emulated[4],
				  1.0f / this->emulated[5],
				  1.0f / this->emulated[6],
				  1.0f / this->emulated[7]
				);
			#endif
		}
		// 1 / sqrt(x)
		//   Useful for normalizing vectors
		F32x8 reciprocalSquareRoot() const {
			#ifdef USE_AVX2
				//__m128 reciRoot = _mm256_rsqrt_ps(this->v);
				SIMD_F32x8 reciRoot = _mm256_rsqrt_ps(this->v);
				SIMD_F32x8 mul = MUL_F32_SIMD256(MUL_F32_SIMD256(this->v, reciRoot), reciRoot);
				reciRoot = MUL_F32_SIMD256(MUL_F32_SIMD256(LOAD_SCALAR_F32_SIMD256(0.5f), reciRoot), SUB_F32_SIMD256(LOAD_SCALAR_F32_SIMD256(3.0f), mul));
				return F32x8(reciRoot);
			#else
				return F32x8(
				  1.0f / sqrt(this->emulated[0]),
				  1.0f / sqrt(this->emulated[1]),
				  1.0f / sqrt(this->emulated[2]),
				  1.0f / sqrt(this->emulated[3]),
				  1.0f / sqrt(this->emulated[4]),
				  1.0f / sqrt(this->emulated[5]),
				  1.0f / sqrt(this->emulated[6]),
				  1.0f / sqrt(this->emulated[7])
				);
			#endif
		}
		// sqrt(x)
		//   Useful for getting lengths of vectors
		F32x8 squareRoot() const {
			#ifdef USE_AVX2
				SIMD_F32x8 half = LOAD_SCALAR_F32_SIMD256(0.5f);
				// Approximate
				SIMD_F32x8 root = _mm256_sqrt_ps(this->v);
				// Refine
				root = _mm256_mul_ps(_mm256_add_ps(root, _mm256_div_ps(this->v, root)), half);
				return F32x8(root);
			#else
				return F32x8(
				  sqrt(this->emulated[0]),
				  sqrt(this->emulated[1]),
				  sqrt(this->emulated[2]),
				  sqrt(this->emulated[3]),
				  sqrt(this->emulated[4]),
				  sqrt(this->emulated[5]),
				  sqrt(this->emulated[6]),
				  sqrt(this->emulated[7]));
			#endif
		}
		F32x8 clamp(float minimum, float maximum) const {
			#ifdef USE_256BIT_SIMD
				return F32x8(MIN_F32_SIMD256(MAX_F32_SIMD256(this->v, LOAD_SCALAR_F32_SIMD256(minimum)), LOAD_SCALAR_F32_SIMD256(maximum)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				float val4 = this->emulated[4];
				float val5 = this->emulated[5];
				float val6 = this->emulated[6];
				float val7 = this->emulated[7];
				if (minimum > val0) { val0 = minimum; }
				if (maximum < val0) { val0 = maximum; }
				if (minimum > val1) { val1 = minimum; }
				if (maximum < val1) { val1 = maximum; }
				if (minimum > val2) { val2 = minimum; }
				if (maximum < val2) { val2 = maximum; }
				if (minimum > val3) { val3 = minimum; }
				if (maximum < val3) { val3 = maximum; }
				if (minimum > val4) { val4 = minimum; }
				if (maximum < val4) { val4 = maximum; }
				if (minimum > val5) { val5 = minimum; }
				if (maximum < val5) { val5 = maximum; }
				if (minimum > val6) { val6 = minimum; }
				if (maximum < val6) { val6 = maximum; }
				if (minimum > val7) { val7 = minimum; }
				if (maximum < val7) { val7 = maximum; }
				return F32x8(val0, val1, val2, val3, val4, val5, val6, val7);
			#endif
		}
		F32x8 clampLower(float minimum) const {
			#ifdef USE_256BIT_SIMD
				return F32x8(MAX_F32_SIMD256(this->v, LOAD_SCALAR_F32_SIMD256(minimum)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				float val4 = this->emulated[4];
				float val5 = this->emulated[5];
				float val6 = this->emulated[6];
				float val7 = this->emulated[7];
				if (minimum > val0) { val0 = minimum; }
				if (minimum > val1) { val1 = minimum; }
				if (minimum > val2) { val2 = minimum; }
				if (minimum > val3) { val3 = minimum; }
				if (minimum > val4) { val4 = minimum; }
				if (minimum > val5) { val5 = minimum; }
				if (minimum > val6) { val6 = minimum; }
				if (minimum > val7) { val7 = minimum; }
				return F32x8(val0, val1, val2, val3, val4, val5, val6, val7);
			#endif
		}
		F32x8 clampUpper(float maximum) const {
			#ifdef USE_256BIT_SIMD
				return F32x8(MIN_F32_SIMD256(this->v, LOAD_SCALAR_F32_SIMD256(maximum)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				float val4 = this->emulated[4];
				float val5 = this->emulated[5];
				float val6 = this->emulated[6];
				float val7 = this->emulated[7];
				if (maximum < val0) { val0 = maximum; }
				if (maximum < val1) { val1 = maximum; }
				if (maximum < val2) { val2 = maximum; }
				if (maximum < val3) { val3 = maximum; }
				if (maximum < val4) { val4 = maximum; }
				if (maximum < val5) { val5 = maximum; }
				if (maximum < val6) { val6 = maximum; }
				if (maximum < val7) { val7 = maximum; }
				return F32x8(val0, val1, val2, val3, val4, val5, val6, val7);
			#endif
		}
	};

	union I32x8 {
		#ifdef USE_256BIT_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				int32_t shared_memory[8];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_I32x8 v;
			// Construct a portable vector from a native SIMD vector
			explicit I32x8(const SIMD_I32x8& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			I32x8(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8)
			: v(LOAD_VECTOR_I32_SIMD256(a1, a2, a3, a4, a5, a6, a7, a8)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit I32x8(int32_t scalar) : v(LOAD_SCALAR_I32_SIMD256(scalar)) {}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			int32_t emulated[8];
			// Uninitialized default constructor for looping over elemets during emulation.
			I32x8() {}
			// Construct a portable vector from a set of scalars
			I32x8(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
				this->emulated[4] = a5;
				this->emulated[5] = a6;
				this->emulated[6] = a7;
				this->emulated[7] = a8;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit I32x8(int32_t scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
				this->emulated[4] = scalar;
				this->emulated[5] = scalar;
				this->emulated[6] = scalar;
				this->emulated[7] = scalar;
			}
		#endif
		// Construct a portable SIMD vector from a pointer to aligned data
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		static inline I32x8 readAlignedUnsafe(const int32_t* data) {
			#ifdef USE_AVX2
				return I32x8(_mm256_load_si256((const __m256i*)data));
			#else
				return I32x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(int32_t* data) const {
			#ifdef USE_AVX2
				_mm256_store_si256((__m256i*)data, this->v);
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
				data[4] = this->emulated[4];
				data[5] = this->emulated[5];
				data[6] = this->emulated[6];
				data[7] = this->emulated[7];
			#endif
		}
		// Bound and alignment checked reading
		static inline I32x8 readAligned(const dsr::SafePointer<int32_t> data, const char* methodName) {
			const int32_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			return I32x8::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<int32_t> data, const char* methodName) const {
			int32_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	union U32x8 {
		#ifdef USE_256BIT_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				uint32_t shared_memory[8];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U32x8 v;
			// Construct a portable vector from a native SIMD vector
			explicit U32x8(const SIMD_U32x8& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			U32x8(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7, uint32_t a8)
			: v(LOAD_VECTOR_U32_SIMD256(a1, a2, a3, a4, a5, a6, a7, a8)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit U32x8(uint32_t scalar) : v(LOAD_SCALAR_U32_SIMD256(scalar)) {}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			uint32_t emulated[8];
			// Uninitialized default constructor for looping over elemets during emulation.
			U32x8() {}
			// Construct a portable vector from a set of scalars
			U32x8(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7, uint32_t a8) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
				this->emulated[4] = a5;
				this->emulated[5] = a6;
				this->emulated[6] = a7;
				this->emulated[7] = a8;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U32x8(uint32_t scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
				this->emulated[4] = scalar;
				this->emulated[5] = scalar;
				this->emulated[6] = scalar;
				this->emulated[7] = scalar;
			}
		#endif
		// Construct a portable SIMD vector from a pointer to aligned data
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		static inline U32x8 readAlignedUnsafe(const uint32_t* data) {
			#ifdef USE_AVX2
				return U32x8(_mm256_load_si256((const __m256i*)data));
			#else
				return U32x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(uint32_t* data) const {
			#ifdef USE_AVX2
				_mm256_store_si256((__m256i*)data, this->v);
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
				data[4] = this->emulated[4];
				data[5] = this->emulated[5];
				data[6] = this->emulated[6];
				data[7] = this->emulated[7];
			#endif
		}
		// Bound and alignment checked reading
		static inline U32x8 readAligned(const dsr::SafePointer<uint32_t> data, const char* methodName) {
			const uint32_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			return U32x8::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint32_t> data, const char* methodName) const {
			uint32_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	union U16x16 {
		#ifdef USE_256BIT_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				uint16_t shared_memory[16];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U16x16 v;
			// Construct a portable vector from a native SIMD vector
			explicit U16x16(const SIMD_U16x16& v) : v(v) {}
			// Construct a vector of 16 x 16-bit unsigned integers from a vector of 8 x 32-bit unsigned integers
			//   Reinterpret casting is used
			explicit U16x16(const U32x8& vector) : v(REINTERPRET_U32_TO_U16_SIMD256(vector.v)) {}
			// Construct a portable vector from a set of scalars
			U16x16(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8,
			       uint16_t a9, uint16_t a10, uint16_t a11, uint16_t a12, uint16_t a13, uint16_t a14, uint16_t a15, uint16_t a16)
			: v(LOAD_VECTOR_U16_SIMD256(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16)) {}
			// Construct a vector of 8 x 16-bit unsigned integers from a single duplicated 32-bit unsigned integer
			//   Reinterpret casting is used
			// TODO: Remove all reintreprets from constructors to improve readability
			explicit U16x16(uint32_t scalar) : v(REINTERPRET_U32_TO_U16_SIMD256(LOAD_SCALAR_U32_SIMD256(scalar))) {}
			// Construct a portable vector from a single duplicated scalar
			explicit U16x16(uint16_t scalar) : v(LOAD_SCALAR_U16_SIMD256(scalar)) {}
			// Reinterpret cast to a vector of 4 x 32-bit unsigned integers
			U32x8 get_U32() const {
				return U32x8(REINTERPRET_U16_TO_U32_SIMD256(this->v));
			}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			uint16_t emulated[16];
			// Uninitialized default constructor for looping over elemets during emulation.
			U16x16() {}
			// Construct a vector of 8 x 16-bit unsigned integers from a vector of 4 x 32-bit unsigned integers
			//   Reinterpret casting is used
			explicit U16x16(const U32x8& vector) {
				uint64_t *target = (uint64_t*)this->emulated;
				uint64_t *source = (uint64_t*)vector.emulated;
				target[0] = source[0];
				target[1] = source[1];
				target[2] = source[2];
				target[3] = source[3];
			}
			// Construct a portable vector from a set of scalars
			U16x16(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8,
			       uint16_t a9, uint16_t a10, uint16_t a11, uint16_t a12, uint16_t a13, uint16_t a14, uint16_t a15, uint16_t a16) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
				this->emulated[4] = a5;
				this->emulated[5] = a6;
				this->emulated[6] = a7;
				this->emulated[7] = a8;
				this->emulated[8] = a9;
				this->emulated[9] = a10;
				this->emulated[10] = a11;
				this->emulated[11] = a12;
				this->emulated[12] = a13;
				this->emulated[13] = a14;
				this->emulated[14] = a15;
				this->emulated[15] = a16;
			}
			// Construct a vector of 8 x 16-bit unsigned integers from a single duplicated 32-bit unsigned integer
			//   Reinterpret casting is used
			explicit U16x16(uint32_t scalar) {
				uint32_t *target = (uint32_t*)this->emulated;
				target[0] = scalar;
				target[1] = scalar;
				target[2] = scalar;
				target[3] = scalar;
				target[4] = scalar;
				target[5] = scalar;
				target[6] = scalar;
				target[7] = scalar;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U16x16(uint16_t scalar) {
				this->emulated[0] = scalar;
				this->emulated[1] = scalar;
				this->emulated[2] = scalar;
				this->emulated[3] = scalar;
				this->emulated[4] = scalar;
				this->emulated[5] = scalar;
				this->emulated[6] = scalar;
				this->emulated[7] = scalar;
				this->emulated[8] = scalar;
				this->emulated[9] = scalar;
				this->emulated[10] = scalar;
				this->emulated[11] = scalar;
				this->emulated[12] = scalar;
				this->emulated[13] = scalar;
				this->emulated[14] = scalar;
				this->emulated[15] = scalar;
			}
			// Reinterpret cast to a vector of 8 x 32-bit unsigned integers
			U32x8 get_U32() const {
				U32x8 result(0);
				uint64_t *target = (uint64_t*)result.emulated;
				uint64_t *source = (uint64_t*)this->emulated;
				target[0] = source[0];
				target[1] = source[1];
				target[2] = source[2];
				target[3] = source[3];
				return result;
			}
		#endif
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		//static inline U16x16 readSlow(uint16_t* data) {
		//	return U16x16(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
		//}
		static inline U16x16 readAlignedUnsafe(const uint16_t* data) {
			#ifdef USE_AVX2
				return U16x16(_mm256_load_si256((const __m256i*)data));
			#else
				return U16x16(
				  data[0],
				  data[1],
				  data[2],
				  data[3],
				  data[4],
				  data[5],
				  data[6],
				  data[7],
				  data[8],
				  data[9],
				  data[10],
				  data[11],
				  data[12],
				  data[13],
				  data[14],
				  data[15]
				);
			#endif
		}
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(uint16_t* data) const {
			#ifdef USE_AVX2
				_mm256_store_si256((__m256i*)data, this->v);
			#else
				data[0] = this->emulated[0];
				data[1] = this->emulated[1];
				data[2] = this->emulated[2];
				data[3] = this->emulated[3];
				data[4] = this->emulated[4];
				data[5] = this->emulated[5];
				data[6] = this->emulated[6];
				data[7] = this->emulated[7];
				data[8] = this->emulated[8];
				data[9] = this->emulated[9];
				data[10] = this->emulated[10];
				data[11] = this->emulated[11];
				data[12] = this->emulated[12];
				data[13] = this->emulated[13];
				data[14] = this->emulated[14];
				data[15] = this->emulated[15];
			#endif
		}
		// Bound and alignment checked reading
		static inline U16x16 readAligned(const dsr::SafePointer<uint16_t> data, const char* methodName) {
			const uint16_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			return U16x16::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint16_t> data, const char* methodName) const {
			uint16_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	union U8x32 {
		#ifdef USE_256BIT_SIMD
			public:
			#ifdef USE_DIRECT_SIMD_MEMORY_ACCESS
				// Only use if USE_DIRECT_SIMD_MEMORY_ACCESS is defined!
				uint8_t shared_memory[32];
			#endif
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U8x32 v;
			// Construct a portable vector from a native SIMD vector
			explicit U8x32(const SIMD_U8x32& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			U8x32(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8,
				  uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12, uint8_t a13, uint8_t a14, uint8_t a15, uint8_t a16,
				  uint8_t a17, uint8_t a18, uint8_t a19, uint8_t a20, uint8_t a21, uint8_t a22, uint8_t a23, uint8_t a24,
				  uint8_t a25, uint8_t a26, uint8_t a27, uint8_t a28, uint8_t a29, uint8_t a30, uint8_t a31, uint8_t a32)
			: v(LOAD_VECTOR_U8_SIMD256(
			  a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16,
			  a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32
			)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit U8x32(uint8_t scalar) : v(LOAD_SCALAR_U8_SIMD256(scalar)) {}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support
			// Only accessible while emulating!
			uint8_t emulated[32];
			// Uninitialized default constructor for looping over elemets during emulation.
			U8x32() {}
			// Construct a portable vector from a set of scalars
			U8x32(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8,
				  uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12, uint8_t a13, uint8_t a14, uint8_t a15, uint8_t a16,
				  uint8_t a17, uint8_t a18, uint8_t a19, uint8_t a20, uint8_t a21, uint8_t a22, uint8_t a23, uint8_t a24,
				  uint8_t a25, uint8_t a26, uint8_t a27, uint8_t a28, uint8_t a29, uint8_t a30, uint8_t a31, uint8_t a32) {
				this->emulated[0] = a1;
				this->emulated[1] = a2;
				this->emulated[2] = a3;
				this->emulated[3] = a4;
				this->emulated[4] = a5;
				this->emulated[5] = a6;
				this->emulated[6] = a7;
				this->emulated[7] = a8;
				this->emulated[8] = a9;
				this->emulated[9] = a10;
				this->emulated[10] = a11;
				this->emulated[11] = a12;
				this->emulated[12] = a13;
				this->emulated[13] = a14;
				this->emulated[14] = a15;
				this->emulated[15] = a16;
				this->emulated[16] = a17;
				this->emulated[17] = a18;
				this->emulated[18] = a19;
				this->emulated[19] = a20;
				this->emulated[20] = a21;
				this->emulated[21] = a22;
				this->emulated[22] = a23;
				this->emulated[23] = a24;
				this->emulated[24] = a25;
				this->emulated[25] = a26;
				this->emulated[26] = a27;
				this->emulated[27] = a28;
				this->emulated[28] = a29;
				this->emulated[29] = a30;
				this->emulated[30] = a31;
				this->emulated[31] = a32;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U8x32(uint8_t scalar) {
				for (int i = 0; i < 32; i++) {
					this->emulated[i] = scalar;
				}
			}
		#endif
		static inline U8x32 readAlignedUnsafe(const uint8_t* data) {
			#ifdef USE_AVX2
				return U8x32(_mm256_load_si256((const __m256i*)data));
			#else
				U8x32 result;
				for (int i = 0; i < 32; i++) {
					result.emulated[i] = data[i];
				}
				return result;
			#endif
		}
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		inline void writeAlignedUnsafe(uint8_t* data) const {
			#ifdef USE_AVX2
				_mm256_store_si256((__m256i*)data, this->v);
			#else
				for (int i = 0; i < 32; i++) {
					data[i] = this->emulated[i];
				}
			#endif
		}
		// Bound and alignment checked reading
		static inline U8x32 readAligned(const dsr::SafePointer<uint8_t> data, const char* methodName) {
			const uint8_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			return U8x32::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint8_t> data, const char* methodName) const {
			uint8_t* pointer = data.getUnsafe();
			assert(((uintptr_t)pointer & 31) == 0);
			#ifdef SAFE_POINTER_CHECKS
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	// Helper macros for doing things to certain sets of SIMD vector types
	//  Performing do(vector_type, element_type, lane_count)
	#define FOR_ALL_VECTOR_TYPES(DO) \
		DO(F32x4,  float,     4) \
		DO(I32x4,  int32_t,   4) \
		DO(U32x4,  uint32_t,  4) \
		DO(U16x8,  uint16_t,  8) \
		DO(U8x16,  uint8_t,  16) \
		DO(F32x8,  float,     8) \
		DO(I32x8,  int32_t,   8) \
		DO(U32x8,  uint32_t,  8) \
		DO(U16x16, uint16_t, 16) \
		DO(U8x32,  uint8_t,  32)
	#define FOR_FLOAT_VECTOR_TYPES(DO) \
		DO(F32x4,  float,     4) \
		DO(F32x8,  float,     8)
	#define FOR_INTEGER_VECTOR_TYPES(DO) \
		DO(I32x4,  int32_t,   4) \
		DO(U32x4,  uint32_t,  4) \
		DO(U16x8,  uint16_t,  8) \
		DO(U8x16,  uint8_t,  16) \
		DO(I32x8,  int32_t,   8) \
		DO(U32x8,  uint32_t,  8) \
		DO(U16x16, uint16_t, 16) \
		DO(U8x32,  uint8_t,  32)
	#define FOR_SIGNED_VECTOR_TYPES(DO) \
		DO(F32x4,  float,     4) \
		DO(I32x4,  int32_t,   4) \
		DO(F32x8,  float,     8) \
		DO(I32x8,  int32_t,   8)
	#define FOR_UNSIGNED_VECTOR_TYPES(DO) \
		DO(U32x4,  uint32_t,  4) \
		DO(U16x8,  uint16_t,  8) \
		DO(U8x16,  uint8_t,  16) \
		DO(U32x8,  uint32_t,  8) \
		DO(U16x16, uint16_t, 16) \
		DO(U8x32,  uint8_t,  32)

	// Print SIMD vectors to the terminal or append them to strings.
	#define CREATE_METHOD_PRINT(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline dsr::String& string_toStreamIndented(dsr::String& target, const VECTOR_TYPE& source, const dsr::ReadableString& indentation) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			source.writeAlignedUnsafe(a); \
			dsr::string_append(target, indentation, a[0]); \
			for (int i = 1; i < LANE_COUNT; i++) { \
				string_append(target, U", ", a[i]); \
			} \
			return target; \
		}
		// All SIMD vectors can be printed.
		FOR_ALL_VECTOR_TYPES(CREATE_METHOD_PRINT)
	#undef CREATE_METHOD_PRINT

	// Whole comparisons returning a single boolean, mainly for regression tests.
	#define CREATE_EXACT_EQUALITY(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline bool operator==(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(a); \
			right.writeAlignedUnsafe(b); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (a[i] != b[i]) return false; \
			} \
			return true; \
		}
		// Integer SIMD vectors have exact equlity.
		FOR_INTEGER_VECTOR_TYPES(CREATE_EXACT_EQUALITY)
	#undef CREATE_EXACT_EQUALITY

	#define CREATE_TOLERANT_EQUALITY(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline bool operator==(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(a); \
			right.writeAlignedUnsafe(b); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (fabs(a[i] - b[i]) >= 0.0001f) return false; \
			} \
			return true; \
		}
		// Float SIMD vectors have inexact equality.
		FOR_FLOAT_VECTOR_TYPES(CREATE_TOLERANT_EQUALITY)
	#undef CREATE_TOLERANT_EQUALITY

	#define CREATE_INEQUALITY(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline bool operator!=(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			return !(left == right); \
		}
		// All SIMD vectors have inequality.
		FOR_ALL_VECTOR_TYPES(CREATE_INEQUALITY)
	#undef CREATE_INEQUALITY

	inline F32x4 operator+(const F32x4& left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(ADD_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.emulated[0] + right.emulated[0], left.emulated[1] + right.emulated[1], left.emulated[2] + right.emulated[2], left.emulated[3] + right.emulated[3]);
		#endif
	}

	inline F32x4 operator-(const F32x4& left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(SUB_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.emulated[0] - right.emulated[0], left.emulated[1] - right.emulated[1], left.emulated[2] - right.emulated[2], left.emulated[3] - right.emulated[3]);
		#endif
	}

	inline F32x4 operator*(const F32x4& left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(MUL_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.emulated[0] * right.emulated[0], left.emulated[1] * right.emulated[1], left.emulated[2] * right.emulated[2], left.emulated[3] * right.emulated[3]);
		#endif
	}
	inline F32x4 min(const F32x4& left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(MIN_F32_SIMD(left.v, right.v));
		#else
			float v0 = left.emulated[0];
			float v1 = left.emulated[1];
			float v2 = left.emulated[2];
			float v3 = left.emulated[3];
			float r0 = right.emulated[0];
			float r1 = right.emulated[1];
			float r2 = right.emulated[2];
			float r3 = right.emulated[3];
			if (r0 < v0) { v0 = r0; }
			if (r1 < v1) { v1 = r1; }
			if (r2 < v2) { v2 = r2; }
			if (r3 < v3) { v3 = r3; }
			return F32x4(v0, v1, v2, v3);
		#endif
	}
	inline F32x4 max(const F32x4& left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(MAX_F32_SIMD(left.v, right.v));
		#else
			float v0 = left.emulated[0];
			float v1 = left.emulated[1];
			float v2 = left.emulated[2];
			float v3 = left.emulated[3];
			float r0 = right.emulated[0];
			float r1 = right.emulated[1];
			float r2 = right.emulated[2];
			float r3 = right.emulated[3];
			if (r0 > v0) { v0 = r0; }
			if (r1 > v1) { v1 = r1; }
			if (r2 > v2) { v2 = r2; }
			if (r3 > v3) { v3 = r3; }
			return F32x4(v0, v1, v2, v3);
		#endif
	}
	inline I32x4 operator+(const I32x4& left, const I32x4& right) {
		#ifdef USE_BASIC_SIMD
			return I32x4(ADD_I32_SIMD(left.v, right.v));
		#else
			return I32x4(left.emulated[0] + right.emulated[0], left.emulated[1] + right.emulated[1], left.emulated[2] + right.emulated[2], left.emulated[3] + right.emulated[3]);
		#endif
	}
	inline I32x4 operator-(const I32x4& left, const I32x4& right) {
		#ifdef USE_BASIC_SIMD
			return I32x4(SUB_I32_SIMD(left.v, right.v));
		#else
			return I32x4(left.emulated[0] - right.emulated[0], left.emulated[1] - right.emulated[1], left.emulated[2] - right.emulated[2], left.emulated[3] - right.emulated[3]);
		#endif
	}
	inline I32x4 operator*(const I32x4& left, const I32x4& right) {
		#ifdef USE_BASIC_SIMD
			#ifdef USE_SSE2
				// Emulate a NEON instruction
				return I32x4(left.shared_memory[0] * right.shared_memory[0], left.shared_memory[1] * right.shared_memory[1], left.shared_memory[2] * right.shared_memory[2], left.shared_memory[3] * right.shared_memory[3]);
			#elif USE_NEON
				return I32x4(MUL_I32_NEON(left.v, right.v));
			#endif
		#else
			return I32x4(left.emulated[0] * right.emulated[0], left.emulated[1] * right.emulated[1], left.emulated[2] * right.emulated[2], left.emulated[3] * right.emulated[3]);
		#endif
	}
	inline U32x4 operator+(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(ADD_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] + right.emulated[0], left.emulated[1] + right.emulated[1], left.emulated[2] + right.emulated[2], left.emulated[3] + right.emulated[3]);
		#endif
	}
	inline U32x4 operator-(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(SUB_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] - right.emulated[0], left.emulated[1] - right.emulated[1], left.emulated[2] - right.emulated[2], left.emulated[3] - right.emulated[3]);
		#endif
	}
	inline U32x4 operator*(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			#ifdef USE_SSE2
				// Emulate a NEON instruction on SSE2 registers
				return U32x4(left.shared_memory[0] * right.shared_memory[0], left.shared_memory[1] * right.shared_memory[1], left.shared_memory[2] * right.shared_memory[2], left.shared_memory[3] * right.shared_memory[3]);
			#else // NEON
				return U32x4(MUL_U32_NEON(left.v, right.v));
			#endif
		#else
			return U32x4(left.emulated[0] * right.emulated[0], left.emulated[1] * right.emulated[1], left.emulated[2] * right.emulated[2], left.emulated[3] * right.emulated[3]);
		#endif
	}
	inline U32x4 operator&(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(BITWISE_AND_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] & right.emulated[0], left.emulated[1] & right.emulated[1], left.emulated[2] & right.emulated[2], left.emulated[3] & right.emulated[3]);
		#endif
	}
	inline U32x4 operator|(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(BITWISE_OR_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] | right.emulated[0], left.emulated[1] | right.emulated[1], left.emulated[2] | right.emulated[2], left.emulated[3] | right.emulated[3]);
		#endif
	}
	inline U32x4 operator^(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(BITWISE_XOR_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] ^ right.emulated[0], left.emulated[1] ^ right.emulated[1], left.emulated[2] ^ right.emulated[2], left.emulated[3] ^ right.emulated[3]);
		#endif
	}
	inline U32x4 operator<<(const U32x4& left, uint32_t bitOffset) {
		#ifdef USE_SSE2
			return U32x4(_mm_slli_epi32(left.v, bitOffset));
		#else
			#ifdef USE_NEON
				return U32x4(vshlq_u32(left.v, LOAD_SCALAR_I32_SIMD(bitOffset)));
			#else
				return U32x4(left.emulated[0] << bitOffset, left.emulated[1] << bitOffset, left.emulated[2] << bitOffset, left.emulated[3] << bitOffset);
			#endif
		#endif
	}
	inline U32x4 operator>>(const U32x4& left, uint32_t bitOffset) {
		#ifdef USE_SSE2
			return U32x4(_mm_srli_epi32(left.v, bitOffset));
		#else
			#ifdef USE_NEON
				return U32x4(vshlq_u32(left.v, LOAD_SCALAR_I32_SIMD(-bitOffset)));
			#else
				return U32x4(left.emulated[0] >> bitOffset, left.emulated[1] >> bitOffset, left.emulated[2] >> bitOffset, left.emulated[3] >> bitOffset);
			#endif
		#endif
	}
	inline U16x8 operator+(const U16x8& left, const U16x8& right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(ADD_U16_SIMD(left.v, right.v));
		#else
			return U16x8(left.emulated[0] + right.emulated[0], left.emulated[1] + right.emulated[1], left.emulated[2] + right.emulated[2], left.emulated[3] + right.emulated[3],
			           left.emulated[4] + right.emulated[4], left.emulated[5] + right.emulated[5], left.emulated[6] + right.emulated[6], left.emulated[7] + right.emulated[7]);
		#endif
	}
	inline U16x8 operator-(const U16x8& left, const U16x8& right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(SUB_U16_SIMD(left.v, right.v));
		#else
			return U16x8(left.emulated[0] - right.emulated[0], left.emulated[1] - right.emulated[1], left.emulated[2] - right.emulated[2], left.emulated[3] - right.emulated[3],
			           left.emulated[4] - right.emulated[4], left.emulated[5] - right.emulated[5], left.emulated[6] - right.emulated[6], left.emulated[7] - right.emulated[7]);
		#endif
	}
	inline U16x8 operator*(const U16x8& left, const U16x8& right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(MUL_U16_SIMD(left.v, right.v));
		#else
			return U16x8(left.emulated[0] * right.emulated[0], left.emulated[1] * right.emulated[1], left.emulated[2] * right.emulated[2], left.emulated[3] * right.emulated[3],
			           left.emulated[4] * right.emulated[4], left.emulated[5] * right.emulated[5], left.emulated[6] * right.emulated[6], left.emulated[7] * right.emulated[7]);
		#endif
	}
	inline U8x16 operator+(const U8x16& left, const U8x16& right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(ADD_U8_SIMD(left.v, right.v));
		#else
			return U8x16(
			  left.emulated[0] + right.emulated[0],
			  left.emulated[1] + right.emulated[1],
			  left.emulated[2] + right.emulated[2],
			  left.emulated[3] + right.emulated[3],
			  left.emulated[4] + right.emulated[4],
			  left.emulated[5] + right.emulated[5],
			  left.emulated[6] + right.emulated[6],
			  left.emulated[7] + right.emulated[7],
			  left.emulated[8] + right.emulated[8],
			  left.emulated[9] + right.emulated[9],
			  left.emulated[10] + right.emulated[10],
			  left.emulated[11] + right.emulated[11],
			  left.emulated[12] + right.emulated[12],
			  left.emulated[13] + right.emulated[13],
			  left.emulated[14] + right.emulated[14],
			  left.emulated[15] + right.emulated[15]
			);
		#endif
	}
	inline U8x16 operator-(const U8x16& left, const U8x16& right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(SUB_U8_SIMD(left.v, right.v));
		#else
			return U8x16(
			  left.emulated[0] - right.emulated[0],
			  left.emulated[1] - right.emulated[1],
			  left.emulated[2] - right.emulated[2],
			  left.emulated[3] - right.emulated[3],
			  left.emulated[4] - right.emulated[4],
			  left.emulated[5] - right.emulated[5],
			  left.emulated[6] - right.emulated[6],
			  left.emulated[7] - right.emulated[7],
			  left.emulated[8] - right.emulated[8],
			  left.emulated[9] - right.emulated[9],
			  left.emulated[10] - right.emulated[10],
			  left.emulated[11] - right.emulated[11],
			  left.emulated[12] - right.emulated[12],
			  left.emulated[13] - right.emulated[13],
			  left.emulated[14] - right.emulated[14],
			  left.emulated[15] - right.emulated[15]
			);
		#endif
	}

	inline uint8_t impl_limit0(int32_t x) { return x < 0 ? 0 : x; }
	inline uint8_t impl_limit255(uint32_t x) { return x > 255 ? 255 : x; }
	inline U8x16 saturatedAddition(const U8x16& left, const U8x16& right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(ADD_SAT_U8_SIMD(left.v, right.v));
		#else
			return U8x16(
			  impl_limit255((uint32_t)left.emulated[0] + (uint32_t)right.emulated[0]),
			  impl_limit255((uint32_t)left.emulated[1] + (uint32_t)right.emulated[1]),
			  impl_limit255((uint32_t)left.emulated[2] + (uint32_t)right.emulated[2]),
			  impl_limit255((uint32_t)left.emulated[3] + (uint32_t)right.emulated[3]),
			  impl_limit255((uint32_t)left.emulated[4] + (uint32_t)right.emulated[4]),
			  impl_limit255((uint32_t)left.emulated[5] + (uint32_t)right.emulated[5]),
			  impl_limit255((uint32_t)left.emulated[6] + (uint32_t)right.emulated[6]),
			  impl_limit255((uint32_t)left.emulated[7] + (uint32_t)right.emulated[7]),
			  impl_limit255((uint32_t)left.emulated[8] + (uint32_t)right.emulated[8]),
			  impl_limit255((uint32_t)left.emulated[9] + (uint32_t)right.emulated[9]),
			  impl_limit255((uint32_t)left.emulated[10] + (uint32_t)right.emulated[10]),
			  impl_limit255((uint32_t)left.emulated[11] + (uint32_t)right.emulated[11]),
			  impl_limit255((uint32_t)left.emulated[12] + (uint32_t)right.emulated[12]),
			  impl_limit255((uint32_t)left.emulated[13] + (uint32_t)right.emulated[13]),
			  impl_limit255((uint32_t)left.emulated[14] + (uint32_t)right.emulated[14]),
			  impl_limit255((uint32_t)left.emulated[15] + (uint32_t)right.emulated[15])
			);
		#endif
	}
	inline U8x16 saturatedSubtraction(const U8x16& left, const U8x16& right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(SUB_SAT_U8_SIMD(left.v, right.v));
		#else
			return U8x16(
			  impl_limit0((int32_t)left.emulated[0] - (int32_t)right.emulated[0]),
			  impl_limit0((int32_t)left.emulated[1] - (int32_t)right.emulated[1]),
			  impl_limit0((int32_t)left.emulated[2] - (int32_t)right.emulated[2]),
			  impl_limit0((int32_t)left.emulated[3] - (int32_t)right.emulated[3]),
			  impl_limit0((int32_t)left.emulated[4] - (int32_t)right.emulated[4]),
			  impl_limit0((int32_t)left.emulated[5] - (int32_t)right.emulated[5]),
			  impl_limit0((int32_t)left.emulated[6] - (int32_t)right.emulated[6]),
			  impl_limit0((int32_t)left.emulated[7] - (int32_t)right.emulated[7]),
			  impl_limit0((int32_t)left.emulated[8] - (int32_t)right.emulated[8]),
			  impl_limit0((int32_t)left.emulated[9] - (int32_t)right.emulated[9]),
			  impl_limit0((int32_t)left.emulated[10] - (int32_t)right.emulated[10]),
			  impl_limit0((int32_t)left.emulated[11] - (int32_t)right.emulated[11]),
			  impl_limit0((int32_t)left.emulated[12] - (int32_t)right.emulated[12]),
			  impl_limit0((int32_t)left.emulated[13] - (int32_t)right.emulated[13]),
			  impl_limit0((int32_t)left.emulated[14] - (int32_t)right.emulated[14]),
			  impl_limit0((int32_t)left.emulated[15] - (int32_t)right.emulated[15])
			);
		#endif
	}

	inline I32x4 truncateToI32(const F32x4& vector) {
		#ifdef USE_BASIC_SIMD
			return I32x4(F32_TO_I32_SIMD(vector.v));
		#else
			return I32x4((int32_t)vector.emulated[0], (int32_t)vector.emulated[1], (int32_t)vector.emulated[2], (int32_t)vector.emulated[3]);
		#endif
	}
	inline U32x4 truncateToU32(const F32x4& vector) {
		#ifdef USE_BASIC_SIMD
			return U32x4(F32_TO_U32_SIMD(vector.v));
		#else
			return U32x4((uint32_t)vector.emulated[0], (uint32_t)vector.emulated[1], (uint32_t)vector.emulated[2], (uint32_t)vector.emulated[3]);
		#endif
	}
	inline F32x4 floatFromI32(const I32x4& vector) {
		#ifdef USE_BASIC_SIMD
			return F32x4(I32_TO_F32_SIMD(vector.v));
		#else
			return F32x4((float)vector.emulated[0], (float)vector.emulated[1], (float)vector.emulated[2], (float)vector.emulated[3]);
		#endif
	}
	inline F32x4 floatFromU32(const U32x4& vector) {
		#ifdef USE_BASIC_SIMD
			return F32x4(U32_TO_F32_SIMD(vector.v));
		#else
			return F32x4((float)vector.emulated[0], (float)vector.emulated[1], (float)vector.emulated[2], (float)vector.emulated[3]);
		#endif
	}
	inline I32x4 I32FromU32(const U32x4& vector) {
		#ifdef USE_BASIC_SIMD
			return I32x4(REINTERPRET_U32_TO_I32_SIMD(vector.v));
		#else
			return I32x4((int32_t)vector.emulated[0], (int32_t)vector.emulated[1], (int32_t)vector.emulated[2], (int32_t)vector.emulated[3]);
		#endif
	}
	inline U32x4 U32FromI32(const I32x4& vector) {
		#ifdef USE_BASIC_SIMD
			return U32x4(REINTERPRET_I32_TO_U32_SIMD(vector.v));
		#else
			return U32x4((uint32_t)vector.emulated[0], (uint32_t)vector.emulated[1], (uint32_t)vector.emulated[2], (uint32_t)vector.emulated[3]);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U8x16 reinterpret_U8FromU32(const U32x4& vector) {
		#ifdef USE_BASIC_SIMD
			return U8x16(REINTERPRET_U32_TO_U8_SIMD(vector.v));
		#else
			const uint8_t *source = (const uint8_t*)vector.emulated;
			return U8x16(
			  source[0], source[1], source[2], source[3], source[4], source[5], source[6], source[7],
			  source[8], source[9], source[10], source[11], source[12], source[13], source[14], source[15]
			);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U32x4 reinterpret_U32FromU8(const U8x16& vector) {
		#ifdef USE_BASIC_SIMD
			return U32x4(REINTERPRET_U8_TO_U32_SIMD(vector.v));
		#else
			const uint32_t *source = (const uint32_t*)vector.emulated;
			return U32x4(source[0], source[1], source[2], source[3]);
		#endif
	}

	// Unpacking to larger integers
	inline U32x4 lowerToU32(const U16x8& vector) {
		#ifdef USE_BASIC_SIMD
			return U32x4(U16_LOW_TO_U32_SIMD(vector.v));
		#else
			return U32x4(vector.emulated[0], vector.emulated[1], vector.emulated[2], vector.emulated[3]);
		#endif
	}
	inline U32x4 higherToU32(const U16x8& vector) {
		#ifdef USE_BASIC_SIMD
			return U32x4(U16_HIGH_TO_U32_SIMD(vector.v));
		#else
			return U32x4(vector.emulated[4], vector.emulated[5], vector.emulated[6], vector.emulated[7]);
		#endif
	}
	inline U16x8 lowerToU16(const U8x16& vector) {
		#ifdef USE_BASIC_SIMD
			return U16x8(U8_LOW_TO_U16_SIMD(vector.v));
		#else
			return U16x8(
			  vector.emulated[0], vector.emulated[1], vector.emulated[2], vector.emulated[3],
			  vector.emulated[4], vector.emulated[5], vector.emulated[6], vector.emulated[7]
			);
		#endif
	}
	inline U16x8 higherToU16(const U8x16& vector) {
		#ifdef USE_BASIC_SIMD
			return U16x8(U8_HIGH_TO_U16_SIMD(vector.v));
		#else
			return U16x8(
			  vector.emulated[8], vector.emulated[9], vector.emulated[10], vector.emulated[11],
			  vector.emulated[12], vector.emulated[13], vector.emulated[14], vector.emulated[15]
			);
		#endif
	}

	// Saturated packing
	inline U8x16 saturateToU8(const U16x8& lower, const U16x8& upper) {
		#ifdef USE_BASIC_SIMD
			return U8x16(PACK_SAT_U16_TO_U8_SIMD(lower.v, upper.v));
		#else
			return U8x16(
			  impl_limit255(lower.emulated[0]),
			  impl_limit255(lower.emulated[1]),
			  impl_limit255(lower.emulated[2]),
			  impl_limit255(lower.emulated[3]),
			  impl_limit255(lower.emulated[4]),
			  impl_limit255(lower.emulated[5]),
			  impl_limit255(lower.emulated[6]),
			  impl_limit255(lower.emulated[7]),
			  impl_limit255(upper.emulated[0]),
			  impl_limit255(upper.emulated[1]),
			  impl_limit255(upper.emulated[2]),
			  impl_limit255(upper.emulated[3]),
			  impl_limit255(upper.emulated[4]),
			  impl_limit255(upper.emulated[5]),
			  impl_limit255(upper.emulated[6]),
			  impl_limit255(upper.emulated[7])
			);
		#endif
	}

	// Unary negation for convenience and code readability.
	//   Before using unary negation, always check if:
	//    * An addition can be turned into a subtraction?
	//      x = -a + b
	//      x = b - a
	//    * A multiplying constant or scalar can be negated instead?
	//      x = -b * 2
	//      x = b * -2
	inline F32x4 operator-(const F32x4& value) {
		#ifdef USE_BASIC_SIMD
			return F32x4(0) - value;
		#else
			return F32x4(-value.emulated[0], -value.emulated[1], -value.emulated[2], -value.emulated[3]);
		#endif
	}
	inline I32x4 operator-(const I32x4& value) {
		#ifdef USE_BASIC_SIMD
			return I32x4(0.0f) - value;
		#else
			return I32x4(-value.emulated[0], -value.emulated[1], -value.emulated[2], -value.emulated[3]);
		#endif
	}

	// Helper macros for generating the vector extract functions.
	//   Having one function for each type and offset makes sure that the compiler gets an immediate integer within the valid range.
	#ifdef USE_BASIC_SIMD
		#ifdef USE_SSE2
			#ifdef USE_SSSE3
				#define _MM_ALIGNR_EPI8(A, B, OFFSET) _mm_alignr_epi8(A, B, OFFSET)
			#else
				// If SSSE3 is not used, emulate it using stack memory and unaligned reading of data.
				static inline SIMD_U8x16 _MM_ALIGNR_EPI8(SIMD_U8x16 a, SIMD_U8x16 b, int offset) {
					ALIGN16 uint8_t vectorBuffer[32];
					_mm_store_si128((SIMD_U8x16*)(vectorBuffer), b);
					_mm_store_si128((SIMD_U8x16*)(vectorBuffer + 16), a);
					return _mm_loadu_si128((SIMD_U8x16*)(vectorBuffer + offset));
				}
			#endif
			#define VECTOR_EXTRACT_GENERATOR_U8(OFFSET, FALLBACK_RESULT) return U8x16(_MM_ALIGNR_EPI8(b.v, a.v, OFFSET));
			#define VECTOR_EXTRACT_GENERATOR_U16(OFFSET, FALLBACK_RESULT) return U16x8(_MM_ALIGNR_EPI8(b.v, a.v, OFFSET * 2));
			#define VECTOR_EXTRACT_GENERATOR_U32(OFFSET, FALLBACK_RESULT) return U32x4(_MM_ALIGNR_EPI8(b.v, a.v, OFFSET * 4));
			#define VECTOR_EXTRACT_GENERATOR_I32(OFFSET, FALLBACK_RESULT) return I32x4(_MM_ALIGNR_EPI8(b.v, a.v, OFFSET * 4));
			#define VECTOR_EXTRACT_GENERATOR_F32(OFFSET, FALLBACK_RESULT) return F32x4(SIMD_F32x4(_MM_ALIGNR_EPI8(SIMD_U32x4(b.v), SIMD_U32x4(a.v), OFFSET * 4)));
		#elif USE_NEON
			#define VECTOR_EXTRACT_GENERATOR_U8(OFFSET, FALLBACK_RESULT) return U8x16(vextq_u8(a.v, b.v, OFFSET));
			#define VECTOR_EXTRACT_GENERATOR_U16(OFFSET, FALLBACK_RESULT) return U16x8(vextq_u16(a.v, b.v, OFFSET));
			#define VECTOR_EXTRACT_GENERATOR_U32(OFFSET, FALLBACK_RESULT) return U32x4(vextq_u32(a.v, b.v, OFFSET));
			#define VECTOR_EXTRACT_GENERATOR_I32(OFFSET, FALLBACK_RESULT) return I32x4(vextq_s32(a.v, b.v, OFFSET));
			#define VECTOR_EXTRACT_GENERATOR_F32(OFFSET, FALLBACK_RESULT) return F32x4(vextq_f32(a.v, b.v, OFFSET));
		#endif
	#else
		#define VECTOR_EXTRACT_GENERATOR_U8(OFFSET, FALLBACK_RESULT) return FALLBACK_RESULT;
		#define VECTOR_EXTRACT_GENERATOR_U16(OFFSET, FALLBACK_RESULT) return FALLBACK_RESULT;
		#define VECTOR_EXTRACT_GENERATOR_U32(OFFSET, FALLBACK_RESULT) return FALLBACK_RESULT;
		#define VECTOR_EXTRACT_GENERATOR_I32(OFFSET, FALLBACK_RESULT) return FALLBACK_RESULT;
		#define VECTOR_EXTRACT_GENERATOR_F32(OFFSET, FALLBACK_RESULT) return FALLBACK_RESULT;
	#endif

	// Vector extraction concatunates two input vectors and reads a vector between them using an offset.
	//   The first and last offsets that only return one of the inputs can be used for readability, because they will be inlined and removed by the compiler.
	//   To get elements from the right side, combine the center vector with the right vector and shift one element to the left using vectorExtract_1 for the given type.
	//   To get elements from the left side, combine the left vector with the center vector and shift one element to the right using vectorExtract_15 for 16 lanes, vectorExtract_7 for 8 lanes, or vectorExtract_3 for 4 lanes.
	U8x16 inline vectorExtract_0(const U8x16 &a, const U8x16 &b) { return a; }
	U8x16 inline vectorExtract_1(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(1, U8x16(a.emulated[1], a.emulated[2], a.emulated[3], a.emulated[4], a.emulated[5], a.emulated[6], a.emulated[7], a.emulated[8], a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0])) }
	U8x16 inline vectorExtract_2(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(2, U8x16(a.emulated[2], a.emulated[3], a.emulated[4], a.emulated[5], a.emulated[6], a.emulated[7], a.emulated[8], a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1])) }
	U8x16 inline vectorExtract_3(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(3, U8x16(a.emulated[3], a.emulated[4], a.emulated[5], a.emulated[6], a.emulated[7], a.emulated[8], a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2])) }
	U8x16 inline vectorExtract_4(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(4, U8x16(a.emulated[4], a.emulated[5], a.emulated[6], a.emulated[7], a.emulated[8], a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3])) }
	U8x16 inline vectorExtract_5(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(5, U8x16(a.emulated[5], a.emulated[6], a.emulated[7], a.emulated[8], a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4])) }
	U8x16 inline vectorExtract_6(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(6, U8x16(a.emulated[6], a.emulated[7], a.emulated[8], a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5])) }
	U8x16 inline vectorExtract_7(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(7, U8x16(a.emulated[7], a.emulated[8], a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6])) }
	U8x16 inline vectorExtract_8(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(8, U8x16(a.emulated[8], a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6], b.emulated[7])) }
	U8x16 inline vectorExtract_9(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(9, U8x16(a.emulated[9], a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6], b.emulated[7], b.emulated[8])) }
	U8x16 inline vectorExtract_10(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(10, U8x16(a.emulated[10], a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6], b.emulated[7], b.emulated[8], b.emulated[9])) }
	U8x16 inline vectorExtract_11(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(11, U8x16(a.emulated[11], a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6], b.emulated[7], b.emulated[8], b.emulated[9], b.emulated[10])) }
	U8x16 inline vectorExtract_12(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(12, U8x16(a.emulated[12], a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6], b.emulated[7], b.emulated[8], b.emulated[9], b.emulated[10], b.emulated[11])) }
	U8x16 inline vectorExtract_13(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(13, U8x16(a.emulated[13], a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6], b.emulated[7], b.emulated[8], b.emulated[9], b.emulated[10], b.emulated[11], b.emulated[12])) }
	U8x16 inline vectorExtract_14(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(14, U8x16(a.emulated[14], a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6], b.emulated[7], b.emulated[8], b.emulated[9], b.emulated[10], b.emulated[11], b.emulated[12], b.emulated[13])) }
	U8x16 inline vectorExtract_15(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(15, U8x16(a.emulated[15], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6], b.emulated[7], b.emulated[8], b.emulated[9], b.emulated[10], b.emulated[11], b.emulated[12], b.emulated[13], b.emulated[14])) }
	U8x16 inline vectorExtract_16(const U8x16 &a, const U8x16 &b) { return b; }

	U16x8 inline vectorExtract_0(const U16x8 &a, const U16x8 &b) { return a; }
	U16x8 inline vectorExtract_1(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(1, U16x8(a.emulated[1], a.emulated[2], a.emulated[3], a.emulated[4], a.emulated[5], a.emulated[6], a.emulated[7], b.emulated[0])) }
	U16x8 inline vectorExtract_2(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(2, U16x8(a.emulated[2], a.emulated[3], a.emulated[4], a.emulated[5], a.emulated[6], a.emulated[7], b.emulated[0], b.emulated[1])) }
	U16x8 inline vectorExtract_3(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(3, U16x8(a.emulated[3], a.emulated[4], a.emulated[5], a.emulated[6], a.emulated[7], b.emulated[0], b.emulated[1], b.emulated[2])) }
	U16x8 inline vectorExtract_4(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(4, U16x8(a.emulated[4], a.emulated[5], a.emulated[6], a.emulated[7], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3])) }
	U16x8 inline vectorExtract_5(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(5, U16x8(a.emulated[5], a.emulated[6], a.emulated[7], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4])) }
	U16x8 inline vectorExtract_6(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(6, U16x8(a.emulated[6], a.emulated[7], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5])) }
	U16x8 inline vectorExtract_7(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(7, U16x8(a.emulated[7], b.emulated[0], b.emulated[1], b.emulated[2], b.emulated[3], b.emulated[4], b.emulated[5], b.emulated[6])) }
	U16x8 inline vectorExtract_8(const U16x8 &a, const U16x8 &b) { return b; }

	U32x4 inline vectorExtract_0(const U32x4 &a, const U32x4 &b) { return a; }
	U32x4 inline vectorExtract_1(const U32x4 &a, const U32x4 &b) { VECTOR_EXTRACT_GENERATOR_U32(1, U32x4(a.emulated[1], a.emulated[2], a.emulated[3], b.emulated[0])) }
	U32x4 inline vectorExtract_2(const U32x4 &a, const U32x4 &b) { VECTOR_EXTRACT_GENERATOR_U32(2, U32x4(a.emulated[2], a.emulated[3], b.emulated[0], b.emulated[1])) }
	U32x4 inline vectorExtract_3(const U32x4 &a, const U32x4 &b) { VECTOR_EXTRACT_GENERATOR_U32(3, U32x4(a.emulated[3], b.emulated[0], b.emulated[1], b.emulated[2])) }
	U32x4 inline vectorExtract_4(const U32x4 &a, const U32x4 &b) { return b; }

	I32x4 inline vectorExtract_0(const I32x4 &a, const I32x4 &b) { return a; }
	I32x4 inline vectorExtract_1(const I32x4 &a, const I32x4 &b) { VECTOR_EXTRACT_GENERATOR_I32(1, I32x4(a.emulated[1], a.emulated[2], a.emulated[3], b.emulated[0])) }
	I32x4 inline vectorExtract_2(const I32x4 &a, const I32x4 &b) { VECTOR_EXTRACT_GENERATOR_I32(2, I32x4(a.emulated[2], a.emulated[3], b.emulated[0], b.emulated[1])) }
	I32x4 inline vectorExtract_3(const I32x4 &a, const I32x4 &b) { VECTOR_EXTRACT_GENERATOR_I32(3, I32x4(a.emulated[3], b.emulated[0], b.emulated[1], b.emulated[2])) }
	I32x4 inline vectorExtract_4(const I32x4 &a, const I32x4 &b) { return b; }

	F32x4 inline vectorExtract_0(const F32x4 &a, const F32x4 &b) { return a; }
	F32x4 inline vectorExtract_1(const F32x4 &a, const F32x4 &b) { VECTOR_EXTRACT_GENERATOR_F32(1, F32x4(a.emulated[1], a.emulated[2], a.emulated[3], b.emulated[0])) }
	F32x4 inline vectorExtract_2(const F32x4 &a, const F32x4 &b) { VECTOR_EXTRACT_GENERATOR_F32(2, F32x4(a.emulated[2], a.emulated[3], b.emulated[0], b.emulated[1])) }
	F32x4 inline vectorExtract_3(const F32x4 &a, const F32x4 &b) { VECTOR_EXTRACT_GENERATOR_F32(3, F32x4(a.emulated[3], b.emulated[0], b.emulated[1], b.emulated[2])) }
	F32x4 inline vectorExtract_4(const F32x4 &a, const F32x4 &b) { return b; }

	// Gather instructions load memory from a pointer at multiple index offsets at the same time.
	//   The given pointers should be aligned with 4 bytes, so that the fallback solution works on machines with strict alignment requirements.
	#ifdef USE_AVX2
		#define GATHER_I32x4_AVX2(SOURCE, FOUR_OFFSETS, SCALE) _mm_i32gather_epi32((const int32_t*)(SOURCE), FOUR_OFFSETS, SCALE)
		#define GATHER_U32x4_AVX2(SOURCE, FOUR_OFFSETS, SCALE) _mm_i32gather_epi32((const int32_t*)(SOURCE), FOUR_OFFSETS, SCALE)
		#define GATHER_F32x4_AVX2(SOURCE, FOUR_OFFSETS, SCALE) _mm_i32gather_ps((const float*)(SOURCE), FOUR_OFFSETS, SCALE)
	#endif
	static inline U32x4 gather(const dsr::SafePointer<uint32_t> data, const U32x4 &elementOffset) {
		#ifdef USE_AVX2
			// TODO: Implement safety checks for debug mode.
			return U32x4(GATHER_U32x4_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			ALIGN16 uint32_t elementOffsets[4];
			elementOffset.writeAlignedUnsafe(elementOffsets);
			return U32x4(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3])
			);
		#endif
	}
	static inline I32x4 gather(const dsr::SafePointer<int32_t> data, const U32x4 &elementOffset) {
		#ifdef USE_AVX2
			// TODO: Implement safety checks for debug mode.
			return I32x4(GATHER_U32x4_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			ALIGN16 uint32_t elementOffsets[4];
			elementOffset.writeAlignedUnsafe(elementOffsets);
			return I32x4(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3])
			);
		#endif
	}
	static inline F32x4 gather(const dsr::SafePointer<float> data, const U32x4 &elementOffset) {
		#ifdef USE_AVX2
			// TODO: Implement safety checks for debug mode.
			return F32x4(GATHER_F32x4_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			ALIGN16 uint32_t elementOffsets[4];
			elementOffset.writeAlignedUnsafe(elementOffsets);
			return F32x4(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3])
			);
		#endif
	}
	inline F32x8 operator+(const F32x8& left, const F32x8& right) {
		#ifdef USE_256BIT_SIMD
			return F32x8(ADD_F32_SIMD256(left.v, right.v));
		#else
			return F32x8(
			  left.emulated[0] + right.emulated[0],
			  left.emulated[1] + right.emulated[1],
			  left.emulated[2] + right.emulated[2],
			  left.emulated[3] + right.emulated[3],
			  left.emulated[4] + right.emulated[4],
			  left.emulated[5] + right.emulated[5],
			  left.emulated[6] + right.emulated[6],
			  left.emulated[7] + right.emulated[7]
			);
		#endif
	}
	inline F32x8 operator-(const F32x8& left, const F32x8& right) {
		#ifdef USE_256BIT_SIMD
			return F32x8(SUB_F32_SIMD256(left.v, right.v));
		#else
			return F32x8(
			  left.emulated[0] - right.emulated[0],
			  left.emulated[1] - right.emulated[1],
			  left.emulated[2] - right.emulated[2],
			  left.emulated[3] - right.emulated[3],
			  left.emulated[4] - right.emulated[4],
			  left.emulated[5] - right.emulated[5],
			  left.emulated[6] - right.emulated[6],
			  left.emulated[7] - right.emulated[7]
			);
		#endif
	}
	inline F32x8 operator*(const F32x8& left, const F32x8& right) {
		#ifdef USE_256BIT_SIMD
			return F32x8(MUL_F32_SIMD256(left.v, right.v));
		#else
			return F32x8(
			  left.emulated[0] * right.emulated[0],
			  left.emulated[1] * right.emulated[1],
			  left.emulated[2] * right.emulated[2],
			  left.emulated[3] * right.emulated[3],
			  left.emulated[4] * right.emulated[4],
			  left.emulated[5] * right.emulated[5],
			  left.emulated[6] * right.emulated[6],
			  left.emulated[7] * right.emulated[7]
			);
		#endif
	}
	inline F32x8 min(const F32x8& left, const F32x8& right) {
		#ifdef USE_256BIT_SIMD
			return F32x8(MIN_F32_SIMD256(left.v, right.v));
		#else
			float v0 = left.emulated[0];
			float v1 = left.emulated[1];
			float v2 = left.emulated[2];
			float v3 = left.emulated[3];
			float v4 = left.emulated[4];
			float v5 = left.emulated[5];
			float v6 = left.emulated[6];
			float v7 = left.emulated[7];
			float r0 = right.emulated[0];
			float r1 = right.emulated[1];
			float r2 = right.emulated[2];
			float r3 = right.emulated[3];
			float r4 = right.emulated[4];
			float r5 = right.emulated[5];
			float r6 = right.emulated[6];
			float r7 = right.emulated[7];
			if (r0 < v0) { v0 = r0; }
			if (r1 < v1) { v1 = r1; }
			if (r2 < v2) { v2 = r2; }
			if (r3 < v3) { v3 = r3; }
			if (r4 < v4) { v4 = r4; }
			if (r5 < v5) { v5 = r5; }
			if (r6 < v6) { v6 = r6; }
			if (r7 < v7) { v7 = r7; }
			return F32x8(v0, v1, v2, v3, v4, v5, v6, v7);
		#endif
	}
	inline F32x8 max(const F32x8& left, const F32x8& right) {
		#ifdef USE_256BIT_SIMD
			return F32x8(MAX_F32_SIMD256(left.v, right.v));
		#else
			float v0 = left.emulated[0];
			float v1 = left.emulated[1];
			float v2 = left.emulated[2];
			float v3 = left.emulated[3];
			float v4 = left.emulated[4];
			float v5 = left.emulated[5];
			float v6 = left.emulated[6];
			float v7 = left.emulated[7];
			float r0 = right.emulated[0];
			float r1 = right.emulated[1];
			float r2 = right.emulated[2];
			float r3 = right.emulated[3];
			float r4 = right.emulated[4];
			float r5 = right.emulated[5];
			float r6 = right.emulated[6];
			float r7 = right.emulated[7];
			if (r0 > v0) { v0 = r0; }
			if (r1 > v1) { v1 = r1; }
			if (r2 > v2) { v2 = r2; }
			if (r3 > v3) { v3 = r3; }
			if (r4 > v4) { v4 = r4; }
			if (r5 > v5) { v5 = r5; }
			if (r6 > v6) { v6 = r6; }
			if (r7 > v7) { v7 = r7; }
			return F32x8(v0, v1, v2, v3, v4, v5, v6, v7);
		#endif
	}
	inline I32x8 operator+(const I32x8& left, const I32x8& right) {
		#ifdef USE_256BIT_SIMD
			return I32x8(ADD_I32_SIMD256(left.v, right.v));
		#else
			return I32x8(
			  left.emulated[0] + right.emulated[0],
			  left.emulated[1] + right.emulated[1],
			  left.emulated[2] + right.emulated[2],
			  left.emulated[3] + right.emulated[3],
			  left.emulated[4] + right.emulated[4],
			  left.emulated[5] + right.emulated[5],
			  left.emulated[6] + right.emulated[6],
			  left.emulated[7] + right.emulated[7]);
		#endif
	}
	inline I32x8 operator-(const I32x8& left, const I32x8& right) {
		#ifdef USE_256BIT_SIMD
			return I32x8(SUB_I32_SIMD256(left.v, right.v));
		#else
			return I32x8(
			  left.emulated[0] - right.emulated[0],
			  left.emulated[1] - right.emulated[1],
			  left.emulated[2] - right.emulated[2],
			  left.emulated[3] - right.emulated[3],
			  left.emulated[4] - right.emulated[4],
			  left.emulated[5] - right.emulated[5],
			  left.emulated[6] - right.emulated[6],
			  left.emulated[7] - right.emulated[7]);
		#endif
	}
	inline I32x8 operator*(const I32x8& left, const I32x8& right) {
		#ifdef USE_AVX2
			return I32x8(MUL_I32_SIMD256(left.v, right.v));
		#else
			return I32x8(
			  left.emulated[0] * right.emulated[0],
			  left.emulated[1] * right.emulated[1],
			  left.emulated[2] * right.emulated[2],
			  left.emulated[3] * right.emulated[3],
			  left.emulated[4] * right.emulated[4],
			  left.emulated[5] * right.emulated[5],
			  left.emulated[6] * right.emulated[6],
			  left.emulated[7] * right.emulated[7]
			);
		#endif
	}
	inline U32x8 operator+(const U32x8& left, const U32x8& right) {
		#ifdef USE_256BIT_SIMD
			return U32x8(ADD_U32_SIMD256(left.v, right.v));
		#else
			return U32x8(
			  left.emulated[0] + right.emulated[0],
			  left.emulated[1] + right.emulated[1],
			  left.emulated[2] + right.emulated[2],
			  left.emulated[3] + right.emulated[3],
			  left.emulated[4] + right.emulated[4],
			  left.emulated[5] + right.emulated[5],
			  left.emulated[6] + right.emulated[6],
			  left.emulated[7] + right.emulated[7]
			);
		#endif
	}
	inline U32x8 operator-(const U32x8& left, const U32x8& right) {
		#ifdef USE_256BIT_SIMD
			return U32x8(SUB_U32_SIMD256(left.v, right.v));
		#else
			return U32x8(
			  left.emulated[0] - right.emulated[0],
			  left.emulated[1] - right.emulated[1],
			  left.emulated[2] - right.emulated[2],
			  left.emulated[3] - right.emulated[3],
			  left.emulated[4] - right.emulated[4],
			  left.emulated[5] - right.emulated[5],
			  left.emulated[6] - right.emulated[6],
			  left.emulated[7] - right.emulated[7]
			);
		#endif
	}
	inline U32x8 operator*(const U32x8& left, const U32x8& right) {
		#ifdef USE_AVX2
			return U32x8(MUL_U32_SIMD256(left.v, right.v));
		#else
			return U32x8(
			  left.emulated[0] * right.emulated[0],
			  left.emulated[1] * right.emulated[1],
			  left.emulated[2] * right.emulated[2],
			  left.emulated[3] * right.emulated[3],
			  left.emulated[4] * right.emulated[4],
			  left.emulated[5] * right.emulated[5],
			  left.emulated[6] * right.emulated[6],
			  left.emulated[7] * right.emulated[7]
			);
		#endif
	}
	inline U32x8 operator&(const U32x8& left, const U32x8& right) {
		#ifdef USE_256BIT_SIMD
			return U32x8(BITWISE_AND_U32_SIMD256(left.v, right.v));
		#else
			return U32x8(
			  left.emulated[0] & right.emulated[0],
			  left.emulated[1] & right.emulated[1],
			  left.emulated[2] & right.emulated[2],
			  left.emulated[3] & right.emulated[3],
			  left.emulated[4] & right.emulated[4],
			  left.emulated[5] & right.emulated[5],
			  left.emulated[6] & right.emulated[6],
			  left.emulated[7] & right.emulated[7]
			);
		#endif
	}
	inline U32x8 operator|(const U32x8& left, const U32x8& right) {
		#ifdef USE_256BIT_SIMD
			return U32x8(BITWISE_OR_U32_SIMD256(left.v, right.v));
		#else
			return U32x8(
			  left.emulated[0] | right.emulated[0],
			  left.emulated[1] | right.emulated[1],
			  left.emulated[2] | right.emulated[2],
			  left.emulated[3] | right.emulated[3],
			  left.emulated[4] | right.emulated[4],
			  left.emulated[5] | right.emulated[5],
			  left.emulated[6] | right.emulated[6],
			  left.emulated[7] | right.emulated[7]
			);
		#endif
	}
	inline U32x8 operator^(const U32x8& left, const U32x8& right) {
		#ifdef USE_256BIT_SIMD
			return U32x8(BITWISE_XOR_U32_SIMD256(left.v, right.v));
		#else
			return U32x8(
			  left.emulated[0] ^ right.emulated[0],
			  left.emulated[1] ^ right.emulated[1],
			  left.emulated[2] ^ right.emulated[2],
			  left.emulated[3] ^ right.emulated[3],
			  left.emulated[4] ^ right.emulated[4],
			  left.emulated[5] ^ right.emulated[5],
			  left.emulated[6] ^ right.emulated[6],
			  left.emulated[7] ^ right.emulated[7]
			);
		#endif
	}
	inline U32x8 operator<<(const U32x8& left, uint32_t bitOffset) {
		#ifdef USE_AVX2
			return U32x8(_mm256_slli_epi32(left.v, bitOffset));
		#else
			return U32x8(
			  left.emulated[0] << bitOffset,
			  left.emulated[1] << bitOffset,
			  left.emulated[2] << bitOffset,
			  left.emulated[3] << bitOffset,
			  left.emulated[4] << bitOffset,
			  left.emulated[5] << bitOffset,
			  left.emulated[6] << bitOffset,
			  left.emulated[7] << bitOffset
			);
		#endif
	}
	inline U32x8 operator>>(const U32x8& left, uint32_t bitOffset) {
		#ifdef USE_AVX2
			return U32x8(_mm256_srli_epi32(left.v, bitOffset));
		#else
			return U32x8(
			  left.emulated[0] >> bitOffset,
			  left.emulated[1] >> bitOffset,
			  left.emulated[2] >> bitOffset,
			  left.emulated[3] >> bitOffset,
			  left.emulated[4] >> bitOffset,
			  left.emulated[5] >> bitOffset,
			  left.emulated[6] >> bitOffset,
			  left.emulated[7] >> bitOffset
			);
		#endif
	}
	inline U16x16 operator+(const U16x16& left, const U16x16& right) {
		#ifdef USE_256BIT_SIMD
			return U16x16(ADD_U16_SIMD256(left.v, right.v));
		#else
			return U16x16(
			  left.emulated[0] + right.emulated[0],
			  left.emulated[1] + right.emulated[1],
			  left.emulated[2] + right.emulated[2],
			  left.emulated[3] + right.emulated[3],
			  left.emulated[4] + right.emulated[4],
			  left.emulated[5] + right.emulated[5],
			  left.emulated[6] + right.emulated[6],
			  left.emulated[7] + right.emulated[7],
			  left.emulated[8] + right.emulated[8],
			  left.emulated[9] + right.emulated[9],
			  left.emulated[10] + right.emulated[10],
			  left.emulated[11] + right.emulated[11],
			  left.emulated[12] + right.emulated[12],
			  left.emulated[13] + right.emulated[13],
			  left.emulated[14] + right.emulated[14],
			  left.emulated[15] + right.emulated[15]
			);
		#endif
	}
	inline U16x16 operator-(const U16x16& left, const U16x16& right) {
		#ifdef USE_256BIT_SIMD
			return U16x16(SUB_U16_SIMD256(left.v, right.v));
		#else
			return U16x16(
			  left.emulated[0] - right.emulated[0],
			  left.emulated[1] - right.emulated[1],
			  left.emulated[2] - right.emulated[2],
			  left.emulated[3] - right.emulated[3],
			  left.emulated[4] - right.emulated[4],
			  left.emulated[5] - right.emulated[5],
			  left.emulated[6] - right.emulated[6],
			  left.emulated[7] - right.emulated[7],
			  left.emulated[8] - right.emulated[8],
			  left.emulated[9] - right.emulated[9],
			  left.emulated[10] - right.emulated[10],
			  left.emulated[11] - right.emulated[11],
			  left.emulated[12] - right.emulated[12],
			  left.emulated[13] - right.emulated[13],
			  left.emulated[14] - right.emulated[14],
			  left.emulated[15] - right.emulated[15]
			);
		#endif
	}
	inline U16x16 operator*(const U16x16& left, const U16x16& right) {
		#ifdef USE_256BIT_SIMD
			return U16x16(MUL_U16_SIMD256(left.v, right.v));
		#else
			return U16x16(
			  left.emulated[0] * right.emulated[0],
			  left.emulated[1] * right.emulated[1],
			  left.emulated[2] * right.emulated[2],
			  left.emulated[3] * right.emulated[3],
			  left.emulated[4] * right.emulated[4],
			  left.emulated[5] * right.emulated[5],
			  left.emulated[6] * right.emulated[6],
			  left.emulated[7] * right.emulated[7],
			  left.emulated[8] * right.emulated[8],
			  left.emulated[9] * right.emulated[9],
			  left.emulated[10] * right.emulated[10],
			  left.emulated[11] * right.emulated[11],
			  left.emulated[12] * right.emulated[12],
			  left.emulated[13] * right.emulated[13],
			  left.emulated[14] * right.emulated[14],
			  left.emulated[15] * right.emulated[15]
			);
		#endif
	}
	inline U8x32 operator+(const U8x32& left, const U8x32& right) {
		#ifdef USE_256BIT_SIMD
			return U8x32(ADD_U8_SIMD256(left.v, right.v));
		#else
			U8x32 result;
			for (int i = 0; i < 32; i++) {
				result.emulated[i] = left.emulated[i] + right.emulated[i];
			}
			return result;
		#endif
	}
	inline U8x32 operator-(const U8x32& left, const U8x32& right) {
		#ifdef USE_256BIT_SIMD
			return U8x32(SUB_U8_SIMD256(left.v, right.v));
		#else
			U8x32 result;
			for (int i = 0; i < 32; i++) {
				result.emulated[i] = left.emulated[i] - right.emulated[i];
			}
			return result;
		#endif
	}
	inline U8x32 saturatedAddition(const U8x32& left, const U8x32& right) {
		#ifdef USE_256BIT_SIMD
			return U8x32(ADD_SAT_U8_SIMD256(left.v, right.v));
		#else
			U8x32 result;
			for (int i = 0; i < 32; i++) {
				result.emulated[i] = impl_limit255((uint32_t)left.emulated[i] + (uint32_t)right.emulated[i]);
			}
			return result;
		#endif
	}
	inline U8x32 saturatedSubtraction(const U8x32& left, const U8x32& right) {
		#ifdef USE_256BIT_SIMD
			return U8x32(SUB_SAT_U8_SIMD256(left.v, right.v));
		#else
			U8x32 result;
			for (int i = 0; i < 32; i++) {
				result.emulated[i] = impl_limit0((int32_t)left.emulated[i] - (int32_t)right.emulated[i]);
			}
			return result;
		#endif
	}

	inline I32x8 truncateToI32(const F32x8& vector) {
		#ifdef USE_256BIT_SIMD
			return I32x8(F32_TO_I32_SIMD256(vector.v));
		#else
			return I32x8(
			  (int32_t)vector.emulated[0], (int32_t)vector.emulated[1], (int32_t)vector.emulated[2], (int32_t)vector.emulated[3],
			  (int32_t)vector.emulated[4], (int32_t)vector.emulated[5], (int32_t)vector.emulated[6], (int32_t)vector.emulated[7]
			);
		#endif
	}
	inline U32x8 truncateToU32(const F32x8& vector) {
		#ifdef USE_256BIT_SIMD
			return U32x8(F32_TO_U32_SIMD256(vector.v));
		#else
			return U32x8(
			  (uint32_t)vector.emulated[0], (uint32_t)vector.emulated[1], (uint32_t)vector.emulated[2], (uint32_t)vector.emulated[3],
			  (uint32_t)vector.emulated[4], (uint32_t)vector.emulated[5], (uint32_t)vector.emulated[6], (uint32_t)vector.emulated[7]
			);
		#endif
	}
	inline F32x8 floatFromI32(const I32x8& vector) {
		#ifdef USE_256BIT_SIMD
			return F32x8(I32_TO_F32_SIMD256(vector.v));
		#else
			return F32x8(
			  (float)vector.emulated[0], (float)vector.emulated[1], (float)vector.emulated[2], (float)vector.emulated[3],
			  (float)vector.emulated[4], (float)vector.emulated[5], (float)vector.emulated[6], (float)vector.emulated[7]
			);
		#endif
	}
	inline F32x8 floatFromU32(const U32x8& vector) {
		#ifdef USE_256BIT_SIMD
			return F32x8(U32_TO_F32_SIMD256(vector.v));
		#else
			return F32x8(
			  (float)vector.emulated[0], (float)vector.emulated[1], (float)vector.emulated[2], (float)vector.emulated[3],
			  (float)vector.emulated[4], (float)vector.emulated[5], (float)vector.emulated[6], (float)vector.emulated[7]
			);
		#endif
	}
	inline I32x8 I32FromU32(const U32x8& vector) {
		#ifdef USE_256BIT_SIMD
			return I32x8(REINTERPRET_U32_TO_I32_SIMD256(vector.v));
		#else
			return I32x8(
			  (int32_t)vector.emulated[0], (int32_t)vector.emulated[1], (int32_t)vector.emulated[2], (int32_t)vector.emulated[3],
			  (int32_t)vector.emulated[4], (int32_t)vector.emulated[5], (int32_t)vector.emulated[6], (int32_t)vector.emulated[7]
			);
		#endif
	}
	inline U32x8 U32FromI32(const I32x8& vector) {
		#ifdef USE_256BIT_SIMD
			return U32x8(REINTERPRET_I32_TO_U32_SIMD256(vector.v));
		#else
			return U32x8(
			  (uint32_t)vector.emulated[0], (uint32_t)vector.emulated[1], (uint32_t)vector.emulated[2], (uint32_t)vector.emulated[3],
			  (uint32_t)vector.emulated[4], (uint32_t)vector.emulated[5], (uint32_t)vector.emulated[6], (uint32_t)vector.emulated[7]
			);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U8x32 reinterpret_U8FromU32(const U32x8& vector) {
		#ifdef USE_256BIT_SIMD
			return U8x32(REINTERPRET_U32_TO_U8_SIMD256(vector.v));
		#else
			const uint8_t *source = (const uint8_t*)vector.emulated;
			U8x32 result;
			for (int i = 0; i < 32; i++) {
				result.emulated[i] = source[i];
			}
			return result;
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U32x8 reinterpret_U32FromU8(const U8x32& vector) {
		#ifdef USE_256BIT_SIMD
			return U32x8(REINTERPRET_U8_TO_U32_SIMD256(vector.v));
		#else
			const uint32_t *source = (const uint32_t*)vector.emulated;
			return U32x8(source[0], source[1], source[2], source[3], source[4], source[5], source[6], source[7]);
		#endif
	}

	// Unpacking to larger integers
	inline U32x8 lowerToU32(const U16x16& vector) {
		#ifdef USE_256BIT_SIMD
			return U32x8(U16_LOW_TO_U32_SIMD256(vector.v));
		#else
			return U32x8(vector.emulated[0], vector.emulated[1], vector.emulated[2], vector.emulated[3], vector.emulated[4], vector.emulated[5], vector.emulated[6], vector.emulated[7]);
		#endif
	}
	inline U32x8 higherToU32(const U16x16& vector) {
		#ifdef USE_256BIT_SIMD
			return U32x8(U16_HIGH_TO_U32_SIMD256(vector.v));
		#else
			return U32x8(vector.emulated[8], vector.emulated[9], vector.emulated[10], vector.emulated[11], vector.emulated[12], vector.emulated[13], vector.emulated[14], vector.emulated[15]);
		#endif
	}
	inline U16x16 lowerToU16(const U8x32& vector) {
		#ifdef USE_256BIT_SIMD
			return U16x16(U8_LOW_TO_U16_SIMD256(vector.v));
		#else
			return U16x16(
			  vector.emulated[0], vector.emulated[1], vector.emulated[2], vector.emulated[3],
			  vector.emulated[4], vector.emulated[5], vector.emulated[6], vector.emulated[7],
			  vector.emulated[8], vector.emulated[9], vector.emulated[10], vector.emulated[11],
			  vector.emulated[12], vector.emulated[13], vector.emulated[14], vector.emulated[15]
			);
		#endif
	}
	inline U16x16 higherToU16(const U8x32& vector) {
		#ifdef USE_256BIT_SIMD
			return U16x16(U8_HIGH_TO_U16_SIMD256(vector.v));
		#else
			return U16x16(
			  vector.emulated[16], vector.emulated[17], vector.emulated[18], vector.emulated[19],
			  vector.emulated[20], vector.emulated[21], vector.emulated[22], vector.emulated[23],
			  vector.emulated[24], vector.emulated[25], vector.emulated[26], vector.emulated[27],
			  vector.emulated[28], vector.emulated[29], vector.emulated[30], vector.emulated[31]
			);
		#endif
	}

	// Saturated packing
	inline U8x32 saturateToU8(const U16x16& lower, const U16x16& upper) {
		#ifdef USE_256BIT_SIMD
			return U8x32(PACK_SAT_U16_TO_U8_SIMD256(lower.v, upper.v));
		#else
			U8x32 result;
			for (int i = 0; i < 16; i++) {
				result.emulated[i] = impl_limit255(lower.emulated[i]);
			}
			for (int i = 0; i < 16; i++) {
				result.emulated[i + 16] = impl_limit255(upper.emulated[i]);
			}
			return result;
		#endif
	}

	// Unary negation for convenience and code readability.
	//   Before using unary negation, always check if:
	//    * An addition can be turned into a subtraction?
	//      x = -a + b
	//      x = b - a
	//    * A multiplying constant or scalar can be negated instead?
	//      x = -b * 2
	//      x = b * -2
	inline F32x8 operator-(const F32x8& value) {
		#ifdef USE_256BIT_SIMD
			return F32x8(0) - value;
		#else
			return F32x8(
			  -value.emulated[0], -value.emulated[1], -value.emulated[2], -value.emulated[3],
			  -value.emulated[4], -value.emulated[5], -value.emulated[6], -value.emulated[7]
			);
		#endif
	}
	inline I32x8 operator-(const I32x8& value) {
		#ifdef USE_256BIT_SIMD
			return I32x8(0.0f) - value;
		#else
			return I32x8(
			  -value.emulated[0], -value.emulated[1], -value.emulated[2], -value.emulated[3],
			  -value.emulated[4], -value.emulated[5], -value.emulated[6], -value.emulated[7]
			);
		#endif
	}

	// Helper macros for generating the vector extract functions.
	//   Having one function for each type and offset makes sure that the compiler gets an immediate integer within the valid range.
	#ifdef USE_AVX2
		// AVX2 does not offer any 256-bit element extraction, only two 128-bit shifts done in parallel, so we might as well use two separate 128-bit extractions.
		template <int OFFSET>
		__m256i impl_extractBytes_AVX2(const __m256i leftInput, const __m256i rightInput) {
			// Extract three halves depending on which ones overlap with the offset.
			__m128i leftPart = _mm256_extractf128_si256(leftInput, (OFFSET < 16) ? 0 : 1);
			__m128i middlePart = _mm256_extractf128_si256(OFFSET < 16 ? rightInput : leftInput, (OFFSET < 16) ? 1 : 0);
			__m128i rightPart = _mm256_extractf128_si256(rightInput, (OFFSET < 16) ? 0 : 1);
			// Combine two 128-bit extracts into a whole 256-bit extract.
			return _mm256_set_m128i(
			  _mm_alignr_epi8(leftPart, middlePart, OFFSET - ((OFFSET < 16) ? 0 : 16)),
			  _mm_alignr_epi8(middlePart, rightPart, OFFSET - ((OFFSET < 16) ? 0 : 16))
			);
		}
		#define VECTOR_EXTRACT_GENERATOR_256_U8(OFFSET) return U8x32(impl_extractBytes_AVX2<OFFSET>(b.v, a.v));
		#define VECTOR_EXTRACT_GENERATOR_256_U16(OFFSET) return U16x16(impl_extractBytes_AVX2<OFFSET * 2>(b.v, a.v));
		#define VECTOR_EXTRACT_GENERATOR_256_U32(OFFSET) return U32x8(impl_extractBytes_AVX2<OFFSET * 4>(b.v, a.v));
		#define VECTOR_EXTRACT_GENERATOR_256_I32(OFFSET) return I32x8(impl_extractBytes_AVX2<OFFSET * 4>(b.v, a.v));
		#define VECTOR_EXTRACT_GENERATOR_256_F32(OFFSET) return F32x8(SIMD_F32x8(impl_extractBytes_AVX2<OFFSET * 4>(SIMD_U32x8(b.v), SIMD_U32x8(a.v))));
	#else
		template<typename T, int elementCount>
		T vectorExtract_emulated(const T &a, const T &b, int offset) {
			T result;
			int t = 0;
			for (int s = offset; s < elementCount; s++) {
				result.emulated[t] = a.emulated[s];
				t++;
			}
			for (int s = 0; s < offset; s++) {
				result.emulated[t] = b.emulated[s];
				t++;
			}
			return result;
		}
		#define VECTOR_EXTRACT_GENERATOR_256_U8(OFFSET) return vectorExtract_emulated<U8x32, 32>(a, b, OFFSET);
		#define VECTOR_EXTRACT_GENERATOR_256_U16(OFFSET) return vectorExtract_emulated<U16x16, 16>(a, b, OFFSET);
		#define VECTOR_EXTRACT_GENERATOR_256_U32(OFFSET) return vectorExtract_emulated<U32x8, 8>(a, b, OFFSET);
		#define VECTOR_EXTRACT_GENERATOR_256_I32(OFFSET) return vectorExtract_emulated<I32x8, 8>(a, b, OFFSET);
		#define VECTOR_EXTRACT_GENERATOR_256_F32(OFFSET) return vectorExtract_emulated<F32x8, 8>(a, b, OFFSET);
	#endif

	// Vector extraction concatunates two input vectors and reads a vector between them using an offset.
	//   The first and last offsets that only return one of the inputs can be used for readability, because they will be inlined and removed by the compiler.
	//   To get elements from the right side, combine the center vector with the right vector and shift one element to the left using vectorExtract_1 for the given type.
	//   To get elements from the left side, combine the left vector with the center vector and shift one element to the right using vectorExtract_15 for 16 lanes, vectorExtract_7 for 8 lanes, or vectorExtract_3 for 4 lanes.

	U8x32 inline vectorExtract_0(const U8x32 &a, const U8x32 &b) { return a; }
	U8x32 inline vectorExtract_1(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(1) }
	U8x32 inline vectorExtract_2(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(2) }
	U8x32 inline vectorExtract_3(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(3) }
	U8x32 inline vectorExtract_4(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(4) }
	U8x32 inline vectorExtract_5(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(5) }
	U8x32 inline vectorExtract_6(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(6) }
	U8x32 inline vectorExtract_7(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(7) }
	U8x32 inline vectorExtract_8(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(8) }
	U8x32 inline vectorExtract_9(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(9) }
	U8x32 inline vectorExtract_10(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(10) }
	U8x32 inline vectorExtract_11(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(11) }
	U8x32 inline vectorExtract_12(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(12) }
	U8x32 inline vectorExtract_13(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(13) }
	U8x32 inline vectorExtract_14(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(14) }
	U8x32 inline vectorExtract_15(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(15) }
	U8x32 inline vectorExtract_16(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(16) }
	U8x32 inline vectorExtract_17(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(17) }
	U8x32 inline vectorExtract_18(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(18) }
	U8x32 inline vectorExtract_19(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(19) }
	U8x32 inline vectorExtract_20(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(20) }
	U8x32 inline vectorExtract_21(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(21) }
	U8x32 inline vectorExtract_22(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(22) }
	U8x32 inline vectorExtract_23(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(23) }
	U8x32 inline vectorExtract_24(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(24) }
	U8x32 inline vectorExtract_25(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(25) }
	U8x32 inline vectorExtract_26(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(26) }
	U8x32 inline vectorExtract_27(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(27) }
	U8x32 inline vectorExtract_28(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(28) }
	U8x32 inline vectorExtract_29(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(29) }
	U8x32 inline vectorExtract_30(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(30) }
	U8x32 inline vectorExtract_31(const U8x32 &a, const U8x32 &b) { VECTOR_EXTRACT_GENERATOR_256_U8(31) }
	U8x32 inline vectorExtract_32(const U8x32 &a, const U8x32 &b) { return b; }

	U16x16 inline vectorExtract_0(const U16x16 &a, const U16x16 &b) { return a; }
	U16x16 inline vectorExtract_1(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(1) }
	U16x16 inline vectorExtract_2(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(2) }
	U16x16 inline vectorExtract_3(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(3) }
	U16x16 inline vectorExtract_4(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(4) }
	U16x16 inline vectorExtract_5(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(5) }
	U16x16 inline vectorExtract_6(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(6) }
	U16x16 inline vectorExtract_7(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(7) }
	U16x16 inline vectorExtract_8(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(8) }
	U16x16 inline vectorExtract_9(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(9) }
	U16x16 inline vectorExtract_10(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(10) }
	U16x16 inline vectorExtract_11(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(11) }
	U16x16 inline vectorExtract_12(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(12) }
	U16x16 inline vectorExtract_13(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(13) }
	U16x16 inline vectorExtract_14(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(14) }
	U16x16 inline vectorExtract_15(const U16x16 &a, const U16x16 &b) { VECTOR_EXTRACT_GENERATOR_256_U16(15) }
	U16x16 inline vectorExtract_16(const U16x16 &a, const U16x16 &b) { return b; }

	U32x8 inline vectorExtract_0(const U32x8 &a, const U32x8 &b) { return a; }
	U32x8 inline vectorExtract_1(const U32x8 &a, const U32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_U32(1) }
	U32x8 inline vectorExtract_2(const U32x8 &a, const U32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_U32(2) }
	U32x8 inline vectorExtract_3(const U32x8 &a, const U32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_U32(3) }
	U32x8 inline vectorExtract_4(const U32x8 &a, const U32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_U32(4) }
	U32x8 inline vectorExtract_5(const U32x8 &a, const U32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_U32(5) }
	U32x8 inline vectorExtract_6(const U32x8 &a, const U32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_U32(6) }
	U32x8 inline vectorExtract_7(const U32x8 &a, const U32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_U32(7) }
	U32x8 inline vectorExtract_8(const U32x8 &a, const U32x8 &b) { return b; }

	I32x8 inline vectorExtract_0(const I32x8 &a, const I32x8 &b) { return a; }
	I32x8 inline vectorExtract_1(const I32x8 &a, const I32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_I32(1) }
	I32x8 inline vectorExtract_2(const I32x8 &a, const I32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_I32(2) }
	I32x8 inline vectorExtract_3(const I32x8 &a, const I32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_I32(3) }
	I32x8 inline vectorExtract_4(const I32x8 &a, const I32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_I32(4) }
	I32x8 inline vectorExtract_5(const I32x8 &a, const I32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_I32(5) }
	I32x8 inline vectorExtract_6(const I32x8 &a, const I32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_I32(6) }
	I32x8 inline vectorExtract_7(const I32x8 &a, const I32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_I32(7) }
	I32x8 inline vectorExtract_8(const I32x8 &a, const I32x8 &b) { return b; }

	F32x8 inline vectorExtract_0(const F32x8 &a, const F32x8 &b) { return a; }
	F32x8 inline vectorExtract_1(const F32x8 &a, const F32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_F32(1) }
	F32x8 inline vectorExtract_2(const F32x8 &a, const F32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_F32(2) }
	F32x8 inline vectorExtract_3(const F32x8 &a, const F32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_F32(3) }
	F32x8 inline vectorExtract_4(const F32x8 &a, const F32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_F32(4) }
	F32x8 inline vectorExtract_5(const F32x8 &a, const F32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_F32(5) }
	F32x8 inline vectorExtract_6(const F32x8 &a, const F32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_F32(6) }
	F32x8 inline vectorExtract_7(const F32x8 &a, const F32x8 &b) { VECTOR_EXTRACT_GENERATOR_256_F32(7) }
	F32x8 inline vectorExtract_8(const F32x8 &a, const F32x8 &b) { return b; }

	// Gather instructions load memory from a pointer at multiple index offsets at the same time.
	//   The given pointers should be aligned with 4 bytes, so that the fallback solution works on machines with strict alignment requirements.
	#ifdef USE_AVX2
		#define GATHER_I32x8_AVX2(SOURCE, EIGHT_OFFSETS, SCALE) _mm256_i32gather_epi32((const int32_t*)(SOURCE), EIGHT_OFFSETS, SCALE)
		#define GATHER_U32x8_AVX2(SOURCE, EIGHT_OFFSETS, SCALE) _mm256_i32gather_epi32((const int32_t*)(SOURCE), EIGHT_OFFSETS, SCALE)
		#define GATHER_F32x8_AVX2(SOURCE, EIGHT_OFFSETS, SCALE) _mm256_i32gather_ps((const float*)(SOURCE), EIGHT_OFFSETS, SCALE)
	#endif
	static inline U32x8 gather(const dsr::SafePointer<uint32_t> data, const U32x8 &elementOffset) {
		#ifdef USE_AVX2
			// TODO: Implement safety checks for debug mode.
			return U32x8(GATHER_I32x8_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			ALIGN32 uint32_t elementOffsets[8];
			elementOffset.writeAlignedUnsafe(elementOffsets);
			return U32x8(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3]),
			  *(data + elementOffsets[4]),
			  *(data + elementOffsets[5]),
			  *(data + elementOffsets[6]),
			  *(data + elementOffsets[7])
			);
		#endif
	}
	static inline I32x8 gather(const dsr::SafePointer<int32_t> data, const U32x8 &elementOffset) {
		#ifdef USE_AVX2
			// TODO: Implement safety checks for debug mode.
			return I32x8(GATHER_U32x8_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			ALIGN32 uint32_t elementOffsets[8];
			elementOffset.writeAlignedUnsafe(elementOffsets);
			return I32x8(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3]),
			  *(data + elementOffsets[4]),
			  *(data + elementOffsets[5]),
			  *(data + elementOffsets[6]),
			  *(data + elementOffsets[7])
			);
		#endif
	}
	static inline F32x8 gather(const dsr::SafePointer<float> data, const U32x8 &elementOffset) {
		#ifdef USE_AVX2
			// TODO: Implement safety checks for debug mode.
			return F32x8(GATHER_F32x8_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			ALIGN32 uint32_t elementOffsets[8];
			elementOffset.writeAlignedUnsafe(elementOffsets);
			return F32x8(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3]),
			  *(data + elementOffsets[4]),
			  *(data + elementOffsets[5]),
			  *(data + elementOffsets[6]),
			  *(data + elementOffsets[7])
			);
		#endif
	}

	// Wrapper functions for explicitly expanding scalars into vectors during math operations.
	#define NUMERICAL_SCALAR_OPERATIONS(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline VECTOR_TYPE operator+(const VECTOR_TYPE& left, ELEMENT_TYPE right) { return left + VECTOR_TYPE(right); } \
		inline VECTOR_TYPE operator+(ELEMENT_TYPE left, const VECTOR_TYPE& right) { return VECTOR_TYPE(left) + right; } \
		inline VECTOR_TYPE operator-(const VECTOR_TYPE& left, ELEMENT_TYPE right) { return left - VECTOR_TYPE(right); } \
		inline VECTOR_TYPE operator-(ELEMENT_TYPE left, const VECTOR_TYPE& right) { return VECTOR_TYPE(left) - right; }
		FOR_ALL_VECTOR_TYPES(NUMERICAL_SCALAR_OPERATIONS)
	#undef NUMERICAL_SCALAR_OPERATIONS

	#define MULTIPLY_SCALAR_OPERATIONS(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline VECTOR_TYPE operator*(const VECTOR_TYPE& left, ELEMENT_TYPE right) { return left * VECTOR_TYPE(right); } \
		inline VECTOR_TYPE operator*(ELEMENT_TYPE left, const VECTOR_TYPE& right) { return VECTOR_TYPE(left) * right; }
		// TODO: Implement multiplication for U8x16 and U8x32.
		//FOR_ALL_VECTOR_TYPES(MULTIPLY_SCALAR_OPERATIONS)
		MULTIPLY_SCALAR_OPERATIONS(F32x4, float, 4)
		MULTIPLY_SCALAR_OPERATIONS(F32x8, float, 8)
		MULTIPLY_SCALAR_OPERATIONS(U32x4, uint32_t, 4)
		MULTIPLY_SCALAR_OPERATIONS(U32x8, uint32_t, 8)
		MULTIPLY_SCALAR_OPERATIONS(I32x4, int32_t, 4)
		MULTIPLY_SCALAR_OPERATIONS(I32x8, int32_t, 8)
		MULTIPLY_SCALAR_OPERATIONS(U16x8, uint16_t, 8)
		MULTIPLY_SCALAR_OPERATIONS(U16x16, uint16_t, 16)
	#undef MULTIPLY_SCALAR_OPERATIONS

	// Wrapper functions for explicitly duplicating bit masks into the same lane count.
	#define BITWISE_SCALAR_OPERATIONS(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline VECTOR_TYPE operator&(const VECTOR_TYPE& left, ELEMENT_TYPE right) { return left & VECTOR_TYPE(right); } \
		inline VECTOR_TYPE operator&(ELEMENT_TYPE left, const VECTOR_TYPE& right) { return VECTOR_TYPE(left) & right; } \
		inline VECTOR_TYPE operator|(const VECTOR_TYPE& left, ELEMENT_TYPE right) { return left | VECTOR_TYPE(right); } \
		inline VECTOR_TYPE operator|(ELEMENT_TYPE left, const VECTOR_TYPE& right) { return VECTOR_TYPE(left) | right; } \
		inline VECTOR_TYPE operator^(const VECTOR_TYPE& left, ELEMENT_TYPE right) { return left ^ VECTOR_TYPE(right); } \
		inline VECTOR_TYPE operator^(ELEMENT_TYPE left, const VECTOR_TYPE& right) { return VECTOR_TYPE(left) ^ right; }
		// TODO: Implement bitwise operations for all unsigned SIMD vectors.
		//FOR_UNSIGNED_VECTOR_TYPES(BITWISE_SCALAR_OPERATIONS)
		BITWISE_SCALAR_OPERATIONS(U32x4, uint32_t, 4)
		BITWISE_SCALAR_OPERATIONS(U32x8, uint32_t, 8)
	#undef BITWISE_SCALAR_OPERATIONS

	// Cleaning up temporary macro definitions to avoid cluttering the namespace.
	#undef FOR_ALL_VECTOR_TYPES
	#undef FOR_FLOAT_VECTOR_TYPES
	#undef FOR_INTEGER_VECTOR_TYPES
	#undef FOR_SIGNED_VECTOR_TYPES
	#undef FOR_UNSIGNED_VECTOR_TYPES

	// Variable length SIMD abstraction
	//   DSR_DEFAULT_ALIGNMENT
	//     The number of bytes memory should be aligned with by default when creating buffers and images.
	//     Also the size of the longest available SIMD vector.
	//     Defined as a macro in case that an immediate integer is required.
	//   F32xX
	//     The longest available SIMD vector for storing 32-bit float values.
	//   I32xX
	//     The longest available SIMD vector for storing signed 32-bit integer values.
	//   U32xX
	//     The longest available SIMD vector for storing unsigned 32-bit integer values.
	//   U16xX
	//     The longest available SIMD vector for storing unsigned 16-bit integer values.
	//   U8xX
	//     The longest available SIMD vector for storing unsigned 8-bit integer values.
	#if defined USE_256BIT_SIMD || defined EMULATE_256BIT_SIMD
		// Using 256-bit SIMD
		#define DSR_DEFAULT_VECTOR_SIZE 32
		using F32xX = F32x8; // Longest available SIMD vector of 32-bit floats.
		using I32xX = I32x8; // Longest available SIMD vector of signed 32-bit integers.
		using U32xX = U32x8; // Longest available SIMD vector of unsigned 32-bit integers.
		using U16xX = U16x16; // Longest available SIMD vector of unsigned 16-bit integers.
		using U8xX = U8x32; // Longest available SIMD vector of unsigned 8-bit integers.
		// Align memory with 256 bits to allow overwriting padding at the end of each pixel row.
		//   Otherwise you would have to preserve data at the end of each row with slow and bloated duplicated code in every filter.
		#define DSR_DEFAULT_ALIGNMENT 32
	#else
		// If there is no hardware support for 256-bit vectors, the emulation of 256-bit vectors when used explicitly, is allowed to be aligned with just 128 bits.
		#define DSR_DEFAULT_VECTOR_SIZE 16
		using F32xX = F32x4; // Longest available SIMD vector of 32-bit floats.
		using I32xX = I32x4; // Longest available SIMD vector of signed 32-bit integers.
		using U32xX = U32x4; // Longest available SIMD vector of unsigned 32-bit integers.
		using U16xX = U16x8; // Longest available SIMD vector of unsigned 16-bit integers.
		using U8xX = U8x16; // Longest available SIMD vector of unsigned 8-bit integers.
		// TODO: Should AVX without AVX2 increase memory alignment to 256 bits to allow manual AVX optimizations?
		//       Having to skip blocks of memory when alignment is larger than default vector size could lead to poor performance from cache misses.
		#define DSR_DEFAULT_ALIGNMENT 16
	#endif
	// In case that we want to exploit partial type support for a vector length in the future, alignment and vector length should have two separate names.
	//   This might also be useful for overriding memory alignment in the framework for a manual vectorizations.
	// How many lanes do the longest available vector have for a specified lane size.
	//   Used to iterate indices and pointers using whole elements.
	static const int laneCountX_32Bit = DSR_DEFAULT_VECTOR_SIZE / 4;
	static const int laneCountX_16Bit = DSR_DEFAULT_VECTOR_SIZE / 2;
	static const int laneCountX_8Bit = DSR_DEFAULT_VECTOR_SIZE;
#endif
