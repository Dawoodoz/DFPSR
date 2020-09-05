// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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

// An advanced high performance extension to the simpler simd.h
//    The caller is expected to write the reference implementation separatelly for unhandled target machines.
//        Because the code is not as clean as when using infix math operations from simd.h,
//        so you will need to write a separate scalar version anyway for documentating the behaviour.
//    This module can only be used when the USE_SIMD_EXTRA macro is defined.
//        This allow USE_SIMD_EXTRA to be more picky about which SIMD instruction sets to use
//        in order to get access to a larger intersection between the platforms.
//        It also keeps simd.h easy to port and emulate.
//    Works directly with simd vectors using aliases, instead of the wrappers.
//        This makes it easier to mix directly with SIMD intrinsics for a specific target.

#ifndef DFPSR_SIMD_EXTRA
#define DFPSR_SIMD_EXTRA
	#include "simd.h"

	#ifdef USE_SSE2
		#define USE_SIMD_EXTRA
		//struct SIMD_F32x4x2 {
		//	SIMD_F32x4 val[2];
		//};
		//struct SIMD_U16x8x2 {
		//	SIMD_U16x8 val[2];
		//};
		struct SIMD_U32x4x2 {
			SIMD_U32x4 val[2];
		};
		//struct SIMD_I32x4x2 {
		//	SIMD_I32x4 val[2];
		//};
		static inline SIMD_U32x4x2 ZIP_U32_SIMD(SIMD_U32x4 lower, SIMD_U32x4 higher) {
			ALIGN16 SIMD_U32x4x2 result;
			result.val[0] = _mm_unpacklo_epi32(lower, higher);
			result.val[1] = _mm_unpackhi_epi32(lower, higher);
			return result;
		}
		static inline SIMD_U32x4 ZIP_LOW_U32_SIMD(SIMD_U32x4 lower, SIMD_U32x4 higher) {
			return _mm_unpacklo_epi32(lower, higher);
		}
		static inline SIMD_U32x4 ZIP_HIGH_U32_SIMD(SIMD_U32x4 lower, SIMD_U32x4 higher) {
			return _mm_unpackhi_epi32(lower, higher);
		}
	#elif USE_NEON
		#define USE_SIMD_EXTRA
		// TODO: Write regression tests and try simdExtra.h with NEON activated
		//#define SIMD_F32x4x2 float32x4x2_t
		//#define SIMD_U16x8x2 uint16x8x2_t
		#define SIMD_U32x4x2 uint32x4x2_t
		//#define SIMD_I32x4x2 int32x4x2_t
		static inline SIMD_U32x4x2 ZIP_U32_SIMD(SIMD_U32x4 lower, SIMD_U32x4 higher) {
			return vzipq_u32(lower, higher);
		}
		static inline SIMD_U32x4 ZIP_LOW_U32_SIMD(SIMD_U32x4 lower, SIMD_U32x4 higher) {
			//return vzipq_u32(lower, higher).val[0];
			return float32x2x2_t vzip_u32(vget_low_u32(lower), vget_low_u32(higher));
		}
		static inline SIMD_U32x4 ZIP_HIGH_U32_SIMD(SIMD_U32x4 lower, SIMD_U32x4 higher) {
			//return vzipq_u32(lower, higher).val[1];
			return float32x2x2_t vzip_u32(vget_high_u32(lower), vget_high_u32(higher));
		}
	#endif
#endif
