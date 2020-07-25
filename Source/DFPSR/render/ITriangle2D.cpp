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

#include "ITriangle2D.h"
#include "../math/scalar.h"
#include "../math/FMatrix2x2.h"
#include <algorithm>

using namespace dsr;

IRect dsr::getTriangleBound(LVector2D a, LVector2D b, LVector2D c) {
	int32_t rX1 = (a.x + constants::unitsPerHalfPixel) / constants::unitsPerPixel;
	int32_t rY1 = (a.y + constants::unitsPerHalfPixel) / constants::unitsPerPixel;
	int32_t rX2 = (b.x + constants::unitsPerHalfPixel) / constants::unitsPerPixel;
	int32_t rY2 = (b.y + constants::unitsPerHalfPixel) / constants::unitsPerPixel;
	int32_t rX3 = (c.x + constants::unitsPerHalfPixel) / constants::unitsPerPixel;
	int32_t rY3 = (c.y + constants::unitsPerHalfPixel) / constants::unitsPerPixel;
	int leftBound = std::min(std::min(rX1, rX2), rX3) - 1;
	int topBound = std::min(std::min(rY1, rY2), rY3) - 1;
	int rightBound = std::max(std::max(rX1, rX2), rX3) + 1;
	int bottomBound = std::max(std::max(rY1, rY2), rY3) + 1;
	return IRect(leftBound, topBound, rightBound - leftBound, bottomBound - topBound);
}

FVector3D dsr::getAffineWeight(const FVector2D& cornerA, const FVector2D& cornerB, const FVector2D& cornerC, const FVector2D& point) {
	FMatrix2x2 offsetToWeight = inverse(FMatrix2x2(cornerB - cornerA, cornerC - cornerA));
	FVector2D weightBC = offsetToWeight.transform(point - cornerA);
	return FVector3D(1.0f - (weightBC.x + weightBC.y), weightBC.x, weightBC.y);
}

ITriangle2D::ITriangle2D(ProjectedPoint posA, ProjectedPoint posB, ProjectedPoint posC)
 : position{posA, posB, posC}, wholeBound(getTriangleBound(this->position[0].flat, this->position[1].flat, this->position[2].flat)) {}

// Will produce weird results if called on a triangle that needs clipping against the near plane
bool ITriangle2D::isFrontfacing() const {
	LVector2D flatA = this->position[0].flat;
	LVector2D flatB = this->position[1].flat;
	LVector2D flatC = this->position[2].flat;
	return ((flatC.x - flatA.x) * (flatB.y - flatA.y)) + ((flatC.y - flatA.y) * (flatA.x - flatB.x)) < 0;
}

#define INSIDE(VALUE,TRESH) !((VALUE)[0] > (TRESH)[0] || (VALUE)[1] > (TRESH)[1] || (VALUE)[2] > (TRESH)[2])

inline static void cutRight(int32_t& rightBound, int32_t value) {
	rightBound = std::min(rightBound, value);
}

inline static void cutLeft(int32_t& leftBound, int32_t value) {
	leftBound = std::max(leftBound, value);
}

IRect ITriangle2D::getAlignedRasterBound(const IRect& clipBound, int alignX, int alignY) const {
	IRect unaligned = IRect::cut(this->wholeBound, clipBound);
	int alignedTop = roundDown(unaligned.top(), 2);
	int alignedBottom = roundUp(unaligned.bottom(), 2);
	return IRect(unaligned.left(), alignedTop, unaligned.width(), alignedBottom - alignedTop);
}

int ITriangle2D::getBufferSize(const IRect& clipBound, int alignX, int alignY) const {
	if (IRect::overlaps(this->wholeBound, clipBound)) {
		IRect rasterBound = this->getAlignedRasterBound(clipBound, alignX, alignY);
		return rasterBound.bottom() - rasterBound.top();
	} else {
		return 0;
	}
}

