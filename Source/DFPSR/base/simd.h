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
	#define ALIGN16  __attribute__((aligned(16)))

	// To allow turning off SIMD intrinsics for testing
	#ifdef __SSE2__
		// Comment out this line to test without SSE2
		#define USE_SSE2
	#elif __ARM_NEON
		// Comment out this line to test without NEON
		#define USE_NEON
	#endif

	// Everything declared in here handles things specific for SSE.
	// Direct use of the macros will not provide portability to all hardware.
	#ifdef USE_SSE2
		#define USE_BASIC_SIMD
		#define USE_DIRECT_SIMD_MEMORY_ACCESS
		#include <emmintrin.h> // SSE2

		#ifdef __AVX2__
			#include <immintrin.h> // AVX2
			#define GATHER_U32_AVX2(SOURCE, FOUR_OFFSETS, SCALE) _mm_i32gather_epi32((const int32_t*)(SOURCE), FOUR_OFFSETS, SCALE)
			// Comment out this line to test without AVX2
			#define USE_AVX2
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
		inline SIMD_U8x16 PACK_SAT_U16_TO_U8(const SIMD_U16x8& a, const SIMD_U16x8& b) {
			SIMD_U16x8 mask, a2, b2;
			mask = _mm_set1_epi16(0x7fff);
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
		// No 8-bit multiplications

		// Statistics
		#define MIN_F32_SIMD(A, B) _mm_min_ps(A, B)
		#define MAX_F32_SIMD(A, B) _mm_max_ps(A, B)

		// Bitwise
		#define BITWISE_AND_U32_SIMD(A, B) _mm_and_si128(A, B)
		#define BITWISE_OR_U32_SIMD(A, B) _mm_or_si128(A, B)
	#endif

	// Everything declared in here handles things specific for NEON.
	// Direct use of the macros will not provide portability to all hardware.
	#ifdef USE_NEON
		#define USE_BASIC_SIMD
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
		#define PACK_SAT_U16_TO_U8(A, B) vcombine_u8(vqmovn_u16(A), vqmovn_u16(B))

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
		// No 8-bit multiplications

		// Statistics
		#define MIN_F32_SIMD(A, B) vminq_f32(A, B)
		#define MAX_F32_SIMD(A, B) vmaxq_f32(A, B)

		// Bitwise
		#define BITWISE_AND_U32_SIMD(A, B) vandq_u32(A, B)
		#define BITWISE_OR_U32_SIMD(A, B) vorrq_u32(A, B)
	#endif

	/*
	The vector types (F32x4, I32x4, U32x4, U16x8) below are supposed to be portable across different CPU architectures.
	When this abstraction layer is mixed with handwritten SIMD intrinsics:
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
			Will not work when either SSE or NEON is enabled.
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
					//__m128 reciRoot = _mm_rsqrt_ps(this->v);
					SIMD_F32x4 reciRoot = _mm_rsqrt_ps(this->v);
					SIMD_F32x4 mul = MUL_F32_SIMD(MUL_F32_SIMD(this->v, reciRoot), reciRoot);
					reciRoot = MUL_F32_SIMD(MUL_F32_SIMD(LOAD_SCALAR_F32_SIMD(0.5f), reciRoot), SUB_F32_SIMD(LOAD_SCALAR_F32_SIMD(3.0f), mul));
					return F32x4(reciRoot);
				#elif USE_NEON
					// TODO: Test on ARM
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
					// TODO: Test on ARM
					return F32x4(MUL_F32_SIMD(this->v, this->reciprocalSquareRoot().v));
				#else
					assert(false);
					return F32x4(0);
				#endif
			#else
				return F32x4(sqrt(this->emulated[0]), sqrt(this->emulated[1]), sqrt(this->emulated[2]), sqrt(this->emulated[3]));
			#endif
		}
		F32x4 clamp(float min, float max) const {
			#ifdef USE_BASIC_SIMD
				return F32x4(MIN_F32_SIMD(MAX_F32_SIMD(this->v, LOAD_SCALAR_F32_SIMD(min)), LOAD_SCALAR_F32_SIMD(max)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				if (min > val0) { val0 = min; }
				if (max < val0) { val0 = max; }
				if (min > val1) { val1 = min; }
				if (max < val1) { val1 = max; }
				if (min > val2) { val2 = min; }
				if (max < val2) { val2 = max; }
				if (min > val3) { val3 = min; }
				if (max < val3) { val3 = max; }
				return F32x4(val0, val1, val2, val3);
			#endif
		}
		F32x4 clampLower(float min) const {
			#ifdef USE_BASIC_SIMD
				return F32x4(MAX_F32_SIMD(this->v, LOAD_SCALAR_F32_SIMD(min)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				if (min > val0) { val0 = min; }
				if (min > val1) { val1 = min; }
				if (min > val2) { val2 = min; }
				if (min > val3) { val3 = min; }
				return F32x4(val0, val1, val2, val3);
			#endif
		}
		F32x4 clampUpper(float max) const {
			#ifdef USE_BASIC_SIMD
				return F32x4(MIN_F32_SIMD(this->v, LOAD_SCALAR_F32_SIMD(max)));
			#else
				float val0 = this->emulated[0];
				float val1 = this->emulated[1];
				float val2 = this->emulated[2];
				float val3 = this->emulated[3];
				if (max < val0) { val0 = max; }
				if (max < val1) { val1 = max; }
				if (max < val2) { val2 = max; }
				if (max < val3) { val3 = max; }
				return F32x4(val0, val1, val2, val3);
			#endif
		}
	};
	inline dsr::String& string_toStreamIndented(dsr::String& target, const F32x4& source, const dsr::ReadableString& indentation) {
		string_append(target, indentation, source.get());
		return target;
	}
	inline bool operator==(const F32x4& left, const F32x4& right) {
		float a[4] ALIGN16;
		float b[4] ALIGN16;
		left.writeAlignedUnsafe(a);
		right.writeAlignedUnsafe(b);
		return fabs(a[0] - b[0]) < 0.0001f && fabs(a[1] - b[1]) < 0.0001f && fabs(a[2] - b[2]) < 0.0001f && fabs(a[3] - b[3]) < 0.0001f;
	}
	inline bool operator!=(const F32x4& left, const F32x4& right) {
		return !(left == right);
	}
	inline F32x4 operator+(const F32x4& left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(ADD_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.emulated[0] + right.emulated[0], left.emulated[1] + right.emulated[1], left.emulated[2] + right.emulated[2], left.emulated[3] + right.emulated[3]);
		#endif
	}
	inline F32x4 operator+(float left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(ADD_F32_SIMD(LOAD_SCALAR_F32_SIMD(left), right.v));
		#else
			return F32x4(left + right.emulated[0], left + right.emulated[1], left + right.emulated[2], left + right.emulated[3]);
		#endif
	}
	inline F32x4 operator+(const F32x4& left, float right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(ADD_F32_SIMD(left.v, LOAD_SCALAR_F32_SIMD(right)));
		#else
			return F32x4(left.emulated[0] + right, left.emulated[1] + right, left.emulated[2] + right, left.emulated[3] + right);
		#endif
	}
	inline F32x4 operator-(const F32x4& left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(SUB_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.emulated[0] - right.emulated[0], left.emulated[1] - right.emulated[1], left.emulated[2] - right.emulated[2], left.emulated[3] - right.emulated[3]);
		#endif
	}
	inline F32x4 operator-(float left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(SUB_F32_SIMD(LOAD_SCALAR_F32_SIMD(left), right.v));
		#else
			return F32x4(left - right.emulated[0], left - right.emulated[1], left - right.emulated[2], left - right.emulated[3]);
		#endif
	}
	inline F32x4 operator-(const F32x4& left, float right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(SUB_F32_SIMD(left.v, LOAD_SCALAR_F32_SIMD(right)));
		#else
			return F32x4(left.emulated[0] - right, left.emulated[1] - right, left.emulated[2] - right, left.emulated[3] - right);
		#endif
	}
	inline F32x4 operator*(const F32x4& left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(MUL_F32_SIMD(left.v, right.v));
		#else
			return F32x4(left.emulated[0] * right.emulated[0], left.emulated[1] * right.emulated[1], left.emulated[2] * right.emulated[2], left.emulated[3] * right.emulated[3]);
		#endif
	}
	inline F32x4 operator*(float left, const F32x4& right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(MUL_F32_SIMD(LOAD_SCALAR_F32_SIMD(left), right.v));
		#else
			return F32x4(left * right.emulated[0], left * right.emulated[1], left * right.emulated[2], left * right.emulated[3]);
		#endif
	}
	inline F32x4 operator*(const F32x4& left, float right) {
		#ifdef USE_BASIC_SIMD
			return F32x4(MUL_F32_SIMD(left.v, LOAD_SCALAR_F32_SIMD(right)));
		#else
			return F32x4(left.emulated[0] * right, left.emulated[1] * right, left.emulated[2] * right, left.emulated[3] * right);
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
	inline dsr::String& string_toStreamIndented(dsr::String& target, const I32x4& source, const dsr::ReadableString& indentation) {
		string_append(target, indentation, source.get());
		return target;
	}
	inline bool operator==(const I32x4& left, const I32x4& right) {
		int32_t a[4] ALIGN16;
		int32_t b[4] ALIGN16;
		left.writeAlignedUnsafe(a);
		right.writeAlignedUnsafe(b);
		return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
	}
	inline bool operator!=(const I32x4& left, const I32x4& right) {
		return !(left == right);
	}
	inline I32x4 operator+(const I32x4& left, const I32x4& right) {
		#ifdef USE_BASIC_SIMD
			return I32x4(ADD_I32_SIMD(left.v, right.v));
		#else
			return I32x4(left.emulated[0] + right.emulated[0], left.emulated[1] + right.emulated[1], left.emulated[2] + right.emulated[2], left.emulated[3] + right.emulated[3]);
		#endif
	}
	inline I32x4 operator+(int32_t left, const I32x4& right) {
		return I32x4(left) + right;
	}
	inline I32x4 operator+(const I32x4& left, int32_t right) {
		return left + I32x4(right);
	}
	inline I32x4 operator-(const I32x4& left, const I32x4& right) {
		#ifdef USE_BASIC_SIMD
			return I32x4(SUB_I32_SIMD(left.v, right.v));
		#else
			return I32x4(left.emulated[0] - right.emulated[0], left.emulated[1] - right.emulated[1], left.emulated[2] - right.emulated[2], left.emulated[3] - right.emulated[3]);
		#endif
	}
	inline I32x4 operator-(int32_t left, const I32x4& right) {
		return I32x4(left) - right;
	}
	inline I32x4 operator-(const I32x4& left, int32_t right) {
		return left - I32x4(right);
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
	inline I32x4 operator*(int32_t left, const I32x4& right) {
		return I32x4(left) * right;
	}
	inline I32x4 operator*(const I32x4& left, int32_t right) {
		return left * I32x4(right);
	}

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
	inline dsr::String& string_toStreamIndented(dsr::String& target, const U32x4& source, const dsr::ReadableString& indentation) {
		string_append(target, indentation, source.get());
		return target;
	}
	inline bool operator==(const U32x4& left, const U32x4& right) {
		uint32_t a[4] ALIGN16;
		uint32_t b[4] ALIGN16;
		left.writeAlignedUnsafe(a);
		right.writeAlignedUnsafe(b);
		return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
	}
	inline bool operator!=(const U32x4& left, const U32x4& right) {
		return !(left == right);
	}
	inline U32x4 operator+(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(ADD_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] + right.emulated[0], left.emulated[1] + right.emulated[1], left.emulated[2] + right.emulated[2], left.emulated[3] + right.emulated[3]);
		#endif
	}
	inline U32x4 operator+(uint32_t left, const U32x4& right) {
		return U32x4(left) + right;
	}
	inline U32x4 operator+(const U32x4& left, uint32_t right) {
		return left + U32x4(right);
	}
	inline U32x4 operator-(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(SUB_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] - right.emulated[0], left.emulated[1] - right.emulated[1], left.emulated[2] - right.emulated[2], left.emulated[3] - right.emulated[3]);
		#endif
	}
	inline U32x4 operator-(uint32_t left, const U32x4& right) {
		return U32x4(left) - right;
	}
	inline U32x4 operator-(const U32x4& left, uint32_t right) {
		return left - U32x4(right);
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
	inline U32x4 operator*(uint32_t left, const U32x4& right) {
		return U32x4(left) * right;
	}
	inline U32x4 operator*(const U32x4& left, uint32_t right) {
		return left * U32x4(right);
	}
	inline U32x4 operator&(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(BITWISE_AND_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] & right.emulated[0], left.emulated[1] & right.emulated[1], left.emulated[2] & right.emulated[2], left.emulated[3] & right.emulated[3]);
		#endif
	}
	inline U32x4 operator&(const U32x4& left, uint32_t mask) {
		#ifdef USE_BASIC_SIMD
			return U32x4(BITWISE_AND_U32_SIMD(left.v, LOAD_SCALAR_U32_SIMD(mask)));
		#else
			return U32x4(left.emulated[0] & mask, left.emulated[1] & mask, left.emulated[2] & mask, left.emulated[3] & mask);
		#endif
	}
	inline U32x4 operator|(const U32x4& left, const U32x4& right) {
		#ifdef USE_BASIC_SIMD
			return U32x4(BITWISE_OR_U32_SIMD(left.v, right.v));
		#else
			return U32x4(left.emulated[0] | right.emulated[0], left.emulated[1] | right.emulated[1], left.emulated[2] | right.emulated[2], left.emulated[3] | right.emulated[3]);
		#endif
	}
	inline U32x4 operator|(const U32x4& left, uint32_t mask) {
		#ifdef USE_BASIC_SIMD
			return U32x4(BITWISE_OR_U32_SIMD(left.v, LOAD_SCALAR_U32_SIMD(mask)));
		#else
			return U32x4(left.emulated[0] | mask, left.emulated[1] | mask, left.emulated[2] | mask, left.emulated[3] | mask);
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
		// data must be aligned with at least 8 bytes, but preferrably 16 bytes
		//static inline U16x8 readSlow(uint16_t* data) {
		//	return U16x8(data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
		//}
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
	inline dsr::String& string_toStreamIndented(dsr::String& target, const U16x8& source, const dsr::ReadableString& indentation) {
		ALIGN16 uint16_t data[8];
		source.writeAlignedUnsafe(data);
		string_append(target, indentation, "(", data[0], ", ", data[1], ", ", data[2], ", ", data[3], ", ", data[4], ", ", data[5], ", ", data[6], ", ", data[7], ")");
		return target;
	}
	inline bool operator==(const U16x8& left, const U16x8& right) {
		ALIGN16 uint16_t a[8];
		ALIGN16 uint16_t b[8];
		left.writeAlignedUnsafe(a);
		right.writeAlignedUnsafe(b);
		return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] && a[4] == b[4] && a[5] == b[5] && a[6] == b[6] && a[7] == b[7];
	}
	inline bool operator!=(const U16x8& left, const U16x8& right) {
		return !(left == right);
	}
	inline U16x8 operator+(const U16x8& left, const U16x8& right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(ADD_U16_SIMD(left.v, right.v));
		#else
			return U16x8(left.emulated[0] + right.emulated[0], left.emulated[1] + right.emulated[1], left.emulated[2] + right.emulated[2], left.emulated[3] + right.emulated[3],
			           left.emulated[4] + right.emulated[4], left.emulated[5] + right.emulated[5], left.emulated[6] + right.emulated[6], left.emulated[7] + right.emulated[7]);
		#endif
	}
	inline U16x8 operator+(uint16_t left, const U16x8& right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(ADD_U16_SIMD(LOAD_SCALAR_U16_SIMD(left), right.v));
		#else
			return U16x8(left + right.emulated[0], left + right.emulated[1], left + right.emulated[2], left + right.emulated[3],
			           left + right.emulated[4], left + right.emulated[5], left + right.emulated[6], left + right.emulated[7]);
		#endif
	}
	inline U16x8 operator+(const U16x8& left, uint16_t right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(ADD_U16_SIMD(left.v, LOAD_SCALAR_U16_SIMD(right)));
		#else
			return U16x8(left.emulated[0] + right, left.emulated[1] + right, left.emulated[2] + right, left.emulated[3] + right,
			           left.emulated[4] + right, left.emulated[5] + right, left.emulated[6] + right, left.emulated[7] + right);
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
	inline U16x8 operator-(uint16_t left, const U16x8& right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(SUB_U16_SIMD(LOAD_SCALAR_U16_SIMD(left), right.v));
		#else
			return U16x8(left - right.emulated[0], left - right.emulated[1], left - right.emulated[2], left - right.emulated[3],
			           left - right.emulated[4], left - right.emulated[5], left - right.emulated[6], left - right.emulated[7]);
		#endif
	}
	inline U16x8 operator-(const U16x8& left, uint16_t right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(SUB_U16_SIMD(left.v, LOAD_SCALAR_U16_SIMD(right)));
		#else
			return U16x8(left.emulated[0] - right, left.emulated[1] - right, left.emulated[2] - right, left.emulated[3] - right,
			           left.emulated[4] - right, left.emulated[5] - right, left.emulated[6] - right, left.emulated[7] - right);
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
	inline U16x8 operator*(uint16_t left, const U16x8& right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(MUL_U16_SIMD(LOAD_SCALAR_U16_SIMD(left), right.v));
		#else
			return U16x8(left * right.emulated[0], left * right.emulated[1], left * right.emulated[2], left * right.emulated[3],
			           left * right.emulated[4], left * right.emulated[5], left * right.emulated[6], left * right.emulated[7]);
		#endif
	}
	inline U16x8 operator*(const U16x8& left, uint16_t right) {
		#ifdef USE_BASIC_SIMD
			return U16x8(MUL_U16_SIMD(left.v, LOAD_SCALAR_U16_SIMD(right)));
		#else
			return U16x8(
			  left.emulated[0] * right, left.emulated[1] * right, left.emulated[2] * right, left.emulated[3] * right,
			  left.emulated[4] * right, left.emulated[5] * right, left.emulated[6] * right, left.emulated[7] * right
			);
		#endif
	}

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
	inline dsr::String& string_toStreamIndented(dsr::String& target, const U8x16& source, const dsr::ReadableString& indentation) {
		ALIGN16 uint8_t data[16];
		source.writeAlignedUnsafe(data);
		string_append(target, indentation,
		  "(", data[0], ", ", data[1], ", ", data[2], ", ", data[3], ", ", data[4], ", ", data[5], ", ", data[6], ", ", data[7],
		  ", ", data[8], ", ", data[9], ", ", data[10], ", ", data[11], ", ", data[12], ", ", data[13], ", ", data[14], ", ", data[15], ")"
		);
		return target;
	}
	inline bool operator==(const U8x16& left, const U8x16& right) {
		ALIGN16 uint8_t a[16];
		ALIGN16 uint8_t b[16];
		left.writeAlignedUnsafe(a);
		right.writeAlignedUnsafe(b);
		return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3] && a[4] == b[4] && a[5] == b[5] && a[6] == b[6] && a[7] == b[7]
		    && a[8] == b[8] && a[9] == b[9] && a[10] == b[10] && a[11] == b[11] && a[12] == b[12] && a[13] == b[13] && a[14] == b[14] && a[15] == b[15];
	}
	inline bool operator!=(const U8x16& left, const U8x16& right) {
		return !(left == right);
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
	inline U8x16 operator+(uint8_t left, const U8x16& right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(ADD_U8_SIMD(LOAD_SCALAR_U8_SIMD(left), right.v));
		#else
			return U8x16(
			  left + right.emulated[0],
			  left + right.emulated[1],
			  left + right.emulated[2],
			  left + right.emulated[3],
			  left + right.emulated[4],
			  left + right.emulated[5],
			  left + right.emulated[6],
			  left + right.emulated[7],
			  left + right.emulated[8],
			  left + right.emulated[9],
			  left + right.emulated[10],
			  left + right.emulated[11],
			  left + right.emulated[12],
			  left + right.emulated[13],
			  left + right.emulated[14],
			  left + right.emulated[15]
			);
		#endif
	}
	inline U8x16 operator+(const U8x16& left, uint8_t right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(ADD_U8_SIMD(left.v, LOAD_SCALAR_U8_SIMD(right)));
		#else
			return U8x16(
			  left.emulated[0] + right,
			  left.emulated[1] + right,
			  left.emulated[2] + right,
			  left.emulated[3] + right,
			  left.emulated[4] + right,
			  left.emulated[5] + right,
			  left.emulated[6] + right,
			  left.emulated[7] + right,
			  left.emulated[8] + right,
			  left.emulated[9] + right,
			  left.emulated[10] + right,
			  left.emulated[11] + right,
			  left.emulated[12] + right,
			  left.emulated[13] + right,
			  left.emulated[14] + right,
			  left.emulated[15] + right
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
	inline U8x16 operator-(uint8_t left, const U8x16& right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(SUB_U8_SIMD(LOAD_SCALAR_U8_SIMD(left), right.v));
		#else
			return U8x16(
			  left - right.emulated[0],
			  left - right.emulated[1],
			  left - right.emulated[2],
			  left - right.emulated[3],
			  left - right.emulated[4],
			  left - right.emulated[5],
			  left - right.emulated[6],
			  left - right.emulated[7],
			  left - right.emulated[8],
			  left - right.emulated[9],
			  left - right.emulated[10],
			  left - right.emulated[11],
			  left - right.emulated[12],
			  left - right.emulated[13],
			  left - right.emulated[14],
			  left - right.emulated[15]
			);
		#endif
	}
	inline U8x16 operator-(const U8x16& left, uint8_t right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(SUB_U8_SIMD(left.v, LOAD_SCALAR_U8_SIMD(right)));
		#else
			return U8x16(
			  left.emulated[0] - right,
			  left.emulated[1] - right,
			  left.emulated[2] - right,
			  left.emulated[3] - right,
			  left.emulated[4] - right,
			  left.emulated[5] - right,
			  left.emulated[6] - right,
			  left.emulated[7] - right,
			  left.emulated[8] - right,
			  left.emulated[9] - right,
			  left.emulated[10] - right,
			  left.emulated[11] - right,
			  left.emulated[12] - right,
			  left.emulated[13] - right,
			  left.emulated[14] - right,
			  left.emulated[15] - right
			);
		#endif
	}
	inline uint8_t saturateToU8(uint32_t x) {
		// No need to check lower bound for unsigned input
		return x > 255 ? 255 : x;
	}
	inline U8x16 saturatedAddition(const U8x16& left, const U8x16& right) {
		#ifdef USE_BASIC_SIMD
			return U8x16(ADD_SAT_U8_SIMD(left.v, right.v));
		#else
			return U8x16(
			  saturateToU8((uint32_t)left.emulated[0] + (uint32_t)right.emulated[0]),
			  saturateToU8((uint32_t)left.emulated[1] + (uint32_t)right.emulated[1]),
			  saturateToU8((uint32_t)left.emulated[2] + (uint32_t)right.emulated[2]),
			  saturateToU8((uint32_t)left.emulated[3] + (uint32_t)right.emulated[3]),
			  saturateToU8((uint32_t)left.emulated[4] + (uint32_t)right.emulated[4]),
			  saturateToU8((uint32_t)left.emulated[5] + (uint32_t)right.emulated[5]),
			  saturateToU8((uint32_t)left.emulated[6] + (uint32_t)right.emulated[6]),
			  saturateToU8((uint32_t)left.emulated[7] + (uint32_t)right.emulated[7]),
			  saturateToU8((uint32_t)left.emulated[8] + (uint32_t)right.emulated[8]),
			  saturateToU8((uint32_t)left.emulated[9] + (uint32_t)right.emulated[9]),
			  saturateToU8((uint32_t)left.emulated[10] + (uint32_t)right.emulated[10]),
			  saturateToU8((uint32_t)left.emulated[11] + (uint32_t)right.emulated[11]),
			  saturateToU8((uint32_t)left.emulated[12] + (uint32_t)right.emulated[12]),
			  saturateToU8((uint32_t)left.emulated[13] + (uint32_t)right.emulated[13]),
			  saturateToU8((uint32_t)left.emulated[14] + (uint32_t)right.emulated[14]),
			  saturateToU8((uint32_t)left.emulated[15] + (uint32_t)right.emulated[15])
			);
		#endif
	}

	// TODO: Use overloading to only name the target type
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
	// Warning! Behaviour depends on endianness.
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
	// Warning! Behaviour depends on endianness.
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
			return U8x16(PACK_SAT_U16_TO_U8(lower.v, upper.v));
		#else
			return U8x16(
			  saturateToU8(lower.emulated[0]),
			  saturateToU8(lower.emulated[1]),
			  saturateToU8(lower.emulated[2]),
			  saturateToU8(lower.emulated[3]),
			  saturateToU8(lower.emulated[4]),
			  saturateToU8(lower.emulated[5]),
			  saturateToU8(lower.emulated[6]),
			  saturateToU8(lower.emulated[7]),
			  saturateToU8(upper.emulated[0]),
			  saturateToU8(upper.emulated[1]),
			  saturateToU8(upper.emulated[2]),
			  saturateToU8(upper.emulated[3]),
			  saturateToU8(upper.emulated[4]),
			  saturateToU8(upper.emulated[5]),
			  saturateToU8(upper.emulated[6]),
			  saturateToU8(upper.emulated[7])
			);
		#endif
	}

#endif

