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

#ifndef DFPSR_RENDER_PROJECTEDPOINT
#define DFPSR_RENDER_PROJECTEDPOINT

#include <stdint.h>
#include "../math/FVector.h"
#include "../math/LVector.h"

namespace dsr {

class ProjectedPoint {
public:
	// Camera space
	//   The first space where the world is rotated and translated around the camera
	//   The camera in camera space is always located at (0, 0, 0) and facing +Z
	//   Used for view frustum clipping so that a singularity can be replaced with valid values by clipping
	FVector3D cs;
	// Image space
	//   Target pixel coordinates from the upper left corner
	//   Used for perspectiva correct vertex weights
	FVector2D is;
	// Fixed sub-pixel precision target pixel coordinate from the upper left corner
	//   Used for integer rasterization to prevent holes between triangles
	LVector2D flat;
	ProjectedPoint() {}
	ProjectedPoint(FVector3D cs, FVector2D is, LVector2D flat) : cs(cs), is(is), flat(flat) {}
};

}

#endif

