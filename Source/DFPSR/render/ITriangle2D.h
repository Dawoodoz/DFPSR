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

#ifndef DFPSR_RENDER_ITRIANGLE2D
#define DFPSR_RENDER_ITRIANGLE2D

#include <cstdint>
#include <cassert>
#include "ProjectedPoint.h"
#include "../math/FVector.h"
#include "../math/IVector.h"
#include "../math/IRect.h"
#include "constants.h"

namespace dsr {

class RowInterval {
public:
	// Start and end in exclusive pixel intervals
	int32_t left, right;
	// Constructors
	RowInterval() : left(0), right(0) {}
	RowInterval(int32_t left, int32_t right) : left(left), right(right) {}
};

// Get a pixel bound from sub-pixel 2D corners
IRect getTriangleBound(LVector2D a, LVector2D b, LVector2D c);
// Get a list of rows from a triangle of three 2D corners
//   Each corner is expressed in sub-pixels of constants::unitsPerPixel units per pixel
//   The rows must point to an array of at least clipBound.height() elements
//   Writing will be done to rows[r] for the whole range 0 <= r < clipBound.height()
void rasterizeTriangle(const LVector2D& cornerA, const LVector2D& cornerB, const LVector2D& cornerC, RowInterval* rows, const IRect& clipBound);

// The point should be expressed in the same coordinate system as the corners
//   Don't forget to add 0.5 if converting pixel indices to float centers
FVector3D getAffineWeight(const FVector2D& cornerA, const FVector2D& cornerB, const FVector2D& cornerC, const FVector2D& point);

template <typename T>
T interpolateUsingAffineWeight(T valueA, T valueB, T valueC, FVector3D weight) {
	return valueA * weight.x + valueB * weight.y + valueC * weight.z;
}

class Projection {
public:
	// W is the linear depth and 1/W is the reciprocal depth
	// When affine is true, the weights contain (W, U, V)
	//   U and V are the affine vertex weights in a linear scale
	// When affine is false, the weights contain (1/W, U/W, V/W)
	//   1/W is the reciprocal weight used to get U and V
	//   U/W and V/W are the vertex weights divided by the depth W
	bool affine = false;
	FVector3D pWeightStart; // Depth divided weights at the upper left corner of the target image
	FVector3D pWeightDx; // The difference when X increases by 1
	FVector3D pWeightDy; // The difference when Y increases by 1
	// Constructors
	Projection() {}
	Projection(bool affine, FVector3D pWeightStart, FVector3D pWeightDx, FVector3D pWeightDy) :
	  affine(affine), pWeightStart(pWeightStart), pWeightDx(pWeightDx), pWeightDy(pWeightDy) {}

	// Affine interface
	// Precondition: affine is true
		FVector3D getWeight_affine(const IVector2D& screenPixel) const {
			assert(this->affine);
			// pWeightStart is relative to the target's upper left corner so we must add 0.5 to get the center of the pixel
			return this->pWeightStart + (this->pWeightDx * (screenPixel.x + 0.5f)) + (this->pWeightDy * (screenPixel.y + 0.5f));
		}
		// Returns the depth from a linear weight
		float getDepth_affine(const FVector3D& linearWeight) const {
			assert(this->affine);
			return linearWeight.x;
		}

	// Perspective interface
	// Precondition: affine is false
		// Returns (1/W, U/W, V/W) from the center of the pixel at screenPixel
		FVector3D getDepthDividedWeight_perspective(const IVector2D& screenPixel) const {
			assert(!(this->affine));
			// pWeightStart is relative to the target's upper left corner so we must add 0.5 to get the center of the pixel
			return this->pWeightStart + (this->pWeightDx * (screenPixel.x + 0.5f)) + (this->pWeightDy * (screenPixel.y + 0.5f));
		}
		// Returns (1/W, U/W, V/W) from screenPoint in floating pixel coordinates
		FVector3D getDepthDividedWeight_perspective(const FVector2D& screenPoint) const {
			assert(!(this->affine));
			return this->pWeightStart + (this->pWeightDx * screenPoint.x) + (this->pWeightDy * screenPoint.y);
		}
		// Returns the depth from a depth divided weight
		float getDepth_perspective(const FVector3D& depthDividedWeight) const {
			assert(!(this->affine));
			return 1.0f / depthDividedWeight.x;
		}
		FVector3D getWeight_perspective(const FVector3D& depthDividedWeight, float depth) const {
			assert(!(this->affine));
			FVector3D result;
			// Multiply U/W and V/W by W to get the U and V vertex weights
			result.y = depthDividedWeight.y * depth;
			result.z = depthDividedWeight.z * depth;
			// Replace calculate the UV complement now that we have used 1/W
			result.x = 1.0f - result.y - result.z;
			return result;
		}
		void sampleProjection_perspective(const IVector2D& screenPixel, FVector3D& weight, float& depth) const {
			assert(!(this->affine));
			FVector3D invWeight = this->getDepthDividedWeight_perspective(screenPixel);
			depth = this->getDepth_perspective(invWeight);
			weight = this->getWeight_perspective(invWeight, depth);
		}
		void sampleProjection_perspective(const FVector2D& screenPoint, FVector3D& weight, float& depth) const {
			assert(!(this->affine));
			FVector3D invWeight = this->getDepthDividedWeight_perspective(screenPoint);
			depth = this->getDepth_perspective(invWeight);
			weight = this->getWeight_perspective(invWeight, depth);
		}
};

class RowShape {
public:
	// A collection of row intervals telling where pixels should be drawn
	const int startRow;
	const int rowCount;
	const RowInterval *rows;
	// Constructors
	RowShape() : startRow(0), rowCount(0), rows(nullptr) {}
	RowShape(int startRow, int rowCount, RowInterval* rows) : startRow(startRow), rowCount(rowCount), rows(rows) {}
};

// Any extra information will be given to the filling method as this only gives the shape and vertex interpolation data
class ITriangle2D {
public:
	// Per vertex (0, 1, 2)
	const ProjectedPoint position[3];
	// The unconstrained bound of the triangle to rasterize
	const IRect wholeBound;
	// Constructor that generates all data needed for fast rasterization
	ITriangle2D(ProjectedPoint posA, ProjectedPoint posB, ProjectedPoint posC);
	// Returns true iff the triangle is clockwise and may be drawn
	bool isFrontfacing() const;
	// Get the region to rasterize where the first and last rows may go outside of the clipBound with empty rows for alignment
	//   Give a clipBound with top and bottom at even multiples of two if you don't want the result to go outside
	IRect getAlignedRasterBound(const IRect& clipBound, int alignX, int alignY) const;
	int getBufferSize(const IRect& clipBound, int alignX, int alignY) const;
	// Get the row intervals within clipBound into a buffer of a size given by getBufferSize with the same clipBound
	// Output
	//   rows: The exclusive row interval for each row from top to bottom
	//   startRow: The Y coordinate of the first row
	// Input
	//   clipBound: The pixel region where the resulting rows may draw.
	void getShape(int& startRow, RowInterval* rows, const IRect& clipBound, int alignX, int alignY) const;
	// Returns the vertex weight projection for specified sub-vertex weights
	Projection getProjection(const FVector3D& subB, const FVector3D& subC, bool perspective) const;
	// Returns the vertex weight projection for default sub-vertex weights
	Projection getProjection(bool perspective) const;
};

}

#endif

