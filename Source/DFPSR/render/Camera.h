// zlib open source license
//
// Copyright (c) 2017 to 2022 David Forsgren Piuva
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

#ifndef DFPSR_RENDER_CAMERA
#define DFPSR_RENDER_CAMERA

#include <cstdint>
#include <cassert>
#include "../math/FVector.h"
#include "../math/LVector.h"
#include "../math/FPlane3D.h"
#include "../math/Transform3D.h"
#include "../math/scalar.h"
#include "constants.h"
#include "ProjectedPoint.h"
#include <limits>

namespace dsr {

// A special rounding used for vertex projection
inline int64_t safeRoundInt64(float value) {
	int64_t result = (int64_t)value;
	if (value <= -1048576.0f || value >= 1048576.0f) { result = 0; }
	return result;
}

class ViewFrustum {
private:
	FPlane3D planes[6];
	int planeCount;
public:
	ViewFrustum() : planeCount(0) {}
	// Orthogonal view frustum in camera space
	ViewFrustum(float halfWidth, float halfHeight)
	: planeCount(4) {
		// Sides
		planes[0] = FPlane3D(FVector3D(1.0f, 0.0f, 0.0f), halfWidth);
		planes[1] = FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), halfWidth);
		planes[2] = FPlane3D(FVector3D(0.0f, 1.0f, 0.0f), halfHeight);
		planes[3] = FPlane3D(FVector3D(0.0f, -1.0f, 0.0f), halfHeight);
	}
	// Perspective view frustum in camera space
	ViewFrustum(float nearClip, float farClip, float widthSlope, float heightSlope)
	: planeCount(farClip == std::numeric_limits<float>::infinity() ? 5 : 6) { // Skip the far clip plane if its distance is infinite.
		// Sides
		planes[0] = FPlane3D(FVector3D(1.0f, 0.0f, -widthSlope), 0.0f);
		planes[1] = FPlane3D(FVector3D(-1.0f, 0.0f, -widthSlope), 0.0f);
		planes[2] = FPlane3D(FVector3D(0.0f, 1.0f, -heightSlope), 0.0f);
		planes[3] = FPlane3D(FVector3D(0.0f, -1.0f, -heightSlope), 0.0f);
		// Near and far clip planes
		planes[4] = FPlane3D(FVector3D(0.0f, 0.0f, -1.0f), -nearClip);
		planes[5] = FPlane3D(FVector3D(0.0f, 0.0f, 1.0f), farClip);
	}
	int getPlaneCount() const {
		return this->planeCount;
	}
	FPlane3D getPlane(int sideIndex) const {
		assert(sideIndex >= 0 && sideIndex < this->planeCount);
		return planes[sideIndex];
	}
};

// How much is the image region magnified for skipping entire triangles.
//   A small margin is needed to prevent missing pixels from rounding errors along the borders in high image resolutions.
static const float cullRatio = 1.0001f;
// How much is the image region magnified for clipping triangles.
//   The larger you make the clip region, the less triangles you have to apply clipping to.
//   The triangle rasterization can handle clipping triangles in integer coordinates,
//     but there are limits to how large those integers can become before overflowing.
static const float clipRatio = 2.0f;
// To prevent division by zero, a near clipping distance is slightly above zero to
//   clip triangles in 3D camera space before projecting the coordinates to the target image.
static const float defaultNearClip = 0.01f;
static const float defaultFarClip = 1000.0f;

