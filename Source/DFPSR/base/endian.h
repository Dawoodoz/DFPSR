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

// Endianness abstraction layer for manipulating byte arrays within unsigned integers
//   ENDIAN_POS_ADDR
//     Bit-shift in the positive direction of addresses
//     Precondition: OFFSET % 8 == 0
//   ENDIAN_NEG_ADDR
//     Bit-shift in the negative direction of addresses
//     Precondition: OFFSET % 8 == 0
//   ENDIAN32_BYTE_0, A mask from the byte array {255, 0, 0, 0}
//   ENDIAN32_BYTE_1, A mask from the byte array {0, 255, 0, 0}
//   ENDIAN32_BYTE_2, A mask from the byte array {0, 0, 255, 0}
//   ENDIAN32_BYTE_3, A mask from the byte array {0, 0, 0, 255}
//   The DSR_BIG_ENDIAN flag should be given manually as a compiler argument when compiling for big-endian hardware

#ifndef DFPSR_ENDIAN
#define DFPSR_ENDIAN
	#include <stdint.h>
	// TODO: Detect endianness automatically
	#ifdef DSR_BIG_ENDIAN
		// TODO: Not yet tested on a big-endian machine!
		#define ENDIAN_POS_ADDR(VALUE,OFFSET) ((VALUE) >> (OFFSET))
		#define ENDIAN_NEG_ADDR(VALUE,OFFSET) ((VALUE) << (OFFSET))
		#define ENDIAN32_BYTE_0 0xFF000000u
		static_assert(false, "Big-endian mode has not been officially tested!");
	#else
		#define ENDIAN_POS_ADDR(VALUE,OFFSET) ((VALUE) << (OFFSET))
		#define ENDIAN_NEG_ADDR(VALUE,OFFSET) ((VALUE) >> (OFFSET))
		#define ENDIAN32_BYTE_0 0x000000FFu
	#endif
	#define ENDIAN32_BYTE_1 ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, 8)
	#define ENDIAN32_BYTE_2 ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, 16)
	#define ENDIAN32_BYTE_3 ENDIAN_POS_ADDR(ENDIAN32_BYTE_0, 24)
#endif

