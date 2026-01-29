
// zlib open source license
//
// Copyright (c) 2019 to 2022 David Forsgren Piuva
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

// Because most collections have little need for their own implementation files, all the bound checks are placed in a common implementation file.

#ifndef DFPSR_COLLECTIONS
#define DFPSR_COLLECTIONS

#include "../base/heap.h"
#include <cstdint>

namespace dsr {

// Helper functions for collections.
void impl_nonZeroLengthCheck(int64_t length, const char32_t* property);
void impl_baseZeroBoundCheck(int64_t index, int64_t length, const char32_t* property);

}

#endif