// Just create a new camera on stack memory every time you need to render something
class Camera {
public: // Do not modify individual settings without assigning whole new cameras
	bool perspective; // When off, widthSlope and heightSlope will be used as halfWidth and halfHeight.
	Transform3D location; // Only translation and rotation allowed. Scaling and tilting will obviously not work for cameras.
	float widthSlope, heightSlope, invWidthSlope, invHeightSlope, imageWidth, imageHeight, nearClip, farClip;
	ViewFrustum cullFrustum, clipFrustum;
	Camera() :
	  perspective(true), location(Transform3D()), widthSlope(0.0f), heightSlope(0.0f),
	  invWidthSlope(0.0f), invHeightSlope(0.0f), imageWidth(0), imageHeight(0),
	  nearClip(0.0f), farClip(0.0f), cullFrustum(ViewFrustum()), clipFrustum(ViewFrustum()) {}
	Camera(bool perspective, const Transform3D &location, float imageWidth, float imageHeight, float widthSlope, float heightSlope, float nearClip, float farClip, const ViewFrustum &cullFrustum, const ViewFrustum &clipFrustum) :
	  perspective(perspective), location(location), widthSlope(widthSlope), heightSlope(heightSlope),
	  invWidthSlope(0.5f / widthSlope), invHeightSlope(0.5f / heightSlope), imageWidth(imageWidth), imageHeight(imageHeight),
	  nearClip(nearClip), farClip(farClip), cullFrustum(cullFrustum), clipFrustum(clipFrustum) {}
public:
	static Camera createPerspective(const Transform3D &location, float imageWidth, float imageHeight, float widthSlope = 1.0f, float nearClip = defaultNearClip, float farClip = defaultFarClip) {
		float heightSlope = widthSlope * imageHeight / imageWidth;
		return Camera(true, location, imageWidth, imageHeight, widthSlope, heightSlope, nearClip, farClip,
		  ViewFrustum(nearClip, farClip, widthSlope * cullRatio, heightSlope * cullRatio),
		  ViewFrustum(nearClip, farClip, widthSlope * clipRatio, heightSlope * clipRatio));
	}
	// Orthogonal cameras doesn't have any near or far clip planes
	static Camera createOrthogonal(const Transform3D &location, float imageWidth, float imageHeight, float halfWidth) {
		float halfHeight = halfWidth * imageHeight / imageWidth;
		return Camera(false, location, imageWidth, imageHeight, halfWidth, halfHeight, -std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
		  ViewFrustum(halfWidth * cullRatio, halfHeight * cullRatio),
		  ViewFrustum(halfWidth * clipRatio, halfHeight * clipRatio));
	}
	FVector3D worldToCamera(const FVector3D &worldSpace) const {
		return this->location.transformPointTransposedInverse(worldSpace);
	}
	ProjectedPoint cameraToScreen(const FVector3D &cameraSpace) const {
		// Camera to image space
		if (this->perspective) {
			float invDepth;
			if (cameraSpace.z > 0.0f) {
				invDepth = 1.0f / cameraSpace.z;
			} else {
				invDepth = 0.0f;
			}
			float centerShear = cameraSpace.z * 0.5f;
			FVector2D preProjection = FVector2D(
			  ( cameraSpace.x * this->invWidthSlope  + centerShear) * this->imageWidth,
			  (-cameraSpace.y * this->invHeightSlope + centerShear) * this->imageHeight
			);
			FVector2D projectedFloat = preProjection * invDepth;
			FVector2D subPixel = projectedFloat * constants::unitsPerPixel;
			LVector2D rounded = LVector2D(safeRoundInt64(subPixel.x), safeRoundInt64(subPixel.y));
			return ProjectedPoint(cameraSpace, projectedFloat, rounded);
		} else {
			FVector2D projectedFloat = FVector2D(
			  ( cameraSpace.x * this->invWidthSlope + 0.5f) * this->imageWidth,
			  (-cameraSpace.y * this->invHeightSlope + 0.5f) * this->imageHeight
			);
			FVector2D subPixel = projectedFloat * constants::unitsPerPixel;
			LVector2D rounded = LVector2D(safeRoundInt64(subPixel.x), safeRoundInt64(subPixel.y));
			return ProjectedPoint(cameraSpace, projectedFloat, rounded);
		}
	}
	ProjectedPoint worldToScreen(const FVector3D &worldSpace) const {
		return this->cameraToScreen(this->worldToCamera(worldSpace));
	}
	int getFrustumPlaneCount(bool clipping) const {
		return clipping ? this->clipFrustum.getPlaneCount() : this->cullFrustum.getPlaneCount();
	}
	FPlane3D getFrustumPlane(int sideIndex, bool clipping) const {
		return clipping ? this->clipFrustum.getPlane(sideIndex) : this->cullFrustum.getPlane(sideIndex);
	}
	// Returns false iff all 6 points from the box of minBound and maxBound multiplied by transform are outside of the same plane of cullFrustum
	//   This is a quick indication to if something within that bound would be rendered
	bool isBoxSeen(const FVector3D& minBound, const FVector3D& maxBound, const Transform3D &modelToWorld) const {
		FVector3D corner[8] = {
			FVector3D(minBound.x, minBound.y, minBound.z),
			FVector3D(maxBound.x, minBound.y, minBound.z),
			FVector3D(minBound.x, maxBound.y, minBound.z),
			FVector3D(maxBound.x, maxBound.y, minBound.z),
			FVector3D(minBound.x, minBound.y, maxBound.z),
			FVector3D(maxBound.x, minBound.y, maxBound.z),
			FVector3D(minBound.x, maxBound.y, maxBound.z),
			FVector3D(maxBound.x, maxBound.y, maxBound.z)
		};
		for (int c = 0; c < 8; c++) {
			corner[c] = worldToCamera(modelToWorld.transformPoint(corner[c]));
		}
		for (int s = 0; s < this->cullFrustum.getPlaneCount(); s++) {
			FPlane3D plane = this->cullFrustum.getPlane(s);
			if (!(plane.inside(corner[0])
			   || plane.inside(corner[1])
			   || plane.inside(corner[2])
			   || plane.inside(corner[3])
			   || plane.inside(corner[4])
			   || plane.inside(corner[5])
			   || plane.inside(corner[6])
			   || plane.inside(corner[7]))) {
				return false;
			}
		}
		return true;
	}
};

}

#endif

