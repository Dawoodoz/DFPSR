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

// Hardware abstraction layer for portable SIMD math.
//   Used to make calculations faster without having to mess around with any hardware specific assembler code nor intrinsic functions.
//   You get the performance you need today and the ability to compile with automatic scalar emulation when building for a future processor that this module has not yet been ported to.
//   When you can generate the vectorized code using the same template function as your non-vectorized code, you don't even need to write a reference implementation.

// Using with 128-bit SIMD: (beginner friendly, test once, compile anywhere, no compiler flags)
//   If you are new to vectorization or only plan to work with 128-bit extensions such as ARM NEON, you can keep it simple by only using the 128-bit vector types.
//   Pros and cons:
//     + Most target platforms (excluding older systems such as ARMv6) have 128-bit SIMD extensions such as Intel SSE2 or ARM NEON enabled by default.
//       ARMv6 does not support ARM NEON, but most ARMv7 processors support it, so that compilers enable NEON by default.
//       All 64-bit ARM processors have ARM NEON, because it stopped being optional in ARMv8.
//       Building for 64-bit Intel processors usually have SSE2 enabled by default, so you don't have to change any compiler flags when building on a different system.
//     + One build for all computers of the same instruction set.
//       Great when your application is not so resource heavy, because the least powerful systems don't have the fancy extensions anyway.
//     - You might end up enabling the additional SIMD extensions anyway because the library is already using it to become faster.
//   Types:
// 	   * Use F32x4, I32x4 and U32x4 for 4 elements at a time
//	   * U16x8 for 8 elements at a time
//	   * U8x16 for 16 elements at a time

// Using the X vector size: (advanced, having to test with different build flags or emulation)
//   If you want more performance, you can use variable length type aliases.
//   Pros and cons:
//     + For heavy calculations where memory access is not the bottleneck, using larger SIMD vectors when enabled allow saving energy and increasing performance.
//     - If you forget to test with longer vector lengths (compiling with -mavx2 or -mEMULATE_256BIT_SIMD) then you might find bugs from not iterating or aligning memory correctly.
//   Types:
// 	   * Use F32xX, I32xX and U32xX for laneCountX_32Bit elements at a time
//	   * U16xX for laneCountX_16Bit elements at a time
//	   * U8xX for laneCountX_8Bit elements at a time

// Using the F vector size: (very dangerous, no test can confirm that memory alignment is correct)
//   If you want even more performance, you can let float operations use the longest available F vector size, which might exceed the X vector size.
//   Pros and cons:
//     - Have to manually set the alignment of buffers to DSR_FLOAT_ALIGNMENT to prevent crashing.
//       If the default alignment for buffers changed based on the size of F vectors, the more commonly used X vector would get slowed down from cache misses from padding larger than X vectors.
//       AlignedImageF32 and sound backends are already aligned with the F vector size, because they are not generic like Buffer.
//     - It can be difficult to detect incorrect memory alignment, because a pointer can accidentally be aligned to more than what was requested.
//       If accidentally aligning to 128 bits instead of 256 bits, there is a 50% risk of failing to detect it at runtime and later fail on another computer.
//       If sticking with 128-bit or X vectors, all buffers will be correctly aligned automatically.
//     + For heavy calculations where memory access is not the bottleneck, using larger SIMD vectors when enabled allow saving energy and increasing performance.
//     - If you forget to test with longer vector lengths (compiling with -mavx2 or -mEMULATE_256BIT_SIMD) then you might find bugs from not iterating or aligning memory correctly.
//   Types:
// 	   * Use F32xX, I32xX and U32xX for laneCountX_32Bit elements at a time

// Compiler extensions
//   On Intel/AMD processors:
//     SSE2 is usually enabled by default, because SSE2 is mandatory for 64-bit Intel instructions.
//     Use -mavx as a G++ compiler flag to enable the AVX extension, enabling the USE_AVX and USE_256BIT_F_SIMD macros.
//       If not available on your computer, you can test your algorithm for 256-bit float SIMD using EMULATE_256BIT_F_SIMD, but only if you use the F vector size.
//     Use -mavx2 as a G++ compiler flag to enable the AVX2 extension, enabling the USE_AVX2 and USE_256BIT_X_SIMD macros.
//       If not available on your computer, you can test your algorithm for 256-bit float and integer SIMD using EMULATE_256BIT_X_SIMD, but only if you use the X vector size.
//   On ARMv6 processors:
//     Scalar emulation is used when compiling for ARMv6, because it does not have NEON and VFP is not supported in this abstraction.
//   On ARMv7 processors:
//     NEON is usually enabled by default for ARMv7, because most of them have the extension.
//   On ARMv8 processors:
//     NEON can not be disabled for ARMv8, because it is mandatory for ARMv8.

// If getting crashes:
// * Disable compiler optimizations and inspect generated assembler code.
//   To see how the variables would be stored in the stack when running out of registers.
//   Otherwise you have to wait until you run out of registers before noticing that a variable was incorrectly aligned.
// * Make sure that the compiler did not automatically generate any non-aligned temporary variables of the __m256 or __m256i types.
//   The Intel ABI strictly requires that 256-bit SIMD vectors are always aligned by 32 bytes. Not doing so will cause crashes on some processor models.
//   The g++ compiler does not treat __m256 nor __m256i as strictly aligned by 32 bytes and sais that it is the developer's responsibility to align the memory according to Intel's ABI.
//   But when you do align all variables explicitly to 32 bytes, g++ inserts unaligned temporary variables that cause crashes anyway.
// * Instead of nesting calls to intrinsic functions, separate them into one statement per call and explicitly align all inputs and outputs.
// * When making a wrapper function around intrinsic AVX2 functions, use aligned wrapper types for both input and output, so that generated temporary variables are explicitly aligned.
//   If you must have inputs or outputs with __m256 or __m256i types, pass by reference and align with 32 bytes at the caller.
// * Check which arguments are required to be immediate constants and either hardcode or pass through a template argument.
//   The expression 5 + 5 will not becomes an immediate constant when optimization is disabled, which may cause a crash if passing the expression as an immediate constant.
//   Sometimes you need to turn optimization off for debugging, so it is good if turning optimizations off does not cause the program to crash.

