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
#include "../../math/FVector.h"
#include "../../math/LVector.h"
#include "../../math/FPlane3D.h"
#include "../../math/Transform3D.h"
#include "../../collection/FixedArray.h"
#include "../math/scalar.h"
#include "constants.h"
#include "ProjectedPoint.h"
#include <limits>

namespace dsr {

class ViewFrustum {
private:
	FixedArray<FPlane3D, 6> planes;
	int32_t planeCount;
public:
	// Named indices to the different planes defining a view frustum.
	static const int32_t view_left   = 0;
	static const int32_t view_right  = 1;
	static const int32_t view_top    = 2;
	static const int32_t view_bottom = 3;
	static const int32_t view_near   = 4;
	static const int32_t view_far    = 5;
	ViewFrustum() : planeCount(0) {}
	// Orthogonal view frustum in camera space
	ViewFrustum(float halfWidth, float halfHeight)
	: planeCount(4) {
		// Sides
		planes[view_left  ] = FPlane3D(FVector3D(-1.0f, 0.0f, 0.0f), halfWidth);
		planes[view_right ] = FPlane3D(FVector3D(1.0f, 0.0f, 0.0f), halfWidth);
		planes[view_top   ] = FPlane3D(FVector3D(0.0f, 1.0f, 0.0f), halfHeight);
		planes[view_bottom] = FPlane3D(FVector3D(0.0f, -1.0f, 0.0f), halfHeight);
	}
	// Perspective view frustum in camera space
	ViewFrustum(float nearClip, float farClip, float widthSlope, float heightSlope)
	: planeCount(farClip == std::numeric_limits<float>::infinity() ? 5 : 6) { // Skip the far clip plane if its distance is infinite.
		// Sides
		planes[view_left  ] = FPlane3D(FVector3D(-1.0f,  0.0f, -widthSlope ), 0.0f);
		planes[view_right ] = FPlane3D(FVector3D( 1.0f,  0.0f, -widthSlope ), 0.0f);
		planes[view_top   ] = FPlane3D(FVector3D( 0.0f,  1.0f, -heightSlope), 0.0f);
		planes[view_bottom] = FPlane3D(FVector3D( 0.0f, -1.0f, -heightSlope), 0.0f);
		// Near and far clip planes
		planes[view_near  ] = FPlane3D(FVector3D(0.0f, 0.0f, -1.0f), -nearClip);
		planes[view_far   ] = FPlane3D(FVector3D(0.0f, 0.0f,  1.0f),   farClip);
	}
	inline int32_t getPlaneCount() const {
		return this->planeCount;
	}
	inline FPlane3D getPlane(int32_t sideIndex) const {
		assert(sideIndex >= 0 && sideIndex < this->planeCount);
		return planes[sideIndex];
	}
	// Quick estimation of potential visibility without caring about edges nor details.
	// The convex hull points to test are relative to the camera's location.
	// Returns 0 if all points are outside of the same plane, so that an object within the convex hull can not be visible.
	// Returns 1 if one or more points are outside of the view frustum but they are not all outside of the same plane, so it may or may not be visible.
	// Returns 2 if all points are inside of the view frustum, so that it is certainly visible, unless hidden by something else.
	int32_t isConvexHullSeen(SafePointer<const FVector3D> cameraSpacePoints, int32_t pointCount) const {
		bool anyOutside = false;
		for (int32_t s = 0; s < this->getPlaneCount(); s++) {
			FPlane3D plane = this->getPlane(s);
			// Check if any point is inside of the current plane.
			bool anyInside = false;
			for (int32_t p = 0; p < pointCount; p++) {
				if (plane.inside(cameraSpacePoints[p])) {
					anyInside = true;
				} else {
					anyOutside = true;
				}
			}
			// If none was inside of the plane, then the point clound is not visible.
			if (!anyInside) {
				// All points were outside of the current side, so the hull is not visible.
				return 0;
			}
		}
		// Every side had at least one point inside, so the hull is visible.
		return anyOutside ? 1 : 2;
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

// Just create a new camera on stack memory every time you need to render something.
class Camera {
public: // Do not modify individual settings without assigning whole new cameras.
	bool perspective; // When off, widthSlope and heightSlope will be used as halfWidth and halfHeight.
	Transform3D location; // Only translation and rotation allowed. Scaling and tilting will obviously not work for cameras.
	float widthSlope, heightSlope, invWidthSlope, invHeightSlope, imageWidth, imageHeight, nearClip, farClip;
	// The tight view frustum, used for skipping rendering as soon as something is fully out of sight.
	ViewFrustum cullFrustum;
	// The extra large frustum outside of the visible border, used to clip rendering of partial visibility to prevent integer overflow in perspective projection.
	//   The clip frustum is much larger than the cull frustum because clipping is expensive and can not be done using exact integers.
	ViewFrustum clipFrustum;
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
	inline FVector3D worldToCamera(const FVector3D &worldSpace) const {
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
			LVector2D rounded = LVector2D(int64_t(subPixel.x), int64_t(subPixel.y));
			return ProjectedPoint(cameraSpace, projectedFloat, rounded);
		} else {
			FVector2D projectedFloat = FVector2D(
			  ( cameraSpace.x * this->invWidthSlope + 0.5f) * this->imageWidth,
			  (-cameraSpace.y * this->invHeightSlope + 0.5f) * this->imageHeight
			);
			FVector2D subPixel = projectedFloat * constants::unitsPerPixel;
			LVector2D rounded = LVector2D(int64_t(subPixel.x), int64_t(subPixel.y));
			return ProjectedPoint(cameraSpace, projectedFloat, rounded);
		}
	}
	inline ProjectedPoint worldToScreen(const FVector3D &worldSpace) const {
		return this->cameraToScreen(this->worldToCamera(worldSpace));
	}
	// Get the number of planes in the clipping or culling frustum.
	inline int32_t getFrustumPlaneCount(bool clipping = false) const {
		return clipping ? this->clipFrustum.getPlaneCount() : this->cullFrustum.getPlaneCount();
	}
	// Get a certain plane from the clipping or culling frustum.
	//   The plane is expressed in camera space.
	inline FPlane3D getFrustumPlane(int32_t sideIndex, bool clipping = false) const {
		return clipping ? this->clipFrustum.getPlane(sideIndex) : this->cullFrustum.getPlane(sideIndex);
	}
	// Returns 0 iff the model inside of the bound can clearly not be visible, 1 if it intersects with the view frustum, or 2 if fully in view.
	//   by having all corners outside of the same side in the camera's culling frustum.
	int32_t isBoxSeen(const FVector3D& minModelSpaceBound, const FVector3D& maxModelSpaceBound, const Transform3D &modelToWorld) const {
		// Allocate memory for the corners.
		FVector3D cornerBuffer[8];
		SafePointer<FVector3D> corners = SafePointer<FVector3D>("corners in Camera::isBoxSeen", cornerBuffer, sizeof(cornerBuffer));
		// Convert from model space bounds to camera space point cloud.
		corners[0] = this->worldToCamera(modelToWorld.transformPoint(FVector3D(minModelSpaceBound.x, minModelSpaceBound.y, minModelSpaceBound.z)));
		corners[1] = this->worldToCamera(modelToWorld.transformPoint(FVector3D(maxModelSpaceBound.x, minModelSpaceBound.y, minModelSpaceBound.z)));
		corners[2] = this->worldToCamera(modelToWorld.transformPoint(FVector3D(minModelSpaceBound.x, maxModelSpaceBound.y, minModelSpaceBound.z)));
		corners[3] = this->worldToCamera(modelToWorld.transformPoint(FVector3D(maxModelSpaceBound.x, maxModelSpaceBound.y, minModelSpaceBound.z)));
		corners[4] = this->worldToCamera(modelToWorld.transformPoint(FVector3D(minModelSpaceBound.x, minModelSpaceBound.y, maxModelSpaceBound.z)));
		corners[5] = this->worldToCamera(modelToWorld.transformPoint(FVector3D(maxModelSpaceBound.x, minModelSpaceBound.y, maxModelSpaceBound.z)));
		corners[6] = this->worldToCamera(modelToWorld.transformPoint(FVector3D(minModelSpaceBound.x, maxModelSpaceBound.y, maxModelSpaceBound.z)));
		corners[7] = this->worldToCamera(modelToWorld.transformPoint(FVector3D(maxModelSpaceBound.x, maxModelSpaceBound.y, maxModelSpaceBound.z)));
		// Apply a fast visibility test, which might return true even when the object is not visible.
		return this->cullFrustum.isConvexHullSeen(corners, 8);
	}
};

}

#endif
