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

#ifndef DFPSR_GEOMETRY_LVECTOR
#define DFPSR_GEOMETRY_LVECTOR

#include "vectorMethods.h"

namespace dsr {

struct LVector2D {
	VECTOR_BODY_2D(LVector2D, int64_t, 0);
};
struct LVector3D {
	VECTOR_BODY_3D(LVector3D, int64_t, 0);
};
struct LVector4D {
	VECTOR_BODY_4D(LVector4D, int64_t, 0);
};

OPERATORS_2D(LVector2D, int64_t);
OPERATORS_3D(LVector3D, int64_t);
OPERATORS_4D(LVector4D, int64_t);
SIGNED_OPERATORS_2D(LVector2D, int64_t);
SIGNED_OPERATORS_3D(LVector3D, int64_t);
SIGNED_OPERATORS_4D(LVector4D, int64_t);
EXACT_COMPARE_2D(LVector2D);
EXACT_COMPARE_3D(LVector3D);
EXACT_COMPARE_4D(LVector4D);
SERIALIZATION_2D(LVector2D);
SERIALIZATION_3D(LVector3D);
SERIALIZATION_4D(LVector4D);

}

#endif