#ifndef DFPSR_SIMD
#define DFPSR_SIMD
	#include <cstdint>
	#include <cassert>
	#include "SafePointer.h"
	#include "../math/FVector.h"
	#include "../math/IVector.h"
	#include "../math/UVector.h"
	#include "DsrTraits.h"
	#include "../settings.h"
	#include "../base/noSimd.h"
	#include "../api/stringAPI.h"

	#ifdef USE_SSE2
		#include <emmintrin.h> // SSE2
		#ifdef USE_SSSE3
			#include <tmmintrin.h> // SSSE3
		#endif
		#ifdef USE_AVX
			#include <immintrin.h> // AVX / AVX2
		#endif
	#endif
	#ifdef USE_NEON
		#include <arm_neon.h> // NEON
	#endif

	namespace dsr {

	// Alignment in bytes
	#define ALIGN_BYTES(SIZE) alignas(SIZE)
	#define ALIGN16 ALIGN_BYTES(16) // 128-bit alignment
	#define ALIGN32 ALIGN_BYTES(32) // 256-bit alignment
	#define ALIGN64 ALIGN_BYTES(64) // 512-bit alignment
	#define ALIGN128 ALIGN_BYTES(128) // 1024-bit alignment
	#define ALIGN256 ALIGN_BYTES(256) // 2048-bit alignment

	// Everything declared in here handles things specific for SSE.
	// Direct use of the macros will not provide portability to all hardware.
	#ifdef USE_SSE2
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

		#ifdef USE_AVX
			// 256-bit vector types
			#define SIMD_F32x8 __m256

			// Vector float operations returning SIMD_F32x4
			#define ADD_F32_SIMD256(A, B) _mm256_add_ps(A, B)
			#define SUB_F32_SIMD256(A, B) _mm256_sub_ps(A, B)
			#define MUL_F32_SIMD256(A, B) _mm256_mul_ps(A, B)

			// Statistics
			#define MIN_F32_SIMD256(A, B) _mm256_min_ps(A, B)
			#define MAX_F32_SIMD256(A, B) _mm256_max_ps(A, B)

			#ifdef USE_AVX2
				// 256-bit vector types
				#define SIMD_U8x32 __m256i
				#define SIMD_U16x16 __m256i
				#define SIMD_U32x8 __m256i
				#define SIMD_I32x8 __m256i

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

				// Reinterpret casting
				#define REINTERPRET_U32_TO_U8_SIMD256(A) (A)
				#define REINTERPRET_U32_TO_U16_SIMD256(A) (A)
				#define REINTERPRET_U8_TO_U32_SIMD256(A) (A)
				#define REINTERPRET_U16_TO_U32_SIMD256(A) (A)
				#define REINTERPRET_U32_TO_I32_SIMD256(A) (A)
				#define REINTERPRET_I32_TO_U32_SIMD256(A) (A)

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

				// Bitwise
				#define BITWISE_AND_U32_SIMD256(A, B) _mm256_and_si256(A, B)
				#define BITWISE_OR_U32_SIMD256(A, B) _mm256_or_si256(A, B)
				#define BITWISE_XOR_U32_SIMD256(A, B) _mm256_xor_si256(A, B)
			#endif
		#endif
	#endif

	// Everything declared in here handles things specific for NEON.
	// Direct use of the macros will not provide portability to all hardware.
	#ifdef USE_NEON
		// Vector types
		#define SIMD_F32x4 float32x4_t
		#define SIMD_U8x16 uint8x16_t
		#define SIMD_U16x8 uint16x8_t
		#define SIMD_U32x4 uint32x4_t
		#define SIMD_I32x4 int32x4_t

		// Vector uploads in address order
		inline SIMD_F32x4 LOAD_VECTOR_F32_SIMD(float a, float b, float c, float d) {
			ALIGN16 float data[4] = {a, b, c, d};
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t((void*)data) & 15u) { throwError(U"Unaligned stack memory detected in LOAD_VECTOR_F32_SIMD for NEON!\n"); }
			#endif
			return vld1q_f32(data);
		}
		inline SIMD_F32x4 LOAD_SCALAR_F32_SIMD(float a) {
			return vdupq_n_f32(a);
		}
		inline SIMD_U8x16 LOAD_VECTOR_U8_SIMD(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f, uint8_t g, uint8_t h,
		                                      uint8_t i, uint8_t j, uint8_t k, uint8_t l, uint8_t m, uint8_t n, uint8_t o, uint8_t p) {
			ALIGN16 uint8_t data[16] = {a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p};
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t((void*)data) & 15u) { throwError(U"Unaligned stack memory detected in LOAD_VECTOR_U8_SIMD for NEON!\n"); }
			#endif
			return vld1q_u8(data);
		}
		inline SIMD_U8x16 LOAD_SCALAR_U8_SIMD(uint16_t a) {
			return vdupq_n_u8(a);
		}
		inline SIMD_U16x8 LOAD_VECTOR_U16_SIMD(uint16_t a, uint16_t b, uint16_t c, uint16_t d, uint16_t e, uint16_t f, uint16_t g, uint16_t h) {
			ALIGN16 uint16_t data[8] = {a, b, c, d, e, f, g, h};
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t((void*)data) & 15u) { throwError(U"Unaligned stack memory detected in LOAD_VECTOR_U16_SIMD for NEON!\n"); }
			#endif
			return vld1q_u16(data);
		}
		inline SIMD_U16x8 LOAD_SCALAR_U16_SIMD(uint16_t a) {
			return vdupq_n_u16(a);
		}
		inline SIMD_U32x4 LOAD_VECTOR_U32_SIMD(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
			ALIGN16 uint32_t data[4] = {a, b, c, d};
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t((void*)data) & 15u) { throwError(U"Unaligned stack memory detected in LOAD_VECTOR_U32_SIMD for NEON!\n"); }
			#endif
			return vld1q_u32(data);
		}
		inline SIMD_U32x4 LOAD_SCALAR_U32_SIMD(uint32_t a) {
			return vdupq_n_u32(a);
		}
		inline SIMD_I32x4 LOAD_VECTOR_I32_SIMD(int32_t a, int32_t b, int32_t c, int32_t d) {
			ALIGN16 int32_t data[4] = {a, b, c, d};
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t((void*)data) & 15u) { throwError(U"Unaligned stack memory detected in LOAD_VECTOR_I32_SIMD for NEON!\n"); }
			#endif
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
		#define SUB_SAT_U8_SIMD(A, B) vqsubq_u8(A, B) // Saturated subtraction
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
			So that any new variations of the macro named given from the compiler can be added to simd.h instead of duplicated everywhere.
	Portability exceptions:
		* The "v" variable is the native backend, which is only defined when SIMD is supported by hardware.
			Only use when USE_BASIC_SIMD is defined.
			Will not work on scalar emulation.
		* The "scalars" array is available when emulating a type that does not exist or the SIMD vector has direct access to the memory.
			Do not rely on these for accessing elements, because otherwise your code will not be able to compile for ARM NEON.
	*/

	struct ALIGN16 F32x4 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			F32x4() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline F32x4 create_dangerous_uninitialized() { return F32x4(); }
		#ifdef USE_BASIC_SIMD
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_F32x4 v;
			// Construct a portable vector from a native SIMD vector
			explicit F32x4(const SIMD_F32x4& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			F32x4(float a1, float a2, float a3, float a4) : v(LOAD_VECTOR_F32_SIMD(a1, a2, a3, a4)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit F32x4(float scalar) : v(LOAD_SCALAR_F32_SIMD(scalar)) {}
			// Copy constructor.
			F32x4(const F32x4& other) {
				v = other.v;
			}
			// Assignment operator.
			F32x4& operator=(const F32x4& other) {
				if (this != &other) {
					v = other.v;
				}
				return *this;
			}
			// Move operator.
			F32x4& operator=(F32x4&& other) noexcept {
				v = other.v;
				return *this;
			}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			float scalars[4];
			// Construct a portable vector from a set of scalars
			F32x4(float a1, float a2, float a3, float a4) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit F32x4(float scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline F32x4 createGradient(float start, float increment) {
			return F32x4(start, start + increment, start + increment * 2.0f, start + increment * 3.0f);
		}
		// Construct a portable SIMD vector from a pointer to aligned data.
		// Data must be aligned with at least 16 bytes.
		static inline F32x4 readAlignedUnsafe(const float* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t((const void*)data) & 15u) { throwError(U"Unaligned pointer detected in F32x4::readAlignedUnsafe!\n"); }
			#endif
			#ifdef USE_BASIC_SIMD
				#if defined(USE_SSE2)
					ALIGN16 SIMD_F32x4 result = _mm_load_ps(data);
					return F32x4(result);
				#elif defined(USE_NEON)
					ALIGN16 SIMD_F32x4 result = vld1q_f32(data);
					return F32x4(result);
				#endif
			#else
				return F32x4(data[0], data[1], data[2], data[3]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with 32 bytes.
		inline void writeAlignedUnsafe(float* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t((void*)data) & 15u) { throwError(U"Unaligned pionter detected in F32x4::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					_mm_store_ps(data, this->v);
				#elif defined(USE_NEON)
					vst1q_f32(data, this->v);
				#endif
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
			#endif
		}
		#if defined(DFPSR_GEOMETRY_FVECTOR)
			dsr::FVector4D get() const {
				ALIGN16 float data[4];
				#ifdef SAFE_POINTER_CHECKS
					if (uintptr_t(data) & 15u) { throwError(U"Unaligned stack memory detected in FVector4D F32x4::get!\n"); }
				#endif
				this->writeAlignedUnsafe(data);
				return dsr::FVector4D(data[0], data[1], data[2], data[3]);
			}
		#endif
		// Bound and alignment checked reading
		static inline F32x4 readAligned(dsr::SafePointer<const float> data, const char* methodName) {
			const float* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			return F32x4::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<float> data, const char* methodName) const {
			float* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN16 I32x4 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			I32x4() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline I32x4 create_dangerous_uninitialized() { return I32x4(); }
		#if defined(USE_BASIC_SIMD)
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_I32x4 v;
			// Construct a portable vector from a native SIMD vector
			explicit I32x4(const SIMD_I32x4& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			I32x4(int32_t a1, int32_t a2, int32_t a3, int32_t a4) : v(LOAD_VECTOR_I32_SIMD(a1, a2, a3, a4)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit I32x4(int32_t scalar) : v(LOAD_SCALAR_I32_SIMD(scalar)) {}
			// Copy constructor.
			I32x4(const I32x4& other) {
				v = other.v;
			}
			// Assignment operator.
			I32x4& operator=(const I32x4& other) {
				if (this != &other) {
					v = other.v;
				}
				return *this;
			}
			// Move operator.
			I32x4& operator=(I32x4&& other) noexcept {
				v = other.v;
				return *this;
			}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			int32_t scalars[4];
			// Construct a portable vector from a set of scalars
			I32x4(int32_t a1, int32_t a2, int32_t a3, int32_t a4) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit I32x4(int32_t scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline I32x4 createGradient(int32_t start, int32_t increment) {
			return I32x4(start, start + increment, start + increment * 2, start + increment * 3);
		}
		// Construct a portable SIMD vector from a pointer to aligned data.
		// Data must be aligned with at least 16 bytes.
		static inline I32x4 readAlignedUnsafe(const int32_t* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 15u) { throwError(U"Unaligned pointer detected in I32x4::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					ALIGN16 SIMD_I32x4 result = _mm_load_si128((const __m128i*)data);
					return I32x4(result);
				#elif defined(USE_NEON)
					ALIGN16 SIMD_I32x4 result = vld1q_s32(data);
					return I32x4(result);
				#endif
			#else
				return I32x4(data[0], data[1], data[2], data[3]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with 32 bytes.
		inline void writeAlignedUnsafe(int32_t* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 15u) { throwError(U"Unaligned pointer detected in I32x4::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					_mm_store_si128((__m128i*)data, this->v);
				#elif defined(USE_NEON)
					vst1q_s32(data, this->v);
				#endif
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
			#endif
		}
		#if defined(DFPSR_GEOMETRY_IVECTOR)
			dsr::IVector4D get() const {
				ALIGN16 int32_t data[4];
				#ifdef SAFE_POINTER_CHECKS
					if (uintptr_t(data) & 15u) { throwError(U"Unaligned stack memory detected in IVector4D I32x4::get!\n"); }
				#endif
				this->writeAlignedUnsafe(data);
				return dsr::IVector4D(data[0], data[1], data[2], data[3]);
			}
		#endif
		// Bound and alignment checked reading
		static inline I32x4 readAligned(dsr::SafePointer<const int32_t> data, const char* methodName) {
			const int32_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			return I32x4::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<int32_t> data, const char* methodName) const {
			int32_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN16 U32x4 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			U32x4() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline U32x4 create_dangerous_uninitialized() { return U32x4(); }
		#if defined(USE_BASIC_SIMD)
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U32x4 v;
			// Construct a portable vector from a native SIMD vector
			explicit U32x4(const SIMD_U32x4& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			U32x4(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) : v(LOAD_VECTOR_U32_SIMD(a1, a2, a3, a4)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit U32x4(uint32_t scalar) : v(LOAD_SCALAR_U32_SIMD(scalar)) {}
			// Copy constructor.
			U32x4(const U32x4& other) {
				v = other.v;
			}
			// Assignment operator.
			U32x4& operator=(const U32x4& other) {
				if (this != &other) {
					v = other.v;
				}
				return *this;
			}
			// Move operator.
			U32x4& operator=(U32x4&& other) noexcept {
				v = other.v;
				return *this;
			}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			uint32_t scalars[4];
			// Construct a portable vector from a set of scalars
			U32x4(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U32x4(uint32_t scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline U32x4 createGradient(uint32_t start, uint32_t increment) {
			return U32x4(start, start + increment, start + increment * 2, start + increment * 3);
		}
		// Construct a portable SIMD vector from a pointer to aligned data
		// Data must be aligned with at least 16 bytes.
		static inline U32x4 readAlignedUnsafe(const uint32_t* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 15u) { throwError(U"Unaligned pointer detected in U32x4::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					ALIGN16 SIMD_I32x4 result = _mm_load_si128((const __m128i*)data);
					return U32x4(result);
				#elif defined(USE_NEON)
					ALIGN16 SIMD_I32x4 result = vld1q_u32(data);
					return U32x4(result);
				#endif
			#else
				return U32x4(data[0], data[1], data[2], data[3]);
			#endif
		}
		// Write to aligned memory from the existing vector
		// data must be aligned with 32 bytes.
		inline void writeAlignedUnsafe(uint32_t* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 15u) { throwError(U"Unaligned pointer detected in U32x4::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					_mm_store_si128((__m128i*)data, this->v);
				#elif defined(USE_NEON)
					vst1q_u32(data, this->v);
				#endif
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
			#endif
		}
		#if defined(DFPSR_GEOMETRY_UVECTOR)
			dsr::UVector4D get() const {
				ALIGN16 uint32_t data[4];
				#ifdef SAFE_POINTER_CHECKS
					if (uintptr_t(data) & 15u) { throwError(U"Unaligned stack memory detected in UVector4D U32x4::get!\n"); }
				#endif
				this->writeAlignedUnsafe(data);
				return dsr::UVector4D(data[0], data[1], data[2], data[3]);
			}
		#endif
		// Bound and alignment checked reading
		static inline U32x4 readAligned(dsr::SafePointer<const uint32_t> data, const char* methodName) {
			const uint32_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			return U32x4::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint32_t> data, const char* methodName) const {
			uint32_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN16 U16x8 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			U16x8() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline U16x8 create_dangerous_uninitialized() { return U16x8(); }
		#if defined(USE_BASIC_SIMD)
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U16x8 v;
			// Construct a portable vector from a native SIMD vector
			explicit U16x8(const SIMD_U16x8& v) : v(v) {}
			// Construct a portable vector from a set of scalars
			U16x8(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8) : v(LOAD_VECTOR_U16_SIMD(a1, a2, a3, a4, a5, a6, a7, a8)) {}
			// Construct a portable vector from a single duplicated scalar
			explicit U16x8(uint16_t scalar) : v(LOAD_SCALAR_U16_SIMD(scalar)) {}
			// Copy constructor.
			U16x8(const U16x8& other) {
				v = other.v;
			}
			// Assignment operator.
			U16x8& operator=(const U16x8& other) {
				if (this != &other) {
					v = other.v;
				}
				return *this;
			}
			// Move operator.
			U16x8& operator=(U16x8&& other) noexcept {
				v = other.v;
				return *this;
			}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			uint16_t scalars[8];
			// Construct a portable vector from a set of scalars
			U16x8(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
				this->scalars[4] = a5;
				this->scalars[5] = a6;
				this->scalars[6] = a7;
				this->scalars[7] = a8;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U16x8(uint16_t scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
				this->scalars[4] = scalar;
				this->scalars[5] = scalar;
				this->scalars[6] = scalar;
				this->scalars[7] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline U16x8 createGradient(uint16_t start, uint16_t increment) {
			return U16x8(
			  start,
			  start + increment,
			  start + increment * 2,
			  start + increment * 3,
			  start + increment * 4,
			  start + increment * 5,
			  start + increment * 6,
			  start + increment * 7
			);
		}
		static inline U16x8 readAlignedUnsafe(const uint16_t* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 15u) { throwError(U"Unaligned pointer detected in U16x8::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					ALIGN16 SIMD_I32x4 result = _mm_load_si128((const __m128i*)data);
					return U16x8(result);
				#elif defined(USE_NEON)
					ALIGN16 SIMD_I32x4 result = vld1q_u16(data);
					return U16x8(result);
				#endif
			#else
				return U16x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			#endif
		}
		// Data must be aligned with at least 16 bytes.
		inline void writeAlignedUnsafe(uint16_t* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 15u) { throwError(U"Unaligned pointer detected in U16x8::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					_mm_store_si128((__m128i*)data, this->v);
				#elif defined(USE_NEON)
					vst1q_u16(data, this->v);
				#endif
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
				data[4] = this->scalars[4];
				data[5] = this->scalars[5];
				data[6] = this->scalars[6];
				data[7] = this->scalars[7];
			#endif
		}
		// Bound and alignment checked reading
		static inline U16x8 readAligned(dsr::SafePointer<const uint16_t> data, const char* methodName) {
			const uint16_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			return U16x8::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint16_t> data, const char* methodName) const {
			uint16_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN16 U8x16 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			U8x16() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline U8x16 create_dangerous_uninitialized() { return U8x16(); }
		#if defined(USE_BASIC_SIMD)
			public:
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
			// Copy constructor.
			U8x16(const U8x16& other) {
				v = other.v;
			}
			// Assignment operator.
			U8x16& operator=(const U8x16& other) {
				if (this != &other) {
					v = other.v;
				}
				return *this;
			}
			// Move operator.
			U8x16& operator=(U8x16&& other) noexcept {
				v = other.v;
				return *this;
			}
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			uint8_t scalars[16];
			// Construct a portable vector from a set of scalars
			U8x16(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8,
			      uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12, uint8_t a13, uint8_t a14, uint8_t a15, uint8_t a16) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
				this->scalars[4] = a5;
				this->scalars[5] = a6;
				this->scalars[6] = a7;
				this->scalars[7] = a8;
				this->scalars[8] = a9;
				this->scalars[9] = a10;
				this->scalars[10] = a11;
				this->scalars[11] = a12;
				this->scalars[12] = a13;
				this->scalars[13] = a14;
				this->scalars[14] = a15;
				this->scalars[15] = a16;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U8x16(uint8_t scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
				this->scalars[4] = scalar;
				this->scalars[5] = scalar;
				this->scalars[6] = scalar;
				this->scalars[7] = scalar;
				this->scalars[8] = scalar;
				this->scalars[9] = scalar;
				this->scalars[10] = scalar;
				this->scalars[11] = scalar;
				this->scalars[12] = scalar;
				this->scalars[13] = scalar;
				this->scalars[14] = scalar;
				this->scalars[15] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline U8x16 createGradient(uint8_t start, uint8_t increment) {
			return U8x16(
			  start,
			  start + increment,
			  start + increment * 2,
			  start + increment * 3,
			  start + increment * 4,
			  start + increment * 5,
			  start + increment * 6,
			  start + increment * 7,
			  start + increment * 8,
			  start + increment * 9,
			  start + increment * 10,
			  start + increment * 11,
			  start + increment * 12,
			  start + increment * 13,
			  start + increment * 14,
			  start + increment * 15
			);
		}
		static inline U8x16 readAlignedUnsafe(const uint8_t* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 15u) { throwError(U"Unaligned pointer detected in U8x16::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					ALIGN16 SIMD_I32x4 result = _mm_load_si128((const __m128i*)data);
					return U8x16(result);
				#elif defined(USE_NEON)
					ALIGN16 SIMD_I32x4 result = vld1q_u8(data);
					return U8x16(result);
				#endif
			#else
				return U8x16(
				  data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
				  data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]
				);
			#endif
		}
		// Data must be aligned with at least 16 bytes.
		inline void writeAlignedUnsafe(uint8_t* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 15u) { throwError(U"Unaligned pointer detected in U8x16::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_BASIC_SIMD)
				#if defined(USE_SSE2)
					_mm_store_si128((__m128i*)data, this->v);
				#elif defined(USE_NEON)
					vst1q_u8(data, this->v);
				#endif
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
				data[4] = this->scalars[4];
				data[5] = this->scalars[5];
				data[6] = this->scalars[6];
				data[7] = this->scalars[7];
				data[8] = this->scalars[8];
				data[9] = this->scalars[9];
				data[10] = this->scalars[10];
				data[11] = this->scalars[11];
				data[12] = this->scalars[12];
				data[13] = this->scalars[13];
				data[14] = this->scalars[14];
				data[15] = this->scalars[15];
			#endif
		}
		// Bound and alignment checked reading
		static inline U8x16 readAligned(dsr::SafePointer<const uint8_t> data, const char* methodName) {
			const uint8_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			return U8x16::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint8_t> data, const char* methodName) const {
			uint8_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 16);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN32 F32x8 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			F32x8() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline F32x8 create_dangerous_uninitialized() { return F32x8(); }
		#if defined(USE_256BIT_F_SIMD)
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_F32x8 v;
			// Construct a portable vector from a native SIMD vector.
			explicit F32x8(const SIMD_F32x8& v) : v(v) {}
			#if defined(USE_AVX)
				// Construct a portable vector from a set of scalars.
				F32x8(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
					ALIGN32 __m256 target = _mm256_set_ps(a8, a7, a6, a5, a4, a3, a2, a1);
					this->v = target;
				}
				// Construct a portable vector from a single duplicated scalar.
				explicit F32x8(float scalar) {
					ALIGN32 __m256 target = _mm256_set1_ps(scalar);
					this->v = target;
				}
				// Copy constructor.
				F32x8(const F32x8& other) {
					v = other.v;
				}
				// Assignment operator.
				F32x8& operator=(const F32x8& other) {
					if (this != &other) {
						v = other.v;
					}
					return *this;
				}
				// Move operator.
				F32x8& operator=(F32x8&& other) noexcept {
					v = other.v;
					return *this;
				}
			#else
				#error "Missing constructors for the F32x8 type!\n"
			#endif
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			float scalars[8];
			// Construct a portable vector from a set of scalars
			F32x8(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
				this->scalars[4] = a5;
				this->scalars[5] = a6;
				this->scalars[6] = a7;
				this->scalars[7] = a8;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit F32x8(float scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
				this->scalars[4] = scalar;
				this->scalars[5] = scalar;
				this->scalars[6] = scalar;
				this->scalars[7] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline F32x8 createGradient(float start, float increment) {
			return F32x8(
			  start,
			  start + increment,
			  start + increment * 2.0f,
			  start + increment * 3.0f,
			  start + increment * 4.0f,
			  start + increment * 5.0f,
			  start + increment * 6.0f,
			  start + increment * 7.0f
			);
		}
		// Construct a portable SIMD vector from a pointer to aligned data.
		static inline F32x8 readAlignedUnsafe(const float* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in F32x8::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				ALIGN32 __m256 result = _mm256_load_ps(data);
				return F32x8(result);
			#else
				return F32x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			#endif
		}
		// Write to aligned memory from the existing vector.
		inline void writeAlignedUnsafe(float* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in F32x8::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				_mm256_store_ps(data, this->v);
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
				data[4] = this->scalars[4];
				data[5] = this->scalars[5];
				data[6] = this->scalars[6];
				data[7] = this->scalars[7];
			#endif
		}
		// Bound and alignment checked reading
		static inline F32x8 readAligned(dsr::SafePointer<const float> data, const char* methodName) {
			const float* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			return F32x8::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<float> data, const char* methodName) const {
			float* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN32 I32x8 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			I32x8() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline I32x8 create_dangerous_uninitialized() { return I32x8(); }
		#if defined(USE_256BIT_X_SIMD)
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_I32x8 v;
			// Construct a portable vector from a native SIMD vector.
			explicit I32x8(const SIMD_I32x8& v) : v(v) {}
			#if defined(USE_AVX2)
				// Construct a portable vector from a set of scalars.
				I32x8(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8) {
					ALIGN32 __m256i target = _mm256_set_epi32(a8, a7, a6, a5, a4, a3, a2, a1);
					this->v = target;
				}
				// Construct a portable vector from a single duplicated scalar.
				explicit I32x8(int32_t scalar) {
					ALIGN32 __m256i target = _mm256_set1_epi32(scalar);
					this->v = target;
				}
				// Copy constructor.
				I32x8(const I32x8& other) {
					v = other.v;
				}
				// Assignment operator.
				I32x8& operator=(const I32x8& other) {
					if (this != &other) {
						v = other.v;
					}
					return *this;
				}
				// Move operator.
				I32x8& operator=(I32x8&& other) noexcept {
					v = other.v;
					return *this;
				}
			#else
				#error "Missing constructors for the I32x8 type!\n"
			#endif
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			int32_t scalars[8];
			// Construct a portable vector from a set of scalars
			I32x8(int32_t a1, int32_t a2, int32_t a3, int32_t a4, int32_t a5, int32_t a6, int32_t a7, int32_t a8) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
				this->scalars[4] = a5;
				this->scalars[5] = a6;
				this->scalars[6] = a7;
				this->scalars[7] = a8;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit I32x8(int32_t scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
				this->scalars[4] = scalar;
				this->scalars[5] = scalar;
				this->scalars[6] = scalar;
				this->scalars[7] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline I32x8 createGradient(int32_t start, int32_t increment) {
			return I32x8(
			  start,
			  start + increment,
			  start + increment * 2,
			  start + increment * 3,
			  start + increment * 4,
			  start + increment * 5,
			  start + increment * 6,
			  start + increment * 7
			);
		}
		// Construct a portable SIMD vector from a pointer to aligned data.
		static inline I32x8 readAlignedUnsafe(const int32_t* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in I32x8::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				ALIGN32 __m256i result = _mm256_load_si256((const __m256i*)data);
				return I32x8(result);
			#else
				return I32x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			#endif
		}
		// Write to aligned memory from the existing vector.
		inline void writeAlignedUnsafe(int32_t* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in I32x8::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				_mm256_store_si256((__m256i*)data, this->v);
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
				data[4] = this->scalars[4];
				data[5] = this->scalars[5];
				data[6] = this->scalars[6];
				data[7] = this->scalars[7];
			#endif
		}
		// Bound and alignment checked reading
		static inline I32x8 readAligned(dsr::SafePointer<const int32_t> data, const char* methodName) {
			const int32_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			return I32x8::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<int32_t> data, const char* methodName) const {
			int32_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN32 U32x8 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			U32x8() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline U32x8 create_dangerous_uninitialized() { return U32x8(); }
		#if defined(USE_256BIT_X_SIMD)
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U32x8 v;
			// Construct a portable vector from a native SIMD vector.
			explicit U32x8(const SIMD_U32x8& v) : v(v) {}
			#if defined(USE_AVX2)
				// Construct a portable vector from a set of scalars.
				U32x8(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7, uint32_t a8) {
					ALIGN32 __m256i target = _mm256_set_epi32(a8, a7, a6, a5, a4, a3, a2, a1);
					this->v = target;
				}
				// Construct a portable vector from a single duplicated scalar.
				explicit U32x8(uint32_t scalar) {
					ALIGN32 __m256i target = _mm256_set1_epi32(scalar);
					this->v = target;
				}
				// Copy constructor.
				U32x8(const U32x8& other) {
					v = other.v;
				}
				// Assignment operator.
				U32x8& operator=(const U32x8& other) {
					if (this != &other) {
						v = other.v;
					}
					return *this;
				}
				// Move operator.
				U32x8& operator=(U32x8&& other) noexcept {
					v = other.v;
					return *this;
				}
			#else
				#error "Missing constructors for the U32x8 type!\n"
			#endif
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			uint32_t scalars[8];
			// Construct a portable vector from a set of scalars
			U32x8(uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5, uint32_t a6, uint32_t a7, uint32_t a8) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
				this->scalars[4] = a5;
				this->scalars[5] = a6;
				this->scalars[6] = a7;
				this->scalars[7] = a8;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U32x8(uint32_t scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
				this->scalars[4] = scalar;
				this->scalars[5] = scalar;
				this->scalars[6] = scalar;
				this->scalars[7] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline U32x8 createGradient(uint32_t start, uint32_t increment) {
			return U32x8(
			  start,
			  start + increment,
			  start + increment * 2,
			  start + increment * 3,
			  start + increment * 4,
			  start + increment * 5,
			  start + increment * 6,
			  start + increment * 7
			);
		}
		// Construct a portable SIMD vector from a pointer to aligned data.
		// Data must be aligned with at least 32 bytes.
		static inline U32x8 readAlignedUnsafe(const uint32_t* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in U32x8::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				ALIGN32 __m256i result = _mm256_load_si256((const __m256i*)data);
				return U32x8(result);
			#else
				return U32x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			#endif
		}
		// Write to aligned memory from the existing vector.
		// data must be aligned with 32 bytes.
		inline void writeAlignedUnsafe(uint32_t* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in U32x8::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				_mm256_store_si256((__m256i*)data, this->v);
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
				data[4] = this->scalars[4];
				data[5] = this->scalars[5];
				data[6] = this->scalars[6];
				data[7] = this->scalars[7];
			#endif
		}
		// Bound and alignment checked reading
		static inline U32x8 readAligned(dsr::SafePointer<const uint32_t> data, const char* methodName) {
			const uint32_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			return U32x8::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint32_t> data, const char* methodName) const {
			uint32_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN32 U16x16 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			U16x16() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline U16x16 create_dangerous_uninitialized() { return U16x16(); }
		#if defined(USE_256BIT_X_SIMD)
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U16x16 v;
			// Construct a portable vector from a native SIMD vector
			explicit U16x16(const SIMD_U16x16& v) : v(v) {}
			#if defined(USE_AVX2)
				// Construct a portable vector from a set of scalars.
				U16x16(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8,
				  uint16_t a9, uint16_t a10, uint16_t a11, uint16_t a12, uint16_t a13, uint16_t a14, uint16_t a15, uint16_t a16) {
					ALIGN32 __m256i target = _mm256_set_epi16(a16, a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1);
					this->v = target;
				}
				// Construct a portable vector from a single duplicated scalar.
				explicit U16x16(uint16_t scalar) {
					ALIGN32 __m256i target = _mm256_set1_epi16(scalar);
					this->v = target;
				}
				// Copy constructor.
				U16x16(const U16x16& other) {
					v = other.v;
				}
				// Assignment operator.
				U16x16& operator=(const U16x16& other) {
					if (this != &other) {
						v = other.v;
					}
					return *this;
				}
				// Move operator.
				U16x16& operator=(U16x16&& other) noexcept {
					v = other.v;
					return *this;
				}
			#else
				#error "Missing constructors for the U16x16 type!\n"
			#endif
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			uint16_t scalars[16];
			// Construct a portable vector from a set of scalars
			U16x16(uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5, uint16_t a6, uint16_t a7, uint16_t a8,
			       uint16_t a9, uint16_t a10, uint16_t a11, uint16_t a12, uint16_t a13, uint16_t a14, uint16_t a15, uint16_t a16) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
				this->scalars[4] = a5;
				this->scalars[5] = a6;
				this->scalars[6] = a7;
				this->scalars[7] = a8;
				this->scalars[8] = a9;
				this->scalars[9] = a10;
				this->scalars[10] = a11;
				this->scalars[11] = a12;
				this->scalars[12] = a13;
				this->scalars[13] = a14;
				this->scalars[14] = a15;
				this->scalars[15] = a16;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U16x16(uint16_t scalar) {
				this->scalars[0] = scalar;
				this->scalars[1] = scalar;
				this->scalars[2] = scalar;
				this->scalars[3] = scalar;
				this->scalars[4] = scalar;
				this->scalars[5] = scalar;
				this->scalars[6] = scalar;
				this->scalars[7] = scalar;
				this->scalars[8] = scalar;
				this->scalars[9] = scalar;
				this->scalars[10] = scalar;
				this->scalars[11] = scalar;
				this->scalars[12] = scalar;
				this->scalars[13] = scalar;
				this->scalars[14] = scalar;
				this->scalars[15] = scalar;
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline U16x16 createGradient(uint16_t start, uint16_t increment) {
			return U16x16(
			  start,
			  start + increment,
			  start + increment * 2,
			  start + increment * 3,
			  start + increment * 4,
			  start + increment * 5,
			  start + increment * 6,
			  start + increment * 7,
			  start + increment * 8,
			  start + increment * 9,
			  start + increment * 10,
			  start + increment * 11,
			  start + increment * 12,
			  start + increment * 13,
			  start + increment * 14,
			  start + increment * 15
			);
		}
		// Data must be aligned with at least 32 bytes.
		static inline U16x16 readAlignedUnsafe(const uint16_t* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in U16x16::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				ALIGN32 __m256i result = _mm256_load_si256((const __m256i*)data);
				return U16x16(result);
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
		// Data must be aligned with at least 32 bytes.
		inline void writeAlignedUnsafe(uint16_t* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in U16x16::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				_mm256_store_si256((__m256i*)data, this->v);
			#else
				data[0] = this->scalars[0];
				data[1] = this->scalars[1];
				data[2] = this->scalars[2];
				data[3] = this->scalars[3];
				data[4] = this->scalars[4];
				data[5] = this->scalars[5];
				data[6] = this->scalars[6];
				data[7] = this->scalars[7];
				data[8] = this->scalars[8];
				data[9] = this->scalars[9];
				data[10] = this->scalars[10];
				data[11] = this->scalars[11];
				data[12] = this->scalars[12];
				data[13] = this->scalars[13];
				data[14] = this->scalars[14];
				data[15] = this->scalars[15];
			#endif
		}
		// Bound and alignment checked reading
		static inline U16x16 readAligned(dsr::SafePointer<const uint16_t> data, const char* methodName) {
			const uint16_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			return U16x16::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint16_t> data, const char* methodName) const {
			uint16_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	struct ALIGN32 U8x32 {
		private:
			// The uninitialized default constructor is private for safety reasons.
			U8x32() {}
		public:
			// When the uninitialized constructor is needed for performance, use this named constructor instead.
			static inline U8x32 create_dangerous_uninitialized() { return U8x32(); }
		#if defined(USE_256BIT_X_SIMD)
			public:
			// The SIMD vector of undefined type
			// Not accessible while emulating!
			SIMD_U8x32 v;
			// Construct a portable vector from a native SIMD vector
			explicit U8x32(const SIMD_U8x32& v) : v(v) {}
			#if defined(USE_AVX2)
				// Construct a portable vector from a set of scalars.
				U8x32(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8,
				  uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12, uint8_t a13, uint8_t a14, uint8_t a15, uint8_t a16,
				  uint8_t a17, uint8_t a18, uint8_t a19, uint8_t a20, uint8_t a21, uint8_t a22, uint8_t a23, uint8_t a24,
				  uint8_t a25, uint8_t a26, uint8_t a27, uint8_t a28, uint8_t a29, uint8_t a30, uint8_t a31, uint8_t a32) {
					ALIGN32 __m256i target = _mm256_set_epi8(a32, a31, a30, a29, a28, a27, a26, a25, a24, a23, a22, a21, a20, a19, a18, a17, a16, a15, a14, a13, a12, a11, a10, a9, a8, a7, a6, a5, a4, a3, a2, a1);
					this->v = target;
				}
				// Construct a portable vector from a single duplicated scalar.
				explicit U8x32(uint8_t scalar) {
					ALIGN32 __m256i target = _mm256_set1_epi8(scalar);
					this->v = target;
				}
				// Copy constructor.
				U8x32(const U8x32& other) {
					v = other.v;
				}
				// Assignment operator.
				U8x32& operator=(const U8x32& other) {
					if (this != &other) {
						v = other.v;
					}
					return *this;
				}
				// Move operator.
				U8x32& operator=(U8x32&& other) noexcept {
					v = other.v;
					return *this;
				}
			#else
				#error "Missing constructors for the U8x32 type!\n"
			#endif
		#else
			public:
			// Emulate a SIMD vector as an array of scalars without hardware support.
			// Only accessible while emulating!
			uint8_t scalars[32];
			// Construct a portable vector from a set of scalars
			U8x32(uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6, uint8_t a7, uint8_t a8,
				  uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12, uint8_t a13, uint8_t a14, uint8_t a15, uint8_t a16,
				  uint8_t a17, uint8_t a18, uint8_t a19, uint8_t a20, uint8_t a21, uint8_t a22, uint8_t a23, uint8_t a24,
				  uint8_t a25, uint8_t a26, uint8_t a27, uint8_t a28, uint8_t a29, uint8_t a30, uint8_t a31, uint8_t a32) {
				this->scalars[0] = a1;
				this->scalars[1] = a2;
				this->scalars[2] = a3;
				this->scalars[3] = a4;
				this->scalars[4] = a5;
				this->scalars[5] = a6;
				this->scalars[6] = a7;
				this->scalars[7] = a8;
				this->scalars[8] = a9;
				this->scalars[9] = a10;
				this->scalars[10] = a11;
				this->scalars[11] = a12;
				this->scalars[12] = a13;
				this->scalars[13] = a14;
				this->scalars[14] = a15;
				this->scalars[15] = a16;
				this->scalars[16] = a17;
				this->scalars[17] = a18;
				this->scalars[18] = a19;
				this->scalars[19] = a20;
				this->scalars[20] = a21;
				this->scalars[21] = a22;
				this->scalars[22] = a23;
				this->scalars[23] = a24;
				this->scalars[24] = a25;
				this->scalars[25] = a26;
				this->scalars[26] = a27;
				this->scalars[27] = a28;
				this->scalars[28] = a29;
				this->scalars[29] = a30;
				this->scalars[30] = a31;
				this->scalars[31] = a32;
			}
			// Construct a portable vector from a single duplicated scalar
			explicit U8x32(uint8_t scalar) {
				for (int i = 0; i < 32; i++) {
					this->scalars[i] = scalar;
				}
			}
		#endif
		// Create a gradient vector using start and increment, so that arbitrary length vectors have a way to initialize linear iterations.
		static inline U8x32 createGradient(uint8_t start, uint8_t increment) {
			return U8x32(
			  start,
			  start + increment,
			  start + increment * 2,
			  start + increment * 3,
			  start + increment * 4,
			  start + increment * 5,
			  start + increment * 6,
			  start + increment * 7,
			  start + increment * 8,
			  start + increment * 9,
			  start + increment * 10,
			  start + increment * 11,
			  start + increment * 12,
			  start + increment * 13,
			  start + increment * 14,
			  start + increment * 15,
			  start + increment * 16,
			  start + increment * 17,
			  start + increment * 18,
			  start + increment * 19,
			  start + increment * 20,
			  start + increment * 21,
			  start + increment * 22,
			  start + increment * 23,
			  start + increment * 24,
			  start + increment * 25,
			  start + increment * 26,
			  start + increment * 27,
			  start + increment * 28,
			  start + increment * 29,
			  start + increment * 30,
			  start + increment * 31
			);
		}
		// Data must be aligned with at least 32 bytes.
		static inline U8x32 readAlignedUnsafe(const uint8_t* data) {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in U8x32::readAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				ALIGN32 __m256i result = _mm256_load_si256((const __m256i*)data);
				return U8x32(result);
			#else
				U8x32 result;
				for (int i = 0; i < 32; i++) {
					result.scalars[i] = data[i];
				}
				return result;
			#endif
		}
		// Data must be aligned with at least 32 bytes.
		inline void writeAlignedUnsafe(uint8_t* data) const {
			#ifdef SAFE_POINTER_CHECKS
				if (uintptr_t(data) & 31u) { throwError(U"Unaligned pointer detected in U8x32::writeAlignedUnsafe!\n"); }
			#endif
			#if defined(USE_AVX2)
				_mm256_store_si256((__m256i*)data, this->v);
			#else
				for (int i = 0; i < 32; i++) {
					data[i] = this->scalars[i];
				}
			#endif
		}
		// Bound and alignment checked reading
		static inline U8x32 readAligned(dsr::SafePointer<const uint8_t> data, const char* methodName) {
			const uint8_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			return U8x32::readAlignedUnsafe(pointer);
		}
		// Bound and alignment checked writing
		inline void writeAligned(dsr::SafePointer<uint8_t> data, const char* methodName) const {
			uint8_t* pointer = data.getUnsafe();
			#if defined(SAFE_POINTER_CHECKS)
				data.assertInside(methodName, pointer, 32);
			#endif
			this->writeAlignedUnsafe(pointer);
		}
	};

	#define IMPL_SCALAR_FALLBACK_START(A, B, VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE lanesA[LANE_COUNT]; \
		ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE lanesB[LANE_COUNT]; \
		A.writeAlignedUnsafe(&(lanesA[0])); \
		B.writeAlignedUnsafe(&(lanesB[0]));

	// Used for vector types that have SIMD registers but not the operation needed.
	#define IMPL_SCALAR_FALLBACK_INFIX_4_LANES(A, B, VECTOR_TYPE, ELEMENT_TYPE, OPERATION) { \
		IMPL_SCALAR_FALLBACK_START(A, B, VECTOR_TYPE, ELEMENT_TYPE, 4) \
		return VECTOR_TYPE( \
		  ELEMENT_TYPE(lanesA[ 0] OPERATION lanesB[ 0]), \
		  ELEMENT_TYPE(lanesA[ 1] OPERATION lanesB[ 1]), \
		  ELEMENT_TYPE(lanesA[ 2] OPERATION lanesB[ 2]), \
		  ELEMENT_TYPE(lanesA[ 3] OPERATION lanesB[ 3]) \
		); \
	}
	#define IMPL_SCALAR_FALLBACK_INFIX_8_LANES(A, B, VECTOR_TYPE, ELEMENT_TYPE, OPERATION) { \
		IMPL_SCALAR_FALLBACK_START(A, B, VECTOR_TYPE, ELEMENT_TYPE, 8) \
		return VECTOR_TYPE( \
		  ELEMENT_TYPE(lanesA[ 0] OPERATION lanesB[ 0]), \
		  ELEMENT_TYPE(lanesA[ 1] OPERATION lanesB[ 1]), \
		  ELEMENT_TYPE(lanesA[ 2] OPERATION lanesB[ 2]), \
		  ELEMENT_TYPE(lanesA[ 3] OPERATION lanesB[ 3]), \
		  ELEMENT_TYPE(lanesA[ 4] OPERATION lanesB[ 4]), \
		  ELEMENT_TYPE(lanesA[ 5] OPERATION lanesB[ 5]), \
		  ELEMENT_TYPE(lanesA[ 6] OPERATION lanesB[ 6]), \
		  ELEMENT_TYPE(lanesA[ 7] OPERATION lanesB[ 7]) \
		); \
	}
	#define IMPL_SCALAR_FALLBACK_INFIX_16_LANES(A, B, VECTOR_TYPE, ELEMENT_TYPE, OPERATION) { \
		IMPL_SCALAR_FALLBACK_START(A, B, VECTOR_TYPE, ELEMENT_TYPE, 16) \
		return VECTOR_TYPE( \
		  ELEMENT_TYPE(lanesA[ 0] OPERATION lanesB[ 0]), \
		  ELEMENT_TYPE(lanesA[ 1] OPERATION lanesB[ 1]), \
		  ELEMENT_TYPE(lanesA[ 2] OPERATION lanesB[ 2]), \
		  ELEMENT_TYPE(lanesA[ 3] OPERATION lanesB[ 3]), \
		  ELEMENT_TYPE(lanesA[ 4] OPERATION lanesB[ 4]), \
		  ELEMENT_TYPE(lanesA[ 5] OPERATION lanesB[ 5]), \
		  ELEMENT_TYPE(lanesA[ 6] OPERATION lanesB[ 6]), \
		  ELEMENT_TYPE(lanesA[ 7] OPERATION lanesB[ 7]), \
		  ELEMENT_TYPE(lanesA[ 8] OPERATION lanesB[ 8]), \
		  ELEMENT_TYPE(lanesA[ 9] OPERATION lanesB[ 9]), \
		  ELEMENT_TYPE(lanesA[10] OPERATION lanesB[10]), \
		  ELEMENT_TYPE(lanesA[11] OPERATION lanesB[11]), \
		  ELEMENT_TYPE(lanesA[12] OPERATION lanesB[12]), \
		  ELEMENT_TYPE(lanesA[13] OPERATION lanesB[13]), \
		  ELEMENT_TYPE(lanesA[14] OPERATION lanesB[14]), \
		  ELEMENT_TYPE(lanesA[15] OPERATION lanesB[15]) \
		); \
	}
	#define IMPL_SCALAR_FALLBACK_INFIX_32_LANES(A, B, VECTOR_TYPE, ELEMENT_TYPE, OPERATION) { \
		IMPL_SCALAR_FALLBACK_START(A, B, VECTOR_TYPE, ELEMENT_TYPE, 32) \
		return VECTOR_TYPE( \
		  ELEMENT_TYPE(lanesA[ 0] OPERATION lanesB[ 0]), \
		  ELEMENT_TYPE(lanesA[ 1] OPERATION lanesB[ 1]), \
		  ELEMENT_TYPE(lanesA[ 2] OPERATION lanesB[ 2]), \
		  ELEMENT_TYPE(lanesA[ 3] OPERATION lanesB[ 3]), \
		  ELEMENT_TYPE(lanesA[ 4] OPERATION lanesB[ 4]), \
		  ELEMENT_TYPE(lanesA[ 5] OPERATION lanesB[ 5]), \
		  ELEMENT_TYPE(lanesA[ 6] OPERATION lanesB[ 6]), \
		  ELEMENT_TYPE(lanesA[ 7] OPERATION lanesB[ 7]), \
		  ELEMENT_TYPE(lanesA[ 8] OPERATION lanesB[ 8]), \
		  ELEMENT_TYPE(lanesA[ 9] OPERATION lanesB[ 9]), \
		  ELEMENT_TYPE(lanesA[10] OPERATION lanesB[10]), \
		  ELEMENT_TYPE(lanesA[11] OPERATION lanesB[11]), \
		  ELEMENT_TYPE(lanesA[12] OPERATION lanesB[12]), \
		  ELEMENT_TYPE(lanesA[13] OPERATION lanesB[13]), \
		  ELEMENT_TYPE(lanesA[14] OPERATION lanesB[14]), \
		  ELEMENT_TYPE(lanesA[15] OPERATION lanesB[15]), \
		  ELEMENT_TYPE(lanesA[16] OPERATION lanesB[16]), \
		  ELEMENT_TYPE(lanesA[17] OPERATION lanesB[17]), \
		  ELEMENT_TYPE(lanesA[18] OPERATION lanesB[18]), \
		  ELEMENT_TYPE(lanesA[19] OPERATION lanesB[19]), \
		  ELEMENT_TYPE(lanesA[20] OPERATION lanesB[20]), \
		  ELEMENT_TYPE(lanesA[21] OPERATION lanesB[21]), \
		  ELEMENT_TYPE(lanesA[22] OPERATION lanesB[22]), \
		  ELEMENT_TYPE(lanesA[23] OPERATION lanesB[23]), \
		  ELEMENT_TYPE(lanesA[24] OPERATION lanesB[24]), \
		  ELEMENT_TYPE(lanesA[25] OPERATION lanesB[25]), \
		  ELEMENT_TYPE(lanesA[26] OPERATION lanesB[26]), \
		  ELEMENT_TYPE(lanesA[27] OPERATION lanesB[27]), \
		  ELEMENT_TYPE(lanesA[28] OPERATION lanesB[28]), \
		  ELEMENT_TYPE(lanesA[29] OPERATION lanesB[29]), \
		  ELEMENT_TYPE(lanesA[30] OPERATION lanesB[30]), \
		  ELEMENT_TYPE(lanesA[31] OPERATION lanesB[31]) \
		); \
	}

	// Used for vector types that do not have any supported SIMD register.
	#define IMPL_SCALAR_REFERENCE_INFIX_4_LANES(A, B, VECTOR_TYPE, ELEMENT_TYPE, OPERATION) \
	{ \
		VECTOR_TYPE impl_result = VECTOR_TYPE::create_dangerous_uninitialized(); \
		impl_result.scalars[ 0] = (A).scalars[ 0] OPERATION (B).scalars[ 0]; \
		impl_result.scalars[ 1] = (A).scalars[ 1] OPERATION (B).scalars[ 1]; \
		impl_result.scalars[ 2] = (A).scalars[ 2] OPERATION (B).scalars[ 2]; \
		impl_result.scalars[ 3] = (A).scalars[ 3] OPERATION (B).scalars[ 3]; \
		return impl_result; \
	}
	#define IMPL_SCALAR_REFERENCE_INFIX_8_LANES(A, B, VECTOR_TYPE, ELEMENT_TYPE, OPERATION) \
	{ \
		VECTOR_TYPE impl_result = VECTOR_TYPE::create_dangerous_uninitialized(); \
		impl_result.scalars[ 0] = (A).scalars[ 0] OPERATION (B).scalars[ 0]; \
		impl_result.scalars[ 1] = (A).scalars[ 1] OPERATION (B).scalars[ 1]; \
		impl_result.scalars[ 2] = (A).scalars[ 2] OPERATION (B).scalars[ 2]; \
		impl_result.scalars[ 3] = (A).scalars[ 3] OPERATION (B).scalars[ 3]; \
		impl_result.scalars[ 4] = (A).scalars[ 4] OPERATION (B).scalars[ 4]; \
		impl_result.scalars[ 5] = (A).scalars[ 5] OPERATION (B).scalars[ 5]; \
		impl_result.scalars[ 6] = (A).scalars[ 6] OPERATION (B).scalars[ 6]; \
		impl_result.scalars[ 7] = (A).scalars[ 7] OPERATION (B).scalars[ 7]; \
		return impl_result; \
	}
	#define IMPL_SCALAR_REFERENCE_INFIX_16_LANES(A, B, VECTOR_TYPE, ELEMENT_TYPE, OPERATION) \
	{ \
		VECTOR_TYPE impl_result = VECTOR_TYPE::create_dangerous_uninitialized(); \
		impl_result.scalars[ 0] = (A).scalars[ 0] OPERATION (B).scalars[ 0]; \
		impl_result.scalars[ 1] = (A).scalars[ 1] OPERATION (B).scalars[ 1]; \
		impl_result.scalars[ 2] = (A).scalars[ 2] OPERATION (B).scalars[ 2]; \
		impl_result.scalars[ 3] = (A).scalars[ 3] OPERATION (B).scalars[ 3]; \
		impl_result.scalars[ 4] = (A).scalars[ 4] OPERATION (B).scalars[ 4]; \
		impl_result.scalars[ 5] = (A).scalars[ 5] OPERATION (B).scalars[ 5]; \
		impl_result.scalars[ 6] = (A).scalars[ 6] OPERATION (B).scalars[ 6]; \
		impl_result.scalars[ 7] = (A).scalars[ 7] OPERATION (B).scalars[ 7]; \
		impl_result.scalars[ 8] = (A).scalars[ 8] OPERATION (B).scalars[ 8]; \
		impl_result.scalars[ 9] = (A).scalars[ 9] OPERATION (B).scalars[ 9]; \
		impl_result.scalars[10] = (A).scalars[10] OPERATION (B).scalars[10]; \
		impl_result.scalars[11] = (A).scalars[11] OPERATION (B).scalars[11]; \
		impl_result.scalars[12] = (A).scalars[12] OPERATION (B).scalars[12]; \
		impl_result.scalars[13] = (A).scalars[13] OPERATION (B).scalars[13]; \
		impl_result.scalars[14] = (A).scalars[14] OPERATION (B).scalars[14]; \
		impl_result.scalars[15] = (A).scalars[15] OPERATION (B).scalars[15]; \
		return impl_result; \
	}
	#define IMPL_SCALAR_REFERENCE_INFIX_32_LANES(A, B, VECTOR_TYPE, ELEMENT_TYPE, OPERATION) \
	{ \
		VECTOR_TYPE impl_result = VECTOR_TYPE::create_dangerous_uninitialized(); \
		impl_result.scalars[ 0] = (A).scalars[ 0] OPERATION (B).scalars[ 0]; \
		impl_result.scalars[ 1] = (A).scalars[ 1] OPERATION (B).scalars[ 1]; \
		impl_result.scalars[ 2] = (A).scalars[ 2] OPERATION (B).scalars[ 2]; \
		impl_result.scalars[ 3] = (A).scalars[ 3] OPERATION (B).scalars[ 3]; \
		impl_result.scalars[ 4] = (A).scalars[ 4] OPERATION (B).scalars[ 4]; \
		impl_result.scalars[ 5] = (A).scalars[ 5] OPERATION (B).scalars[ 5]; \
		impl_result.scalars[ 6] = (A).scalars[ 6] OPERATION (B).scalars[ 6]; \
		impl_result.scalars[ 7] = (A).scalars[ 7] OPERATION (B).scalars[ 7]; \
		impl_result.scalars[ 8] = (A).scalars[ 8] OPERATION (B).scalars[ 8]; \
		impl_result.scalars[ 9] = (A).scalars[ 9] OPERATION (B).scalars[ 9]; \
		impl_result.scalars[10] = (A).scalars[10] OPERATION (B).scalars[10]; \
		impl_result.scalars[11] = (A).scalars[11] OPERATION (B).scalars[11]; \
		impl_result.scalars[12] = (A).scalars[12] OPERATION (B).scalars[12]; \
		impl_result.scalars[13] = (A).scalars[13] OPERATION (B).scalars[13]; \
		impl_result.scalars[14] = (A).scalars[14] OPERATION (B).scalars[14]; \
		impl_result.scalars[15] = (A).scalars[15] OPERATION (B).scalars[15]; \
		impl_result.scalars[16] = (A).scalars[16] OPERATION (B).scalars[16]; \
		impl_result.scalars[17] = (A).scalars[17] OPERATION (B).scalars[17]; \
		impl_result.scalars[18] = (A).scalars[18] OPERATION (B).scalars[18]; \
		impl_result.scalars[19] = (A).scalars[19] OPERATION (B).scalars[19]; \
		impl_result.scalars[20] = (A).scalars[20] OPERATION (B).scalars[20]; \
		impl_result.scalars[21] = (A).scalars[21] OPERATION (B).scalars[21]; \
		impl_result.scalars[22] = (A).scalars[22] OPERATION (B).scalars[22]; \
		impl_result.scalars[23] = (A).scalars[23] OPERATION (B).scalars[23]; \
		impl_result.scalars[24] = (A).scalars[24] OPERATION (B).scalars[24]; \
		impl_result.scalars[25] = (A).scalars[25] OPERATION (B).scalars[25]; \
		impl_result.scalars[26] = (A).scalars[26] OPERATION (B).scalars[26]; \
		impl_result.scalars[27] = (A).scalars[27] OPERATION (B).scalars[27]; \
		impl_result.scalars[28] = (A).scalars[28] OPERATION (B).scalars[28]; \
		impl_result.scalars[29] = (A).scalars[29] OPERATION (B).scalars[29]; \
		impl_result.scalars[30] = (A).scalars[30] OPERATION (B).scalars[30]; \
		impl_result.scalars[31] = (A).scalars[31] OPERATION (B).scalars[31]; \
		return impl_result; \
	}

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
			source.writeAlignedUnsafe(&(a[0])); \
			dsr::string_append(target, indentation, a[0]); \
			for (int i = 1; i < LANE_COUNT; i++) { \
				string_append(target, U", ", a[i]); \
			} \
			return target; \
		}
		// All SIMD vectors can be printed.
		FOR_ALL_VECTOR_TYPES(CREATE_METHOD_PRINT)
	#undef CREATE_METHOD_PRINT

	// Integer equality returns true iff all comparisons are identical.
	#define CREATE_EXACT_EQUALITY(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline bool allLanesEqual(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(&(a[0])); \
			right.writeAlignedUnsafe(&(b[0])); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (a[i] != b[i]) return false; \
			} \
			return true; \
		} \
		inline bool allLanesNotEqual(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(&(a[0])); \
			right.writeAlignedUnsafe(&(b[0])); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (a[i] == b[i]) return false; \
			} \
			return true; \
		}
		// Integer SIMD vectors have exact equlity.
		FOR_INTEGER_VECTOR_TYPES(CREATE_EXACT_EQUALITY)
	#undef CREATE_EXACT_EQUALITY

	// Float equality returns true iff all comparisons are near.
	#define CREATE_TOLERANT_EQUALITY(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline bool allLanesEqual(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(&(a[0])); \
			right.writeAlignedUnsafe(&(b[0])); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (fabs(a[i] - b[i]) >= 0.0001f) return false; \
			} \
			return true; \
		} \
		inline bool allLanesNotEqual(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(&(a[0])); \
			right.writeAlignedUnsafe(&(b[0])); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (fabs(a[i] - b[i]) < 0.0001f) return false; \
			} \
			return true; \
		}
		// Float SIMD vectors have inexact equality.
		FOR_FLOAT_VECTOR_TYPES(CREATE_TOLERANT_EQUALITY)
	#undef CREATE_TOLERANT_EQUALITY

	#define CREATE_COMPARISONS(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline bool allLanesGreater(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(&(a[0])); \
			right.writeAlignedUnsafe(&(b[0])); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (a[i] <= b[i]) return false; \
			} \
			return true; \
		} \
		inline bool allLanesGreaterOrEqual(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(&(a[0])); \
			right.writeAlignedUnsafe(&(b[0])); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (a[i] < b[i]) return false; \
			} \
			return true; \
		} \
		inline bool allLanesLesser(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(&(a[0])); \
			right.writeAlignedUnsafe(&(b[0])); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (a[i] >= b[i]) return false; \
			} \
			return true; \
		} \
		inline bool allLanesLesserOrEqual(const VECTOR_TYPE& left, const VECTOR_TYPE& right) { \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE a[LANE_COUNT]; \
			ALIGN_BYTES(sizeof(VECTOR_TYPE)) ELEMENT_TYPE b[LANE_COUNT]; \
			left.writeAlignedUnsafe(&(a[0])); \
			right.writeAlignedUnsafe(&(b[0])); \
			for (int i = 0; i < LANE_COUNT; i++) { \
				if (a[i] > b[i]) return false; \
			} \
			return true; \
		}
		FOR_ALL_VECTOR_TYPES(CREATE_COMPARISONS)
	#undef CREATE_COMPARISONS

	inline F32x4 operator+(const F32x4& left, const F32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return F32x4(ADD_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.scalars[0] + right.scalars[0], left.scalars[1] + right.scalars[1], left.scalars[2] + right.scalars[2], left.scalars[3] + right.scalars[3]);
		#endif
	}

	inline F32x4 operator-(const F32x4& left, const F32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return F32x4(SUB_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.scalars[0] - right.scalars[0], left.scalars[1] - right.scalars[1], left.scalars[2] - right.scalars[2], left.scalars[3] - right.scalars[3]);
		#endif
	}

	inline F32x4 operator*(const F32x4& left, const F32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return F32x4(MUL_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.scalars[0] * right.scalars[0], left.scalars[1] * right.scalars[1], left.scalars[2] * right.scalars[2], left.scalars[3] * right.scalars[3]);
		#endif
	}
	inline F32x4 min(const F32x4& left, const F32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return F32x4(MIN_F32_SIMD(left.v, right.v));
		#else
			float v0 = left.scalars[0];
			float v1 = left.scalars[1];
			float v2 = left.scalars[2];
			float v3 = left.scalars[3];
			float r0 = right.scalars[0];
			float r1 = right.scalars[1];
			float r2 = right.scalars[2];
			float r3 = right.scalars[3];
			if (r0 < v0) { v0 = r0; }
			if (r1 < v1) { v1 = r1; }
			if (r2 < v2) { v2 = r2; }
			if (r3 < v3) { v3 = r3; }
			return F32x4(v0, v1, v2, v3);
		#endif
	}
	inline F32x4 max(const F32x4& left, const F32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return F32x4(MAX_F32_SIMD(left.v, right.v));
		#else
			float v0 = left.scalars[0];
			float v1 = left.scalars[1];
			float v2 = left.scalars[2];
			float v3 = left.scalars[3];
			float r0 = right.scalars[0];
			float r1 = right.scalars[1];
			float r2 = right.scalars[2];
			float r3 = right.scalars[3];
			if (r0 > v0) { v0 = r0; }
			if (r1 > v1) { v1 = r1; }
			if (r2 > v2) { v2 = r2; }
			if (r3 > v3) { v3 = r3; }
			return F32x4(v0, v1, v2, v3);
		#endif
	}
	inline I32x4 operator+(const I32x4& left, const I32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return I32x4(ADD_I32_SIMD(left.v, right.v));
		#else
			return I32x4(left.scalars[0] + right.scalars[0], left.scalars[1] + right.scalars[1], left.scalars[2] + right.scalars[2], left.scalars[3] + right.scalars[3]);
		#endif
	}
	inline I32x4 operator-(const I32x4& left, const I32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return I32x4(SUB_I32_SIMD(left.v, right.v));
		#else
			return I32x4(left.scalars[0] - right.scalars[0], left.scalars[1] - right.scalars[1], left.scalars[2] - right.scalars[2], left.scalars[3] - right.scalars[3]);
		#endif
	}
	inline I32x4 operator*(const I32x4& left, const I32x4& right) {
		#if defined(USE_BASIC_SIMD)
			#if defined(USE_SSE2)
				// TODO: Use AVX2 for 32-bit integer multiplication when available.
				IMPL_SCALAR_FALLBACK_INFIX_4_LANES(left, right, I32x4, int32_t, *)
			#elif defined(USE_NEON)
				return I32x4(MUL_I32_NEON(left.v, right.v));
			#endif
		#else
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, right, I32x4, int32_t, *)
		#endif
	}
	// TODO: Specify the behavior of truncated unsigned integer overflow and add it to the tests.
	inline U32x4 operator+(const U32x4& left, const U32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(ADD_U32_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, right, U32x4, uint32_t, +)
		#endif
	}
	inline U32x4 operator-(const U32x4& left, const U32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(SUB_U32_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, right, U32x4, uint32_t, -)
		#endif
	}
	inline U32x4 operator*(const U32x4& left, const U32x4& right) {
		#if defined(USE_BASIC_SIMD)
			#if defined(USE_SSE2)
				// TODO: Use AVX2 for 32-bit integer multiplication when available.
				IMPL_SCALAR_FALLBACK_INFIX_4_LANES(left, right, U32x4, uint32_t, *)
			#else // NEON
				return U32x4(MUL_U32_NEON(left.v, right.v));
			#endif
		#else
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, right, U32x4, uint32_t, *)
		#endif
	}
	// Bitwise and
	inline U32x4 operator&(const U32x4& left, const U32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(BITWISE_AND_U32_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, right, U32x4, uint32_t, &)
		#endif
	}
	// Bitwise or
	inline U32x4 operator|(const U32x4& left, const U32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(BITWISE_OR_U32_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX(left, right, U32x4, uint32_t, 4, |)
		#endif
	}
	// Bitwise xor
	inline U32x4 operator^(const U32x4& left, const U32x4& right) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(BITWISE_XOR_U32_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX(left, right, U32x4, uint32_t, 4, ^)
		#endif
	}
	// Bitwise negation
	inline U32x4 operator~(const U32x4& value) {
		#if defined(USE_NEON)
			return U32x4(vmvnq_u32(value.v));
		#elif defined(USE_BASIC_SIMD)
			// Fall back on xor against all ones.
			return value ^ U32x4(~uint32_t(0));
		#else
			// TODO: Generate automatically using a macro.
			return U32x4(~value.scalars[0], ~value.scalars[1], ~value.scalars[2], ~value.scalars[3]);
		#endif
	}
	inline U32x4 operator<<(const U32x4& left, const U32x4 &bitOffsets) {
		#ifdef SAFE_POINTER_CHECKS
			if(!allLanesLesser(bitOffsets, U32x4(32u))) {
				throwError(U"Tried to shift ", left, U" by bit offsets ", bitOffsets, U", which is non-deterministic from being out of bound 0..31!\n");
			}
		#endif
		#if defined(USE_SSE2)
			IMPL_SCALAR_FALLBACK_INFIX_4_LANES(left, bitOffsets, U32x4, uint32_t, <<)
		#elif defined(USE_NEON)
			return U32x4(vshlq_u32(left.v, vreinterpretq_s32_u32(bitOffsets.v)));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, bitOffsets, U32x4, uint32_t, <<)
		#endif
	}
	inline U32x4 operator>>(const U32x4& left, const U32x4 &bitOffsets) {
		#ifdef SAFE_POINTER_CHECKS
			if(!allLanesLesser(bitOffsets, U32x4(32u))) {
				throwError(U"Tried to shift ", left, U" by bit offsets ", bitOffsets, U", which is non-deterministic from being out of bound 0..31!\n");
			}
		#endif
		#if defined(USE_SSE2)
			IMPL_SCALAR_FALLBACK_INFIX_4_LANES(left, bitOffsets, U32x4, uint32_t, >>)
		#elif defined(USE_NEON)
			// TODO: Why is vshrq_u32 not found?
			//return U32x4(vshrq_u32(left.v, vreinterpretq_s32_u32(bitOffsets.v)));
			return U32x4(vshlq_u32(left.v, vnegq_s32(vreinterpretq_s32_u32(bitOffsets.v))));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, bitOffsets, U32x4, uint32_t, >>)
		#endif
	}
	// bitOffset must be an immediate constant, so a template argument is used.
	template <uint32_t bitOffset>
	inline U32x4 bitShiftLeftImmediate(const U32x4& left) {
		static_assert(bitOffset < 32u, "Immediate left shift of 32-bit values may not shift more than 31 bits!");
		#if defined(USE_SSE2)
			return U32x4(_mm_slli_epi32(left.v, bitOffset));
		#elif defined(USE_NEON)
			return U32x4(vshlq_u32(left.v, LOAD_SCALAR_I32_SIMD(bitOffset)));
		#else
			U32x4 bitOffsets = U32x4(bitOffset);
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, bitOffsets, U32x4, uint32_t, <<)
		#endif
	}
	// bitOffset must be an immediate constant.
	template <uint32_t bitOffset>
	inline U32x4 bitShiftRightImmediate(const U32x4& left) {
		static_assert(bitOffset < 32u, "Immediate right shift of 32-bit values may not shift more than 31 bits!");
		#if defined(USE_SSE2)
			return U32x4(_mm_srli_epi32(left.v, bitOffset));
		#elif defined(USE_NEON)
			// TODO: Why is vshrq_u32 not found?
			//return U32x4(vshrq_u32(left.v, LOAD_SCALAR_I32_SIMD(bitOffset)));
			return U32x4(vshlq_u32(left.v, LOAD_SCALAR_I32_SIMD(-(int32_t)bitOffset)));
		#else
			U32x4 bitOffsets = U32x4(bitOffset);
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(left, bitOffsets, U32x4, uint32_t, >>)
		#endif
	}

	inline U16x8 operator<<(const U16x8& left, const U16x8 &bitOffsets) {
		#ifdef SAFE_POINTER_CHECKS
			if(!allLanesLesser(bitOffsets, U16x8(16u))) {
				throwError(U"Tried to shift ", left, U" by bit offsets ", bitOffsets, U", which is non-deterministic from being out of bound 0..15!\n");
			}
		#endif
		#if defined(USE_SSE2)
			IMPL_SCALAR_FALLBACK_INFIX_8_LANES(left, bitOffsets, U16x8, uint16_t, <<)
		#elif defined(USE_NEON)
			return U16x8(vshlq_u16(left.v, vreinterpretq_s16_u16(bitOffsets.v)));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, bitOffsets, U16x8, uint16_t, <<)
		#endif
	}
	inline U16x8 operator>>(const U16x8& left, const U16x8 &bitOffsets) {
		#ifdef SAFE_POINTER_CHECKS
			if(!allLanesLesser(bitOffsets, U16x8(16u))) {
				throwError(U"Tried to shift ", left, U" by bit offsets ", bitOffsets, U", which is non-deterministic from being out of bound 0..15!\n");
			}
		#endif
		#if defined(USE_SSE2)
			IMPL_SCALAR_FALLBACK_INFIX_8_LANES(left, bitOffsets, U16x8, uint16_t, >>)
		#elif defined(USE_NEON)
			//return U16x8(vshrq_u16(left.v, vreinterpretq_s16_u16(bitOffsets.v)));
			return U16x8(vshlq_u16(left.v, vnegq_s16(vreinterpretq_s16_u16(bitOffsets.v))));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, bitOffsets, U16x8, uint16_t, >>)
		#endif
	}
	// bitOffset must be an immediate constant, so a template argument is used.
	template <uint32_t bitOffset>
	inline U16x8 bitShiftLeftImmediate(const U16x8& left) {
		static_assert(bitOffset < 16u, "Immediate left shift of 16-bit values may not shift more than 15 bits!");
		#if defined(USE_SSE2)
			return U16x8(_mm_slli_epi16(left.v, bitOffset));
		#elif defined(USE_NEON)
			return U16x8(vshlq_u32(left.v, vdupq_n_s16(bitOffset)));
		#else
			U16x8 bitOffsets = U16x8(bitOffset);
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, bitOffsets, U16x8, uint16_t, <<)
		#endif
	}
	// bitOffset must be an immediate constant.
	template <uint32_t bitOffset>
	inline U16x8 bitShiftRightImmediate(const U16x8& left) {
		static_assert(bitOffset < 16u, "Immediate right shift of 16-bit values may not shift more than 15 bits!");
		#if defined(USE_SSE2)
			return U16x8(_mm_srli_epi16(left.v, bitOffset));
		#elif defined(USE_NEON)
			//return U16x8(vshrq_u16(left.v, vdupq_n_s16(bitOffset)));
			return U16x8(vshlq_u16(left.v, vdupq_n_s16(-(int32_t)bitOffset)));
		#else
			U16x8 bitOffsets = U16x8(bitOffset);
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, bitOffsets, U16x8, uint16_t, >>)
		#endif
	}

	inline U16x8 operator+(const U16x8& left, const U16x8& right) {
		#if defined(USE_BASIC_SIMD)
			return U16x8(ADD_U16_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U16x8, uint16_t, +)
		#endif
	}
	inline U16x8 operator-(const U16x8& left, const U16x8& right) {
		#if defined(USE_BASIC_SIMD)
			return U16x8(SUB_U16_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U16x8, uint16_t, -)
		#endif
	}
	inline U16x8 operator*(const U16x8& left, const U16x8& right) {
		#if defined(USE_BASIC_SIMD)
			return U16x8(MUL_U16_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U16x8, uint16_t, *)
		#endif
	}
	inline U8x16 operator+(const U8x16& left, const U8x16& right) {
		#if defined(USE_BASIC_SIMD)
			return U8x16(ADD_U8_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, right, U8x16, uint8_t, +)
		#endif
	}
	inline U8x16 operator-(const U8x16& left, const U8x16& right) {
		#if defined(USE_BASIC_SIMD)
			return U8x16(SUB_U8_SIMD(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, right, U8x16, uint8_t, -)
		#endif
	}

	inline uint8_t impl_limit0(int32_t x) { return x < 0 ? 0 : x; }
	inline uint8_t impl_limit255(uint32_t x) { return x > 255 ? 255 : x; }
	inline U8x16 saturatedAddition(const U8x16& left, const U8x16& right) {
		#if defined(USE_BASIC_SIMD)
			return U8x16(ADD_SAT_U8_SIMD(left.v, right.v));
		#else
			return U8x16(
			  impl_limit255((uint32_t)left.scalars[0] + (uint32_t)right.scalars[0]),
			  impl_limit255((uint32_t)left.scalars[1] + (uint32_t)right.scalars[1]),
			  impl_limit255((uint32_t)left.scalars[2] + (uint32_t)right.scalars[2]),
			  impl_limit255((uint32_t)left.scalars[3] + (uint32_t)right.scalars[3]),
			  impl_limit255((uint32_t)left.scalars[4] + (uint32_t)right.scalars[4]),
			  impl_limit255((uint32_t)left.scalars[5] + (uint32_t)right.scalars[5]),
			  impl_limit255((uint32_t)left.scalars[6] + (uint32_t)right.scalars[6]),
			  impl_limit255((uint32_t)left.scalars[7] + (uint32_t)right.scalars[7]),
			  impl_limit255((uint32_t)left.scalars[8] + (uint32_t)right.scalars[8]),
			  impl_limit255((uint32_t)left.scalars[9] + (uint32_t)right.scalars[9]),
			  impl_limit255((uint32_t)left.scalars[10] + (uint32_t)right.scalars[10]),
			  impl_limit255((uint32_t)left.scalars[11] + (uint32_t)right.scalars[11]),
			  impl_limit255((uint32_t)left.scalars[12] + (uint32_t)right.scalars[12]),
			  impl_limit255((uint32_t)left.scalars[13] + (uint32_t)right.scalars[13]),
			  impl_limit255((uint32_t)left.scalars[14] + (uint32_t)right.scalars[14]),
			  impl_limit255((uint32_t)left.scalars[15] + (uint32_t)right.scalars[15])
			);
		#endif
	}
	inline U8x16 saturatedSubtraction(const U8x16& left, const U8x16& right) {
		#if defined(USE_BASIC_SIMD)
			return U8x16(SUB_SAT_U8_SIMD(left.v, right.v));
		#else
			return U8x16(
			  impl_limit0((int32_t)left.scalars[0] - (int32_t)right.scalars[0]),
			  impl_limit0((int32_t)left.scalars[1] - (int32_t)right.scalars[1]),
			  impl_limit0((int32_t)left.scalars[2] - (int32_t)right.scalars[2]),
			  impl_limit0((int32_t)left.scalars[3] - (int32_t)right.scalars[3]),
			  impl_limit0((int32_t)left.scalars[4] - (int32_t)right.scalars[4]),
			  impl_limit0((int32_t)left.scalars[5] - (int32_t)right.scalars[5]),
			  impl_limit0((int32_t)left.scalars[6] - (int32_t)right.scalars[6]),
			  impl_limit0((int32_t)left.scalars[7] - (int32_t)right.scalars[7]),
			  impl_limit0((int32_t)left.scalars[8] - (int32_t)right.scalars[8]),
			  impl_limit0((int32_t)left.scalars[9] - (int32_t)right.scalars[9]),
			  impl_limit0((int32_t)left.scalars[10] - (int32_t)right.scalars[10]),
			  impl_limit0((int32_t)left.scalars[11] - (int32_t)right.scalars[11]),
			  impl_limit0((int32_t)left.scalars[12] - (int32_t)right.scalars[12]),
			  impl_limit0((int32_t)left.scalars[13] - (int32_t)right.scalars[13]),
			  impl_limit0((int32_t)left.scalars[14] - (int32_t)right.scalars[14]),
			  impl_limit0((int32_t)left.scalars[15] - (int32_t)right.scalars[15])
			);
		#endif
	}

	inline I32x4 truncateToI32(const F32x4& vector) {
		#if defined(USE_BASIC_SIMD)
			return I32x4(F32_TO_I32_SIMD(vector.v));
		#else
			return I32x4((int32_t)vector.scalars[0], (int32_t)vector.scalars[1], (int32_t)vector.scalars[2], (int32_t)vector.scalars[3]);
		#endif
	}
	inline U32x4 truncateToU32(const F32x4& vector) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(F32_TO_U32_SIMD(vector.v));
		#else
			return U32x4((uint32_t)vector.scalars[0], (uint32_t)vector.scalars[1], (uint32_t)vector.scalars[2], (uint32_t)vector.scalars[3]);
		#endif
	}
	inline F32x4 floatFromI32(const I32x4& vector) {
		#if defined(USE_BASIC_SIMD)
			return F32x4(I32_TO_F32_SIMD(vector.v));
		#else
			return F32x4((float)vector.scalars[0], (float)vector.scalars[1], (float)vector.scalars[2], (float)vector.scalars[3]);
		#endif
	}
	inline F32x4 floatFromU32(const U32x4& vector) {
		#if defined(USE_BASIC_SIMD)
			return F32x4(U32_TO_F32_SIMD(vector.v));
		#else
			return F32x4((float)vector.scalars[0], (float)vector.scalars[1], (float)vector.scalars[2], (float)vector.scalars[3]);
		#endif
	}
	inline I32x4 I32FromU32(const U32x4& vector) {
		#if defined(USE_BASIC_SIMD)
			return I32x4(REINTERPRET_U32_TO_I32_SIMD(vector.v));
		#else
			return I32x4((int32_t)vector.scalars[0], (int32_t)vector.scalars[1], (int32_t)vector.scalars[2], (int32_t)vector.scalars[3]);
		#endif
	}
	inline U32x4 U32FromI32(const I32x4& vector) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(REINTERPRET_I32_TO_U32_SIMD(vector.v));
		#else
			return U32x4((uint32_t)vector.scalars[0], (uint32_t)vector.scalars[1], (uint32_t)vector.scalars[2], (uint32_t)vector.scalars[3]);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U8x16 reinterpret_U8FromU32(const U32x4& vector) {
		#if defined(USE_BASIC_SIMD)
			return U8x16(REINTERPRET_U32_TO_U8_SIMD(vector.v));
		#else
			const uint8_t *source = (const uint8_t*)vector.scalars;
			return U8x16(
			  source[0], source[1], source[2], source[3], source[4], source[5], source[6], source[7],
			  source[8], source[9], source[10], source[11], source[12], source[13], source[14], source[15]
			);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U32x4 reinterpret_U32FromU8(const U8x16& vector) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(REINTERPRET_U8_TO_U32_SIMD(vector.v));
		#else
			const uint32_t *source = (const uint32_t*)vector.scalars;
			return U32x4(source[0], source[1], source[2], source[3]);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U16x8 reinterpret_U16FromU32(const U32x4& vector) {
		#if defined(USE_BASIC_SIMD)
			return U16x8(REINTERPRET_U32_TO_U16_SIMD(vector.v));
		#else
			const uint16_t *source = (const uint16_t*)vector.scalars;
			return U16x8(
			  source[0], source[1], source[2], source[3], source[4], source[5], source[6], source[7]
			);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U32x4 reinterpret_U32FromU16(const U16x8& vector) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(REINTERPRET_U16_TO_U32_SIMD(vector.v));
		#else
			const uint32_t *source = (const uint32_t*)vector.scalars;
			return U32x4(source[0], source[1], source[2], source[3]);
		#endif
	}

	// Unpacking to larger integers
	inline U32x4 lowerToU32(const U16x8& vector) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(U16_LOW_TO_U32_SIMD(vector.v));
		#else
			return U32x4(vector.scalars[0], vector.scalars[1], vector.scalars[2], vector.scalars[3]);
		#endif
	}
	inline U32x4 higherToU32(const U16x8& vector) {
		#if defined(USE_BASIC_SIMD)
			return U32x4(U16_HIGH_TO_U32_SIMD(vector.v));
		#else
			return U32x4(vector.scalars[4], vector.scalars[5], vector.scalars[6], vector.scalars[7]);
		#endif
	}
	inline U16x8 lowerToU16(const U8x16& vector) {
		#if defined(USE_BASIC_SIMD)
			return U16x8(U8_LOW_TO_U16_SIMD(vector.v));
		#else
			return U16x8(
			  vector.scalars[0], vector.scalars[1], vector.scalars[2], vector.scalars[3],
			  vector.scalars[4], vector.scalars[5], vector.scalars[6], vector.scalars[7]
			);
		#endif
	}
	inline U16x8 higherToU16(const U8x16& vector) {
		#if defined(USE_BASIC_SIMD)
			return U16x8(U8_HIGH_TO_U16_SIMD(vector.v));
		#else
			return U16x8(
			  vector.scalars[8], vector.scalars[9], vector.scalars[10], vector.scalars[11],
			  vector.scalars[12], vector.scalars[13], vector.scalars[14], vector.scalars[15]
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
		#if defined(USE_BASIC_SIMD)
			return F32x4(0.0f) - value;
		#else
			return F32x4(-value.scalars[0], -value.scalars[1], -value.scalars[2], -value.scalars[3]);
		#endif
	}
	inline I32x4 operator-(const I32x4& value) {
		#if defined(USE_BASIC_SIMD)
			return I32x4(0) - value;
		#else
			return I32x4(-value.scalars[0], -value.scalars[1], -value.scalars[2], -value.scalars[3]);
		#endif
	}

	// Helper macros for generating the vector extract functions.
	//   Having one function for each type and offset makes sure that the compiler gets an immediate integer within the valid range.
	#if defined(USE_BASIC_SIMD)
		#if defined(USE_SSE2)
			#if defined(USE_SSSE3)
				#define _MM_ALIGNR_EPI8(A, B, OFFSET) _mm_alignr_epi8(A, B, OFFSET)
			#else
				// If SSSE3 is not used, emulate it using stack memory and unaligned reading of data.
				static inline SIMD_U8x16 _MM_ALIGNR_EPI8(SIMD_U8x16 a, SIMD_U8x16 b, int offset) {
					ALIGN16 uint8_t vectorBuffer[32];
					#ifdef SAFE_POINTER_CHECKS
						if (uintptr_t((void*)vectorBuffer) & 15u) { throwError(U"Unaligned stack memory detected in 128-bit VECTOR_EXTRACT_GENERATOR!\n"); }
					#endif
					_mm_store_si128((SIMD_U8x16*)(vectorBuffer), b);
					_mm_store_si128((SIMD_U8x16*)(vectorBuffer + 16), a);
					ALIGN16 SIMD_U8x16 result = _mm_loadu_si128((SIMD_U8x16*)(vectorBuffer + offset));
					return result;
				}
			#endif
			#define VECTOR_EXTRACT_GENERATOR_U8(OFFSET, FALLBACK_RESULT) return U8x16(_MM_ALIGNR_EPI8(b.v, a.v, OFFSET));
			#define VECTOR_EXTRACT_GENERATOR_U16(OFFSET, FALLBACK_RESULT) return U16x8(_MM_ALIGNR_EPI8(b.v, a.v, OFFSET * 2));
			#define VECTOR_EXTRACT_GENERATOR_U32(OFFSET, FALLBACK_RESULT) return U32x4(_MM_ALIGNR_EPI8(b.v, a.v, OFFSET * 4));
			#define VECTOR_EXTRACT_GENERATOR_I32(OFFSET, FALLBACK_RESULT) return I32x4(_MM_ALIGNR_EPI8(b.v, a.v, OFFSET * 4));
			#define VECTOR_EXTRACT_GENERATOR_F32(OFFSET, FALLBACK_RESULT) return F32x4(SIMD_F32x4(_MM_ALIGNR_EPI8(SIMD_U32x4(b.v), SIMD_U32x4(a.v), OFFSET * 4)));
		#elif defined(USE_NEON)
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
	U8x16 inline vectorExtract_1(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(1, U8x16(a.scalars[1], a.scalars[2], a.scalars[3], a.scalars[4], a.scalars[5], a.scalars[6], a.scalars[7], a.scalars[8], a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0])) }
	U8x16 inline vectorExtract_2(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(2, U8x16(a.scalars[2], a.scalars[3], a.scalars[4], a.scalars[5], a.scalars[6], a.scalars[7], a.scalars[8], a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1])) }
	U8x16 inline vectorExtract_3(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(3, U8x16(a.scalars[3], a.scalars[4], a.scalars[5], a.scalars[6], a.scalars[7], a.scalars[8], a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2])) }
	U8x16 inline vectorExtract_4(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(4, U8x16(a.scalars[4], a.scalars[5], a.scalars[6], a.scalars[7], a.scalars[8], a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3])) }
	U8x16 inline vectorExtract_5(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(5, U8x16(a.scalars[5], a.scalars[6], a.scalars[7], a.scalars[8], a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4])) }
	U8x16 inline vectorExtract_6(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(6, U8x16(a.scalars[6], a.scalars[7], a.scalars[8], a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5])) }
	U8x16 inline vectorExtract_7(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(7, U8x16(a.scalars[7], a.scalars[8], a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6])) }
	U8x16 inline vectorExtract_8(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(8, U8x16(a.scalars[8], a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6], b.scalars[7])) }
	U8x16 inline vectorExtract_9(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(9, U8x16(a.scalars[9], a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6], b.scalars[7], b.scalars[8])) }
	U8x16 inline vectorExtract_10(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(10, U8x16(a.scalars[10], a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6], b.scalars[7], b.scalars[8], b.scalars[9])) }
	U8x16 inline vectorExtract_11(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(11, U8x16(a.scalars[11], a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6], b.scalars[7], b.scalars[8], b.scalars[9], b.scalars[10])) }
	U8x16 inline vectorExtract_12(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(12, U8x16(a.scalars[12], a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6], b.scalars[7], b.scalars[8], b.scalars[9], b.scalars[10], b.scalars[11])) }
	U8x16 inline vectorExtract_13(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(13, U8x16(a.scalars[13], a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6], b.scalars[7], b.scalars[8], b.scalars[9], b.scalars[10], b.scalars[11], b.scalars[12])) }
	U8x16 inline vectorExtract_14(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(14, U8x16(a.scalars[14], a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6], b.scalars[7], b.scalars[8], b.scalars[9], b.scalars[10], b.scalars[11], b.scalars[12], b.scalars[13])) }
	U8x16 inline vectorExtract_15(const U8x16 &a, const U8x16 &b) { VECTOR_EXTRACT_GENERATOR_U8(15, U8x16(a.scalars[15], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6], b.scalars[7], b.scalars[8], b.scalars[9], b.scalars[10], b.scalars[11], b.scalars[12], b.scalars[13], b.scalars[14])) }
	U8x16 inline vectorExtract_16(const U8x16 &a, const U8x16 &b) { return b; }

	U16x8 inline vectorExtract_0(const U16x8 &a, const U16x8 &b) { return a; }
	U16x8 inline vectorExtract_1(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(1, U16x8(a.scalars[1], a.scalars[2], a.scalars[3], a.scalars[4], a.scalars[5], a.scalars[6], a.scalars[7], b.scalars[0])) }
	U16x8 inline vectorExtract_2(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(2, U16x8(a.scalars[2], a.scalars[3], a.scalars[4], a.scalars[5], a.scalars[6], a.scalars[7], b.scalars[0], b.scalars[1])) }
	U16x8 inline vectorExtract_3(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(3, U16x8(a.scalars[3], a.scalars[4], a.scalars[5], a.scalars[6], a.scalars[7], b.scalars[0], b.scalars[1], b.scalars[2])) }
	U16x8 inline vectorExtract_4(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(4, U16x8(a.scalars[4], a.scalars[5], a.scalars[6], a.scalars[7], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3])) }
	U16x8 inline vectorExtract_5(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(5, U16x8(a.scalars[5], a.scalars[6], a.scalars[7], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4])) }
	U16x8 inline vectorExtract_6(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(6, U16x8(a.scalars[6], a.scalars[7], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5])) }
	U16x8 inline vectorExtract_7(const U16x8 &a, const U16x8 &b) { VECTOR_EXTRACT_GENERATOR_U16(7, U16x8(a.scalars[7], b.scalars[0], b.scalars[1], b.scalars[2], b.scalars[3], b.scalars[4], b.scalars[5], b.scalars[6])) }
	U16x8 inline vectorExtract_8(const U16x8 &a, const U16x8 &b) { return b; }

	U32x4 inline vectorExtract_0(const U32x4 &a, const U32x4 &b) { return a; }
	U32x4 inline vectorExtract_1(const U32x4 &a, const U32x4 &b) { VECTOR_EXTRACT_GENERATOR_U32(1, U32x4(a.scalars[1], a.scalars[2], a.scalars[3], b.scalars[0])) }
	U32x4 inline vectorExtract_2(const U32x4 &a, const U32x4 &b) { VECTOR_EXTRACT_GENERATOR_U32(2, U32x4(a.scalars[2], a.scalars[3], b.scalars[0], b.scalars[1])) }
	U32x4 inline vectorExtract_3(const U32x4 &a, const U32x4 &b) { VECTOR_EXTRACT_GENERATOR_U32(3, U32x4(a.scalars[3], b.scalars[0], b.scalars[1], b.scalars[2])) }
	U32x4 inline vectorExtract_4(const U32x4 &a, const U32x4 &b) { return b; }

	I32x4 inline vectorExtract_0(const I32x4 &a, const I32x4 &b) { return a; }
	I32x4 inline vectorExtract_1(const I32x4 &a, const I32x4 &b) { VECTOR_EXTRACT_GENERATOR_I32(1, I32x4(a.scalars[1], a.scalars[2], a.scalars[3], b.scalars[0])) }
	I32x4 inline vectorExtract_2(const I32x4 &a, const I32x4 &b) { VECTOR_EXTRACT_GENERATOR_I32(2, I32x4(a.scalars[2], a.scalars[3], b.scalars[0], b.scalars[1])) }
	I32x4 inline vectorExtract_3(const I32x4 &a, const I32x4 &b) { VECTOR_EXTRACT_GENERATOR_I32(3, I32x4(a.scalars[3], b.scalars[0], b.scalars[1], b.scalars[2])) }
	I32x4 inline vectorExtract_4(const I32x4 &a, const I32x4 &b) { return b; }

	F32x4 inline vectorExtract_0(const F32x4 &a, const F32x4 &b) { return a; }
	F32x4 inline vectorExtract_1(const F32x4 &a, const F32x4 &b) { VECTOR_EXTRACT_GENERATOR_F32(1, F32x4(a.scalars[1], a.scalars[2], a.scalars[3], b.scalars[0])) }
	F32x4 inline vectorExtract_2(const F32x4 &a, const F32x4 &b) { VECTOR_EXTRACT_GENERATOR_F32(2, F32x4(a.scalars[2], a.scalars[3], b.scalars[0], b.scalars[1])) }
	F32x4 inline vectorExtract_3(const F32x4 &a, const F32x4 &b) { VECTOR_EXTRACT_GENERATOR_F32(3, F32x4(a.scalars[3], b.scalars[0], b.scalars[1], b.scalars[2])) }
	F32x4 inline vectorExtract_4(const F32x4 &a, const F32x4 &b) { return b; }

	// Gather instructions load memory from a pointer at multiple index offsets at the same time.
	//   The given pointers should be aligned with 4 bytes, so that the fallback solution works on machines with strict alignment requirements.
	#if defined(USE_AVX2)
		#define GATHER_I32x4_AVX2(SOURCE, FOUR_OFFSETS, SCALE) _mm_i32gather_epi32((const int32_t*)(SOURCE), FOUR_OFFSETS, SCALE)
		#define GATHER_U32x4_AVX2(SOURCE, FOUR_OFFSETS, SCALE) _mm_i32gather_epi32((const int32_t*)(SOURCE), FOUR_OFFSETS, SCALE)
		#define GATHER_F32x4_AVX2(SOURCE, FOUR_OFFSETS, SCALE) _mm_i32gather_ps((const float*)(SOURCE), FOUR_OFFSETS, SCALE)
	#endif
	static inline U32x4 gather_U32(dsr::SafePointer<const uint32_t> data, const U32x4 &elementOffset) {
		#ifdef SAFE_POINTER_CHECKS
			ALIGN16 uint32_t elementOffsets[4];
			if (uintptr_t((void*)elementOffsets) & 15u) { throwError(U"Unaligned stack memory detected in 128-bit gather_U32!\n"); }
			elementOffset.writeAlignedUnsafe(elementOffsets);
			data.assertInside("U32x4 gather_U32 lane 0", (data + elementOffsets[0]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 1", (data + elementOffsets[1]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 2", (data + elementOffsets[2]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 3", (data + elementOffsets[3]).getUnchecked());
		#endif
		#if defined(USE_AVX2)
			return U32x4(GATHER_U32x4_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			#ifndef SAFE_POINTER_CHECKS
				ALIGN16 uint32_t elementOffsets[4];
				elementOffset.writeAlignedUnsafe(elementOffsets);
			#endif
			return U32x4(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3])
			);
		#endif
	}
	static inline I32x4 gather_I32(dsr::SafePointer<const int32_t> data, const U32x4 &elementOffset) {
		#ifdef SAFE_POINTER_CHECKS
			ALIGN16 uint32_t elementOffsets[4];
			if (uintptr_t((void*)elementOffsets) & 15u) { throwError(U"Unaligned stack memory detected in 128-bit gather_I32!\n"); }
			elementOffset.writeAlignedUnsafe(elementOffsets);
			data.assertInside("I32x4 gather_I32 lane 0", (data + elementOffsets[0]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 1", (data + elementOffsets[1]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 2", (data + elementOffsets[2]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 3", (data + elementOffsets[3]).getUnchecked());
		#endif
		#if defined(USE_AVX2)
			return I32x4(GATHER_U32x4_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			#ifndef SAFE_POINTER_CHECKS
				ALIGN16 uint32_t elementOffsets[4];
				elementOffset.writeAlignedUnsafe(elementOffsets);
			#endif
			return I32x4(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3])
			);
		#endif
	}
	static inline F32x4 gather_F32(dsr::SafePointer<const float> data, const U32x4 &elementOffset) {
		#ifdef SAFE_POINTER_CHECKS
			ALIGN16 uint32_t elementOffsets[4];
			if (uintptr_t((void*)elementOffsets) & 15u) { throwError(U"Unaligned stack memory detected in 128-bit gather_F32!\n"); }
			elementOffset.writeAlignedUnsafe(elementOffsets);
			data.assertInside("F32x4 gather_F32 lane 0", (data + elementOffsets[0]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 1", (data + elementOffsets[1]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 2", (data + elementOffsets[2]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 3", (data + elementOffsets[3]).getUnchecked());
		#endif
		#if defined(USE_AVX2)
			return F32x4(GATHER_F32x4_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			#ifndef SAFE_POINTER_CHECKS
				ALIGN16 uint32_t elementOffsets[4];
				elementOffset.writeAlignedUnsafe(elementOffsets);
			#endif
			return F32x4(
			  *(data + elementOffsets[0]),
			  *(data + elementOffsets[1]),
			  *(data + elementOffsets[2]),
			  *(data + elementOffsets[3])
			);
		#endif
	}
	inline F32x8 operator+(const F32x8& left, const F32x8& right) {
		#if defined(USE_256BIT_F_SIMD)
			return F32x8(ADD_F32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, F32x8, float, +)
		#endif
	}
	inline F32x8 operator-(const F32x8& left, const F32x8& right) {
		#if defined(USE_256BIT_F_SIMD)
			return F32x8(SUB_F32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, F32x8, float, -)
		#endif
	}
	inline F32x8 operator*(const F32x8& left, const F32x8& right) {
		#if defined(USE_256BIT_F_SIMD)
			return F32x8(MUL_F32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, F32x8, float, *)
		#endif
	}
	inline F32x8 min(const F32x8& left, const F32x8& right) {
		#if defined(USE_256BIT_F_SIMD)
			return F32x8(MIN_F32_SIMD256(left.v, right.v));
		#else
			float v0 = left.scalars[0];
			float v1 = left.scalars[1];
			float v2 = left.scalars[2];
			float v3 = left.scalars[3];
			float v4 = left.scalars[4];
			float v5 = left.scalars[5];
			float v6 = left.scalars[6];
			float v7 = left.scalars[7];
			float r0 = right.scalars[0];
			float r1 = right.scalars[1];
			float r2 = right.scalars[2];
			float r3 = right.scalars[3];
			float r4 = right.scalars[4];
			float r5 = right.scalars[5];
			float r6 = right.scalars[6];
			float r7 = right.scalars[7];
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
		#if defined(USE_256BIT_F_SIMD)
			return F32x8(MAX_F32_SIMD256(left.v, right.v));
		#else
			float v0 = left.scalars[0];
			float v1 = left.scalars[1];
			float v2 = left.scalars[2];
			float v3 = left.scalars[3];
			float v4 = left.scalars[4];
			float v5 = left.scalars[5];
			float v6 = left.scalars[6];
			float v7 = left.scalars[7];
			float r0 = right.scalars[0];
			float r1 = right.scalars[1];
			float r2 = right.scalars[2];
			float r3 = right.scalars[3];
			float r4 = right.scalars[4];
			float r5 = right.scalars[5];
			float r6 = right.scalars[6];
			float r7 = right.scalars[7];
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
		#if defined(USE_256BIT_X_SIMD)
			return I32x8(ADD_I32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, I32x8, int32_t, +)
		#endif
	}
	inline I32x8 operator-(const I32x8& left, const I32x8& right) {
		#if defined(USE_256BIT_X_SIMD)
			return I32x8(SUB_I32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, I32x8, int32_t, -)
		#endif
	}
	inline I32x8 operator*(const I32x8& left, const I32x8& right) {
		#if defined(USE_AVX2)
			return I32x8(MUL_I32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, I32x8, int32_t, *)
		#endif
	}
	inline U32x8 operator+(const U32x8& left, const U32x8& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(ADD_U32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U32x8, uint32_t, +)
		#endif
	}
	inline U32x8 operator-(const U32x8& left, const U32x8& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(SUB_U32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U32x8, uint32_t, -)
		#endif
	}
	inline U32x8 operator*(const U32x8& left, const U32x8& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(MUL_U32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U32x8, uint32_t, *)
		#endif
	}
	inline U32x8 operator&(const U32x8& left, const U32x8& right) {
		assert((uintptr_t(&left) & 31u) == 0);
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(BITWISE_AND_U32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U32x8, uint32_t, &)
		#endif
	}
	inline U32x8 operator|(const U32x8& left, const U32x8& right) {
		assert((uintptr_t(&left) & 31u) == 0);
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(BITWISE_OR_U32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U32x8, uint32_t, |)
		#endif
	}
	inline U32x8 operator^(const U32x8& left, const U32x8& right) {
		assert((uintptr_t(&left) & 31u) == 0);
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(BITWISE_XOR_U32_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, right, U32x8, uint32_t, ^)
		#endif
	}
	inline U32x8 operator~(const U32x8& value) {
		assert((uintptr_t(&value) & 31u) == 0);
		#if defined(USE_BASIC_SIMD)
			return value ^ U32x8(~uint32_t(0));
		#else
			return U32x8(
			  ~value.scalars[0],
			  ~value.scalars[1],
			  ~value.scalars[2],
			  ~value.scalars[3],
			  ~value.scalars[4],
			  ~value.scalars[5],
			  ~value.scalars[6],
			  ~value.scalars[7]
			);
		#endif
	}

	// ARM NEON does not support 256-bit vectors and Intel's AVX2 does not support variable shifting.
	inline U32x8 operator<<(const U32x8& left, const U32x8 &bitOffsets) {
		assert((uintptr_t(&left) & 31u) == 0);
		#ifdef SAFE_POINTER_CHECKS
			if(!allLanesLesser(bitOffsets, U32x8(32u))) {
				throwError(U"Tried to shift ", left, U" by bit offsets ", bitOffsets, U", which is non-deterministic from being out of bound 0..31!\n");
			}
		#endif
		#if defined(USE_AVX2)
			IMPL_SCALAR_FALLBACK_INFIX_8_LANES(left, bitOffsets, U32x8, uint32_t, <<)
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, bitOffsets, U32x8, uint32_t, <<)
		#endif
	}
	inline U32x8 operator>>(const U32x8& left, const U32x8 &bitOffsets) {
		assert((uintptr_t(&left) & 31u) == 0);
		#ifdef SAFE_POINTER_CHECKS
			if(!allLanesLesser(bitOffsets, U32x8(32u))) {
				throwError(U"Tried to shift ", left, U" by bit offsets ", bitOffsets, U", which is non-deterministic from being out of bound 0..31!\n");
			}
		#endif
		#if defined(USE_AVX2)
			IMPL_SCALAR_FALLBACK_INFIX_8_LANES(left, bitOffsets, U32x8, uint32_t, >>)
		#else
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, bitOffsets, U32x8, uint32_t, >>)
		#endif
	}
	// bitOffset must be an immediate constant from 0 to 31, so a template argument is used.
	template <uint32_t bitOffset>
	inline U32x8 bitShiftLeftImmediate(const U32x8& left) {
		assert((uintptr_t(&left) & 31u) == 0);
		static_assert(bitOffset < 32u, "Immediate left shift of 32-bit values may not shift more than 31 bits!");
		#if defined(USE_AVX2)
			return U32x8(_mm256_slli_epi32(left.v, bitOffset));
		#else
			U32x8 bitOffsets = U32x8(bitOffset);
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, bitOffsets, U32x8, uint32_t, <<)
		#endif
	}
	// bitOffset must be an immediate constant from 0 to 31, so a template argument is used.
	template <uint32_t bitOffset>
	inline U32x8 bitShiftRightImmediate(const U32x8& left) {
		assert((uintptr_t(&left) & 31u) == 0);
		static_assert(bitOffset < 32u, "Immediate right shift of 32-bit values may not shift more than 31 bits!");
		#if defined(USE_AVX2)
			return U32x8(_mm256_srli_epi32(left.v, bitOffset));
		#else
			U32x8 bitOffsets = U32x8(bitOffset);
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(left, bitOffsets, U32x8, uint32_t, >>)
		#endif
	}

	inline U16x16 operator<<(const U16x16& left, const U16x16 &bitOffsets) {
		#ifdef SAFE_POINTER_CHECKS
			if(!allLanesLesser(bitOffsets, U16x16(16u))) {
				throwError(U"Tried to shift ", left, U" by bit offsets ", bitOffsets, U", which is non-deterministic from being out of bound 0..15!\n");
			}
		#endif
		#if defined(USE_AVX2)
			IMPL_SCALAR_FALLBACK_INFIX_16_LANES(left, bitOffsets, U16x16, uint16_t, <<)
		#else
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, bitOffsets, U16x16, uint16_t, <<)
		#endif
	}
	inline U16x16 operator>>(const U16x16& left, const U16x16 &bitOffsets) {
		#ifdef SAFE_POINTER_CHECKS
			if(!allLanesLesser(bitOffsets, U16x16(16u))) {
				throwError(U"Tried to shift ", left, U" by bit offsets ", bitOffsets, U", which is non-deterministic from being out of bound 0..15!\n");
			}
		#endif
		#if defined(USE_AVX2)
			IMPL_SCALAR_FALLBACK_INFIX_16_LANES(left, bitOffsets, U16x16, uint16_t, >>)
		#else
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, bitOffsets, U16x16, uint16_t, >>)
		#endif
	}
	// bitOffset must be an immediate constant from 0 to 31, so a template argument is used.
	template <uint32_t bitOffset>
	inline U16x16 bitShiftLeftImmediate(const U16x16& left) {
		static_assert(bitOffset < 16u, "Immediate left shift of 16-bit values may not shift more than 15 bits!");
		#if defined(USE_AVX2)
			return U16x16(_mm256_slli_epi16(left.v, bitOffset));
		#else
			U16x16 bitOffsets = U16x16(bitOffset);
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, bitOffsets, U16x16, uint16_t, <<)
		#endif
	}
	// bitOffset must be an immediate constant from 0 to 31, so a template argument is used.
	template <uint32_t bitOffset>
	inline U16x16 bitShiftRightImmediate(const U16x16& left) {
		static_assert(bitOffset < 16u, "Immediate right shift of 16-bit values may not shift more than 15 bits!");
		#if defined(USE_AVX2)
			return U16x16(_mm256_srli_epi16(left.v, bitOffset));
		#else
			U16x16 bitOffsets = U16x16(bitOffset);
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, bitOffsets, U16x16, uint16_t, <<)
		#endif
	}

	inline U16x16 operator+(const U16x16& left, const U16x16& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U16x16(ADD_U16_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, right, U16x16, uint16_t, +)
		#endif
	}
	inline U16x16 operator-(const U16x16& left, const U16x16& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U16x16(SUB_U16_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, right, U16x16, uint16_t, -)
		#endif
	}
	inline U16x16 operator*(const U16x16& left, const U16x16& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U16x16(MUL_U16_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_16_LANES(left, right, U16x16, uint16_t, *)
		#endif
	}
	inline U8x32 operator+(const U8x32& left, const U8x32& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U8x32(ADD_U8_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_32_LANES(left, right, U8x32, uint8_t, +)
		#endif
	}
	inline U8x32 operator-(const U8x32& left, const U8x32& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U8x32(SUB_U8_SIMD256(left.v, right.v));
		#else
			IMPL_SCALAR_REFERENCE_INFIX_32_LANES(left, right, U8x32, uint8_t, -)
		#endif
	}
	inline U8x32 saturatedAddition(const U8x32& left, const U8x32& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U8x32(ADD_SAT_U8_SIMD256(left.v, right.v));
		#else
			U8x32 result = U8x32::create_dangerous_uninitialized();
			for (int i = 0; i < 32; i++) {
				result.scalars[i] = impl_limit255((uint32_t)left.scalars[i] + (uint32_t)right.scalars[i]);
			}
			return result;
		#endif
	}
	inline U8x32 saturatedSubtraction(const U8x32& left, const U8x32& right) {
		#if defined(USE_256BIT_X_SIMD)
			return U8x32(SUB_SAT_U8_SIMD256(left.v, right.v));
		#else
			U8x32 result = U8x32::create_dangerous_uninitialized();
			for (int i = 0; i < 32; i++) {
				result.scalars[i] = impl_limit0((int32_t)left.scalars[i] - (int32_t)right.scalars[i]);
			}
			return result;
		#endif
	}

	inline I32x8 truncateToI32(const F32x8& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return I32x8(F32_TO_I32_SIMD256(vector.v));
		#else
			return I32x8(
			  (int32_t)vector.scalars[0], (int32_t)vector.scalars[1], (int32_t)vector.scalars[2], (int32_t)vector.scalars[3],
			  (int32_t)vector.scalars[4], (int32_t)vector.scalars[5], (int32_t)vector.scalars[6], (int32_t)vector.scalars[7]
			);
		#endif
	}
	inline U32x8 truncateToU32(const F32x8& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(F32_TO_U32_SIMD256(vector.v));
		#else
			return U32x8(
			  (uint32_t)vector.scalars[0], (uint32_t)vector.scalars[1], (uint32_t)vector.scalars[2], (uint32_t)vector.scalars[3],
			  (uint32_t)vector.scalars[4], (uint32_t)vector.scalars[5], (uint32_t)vector.scalars[6], (uint32_t)vector.scalars[7]
			);
		#endif
	}
	inline F32x8 floatFromI32(const I32x8& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return F32x8(I32_TO_F32_SIMD256(vector.v));
		#else
			return F32x8(
			  (float)vector.scalars[0], (float)vector.scalars[1], (float)vector.scalars[2], (float)vector.scalars[3],
			  (float)vector.scalars[4], (float)vector.scalars[5], (float)vector.scalars[6], (float)vector.scalars[7]
			);
		#endif
	}
	inline F32x8 floatFromU32(const U32x8& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return F32x8(U32_TO_F32_SIMD256(vector.v));
		#else
			return F32x8(
			  (float)vector.scalars[0], (float)vector.scalars[1], (float)vector.scalars[2], (float)vector.scalars[3],
			  (float)vector.scalars[4], (float)vector.scalars[5], (float)vector.scalars[6], (float)vector.scalars[7]
			);
		#endif
	}
	inline I32x8 I32FromU32(const U32x8& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return I32x8(REINTERPRET_U32_TO_I32_SIMD256(vector.v));
		#else
			return I32x8(
			  (int32_t)vector.scalars[0], (int32_t)vector.scalars[1], (int32_t)vector.scalars[2], (int32_t)vector.scalars[3],
			  (int32_t)vector.scalars[4], (int32_t)vector.scalars[5], (int32_t)vector.scalars[6], (int32_t)vector.scalars[7]
			);
		#endif
	}
	inline U32x8 U32FromI32(const I32x8& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(REINTERPRET_I32_TO_U32_SIMD256(vector.v));
		#else
			return U32x8(
			  (uint32_t)vector.scalars[0], (uint32_t)vector.scalars[1], (uint32_t)vector.scalars[2], (uint32_t)vector.scalars[3],
			  (uint32_t)vector.scalars[4], (uint32_t)vector.scalars[5], (uint32_t)vector.scalars[6], (uint32_t)vector.scalars[7]
			);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U8x32 reinterpret_U8FromU32(const U32x8& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U8x32(REINTERPRET_U32_TO_U8_SIMD256(vector.v));
		#else
			const uint8_t *source = (const uint8_t*)vector.scalars;
			U8x32 result = U8x32::create_dangerous_uninitialized();
			for (int i = 0; i < 32; i++) {
				result.scalars[i] = source[i];
			}
			return result;
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U32x8 reinterpret_U32FromU8(const U8x32& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(REINTERPRET_U8_TO_U32_SIMD256(vector.v));
		#else
			const uint32_t *source = (const uint32_t*)vector.scalars;
			return U32x8(source[0], source[1], source[2], source[3], source[4], source[5], source[6], source[7]);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U16x16 reinterpret_U16FromU32(const U32x8& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U16x16(REINTERPRET_U32_TO_U16_SIMD256(vector.v));
		#else
			const uint16_t *source = (const uint16_t*)vector.scalars;
			return U16x16(
			  source[0], source[1], source[2] , source[3] , source[4] , source[5] , source[6] , source[7] ,
			  source[8], source[9], source[10], source[11], source[12], source[13], source[14], source[15]
			);
		#endif
	}
	// Warning! Behavior depends on endianness.
	inline U32x8 reinterpret_U32FromU16(const U16x16& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(REINTERPRET_U16_TO_U32_SIMD256(vector.v));
		#else
			const uint32_t *source = (const uint32_t*)vector.scalars;
			return U32x8(source[0], source[1], source[2], source[3], source[4], source[5], source[6], source[7]);
		#endif
	}

	// Unpacking to larger integers
	inline U32x8 lowerToU32(const U16x16& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(U16_LOW_TO_U32_SIMD256(vector.v));
		#else
			return U32x8(vector.scalars[0], vector.scalars[1], vector.scalars[2], vector.scalars[3], vector.scalars[4], vector.scalars[5], vector.scalars[6], vector.scalars[7]);
		#endif
	}
	inline U32x8 higherToU32(const U16x16& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U32x8(U16_HIGH_TO_U32_SIMD256(vector.v));
		#else
			return U32x8(vector.scalars[8], vector.scalars[9], vector.scalars[10], vector.scalars[11], vector.scalars[12], vector.scalars[13], vector.scalars[14], vector.scalars[15]);
		#endif
	}
	inline U16x16 lowerToU16(const U8x32& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U16x16(U8_LOW_TO_U16_SIMD256(vector.v));
		#else
			return U16x16(
			  vector.scalars[0], vector.scalars[1], vector.scalars[2], vector.scalars[3],
			  vector.scalars[4], vector.scalars[5], vector.scalars[6], vector.scalars[7],
			  vector.scalars[8], vector.scalars[9], vector.scalars[10], vector.scalars[11],
			  vector.scalars[12], vector.scalars[13], vector.scalars[14], vector.scalars[15]
			);
		#endif
	}
	inline U16x16 higherToU16(const U8x32& vector) {
		#if defined(USE_256BIT_X_SIMD)
			return U16x16(U8_HIGH_TO_U16_SIMD256(vector.v));
		#else
			return U16x16(
			  vector.scalars[16], vector.scalars[17], vector.scalars[18], vector.scalars[19],
			  vector.scalars[20], vector.scalars[21], vector.scalars[22], vector.scalars[23],
			  vector.scalars[24], vector.scalars[25], vector.scalars[26], vector.scalars[27],
			  vector.scalars[28], vector.scalars[29], vector.scalars[30], vector.scalars[31]
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
	inline F32x8 operator-(const F32x8& value) {
		#if defined(USE_256BIT_F_SIMD)
			return F32x8(0.0f) - value;
		#else
			return F32x8(
			  -value.scalars[0], -value.scalars[1], -value.scalars[2], -value.scalars[3],
			  -value.scalars[4], -value.scalars[5], -value.scalars[6], -value.scalars[7]
			);
		#endif
	}
	inline I32x8 operator-(const I32x8& value) {
		#if defined(USE_256BIT_X_SIMD)
			return I32x8(0) - value;
		#else
			return I32x8(
			  -value.scalars[0], -value.scalars[1], -value.scalars[2], -value.scalars[3],
			  -value.scalars[4], -value.scalars[5], -value.scalars[6], -value.scalars[7]
			);
		#endif
	}

	// Helper macros for generating the vector extract functions.
	//   Having one function for each type and offset makes sure that the compiler gets an immediate integer within the valid range.
	#if defined(USE_AVX2)
		// AVX2 does not offer any 256-bit element extraction, only two 128-bit shifts done in parallel, so we might as well use two separate 128-bit extractions.
		// The __m256i type should never be returned from a non-intrinsic function, because g++ does not automatically enforce
		//   32 byte alignment for __m256i vectors when creating temporary variables in the generated assembler instructions.
		template <typename T, int INNER_OFFSET, int EDGE_HALF_INDEX, int MIDDLE_HALF_INDEX>
		inline T impl_extractBytes_AVX2(const T &leftInput, const T &middleInput, const T &rightInput) {
			static_assert(0 <= INNER_OFFSET && INNER_OFFSET < 16, "impl_extractBytes_AVX2: INNER_OFFSET is out of bound 0..15!\n");
			static_assert(0 <= EDGE_HALF_INDEX && EDGE_HALF_INDEX < 2, "impl_extractBytes_AVX2: INNER_OFFSET is out of bound 0..1!n");
			static_assert(0 <= MIDDLE_HALF_INDEX && MIDDLE_HALF_INDEX < 2, "impl_extractBytes_AVX2: INNER_OFFSET is out of bound 0..1!\n");
			// Extract three halves depending on which ones overlap with the offset.
			ALIGN16 __m128i leftPart   = _mm256_extractf128_si256(leftInput.v  , EDGE_HALF_INDEX  );
			ALIGN16 __m128i middlePart = _mm256_extractf128_si256(middleInput.v, MIDDLE_HALF_INDEX);
			ALIGN16 __m128i rightPart  = _mm256_extractf128_si256(rightInput.v , EDGE_HALF_INDEX  );
			// Make two 128-bit vector extractions.
			ALIGN16 __m128i leftResult = _mm_alignr_epi8(leftPart, middlePart, INNER_OFFSET);
			ALIGN16 __m128i rightResult = _mm_alignr_epi8(middlePart, rightPart, INNER_OFFSET);
			// Combine the results.
			ALIGN32 __m256i result = _mm256_set_m128i(leftResult, rightResult);
			return T(result);
		}
		template <typename T, int INNER_OFFSET, int EDGE_HALF_INDEX, int MIDDLE_HALF_INDEX>
		inline T impl_extractBytes_AVX2_F32(const T &leftInput, const T &middleInput, const T &rightInput) {
			static_assert(0 <= INNER_OFFSET && INNER_OFFSET < 16, "impl_extractBytes_AVX2: INNER_OFFSET is out of bound 0..15!\n");
			static_assert(0 <= EDGE_HALF_INDEX && EDGE_HALF_INDEX < 2, "impl_extractBytes_AVX2: INNER_OFFSET is out of bound 0..1!n");
			static_assert(0 <= MIDDLE_HALF_INDEX && MIDDLE_HALF_INDEX < 2, "impl_extractBytes_AVX2: INNER_OFFSET is out of bound 0..1!\n");
			// Extract three halves depending on which ones overlap with the offset.
			ALIGN16 __m128i leftPart   = _mm256_extractf128_si256(__m256i(leftInput.v)  , EDGE_HALF_INDEX  );
			ALIGN16 __m128i middlePart = _mm256_extractf128_si256(__m256i(middleInput.v), MIDDLE_HALF_INDEX);
			ALIGN16 __m128i rightPart  = _mm256_extractf128_si256(__m256i(rightInput.v) , EDGE_HALF_INDEX  );
			// Make two 128-bit vector extractions.
			ALIGN16 __m128i leftResult = _mm_alignr_epi8(leftPart, middlePart, INNER_OFFSET);
			ALIGN16 __m128i rightResult = _mm_alignr_epi8(middlePart, rightPart, INNER_OFFSET);
			// Combine the results.
			ALIGN32 __m256i result = _mm256_set_m128i(leftResult, rightResult);
			return T(__m256(result));
		}
		#define VECTOR_EXTRACT_GENERATOR_256(METHOD_NAME, VECTOR_TYPE, OFFSET, A, B) \
			METHOD_NAME<VECTOR_TYPE, (OFFSET) - ((OFFSET) < 16 ? 0 : 16), (OFFSET) < 16 ? 0 : 1, (OFFSET) < 16 ? 1 : 0> ((B), (OFFSET) < 16 ? (A) : (B), (A))
		#define VECTOR_EXTRACT_GENERATOR_256_U8( OFFSET) return U8x32 (VECTOR_EXTRACT_GENERATOR_256(impl_extractBytes_AVX2    , U8x32 , OFFSET    , a, b));
		#define VECTOR_EXTRACT_GENERATOR_256_U16(OFFSET) return U16x16(VECTOR_EXTRACT_GENERATOR_256(impl_extractBytes_AVX2    , U16x16, OFFSET * 2, a, b));
		#define VECTOR_EXTRACT_GENERATOR_256_U32(OFFSET) return U32x8 (VECTOR_EXTRACT_GENERATOR_256(impl_extractBytes_AVX2    , U32x8 , OFFSET * 4, a, b));
		#define VECTOR_EXTRACT_GENERATOR_256_I32(OFFSET) return I32x8 (VECTOR_EXTRACT_GENERATOR_256(impl_extractBytes_AVX2    , I32x8 , OFFSET * 4, a, b));
		#define VECTOR_EXTRACT_GENERATOR_256_F32(OFFSET) return F32x8 (VECTOR_EXTRACT_GENERATOR_256(impl_extractBytes_AVX2_F32, F32x8 , OFFSET * 4, a, b));
	#else
		// TODO: Implement bound checks for scalars in debug mode. A static index can be checked in compile time.
		template<typename T, int ELEMENT_COUNT, int OFFSET>
		T impl_vectorExtract_emulated(const T &a, const T &b) {
			static_assert(0 <= OFFSET && OFFSET <= ELEMENT_COUNT, "Offset is out of bound in impl_vectorExtract_emulated!\n");
			static_assert(sizeof(a.scalars) == sizeof(a.scalars[0]) * ELEMENT_COUNT, "A does not match the element count in impl_vectorExtract_emulated!\n");
			static_assert(sizeof(b.scalars) == sizeof(b.scalars[0]) * ELEMENT_COUNT, "B does not match the element count in impl_vectorExtract_emulated!\n");
			T result = T::create_dangerous_uninitialized();
			static_assert(sizeof(result.scalars) == sizeof(result.scalars[0]) * ELEMENT_COUNT, "The result does not match the element count in impl_vectorExtract_emulated!\n");
			int t = 0;
			for (int s = OFFSET; s < ELEMENT_COUNT; s++) {
				assert(0 <= s && s < ELEMENT_COUNT);
				assert(0 <= t && t < ELEMENT_COUNT);
				result.scalars[t] = a.scalars[s];
				t++;
			}
			for (int s = 0; s < OFFSET; s++) {
				assert(0 <= s && s < ELEMENT_COUNT);
				assert(0 <= t && t < ELEMENT_COUNT);
				result.scalars[t] = b.scalars[s];
				t++;
			}
			return result;
		}
		#define VECTOR_EXTRACT_GENERATOR_256_U8( OFFSET) return impl_vectorExtract_emulated< U8x32, 32, OFFSET>(a, b);
		#define VECTOR_EXTRACT_GENERATOR_256_U16(OFFSET) return impl_vectorExtract_emulated<U16x16, 16, OFFSET>(a, b);
		#define VECTOR_EXTRACT_GENERATOR_256_U32(OFFSET) return impl_vectorExtract_emulated< U32x8,  8, OFFSET>(a, b);
		#define VECTOR_EXTRACT_GENERATOR_256_I32(OFFSET) return impl_vectorExtract_emulated< I32x8,  8, OFFSET>(a, b);
		#define VECTOR_EXTRACT_GENERATOR_256_F32(OFFSET) return impl_vectorExtract_emulated< F32x8,  8, OFFSET>(a, b);
	#endif

	// Vector extraction concatunates two input vectors and reads a vector between them using an offset.
	//   The first and last offsets that only return one of the inputs can be used for readability, because they will be inlined and removed by the compiler.
	//   To get elements from the right side, combine the center vector with the right vector and shift one element to the left using vectorExtract_1 for the given type.
	//   To get elements from the left side, combine the left vector with the center vector and shift one element to the right using vectorExtract_15 for 16 lanes, vectorExtract_7 for 8 lanes, or vectorExtract_3 for 4 lanes.

	// TODO: Also allow using a template arguments as the element offset with a static assert for the offset, which might be useful in template programming.

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
	#if defined(USE_AVX2)
		#define GATHER_I32x8_AVX2(SOURCE, EIGHT_OFFSETS, SCALE) _mm256_i32gather_epi32((const int32_t*)(SOURCE), EIGHT_OFFSETS, SCALE)
		#define GATHER_U32x8_AVX2(SOURCE, EIGHT_OFFSETS, SCALE) _mm256_i32gather_epi32((const int32_t*)(SOURCE), EIGHT_OFFSETS, SCALE)
		#define GATHER_F32x8_AVX2(SOURCE, EIGHT_OFFSETS, SCALE) _mm256_i32gather_ps((const float*)(SOURCE), EIGHT_OFFSETS, SCALE)
	#endif
	static inline U32x8 gather_U32(dsr::SafePointer<const uint32_t> data, const U32x8 &elementOffset) {
		#ifdef SAFE_POINTER_CHECKS
			ALIGN32 uint32_t elementOffsets[8];
			if (uintptr_t((void*)elementOffsets) & 31u) { throwError(U"Unaligned stack memory detected in 256-bit gather_U32!\n"); }
			elementOffset.writeAlignedUnsafe(elementOffsets);
			data.assertInside("U32x4 gather_U32 lane 0", (data + elementOffsets[0]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 1", (data + elementOffsets[1]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 2", (data + elementOffsets[2]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 3", (data + elementOffsets[3]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 4", (data + elementOffsets[4]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 5", (data + elementOffsets[5]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 6", (data + elementOffsets[6]).getUnchecked());
			data.assertInside("U32x4 gather_U32 lane 7", (data + elementOffsets[7]).getUnchecked());
		#endif
		#if defined(USE_AVX2)
			return U32x8(GATHER_I32x8_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			#ifndef SAFE_POINTER_CHECKS
				ALIGN32 uint32_t elementOffsets[8];
				elementOffset.writeAlignedUnsafe(elementOffsets);
			#endif
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
	static inline I32x8 gather_I32(dsr::SafePointer<const int32_t> data, const U32x8 &elementOffset) {
		#ifdef SAFE_POINTER_CHECKS
			ALIGN32 uint32_t elementOffsets[8];
			if (uintptr_t((void*)elementOffsets) & 31u) { throwError(U"Unaligned stack memory detected in 256-bit gather_I32!\n"); }
			elementOffset.writeAlignedUnsafe(elementOffsets);
			data.assertInside("I32x4 gather_I32 lane 0", (data + elementOffsets[0]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 1", (data + elementOffsets[1]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 2", (data + elementOffsets[2]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 3", (data + elementOffsets[3]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 4", (data + elementOffsets[4]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 5", (data + elementOffsets[5]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 6", (data + elementOffsets[6]).getUnchecked());
			data.assertInside("I32x4 gather_I32 lane 7", (data + elementOffsets[7]).getUnchecked());
		#endif
		#if defined(USE_AVX2)
			return I32x8(GATHER_U32x8_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			#ifndef SAFE_POINTER_CHECKS
				ALIGN32 uint32_t elementOffsets[8];
				elementOffset.writeAlignedUnsafe(elementOffsets);
			#endif
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
	static inline F32x8 gather_F32(dsr::SafePointer<const float> data, const U32x8 &elementOffset) {
		#ifdef SAFE_POINTER_CHECKS
			ALIGN32 uint32_t elementOffsets[8];
			if (uintptr_t((void*)elementOffsets) & 31u) { throwError(U"Unaligned stack memory detected in 256-bit gather_F32!\n"); }
			elementOffset.writeAlignedUnsafe(elementOffsets);
			data.assertInside("F32x4 gather_F32 lane 0", (data + elementOffsets[0]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 1", (data + elementOffsets[1]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 2", (data + elementOffsets[2]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 3", (data + elementOffsets[3]).getUnchecked());
			data.assertInside("F32x4 gather_I32 lane 4", (data + elementOffsets[4]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 5", (data + elementOffsets[5]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 6", (data + elementOffsets[6]).getUnchecked());
			data.assertInside("F32x4 gather_F32 lane 7", (data + elementOffsets[7]).getUnchecked());
		#endif
		#if defined(USE_AVX2)
			return F32x8(GATHER_F32x8_AVX2(data.getUnsafe(), elementOffset.v, 4));
		#else
			#ifndef SAFE_POINTER_CHECKS
				ALIGN32 uint32_t elementOffsets[8];
				elementOffset.writeAlignedUnsafe(elementOffsets);
			#endif
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

	// TODO: Move to noSimd.h using SFINAE.
	// Wrapper functions for explicitly expanding scalars into vectors during math operations.
	#define NUMERICAL_SCALAR_OPERATIONS(VECTOR_TYPE, ELEMENT_TYPE, LANE_COUNT) \
		inline VECTOR_TYPE operator+(const VECTOR_TYPE& left, ELEMENT_TYPE right) { return left + VECTOR_TYPE(right); } \
		inline VECTOR_TYPE operator+(ELEMENT_TYPE left, const VECTOR_TYPE& right) { return VECTOR_TYPE(left) + right; } \
		inline VECTOR_TYPE operator-(const VECTOR_TYPE& left, ELEMENT_TYPE right) { return left - VECTOR_TYPE(right); } \
		inline VECTOR_TYPE operator-(ELEMENT_TYPE left, const VECTOR_TYPE& right) { return VECTOR_TYPE(left) - right; }
		FOR_ALL_VECTOR_TYPES(NUMERICAL_SCALAR_OPERATIONS)
	#undef NUMERICAL_SCALAR_OPERATIONS

	// TODO: Move to noSimd.h using SFINAE.
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

	// TODO: Move to noSimd.h using SFINAE.
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

	// 1 / value
	inline F32x4 reciprocal(const F32x4 &value) {
		#if defined(USE_BASIC_SIMD)
			#if defined(USE_SSE2)
				// Approximate
				ALIGN16 SIMD_F32x4 lowQS = _mm_rcp_ps(value.v);
				F32x4 lowQ = F32x4(lowQS);
				// Refine
				return ((lowQ + lowQ) - (value * lowQ * lowQ));
			#elif defined(USE_NEON)
				// Approximate
				ALIGN16 SIMD_F32x4 result = vrecpeq_f32(value.v);
				// Refine
				ALIGN16 SIMD_F32x4 a = vrecpsq_f32(value.v, result);
				result = MUL_F32_SIMD(a, result);
				return F32x4(MUL_F32_SIMD(vrecpsq_f32(value.v, result), result));
			#else
				#error "Missing F32x4 implementation of reciprocal!\n");
				return F32x4(0);
			#endif
		#else
			F32x4 one = F32x4(1.0f);
			IMPL_SCALAR_REFERENCE_INFIX_4_LANES(one, value, F32x4, float, /)
		#endif
	}

	// 1 / value
	inline F32x8 reciprocal(const F32x8 &value) {
		#if defined(USE_AVX2)
			// Approximate
			ALIGN32 SIMD_F32x8 lowQ = _mm256_rcp_ps(value.v);
			// Refine
			return F32x8(SUB_F32_SIMD256(ADD_F32_SIMD256(lowQ, lowQ), MUL_F32_SIMD256(value.v, MUL_F32_SIMD256(lowQ, lowQ))));
		#else
			F32x8 one = F32x8(1.0f);
			IMPL_SCALAR_REFERENCE_INFIX_8_LANES(one, value, F32x8, float, /)
		#endif
	}

	// 1 / sqrt(value)
	inline F32x4 reciprocalSquareRoot(const F32x4 &value) {
		#if defined(USE_BASIC_SIMD)
			#if defined(USE_SSE2)
				ALIGN16 SIMD_F32x4 reciRootS = _mm_rsqrt_ps(value.v);
				F32x4 reciRoot = F32x4(reciRootS);
				F32x4 mul = value * reciRoot * reciRoot;
				return (reciRoot * 0.5f) * (3.0f - mul);
			#elif defined(USE_NEON)
				// Approximate
				ALIGN16 SIMD_F32x4 reciRoot = vrsqrteq_f32(value.v);
				// Refine
				ALIGN16 SIMD_F32x4 a = MUL_F32_SIMD(value.v, reciRoot);
				ALIGN16 SIMD_F32x4 b = vrsqrtsq_f32(a, reciRoot);
				ALIGN16 SIMD_F32x4 c = MUL_F32_SIMD(b, reciRoot);
				return F32x4(c);
			#else
				static_assert(false, "Missing SIMD implementation of reciprocalSquareRoot!\n");
				return F32x4(0);
			#endif
		#else
			return F32x4(1.0f / sqrt(value.scalars[0]), 1.0f / sqrt(value.scalars[1]), 1.0f / sqrt(value.scalars[2]), 1.0f / sqrt(value.scalars[3]));
		#endif
	}

	// 1 / sqrt(value)
	inline F32x8 reciprocalSquareRoot(const F32x8 &value) {
		#if defined(USE_AVX2)
			ALIGN32 SIMD_F32x8 reciRootS = _mm256_rsqrt_ps(value.v);
			F32x8 reciRoot = F32x8(reciRootS);
			F32x8 mul = value * reciRoot * reciRoot;
			return (reciRoot * 0.5f) * (3.0f - mul);
		#else
			return F32x8(
			  1.0f / sqrt(value.scalars[0]),
			  1.0f / sqrt(value.scalars[1]),
			  1.0f / sqrt(value.scalars[2]),
			  1.0f / sqrt(value.scalars[3]),
			  1.0f / sqrt(value.scalars[4]),
			  1.0f / sqrt(value.scalars[5]),
			  1.0f / sqrt(value.scalars[6]),
			  1.0f / sqrt(value.scalars[7])
			);
		#endif
	}

	// sqrt(value)
	inline F32x4 squareRoot(const F32x4 &value) {
		#if defined(USE_BASIC_SIMD)
			#if defined(USE_SSE2)
				ALIGN16 SIMD_F32x4 half = _mm_set1_ps(0.5f);
				// Approximate
				ALIGN16 SIMD_F32x4 root = _mm_sqrt_ps(value.v);
				// Refine
				root = _mm_mul_ps(_mm_add_ps(root, _mm_div_ps(value.v, root)), half);
				return F32x4(root);
			#else
				return reciprocalSquareRoot(value) * value;
			#endif
		#else
			return F32x4(sqrt(value.scalars[0]), sqrt(value.scalars[1]), sqrt(value.scalars[2]), sqrt(value.scalars[3]));
		#endif
	}

	// sqrt(value)
	inline F32x8 squareRoot(const F32x8 &value) {
		#if defined(USE_AVX2)
			ALIGN32 SIMD_F32x8 half = _mm256_set1_ps(0.5f);
			// Approximate
			ALIGN32 SIMD_F32x8 root = _mm256_sqrt_ps(value.v);
			// Refine
			root = _mm256_mul_ps(_mm256_add_ps(root, _mm256_div_ps(value.v, root)), half);
			return F32x8(root);
		#else
			return F32x8(
			  sqrt(value.scalars[0]),
			  sqrt(value.scalars[1]),
			  sqrt(value.scalars[2]),
			  sqrt(value.scalars[3]),
			  sqrt(value.scalars[4]),
			  sqrt(value.scalars[5]),
			  sqrt(value.scalars[6]),
			  sqrt(value.scalars[7]));
		#endif
	}

	// TODO: Let SVE define completely separate types for dynamic vectors.
	// The X vectors using the longest SIMD length that is efficient to use for both floating-point and integer types.
	//   DSR_DEFAULT_ALIGNMENT
	//     The number of bytes memory should be aligned with by default when creating buffers and images.
	//   F32xX
	//     The longest available SIMD vector for storing 32-bit float values. Iterating laneCountX_32Bit floats at a time.
	//   I32xX
	//     The longest available SIMD vector for storing signed 32-bit integer values. Iterating laneCountX_32Bit integers at a time.
	//   U32xX
	//     The longest available SIMD vector for storing unsigned 32-bit integer values. Iterating laneCountX_32Bit integers at a time.
	//   U16xX
	//     The longest available SIMD vector for storing unsigned 16-bit integer values. Iterating laneCountX_16Bit integers at a time.
	//   U8xX
	//     The longest available SIMD vector for storing unsigned 8-bit integer values. Iterating laneCountX_8Bit integers at a time.
	#if defined(USE_256BIT_X_SIMD) || defined(EMULATE_256BIT_X_SIMD)
		// Using 256-bit SIMD
		#define DSR_DEFAULT_VECTOR_SIZE 32
		#define DSR_DEFAULT_ALIGNMENT 32
		using F32xX = F32x8;
		using I32xX = I32x8;
		using U32xX = U32x8;
		using U16xX = U16x16;
		using U8xX = U8x32;
		// Align memory with 256 bits to allow overwriting padding at the end of each pixel row.
		//   Otherwise you would have to preserve data at the end of each row with slow and bloated duplicated code in every filter.
	#else
		// If there is no hardware support for 256-bit vectors, the emulation of 256-bit vectors when used explicitly, is allowed to be aligned with just 128 bits.
		#define DSR_DEFAULT_VECTOR_SIZE 16
		#define DSR_DEFAULT_ALIGNMENT 16
		using F32xX = F32x4;
		using I32xX = I32x4;
		using U32xX = U32x4;
		using U16xX = U16x8;
		using U8xX = U8x16;
	#endif

	// How many lanes do the longest available vector have for a specified lane size.
	//   Used to iterate indices and pointers using whole elements.
	static const int laneCountX_32Bit = DSR_DEFAULT_VECTOR_SIZE / 4;
	static const int laneCountX_16Bit = DSR_DEFAULT_VECTOR_SIZE / 2;
	static const int laneCountX_8Bit = DSR_DEFAULT_VECTOR_SIZE;

	// TODO: Let SVE define completely separate types for dynamic vectors.
	// The F vector using the longest SIMD length that is efficient to use when only processing float values, even if no integer types are available in the same size.
	//   Used when you know that your algorithm is only going to work with float types and you need the extra performance.
	//     Some processors have AVX but not AVX2, meaning that it has 256-bit SIMD for floats, but only 128-bit SIMD for integers.
	//   F32xF
	//     The longest available SIMD vector for storing 32-bit float values. Iterating laneCountF_32Bit floats at a time.
	#if defined(USE_256BIT_F_SIMD) || defined(EMULATE_256BIT_F_SIMD)
		#define DSR_FLOAT_VECTOR_SIZE 32
		#define DSR_FLOAT_ALIGNMENT 32
		using F32xF = F32x8;
	#else
		// F vectors are 128-bits.
		#define DSR_FLOAT_VECTOR_SIZE 16
		#define DSR_FLOAT_ALIGNMENT 16
		using F32xF = F32x4;
	#endif
	// Used to iterate over float pointers when using F32xF.
	static const int laneCountF = DSR_FLOAT_VECTOR_SIZE / 4;

	// Define traits.
	DSR_APPLY_PROPERTY(DsrTrait_Any_U8 , U8x16)
	DSR_APPLY_PROPERTY(DsrTrait_Any_U8 , U8x32)
	DSR_APPLY_PROPERTY(DsrTrait_Any_U16, U16x8)
	DSR_APPLY_PROPERTY(DsrTrait_Any_U16, U16x16)
	DSR_APPLY_PROPERTY(DsrTrait_Any_U32, U32x4)
	DSR_APPLY_PROPERTY(DsrTrait_Any_U32, U32x8)
	DSR_APPLY_PROPERTY(DsrTrait_Any_I32, I32x4)
	DSR_APPLY_PROPERTY(DsrTrait_Any_I32, I32x8)
	DSR_APPLY_PROPERTY(DsrTrait_Any_F32, F32x4)
	DSR_APPLY_PROPERTY(DsrTrait_Any_F32, F32x8)
	DSR_APPLY_PROPERTY(DsrTrait_Any , U8x16)
	DSR_APPLY_PROPERTY(DsrTrait_Any , U8x32)
	DSR_APPLY_PROPERTY(DsrTrait_Any, U16x8)
	DSR_APPLY_PROPERTY(DsrTrait_Any, U16x16)
	DSR_APPLY_PROPERTY(DsrTrait_Any, U32x4)
	DSR_APPLY_PROPERTY(DsrTrait_Any, U32x8)
	DSR_APPLY_PROPERTY(DsrTrait_Any, I32x4)
	DSR_APPLY_PROPERTY(DsrTrait_Any, I32x8)
	DSR_APPLY_PROPERTY(DsrTrait_Any, F32x4)
	DSR_APPLY_PROPERTY(DsrTrait_Any, F32x8)

	// TODO: Use as independent types when the largest vector lengths are not known in compile time on ARM SVE.
	//DSR_APPLY_PROPERTY(DsrTrait_Any_U8 , U8xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any_U16, U16xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any_U32, U32xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any_I32, I32xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any_F32, F32xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any_F32, F32xF)
	//DSR_APPLY_PROPERTY(DsrTrait_Any , U8xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any, U16xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any, U32xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any, I32xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any, F32xX)
	//DSR_APPLY_PROPERTY(DsrTrait_Any, F32xF)

	}

#endif