static void cutConvexEdge(const LVector2D& startPoint, const LVector2D& endPoint, RowInterval* rows, const IRect& clipBound) {
	int leftBound = clipBound.left();
	int topBound = clipBound.top();
	int rightBound = clipBound.right();
	int bottomBound = clipBound.bottom();

	// Get origin in units
	int64_t originX = constants::unitsPerHalfPixel + clipBound.left() * constants::unitsPerPixel;
	int64_t originY = constants::unitsPerHalfPixel + clipBound.top() * constants::unitsPerPixel;

	// To turn x > 0 into x >= 0 without branching, just compare to -1 instead as it is equivalent for integers.
	int64_t threshold = (startPoint.x > endPoint.x || (startPoint.x == endPoint.x && startPoint.y > endPoint.y)) ? -1 : 0;
	// Get normals for each edge
	int64_t normalX = endPoint.y - startPoint.y;
	int64_t normalY = startPoint.x - endPoint.x;
	// Get partial derivatives along edge directions in screen space
	int64_t offsetX = normalX * constants::unitsPerPixel;
	int64_t offsetY = normalY * constants::unitsPerPixel;
	// Take the dot product to get an initial weight without normalization.
	int64_t valueOrigin = ((originX - startPoint.x) * normalX) + ((originY - startPoint.y) * normalY);

	// Get vertical bound
	if (normalX != 0) {
		int64_t valueRow = valueOrigin;
		// Proof for the limit variable:
		//   Find the highest x for the left side where offsetX < 0 or the lowest x for the right side where offsetX > 0
		//   x must satisfy the equation valueRow + (offsetX * (x - leftBound)) > threshold
		//   offsetX * (x - leftBound) > threshold - valueRow
		//   (offsetX * x) - (offsetX * leftBound) > threshold - valueRow
		//   offsetX * x > threshold - valueRow + (offsetX * leftBound)
		//   offsetX * x > limit
		int64_t limit = threshold - valueRow + (offsetX * leftBound);
		if (normalX < 0) { // Left
			for (int32_t y = topBound; y < bottomBound; y++) {
				int32_t rowIndex = y - topBound;
				// Find the highest x where offsetX * x > limit
				int32_t leftSide = std::min(std::max(leftBound, (int32_t)((limit + 1) / offsetX + 1)), rightBound);
				cutLeft(rows[rowIndex].left, leftSide);
				limit -= offsetY;
			}
		} else { // Right
			for (int32_t y = topBound; y < bottomBound; y++) {
				int32_t rowIndex = y - topBound;
				// Find the lowest x where offsetX * x > limit
				int32_t rightSide = std::min(std::max(leftBound, (int32_t)(limit / offsetX + 1)), rightBound);
				cutRight(rows[rowIndex].right, rightSide);
				limit -= offsetY;
			}
		}
	} else if (normalY != 0) {
		// Remove pixel rows that are outside of a fully horizontal edge
		int64_t valueRow = valueOrigin + (offsetY * (topBound - topBound));
		for (int32_t y = topBound; y < bottomBound; y++) {
			int32_t rowIndex = y - topBound;
			if (valueRow > threshold) { // If outside of the current edge
				rows[rowIndex].left = rightBound;
				rows[rowIndex].right = leftBound;
			}
			valueRow = valueRow + offsetY;
		}
	}
	// Zero length edges will make the whole triangle invisible because the two other edges must be exact opposites removing all remaining pixels
}

void dsr::rasterizeTriangle(const LVector2D& cornerA, const LVector2D& cornerB, const LVector2D& cornerC, RowInterval* rows, const IRect& clipBound) {
	LVector2D position[3] = {cornerA, cornerB, cornerC};
	if (cornerA == cornerB || cornerB == cornerC || cornerC == cornerA) {
		// Empty case with less than three separate corners
		for (int64_t rowIndex = 0; rowIndex < clipBound.height(); rowIndex++) {
			rows[rowIndex].left = clipBound.right();
			rows[rowIndex].right = clipBound.left();
		}
	} else {
		// Start with a full bounding box
		for (int64_t rowIndex = 0; rowIndex < clipBound.height(); rowIndex++) {
			rows[rowIndex].left = clipBound.left();
			rows[rowIndex].right = clipBound.right();
		}
		// Cut away pixels from each side
		for (int64_t i = 0; i < 3; i++) {
			cutConvexEdge(position[i], position[(i + 1) % 3], rows, clipBound);
		}
	}
}

void ITriangle2D::getShape(int& startRow, RowInterval* rows, const IRect& clipBound, int alignX, int alignY) const {
	// TODO: Move alignment to the render core where it belongs so that all alignX and alignY arguments are removed from the triangle
	IRect alignedBound = this->getAlignedRasterBound(clipBound, alignX, alignY);
	startRow = alignedBound.top();
	rasterizeTriangle(this->position[0].flat, this->position[1].flat, this->position[2].flat, rows, alignedBound);
}

