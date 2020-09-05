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

#ifndef DFPSR_GEOMETRY_UVECTOR
#define DFPSR_GEOMETRY_UVECTOR

#include "vectorMethods.h"

namespace dsr {

struct UVector2D {
	VECTOR_BODY_2D(UVector2D, uint32_t, 0u);
};
struct UVector3D {
	VECTOR_BODY_3D(UVector3D, uint32_t, 0u);
};
struct UVector4D {
	VECTOR_BODY_4D(UVector4D, uint32_t, 0u);
};

OPERATORS_2D(UVector2D, uint32_t);
OPERATORS_3D(UVector3D, uint32_t);
OPERATORS_4D(UVector4D, uint32_t);
SIGNED_OPERATORS_2D(UVector2D, uint32_t);
SIGNED_OPERATORS_3D(UVector3D, uint32_t);
SIGNED_OPERATORS_4D(UVector4D, uint32_t);
EXACT_COMPARE_2D(UVector2D);
EXACT_COMPARE_3D(UVector3D);
EXACT_COMPARE_4D(UVector4D);
SERIALIZATION_2D(UVector2D);
SERIALIZATION_3D(UVector3D);
SERIALIZATION_4D(UVector4D);

}

#endif

