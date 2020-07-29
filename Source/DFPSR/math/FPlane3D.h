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

#ifndef DFPSR_GEOMETRY_FPLANE3D
#define DFPSR_GEOMETRY_FPLANE3D

#include <math.h>
#include "FVector.h"

namespace dsr {

struct FPlane3D {
	FVector3D normal; // The plane's normal facing out
	float offset; // The plane's translation along the normal
	FPlane3D() : normal(FVector3D()), offset(0.0f) {}
	FPlane3D(const FVector3D &normal, float offset) : normal(normalize(normal)), offset(offset) {}
	// Get the closest distance between the point and the plane
	// A negative distance is returned if the point is inside
	float signedDistance(const FVector3D &point) const {
		return dotProduct(this->normal, point) - this->offset;
	}
	bool inside(const FVector3D &point) const {
		return this->signedDistance(point) <= 0.0f;
	}
	// Returns a point on the plane intersecting the line starting at point along direction
	// Returns +-INF or NaN when there's no point of intersection
	FVector3D rayIntersect(const FVector3D &point, const FVector3D &direction) {
		float relativeOffset = -(this->offset + dotProduct(this->normal, point)) / dotProduct(this->normal, direction);
		return point + (direction * relativeOffset);
	}
};

}

#endif