Projection ITriangle2D::getProjection(bool perspective) const {
	return this->getProjection(FVector3D(0.0f, 1.0f, 0.0f), FVector3D(0.0f, 0.0f, 1.0f), perspective);
}

Projection ITriangle2D::getProjection(const FVector3D& subB, const FVector3D& subC, bool perspective) const {
/*
	TODO: Find out why this implementation gives crap precision
	FVector2D pointA = FVector2D(this->position[0].is.x, this->position[0].is.y);
	FVector2D pointB = FVector2D(this->position[1].is.x, this->position[1].is.y);
	FVector2D pointC = FVector2D(this->position[2].is.x, this->position[2].is.y);
	FVector3D targetWeight = getAffineWeight(pointA, pointB, pointC, FVector2D(0.0f, 0.0f));
	FVector3D affineWeightDx = getAffineWeight(pointA, pointB, pointC, FVector2D(1.0f, 0.0f)) - targetWeight;
	FVector3D affineWeightDy = getAffineWeight(pointA, pointB, pointC, FVector2D(0.0f, 1.0f)) - targetWeight;
*/
	// Get offsets
	FVector3D offsetX, offsetY;
	for (int i = 0; i < 3; i++) {
		int j = (i + 1) % 3;      // End
		FVector2D posI = this->position[i].is;
		FVector2D posJ = this->position[j].is;
		// Get offsets for each edge
		offsetX[i] = posJ.y - posI.y;
		offsetY[i] = posI.x - posJ.x;
	}
	// Get the maximum values along the offsets for normalization
	FVector3D weightMultiplier;
	for (int32_t i = 0; i < 3; i++) {
		int o = (i + 2) % 3;
		// Take the same kind of dot product from the point that is furthest away from the edge for normalization.
		float otherSideValue = ((this->position[o].is.x - this->position[i].is.x) * offsetX[i])
		                     + ((this->position[o].is.y - this->position[i].is.y) * offsetY[i]);
		if (otherSideValue == 0.0f) {
			weightMultiplier[o] = 0.0f;
		} else {
			weightMultiplier[o] = 1.0f / otherSideValue;
		}
	}
	// Get normal from weight multiplier and offset
	FVector3D normalX, normalY;
	for (int i = 0; i < 3; i++) {
		normalX[i] = offsetX[i] * weightMultiplier[i];
		normalY[i] = offsetY[i] * weightMultiplier[i];
	}
	// Sample the weight of each corner at the upper left corner of the target image
	FVector3D targetWeight;
	for (int32_t i = 0; i < 3; i++) {
		int o = (i + 2) % 3;
		// Take the dot product to get a normalized weight
		targetWeight[o] = this->position[i].is.x * -normalX[i] + this->position[i].is.y * -normalY[i];
	}
	// In order to calculate the perspective corrected vertex weights, we must first linearly iterate over the affine weights.
	// Calculate affine weight derivatives for vertex indices from edge indices.
	FVector3D affineWeightDx, affineWeightDy;
	affineWeightDx.x = normalX.y;
	affineWeightDx.y = normalX.z;
	affineWeightDx.z = normalX.x;
	affineWeightDy.x = normalY.y;
	affineWeightDy.y = normalY.z;
	affineWeightDy.z = normalY.x;

	if (!perspective) {
		// Get the linear depth
		FVector3D W(this->position[0].cs.z, this->position[1].cs.z, this->position[2].cs.z);
		// Get the affine weights of the first pixel
		FVector3D pTargetWeight;
		pTargetWeight.x = W.x * targetWeight.x + W.y * targetWeight.y + W.z * targetWeight.z;
		pTargetWeight.y = targetWeight.x * subB.x + targetWeight.y * subB.y + targetWeight.z * subB.z;
		pTargetWeight.z = targetWeight.x * subC.x + targetWeight.y * subC.y + targetWeight.z * subC.z;
		// Do the same for derivatives
		FVector3D pWeightDx, pWeightDy;
		// TODO: Use interpolateUsingAffineWeight
		pWeightDx.x = W.x * affineWeightDx.x + W.y * affineWeightDx.y + W.z * affineWeightDx.z;
		pWeightDx.y = affineWeightDx.x * subB.x + affineWeightDx.y * subB.y + affineWeightDx.z * subB.z;
		pWeightDx.z = affineWeightDx.x * subC.x + affineWeightDx.y * subC.y + affineWeightDx.z * subC.z;
		pWeightDy.x = W.x * affineWeightDy.x + W.y * affineWeightDy.y + W.z * affineWeightDy.z;
		pWeightDy.y = affineWeightDy.x * subB.x + affineWeightDy.y * subB.y + affineWeightDy.z * subB.z;
		pWeightDy.z = affineWeightDy.x * subC.x + affineWeightDy.y * subC.y + affineWeightDy.z * subC.z;

		// Store the orthogonal vertex weight projection in the shape head
		return Projection(true, pTargetWeight, pWeightDx, pWeightDy);
	} else {
		// Calculate 1 / W for each corner so that depth and vertex weights can be interpolated in a scale where everything is divided by W.
		//   This is because a linear interpolation in screen space can only produce affine projections that does not work for multiple depths with perspective.
		FVector3D IW(1.0f / this->position[0].cs.z, 1.0f / this->position[1].cs.z, 1.0f / this->position[2].cs.z);

		// Calculate the first depth divided weights needed for perspective correction.
		//   Default W is the linear depth in camera space which everything in the space is divided by.
		//   Default U is 1 for the second corner and 0 for all others.
		//   Default V is 1 for the third corner and 0 for all others.
		//   The first corner's weight can be calculated from the other weights as 1 - (U + V).
		// The U and V vertex weights are locked to a pre-defined pattern because texture coordinates and colors can later be interpolated from them using any values.
		//   |1, U1, V1|   |1, 0, 0|
		//   |1, U2, V2| = |1, 1, 0|
		//   |1, U3, V3|   |1, 0, 1|
		// Create a matrix containing (1/W, U/W, V/W) for each corner.
		// Rows represent corners and columns represent the different weights.
		//     |1/W1|   |1, 0, 0|   |1/W1, 0,    0   |
		// P = |1/W2| x |1, 1, 0| = |1/W2, 1/W2, 0   |
		//     |1/W3|   |1, 0, 1|   |1/W3, 0,    1/W3|

		// In order to efficiently loop over (1/W, U/W, V/W) for each pixel, we need to calculate the values for the first pixels and getting their derivatives.
		//   To get the first pixel's depth divided weights, we multiply the matrix P with the affine vertex weights for each corner.
		//   It is okay to linearly interpolate in the depth divided space because the projected 2D coordinate on the screen is also divided by the linear depth W.
		// Calculate P * affineWeight to get the depth divided weights of the first pixel
		FVector3D pTargetWeight;
		pTargetWeight.x = IW.x * targetWeight.x + IW.y * targetWeight.y + IW.z * targetWeight.z;
		pTargetWeight.y = IW.x * targetWeight.x * subB.x + IW.y * targetWeight.y * subB.y + IW.z * targetWeight.z * subB.z;
		pTargetWeight.z = IW.x * targetWeight.x * subC.x + IW.y * targetWeight.y * subC.y + IW.z * targetWeight.z * subC.z;
		// Do the depth division on derivatives too
		FVector3D pWeightDx, pWeightDy;
		// TODO: Reuse multiplications by multiplying IW with affineWeightDx and affineWeightDy first
		// TODO: Use 3x3 matrices to make the code less error prone
		pWeightDx.x = IW.x * affineWeightDx.x + IW.y * affineWeightDx.y + IW.z * affineWeightDx.z;
		pWeightDx.y = IW.x * affineWeightDx.x * subB.x + IW.y * affineWeightDx.y * subB.y + IW.z * affineWeightDx.z * subB.z;
		pWeightDx.z = IW.x * affineWeightDx.x * subC.x + IW.y * affineWeightDx.y * subC.y + IW.z * affineWeightDx.z * subC.z;
		pWeightDy.x = IW.x * affineWeightDy.x + IW.y * affineWeightDy.y + IW.z * affineWeightDy.z;
		pWeightDy.y = IW.x * affineWeightDy.x * subB.x + IW.y * affineWeightDy.y * subB.y + IW.z * affineWeightDy.z * subB.z;
		pWeightDy.z = IW.x * affineWeightDy.x * subC.x + IW.y * affineWeightDy.y * subC.y + IW.z * affineWeightDy.z * subC.z;

		// Store the perspective vertex weight projection in the shape head
		return Projection(false, pTargetWeight, pWeightDx, pWeightDy);
	}
}

