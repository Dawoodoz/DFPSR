// zlib open source license
//
// Copyright (c) 2019 to 2025 David Forsgren Piuva
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

#define DSR_INTERNAL_ACCESS

#include "rendererAPI.h"
#include "imageAPI.h"
#include "drawAPI.h"
#include "../implementation/render/renderCore.h"
#include "../base/virtualStack.h"
#include <limits>

#define MUST_EXIST(OBJECT, METHOD) if (OBJECT.isNull()) { throwError("The " #OBJECT " handle was null in " #METHOD "\n"); }

namespace dsr {

static const int32_t cellSize = 16;

static bool counterClockwise(const ProjectedPoint& p, const ProjectedPoint& q, const ProjectedPoint& r) {
	return (q.flat.y - p.flat.y) * (r.flat.x - q.flat.x) - (q.flat.x - p.flat.x) * (r.flat.y - q.flat.y) < 0;
}

// outputHullCorners must be at least as big as inputHullCorners, so that it can hold the worst case output size.
// Instead of not allowing less than three points, it copies the input as output when it happens to reduce pre-conditions.
static void jarvisConvexHullAlgorithm(ProjectedPoint* outputHullCorners, int32_t& outputCornerCount, const ProjectedPoint* inputHullCorners, int32_t inputCornerCount) {
	if (inputCornerCount < 3) {
		outputCornerCount = inputCornerCount;
		for (int32_t p = 0; p < inputCornerCount; p++) {
			outputHullCorners[p] = inputHullCorners[p];
		}
	} else {
		int32_t l = 0;
		outputCornerCount = 0;
		for (int32_t i = 1; i < inputCornerCount; i++) {
			if (inputHullCorners[i].flat.x < inputHullCorners[l].flat.x) {
				l = i;
			}
		}
		int32_t p = l;
		do {
			if (outputCornerCount >= inputCornerCount) {
				// Prevent getting stuck in an infinite loop from overflow
				return;
			}
			outputHullCorners[outputCornerCount] = inputHullCorners[p]; outputCornerCount++;
			int32_t q = (p + 1) % inputCornerCount;
			for (int32_t i = 0; i < inputCornerCount; i++) {
				if (counterClockwise(inputHullCorners[p], inputHullCorners[i], inputHullCorners[q])) {
					q = i;
				}
			}
			p = q;
		} while (p != l);
	}
}

// Transform and project the corners of a hull, so that the output can be given to the convex hull algorithm and used for occluding
// Returns true if occluder culling passed, which may skip occluders that could have been visible
static bool projectHull(ProjectedPoint* outputHullCorners, const FVector3D* inputHullCorners, int32_t cornerCount, const Transform3D &modelToWorldTransform, const Camera &camera) {
	for (int32_t p = 0; p < cornerCount; p++) {
		FVector3D worldPoint = modelToWorldTransform.transformPoint(inputHullCorners[p]);
		FVector3D cameraPoint = camera.worldToCamera(worldPoint);
		FVector3D narrowPoint = cameraPoint * FVector3D(0.5f, 0.5f, 1.0f);
		for (int32_t s = 0; s < camera.cullFrustum.getPlaneCount(); s++) {
			FPlane3D plane = camera.cullFrustum.getPlane(s);
			if (!plane.inside(narrowPoint)) {
				return false;
			}
		}
		outputHullCorners[p] = camera.cameraToScreen(cameraPoint);
	}
	return true;
}

static IRect getPixelBoundFromProjection(const ProjectedPoint* convexHullCorners, int32_t cornerCount) {
	IRect result = IRect(convexHullCorners[0].flat.x / constants::unitsPerPixel, convexHullCorners[0].flat.y / constants::unitsPerPixel, 1, 1);
	for (int32_t p = 1; p < cornerCount; p++) {
		result = IRect::merge(result, IRect(convexHullCorners[p].flat.x / constants::unitsPerPixel, convexHullCorners[p].flat.y / constants::unitsPerPixel, 1, 1));
	}
	return result;
}

static bool pointInsideOfEdge(const LVector2D &edgeA, const LVector2D &edgeB, const LVector2D &point) {
	LVector2D edgeDirection = LVector2D(edgeB.y - edgeA.y, edgeA.x - edgeB.x);
	LVector2D relativePosition = point - edgeA;
	return (edgeDirection.x * relativePosition.x) + (edgeDirection.y * relativePosition.y) <= 0;
}

// Returns true iff the point is inside of the hull
// convexHullCorners from 0 to cornerCount-1 must be sorted clockwise and may not include any concave corners
static bool pointInsideOfHull(const ProjectedPoint* convexHullCorners, int32_t cornerCount, const LVector2D &point) {
	for (int32_t c = 0; c < cornerCount; c++) {
		int32_t nc = c + 1;
		if (nc == cornerCount) {
			nc = 0;
		}
		if (!pointInsideOfEdge(convexHullCorners[c].flat, convexHullCorners[nc].flat, point)) {
			// Outside of one edge, not inside
			return false;
		}
	}
	// Passed all edge tests
	return true;
}

// Returns true iff all corners of the rectangle are inside of the hull
static bool rectangleInsideOfHull(const ProjectedPoint* convexHullCorners, int32_t cornerCount, const IRect &rectangle) {
	return pointInsideOfHull(convexHullCorners, cornerCount, LVector2D(rectangle.left(), rectangle.top()))
		&& pointInsideOfHull(convexHullCorners, cornerCount, LVector2D(rectangle.right(), rectangle.top()))
		&& pointInsideOfHull(convexHullCorners, cornerCount, LVector2D(rectangle.left(), rectangle.bottom()))
		&& pointInsideOfHull(convexHullCorners, cornerCount, LVector2D(rectangle.right(), rectangle.bottom()));
}

struct DebugLine {
	int64_t x1, y1, x2, y2;
	ColorRgbaI32 color;
	DebugLine(int64_t x1, int64_t y1, int64_t x2, int64_t y2, const ColorRgbaI32& color)
	: x1(x1), y1(y1), x2(x2), y2(y2), color(color) {}
};

// Context for multi-threaded rendering of triangles in a command queue
struct RendererImpl {
	bool receiving = false; // Preventing version dependency by only allowing calls in the expected order
	ImageRgbaU8 colorBuffer; // The color image being rendered to
	ImageF32 depthBuffer; // Linear depth for isometric cameras, 1 / depth for perspective cameras
	ImageF32 depthGrid; // An occlusion grid of cellSize² cells representing the longest linear depth where something might be visible
	CommandQueue commandQueue; // Triangles to be drawn
	List<DebugLine> debugLines; // Additional lines to be drawn as an overlay for debugging occlusion
	int32_t width = 0, height = 0, gridWidth = 0, gridHeight = 0;
	bool occluded = false;
	RendererImpl() {}
	void beginFrame(ImageRgbaU8& colorBuffer, ImageF32& depthBuffer) {
		if (this->receiving) {
			throwError("Called renderer_begin on the same renderer twice without ending the previous batch!\n");
		}
		this->receiving = true;
		this->colorBuffer = colorBuffer;
		this->depthBuffer = depthBuffer;
		if (image_exists(this->colorBuffer)) {
			this->width = image_getWidth(this->colorBuffer);
			this->height = image_getHeight(this->colorBuffer);
		} else if (image_exists(this->depthBuffer)) {
			this->width = image_getWidth(this->depthBuffer);
			this->height = image_getHeight(this->depthBuffer);
		}
		this->gridWidth = (this->width + (cellSize - 1)) / cellSize;
		this->gridHeight = (this->height + (cellSize - 1)) / cellSize;
		this->occluded = false;
	}
	IRect getOuterCellBound(const IRect &pixelBound) const {
		int32_t minCellX = pixelBound.left() / cellSize;
		int32_t maxCellX = pixelBound.right() / cellSize + 1;
		int32_t minCellY = pixelBound.top() / cellSize;
		int32_t maxCellY = pixelBound.bottom() / cellSize + 1;
		if (minCellX < 0) { minCellX = 0; }
		if (minCellY < 0) { minCellY = 0; }
		if (maxCellX > this->gridWidth) { maxCellX = this->gridWidth; }
		if (maxCellY > this->gridHeight) { maxCellY = this->gridHeight; }
		return IRect(minCellX, minCellY, maxCellX - minCellX, maxCellY - minCellY);
	}
	// Called before occluding so that the grid is initialized once when used and skipped when not used
	void prepareForOcclusion() {
		if (!this->occluded) {
			// Allocate the grid if a sufficiently large one does not already exist
			if (!(image_exists(this->depthGrid) && image_getWidth(this->depthGrid) >= gridWidth && image_getHeight(this->depthGrid) >= gridHeight)) {
				this->depthGrid = image_create_F32(gridWidth, gridHeight);
			}
			// Use inifnite depth in camera space
			image_fill(this->depthGrid, std::numeric_limits<float>::infinity());
		}
		this->occluded = true;
	}
	// If any occluder has been used during this pass, all triangles in the buffer will be filtered based using depthGrid
	void completeOcclusion() {
		if (this->occluded) {
			for (int32_t t = this->commandQueue.buffer.length() - 1; t >= 0; t--) {
				bool anyVisible = false;
				ITriangle2D triangle = this->commandQueue.buffer[t].triangle;
				IRect outerBound = getOuterCellBound(triangle.wholeBound);
				for (int32_t cellY = outerBound.top(); cellY < outerBound.bottom(); cellY++) {
					for (int32_t cellX = outerBound.left(); cellX < outerBound.right(); cellX++) {
						// TODO: Optimize access using SafePointer iteration
						float backgroundDepth = image_readPixel_clamp(this->depthGrid, cellX, cellY);
						float triangleDepth = triangle.position[0].cs.z;
						replaceWithSmaller(triangleDepth, triangle.position[1].cs.z);
						replaceWithSmaller(triangleDepth, triangle.position[2].cs.z);
						if (triangleDepth < backgroundDepth + 0.001) {
							anyVisible = true;
						}
					}
				}
				if (!anyVisible) {
					// TODO: Make triangle swapping work so that the list can be sorted
					this->commandQueue.buffer[t].occluded = true;
				}
			}
		}
	}
	void occludeFromSortedHull(const ProjectedPoint* convexHullCorners, int32_t cornerCount, const IRect& pixelBound) {
		// Loop over the outer bound
		if (pixelBound.width() > cellSize && pixelBound.height() > cellSize) {
			float distance = 0.0f;
			for (int32_t c = 0; c < cornerCount; c++) {
				replaceWithLarger(distance, convexHullCorners[c].cs.z);
			}
			// Loop over all cells within the bound
			IRect outerBound = getOuterCellBound(pixelBound);
			for (int32_t cellY = outerBound.top(); cellY < outerBound.bottom(); cellY++) {
				for (int32_t cellX = outerBound.left(); cellX < outerBound.right(); cellX++) {
					IRect pixelRegion = IRect(cellX * cellSize, cellY * cellSize, cellSize, cellSize);
					IRect subPixelRegion = pixelRegion * constants::unitsPerPixel;
					if (rectangleInsideOfHull(convexHullCorners, cornerCount, subPixelRegion)) {
						float oldDepth = image_readPixel_clamp(this->depthGrid, cellX, cellY);
						if (distance < oldDepth) {
							image_writePixel(this->depthGrid, cellX, cellY, distance);
						}
					}
				}
			}
		}
	}
	void occludeFromSortedHull(const ProjectedPoint* convexHullCorners, int32_t cornerCount) {
		occludeFromSortedHull(convexHullCorners, cornerCount, getPixelBoundFromProjection(convexHullCorners, cornerCount));
	}
	void occludeFromExistingTriangles() {
		if (!this->receiving) {
			throwError("Cannot call renderer_occludeFromExistingTriangles without first calling renderer_begin!\n");
		}
		prepareForOcclusion();
		// Generate a depth grid to remove many small triangles behind larger triangles
		//   This will leave triangles along seams but at least begin to remove the worst unwanted drawing
		for (int32_t t = 0; t < this->commandQueue.buffer.length(); t++) {
			// Get the current triangle from the queue
			Filter filter = this->commandQueue.buffer[t].filter;
			if (filter == Filter::Solid) {
				ITriangle2D triangle = this->commandQueue.buffer[t].triangle;
				occludeFromSortedHull(triangle.position, 3, triangle.wholeBound);
			}
		}
	}
	#define GENERATE_BOX_CORNERS(TARGET, MIN, MAX) \
		TARGET[0] = FVector3D(MIN.x, MIN.y, MIN.z); \
		TARGET[1] = FVector3D(MIN.x, MIN.y, MAX.z); \
		TARGET[2] = FVector3D(MIN.x, MAX.y, MIN.z); \
		TARGET[3] = FVector3D(MIN.x, MAX.y, MAX.z); \
		TARGET[4] = FVector3D(MAX.x, MIN.y, MIN.z); \
		TARGET[5] = FVector3D(MAX.x, MIN.y, MAX.z); \
		TARGET[6] = FVector3D(MAX.x, MAX.y, MIN.z); \
		TARGET[7] = FVector3D(MAX.x, MAX.y, MAX.z);
	// Fills the occlusion grid using the box, so that things behind it can skip rendering
	void occludeFromBox(const FVector3D& minimum, const FVector3D& maximum, const Transform3D &modelToWorldTransform, const Camera &camera, bool debugSilhouette) {
		if (!this->receiving) {
			throwError("Cannot call renderer_occludeFromBox without first calling renderer_begin!\n");
		}
		prepareForOcclusion();
		static const int32_t pointCount = 8;
		FVector3D localPoints[pointCount];
		ProjectedPoint projections[pointCount];
		ProjectedPoint edgeCorners[pointCount];
		GENERATE_BOX_CORNERS(localPoints, minimum, maximum)
		if (projectHull(projections, localPoints, 8, modelToWorldTransform, camera)) {
			// Get a 2D convex hull from the projected corners
			int32_t edgeCornerCount = 0;
			jarvisConvexHullAlgorithm(edgeCorners, edgeCornerCount, projections, 8);
			occludeFromSortedHull(edgeCorners, edgeCornerCount);
			// Allow saving the 2D silhouette for debugging
			if (debugSilhouette) {
				for (int32_t p = 0; p < edgeCornerCount; p++) {
					int32_t q = (p + 1) % edgeCornerCount;
					if (projections[p].cs.z > camera.nearClip) {
						this->debugLines.pushConstruct(
						  edgeCorners[p].flat.x / constants::unitsPerPixel, edgeCorners[p].flat.y / constants::unitsPerPixel,
						  edgeCorners[q].flat.x / constants::unitsPerPixel, edgeCorners[q].flat.y / constants::unitsPerPixel,
						  ColorRgbaI32(0, 255, 255, 255)
						);
					}
				}
			}
		}
	}
	// Occlusion test for whole model bounds.
	// Returns false if the convex hull of the corners has a chance to be seen from the camera.
	bool isHullOccluded(ProjectedPoint* outputHullCorners, const FVector3D* inputHullCorners, int32_t cornerCount, const Transform3D &modelToWorldTransform, const Camera &camera) const {
		VirtualStackAllocation<FVector3D> cameraPoints(cornerCount);
		for (int32_t p = 0; p < cornerCount; p++) {
			cameraPoints[p] = camera.worldToCamera(modelToWorldTransform.transformPoint(inputHullCorners[p]));
			outputHullCorners[p] = camera.cameraToScreen(cameraPoints[p]);
		}
		// Culling test to see if all points are outside of the same plane of the view frustum.
		for (int32_t s = 0; s < camera.cullFrustum.getPlaneCount(); s++) {
			bool allOutside = true; // True until prooven false.
			FPlane3D plane = camera.cullFrustum.getPlane(s);
			for (int32_t p = 0; p < cornerCount; p++) {
				if (plane.inside(cameraPoints[p])) {
					// One point was inside of this plane, so it can not guarantee that all interpolated points between the corners are outside.
					allOutside = false;
					break;
				}
			}
			// If all points are outside of the same plane in the view frustum...
			if (allOutside) {
				// ...then we know that all interpolated points in between are also outside of this plane.
				return true; // Occluded due to failing culling test.
			}
		}
		IRect pixelBound = getPixelBoundFromProjection(outputHullCorners, cornerCount);
		float closestDistance = std::numeric_limits<float>::infinity();
		for (int32_t c = 0; c < cornerCount; c++) {
			replaceWithSmaller(closestDistance, outputHullCorners[c].cs.z);
		}
		// Loop over all cells within the bound
		IRect outerBound = getOuterCellBound(pixelBound);
		for (int32_t cellY = outerBound.top(); cellY < outerBound.bottom(); cellY++) {
			for (int32_t cellX = outerBound.left(); cellX < outerBound.right(); cellX++) {
				if (closestDistance < image_readPixel_clamp(this->depthGrid, cellX, cellY)) {
					return false; // Visible because one cell had a more distant maximum depth.
				}
			}
		}
		return true; // Occluded, because none of the cells had a more distant depth.
	}
	// Checks if the box from minimum to maximum in object space is fully occluded when seen by the camera
	// Must be the same camera as when occluders filled the grid with occlusion depth
	bool isBoxOccluded(const FVector3D &minimum, const FVector3D &maximum, const Transform3D &modelToWorldTransform, const Camera &camera) const {
		if (!this->receiving) {
			throwError("Cannot call renderer_isBoxVisible without first calling renderer_begin and giving occluder shapes to the pass!\n");
		}
		FVector3D corners[8];
		GENERATE_BOX_CORNERS(corners, minimum, maximum)
		ProjectedPoint projections[8];
		return isHullOccluded(projections, corners, 8, modelToWorldTransform, camera);
	}
	void endFrame(bool debugWireframe) {
		if (!this->receiving) {
			throwError("Called renderer_end without renderer_begin!\n");
		}
		this->receiving = false;
		// Mark occluded triangles to prevent them from being rendered
		completeOcclusion();
		this->commandQueue.execute(IRect::FromSize(this->width, this->height));
		if (image_exists(this->colorBuffer)) {
			// Debug drawn triangles
			if (debugWireframe) {
				/*if (image_exists(this->depthGrid)) {
					for (int32_t cellY = 0; cellY < this->gridHeight; cellY++) {
						for (int32_t cellX = 0; cellX < this->gridWidth; cellX++) {
							float depth = image_readPixel_clamp(this->depthGrid, cellX, cellY);
							if (depth < std::numeric_limits<float>::infinity()) {
								int32_t intensity = depth;
								draw_rectangle(this->colorBuffer, IRect(cellX * cellSize + 4, cellY * cellSize + 4, cellSize - 8, cellSize - 8), ColorRgbaI32(intensity, intensity, 0, 255));
							}
						}
					}
				}*/
				for (int32_t t = 0; t < this->commandQueue.buffer.length(); t++) {
					if (!this->commandQueue.buffer[t].occluded) {
						ITriangle2D *triangle = &(this->commandQueue.buffer[t].triangle);
						draw_line(this->colorBuffer,
						  triangle->position[0].flat.x / constants::unitsPerPixel, triangle->position[0].flat.y / constants::unitsPerPixel,
						  triangle->position[1].flat.x / constants::unitsPerPixel, triangle->position[1].flat.y / constants::unitsPerPixel,
						  ColorRgbaI32(255, 255, 255, 255)
						);
						draw_line(this->colorBuffer,
						  triangle->position[1].flat.x / constants::unitsPerPixel, triangle->position[1].flat.y / constants::unitsPerPixel,
						  triangle->position[2].flat.x / constants::unitsPerPixel, triangle->position[2].flat.y / constants::unitsPerPixel,
						  ColorRgbaI32(255, 255, 255, 255)
						);
						draw_line(this->colorBuffer,
						  triangle->position[2].flat.x / constants::unitsPerPixel, triangle->position[2].flat.y / constants::unitsPerPixel,
						  triangle->position[0].flat.x / constants::unitsPerPixel, triangle->position[0].flat.y / constants::unitsPerPixel,
						  ColorRgbaI32(255, 255, 255, 255)
						);
					}
				}
			}
			// Debug anything else added to debugLines
			for (int32_t l = 0; l < this->debugLines.length(); l++) {
				draw_line(this->colorBuffer, this->debugLines[l].x1, this->debugLines[l].y1, this->debugLines[l].x2, this->debugLines[l].y2, this->debugLines[l].color);
			}
			this->debugLines.clear();
		}
		this->commandQueue.clear();
	}
	void occludeFromTopRows(const Camera &camera) {
		// Make sure that the depth grid exists with the correct dimensions.
		this->prepareForOcclusion();
		if (!this->receiving) {
			throwError("Cannot call renderer_occludeFromTopRows without first calling renderer_begin!\n");
		}
		if (!image_exists(this->depthBuffer)) {
			throwError("Cannot call renderer_occludeFromTopRows without having given a depth buffer in renderer_begin!\n");
		}
		SafePointer<float> depthRow = image_getSafePointer(this->depthBuffer);
		int32_t depthStride = image_getStride(this->depthBuffer);
		SafePointer<float> gridRow = image_getSafePointer(this->depthGrid);
		int32_t gridStride = image_getStride(this->depthGrid);
		if (camera.perspective) {
			// Perspective case using 1/depth for the depth buffer.
			for (int32_t y = 0; y < this->height; y += cellSize) {
				SafePointer<float> gridPixel = gridRow;
				SafePointer<float> depthPixel = depthRow;
				int32_t x = 0;
				int32_t right = cellSize - 1;
				float maxInvDistance;
				// Scan bottom row of whole cell width
				for (int32_t gridX = 0; gridX < this->gridWidth; gridX++) {
					maxInvDistance = std::numeric_limits<float>::infinity();
					if (right >= this->width) { right = this->width; }
					while (x < right) {
						float newInvDistance = *depthPixel;
						if (newInvDistance < maxInvDistance) { maxInvDistance = newInvDistance; }
						depthPixel += 1;
						x += 1;
					}
					float maxDistance = 1.0f / maxInvDistance;
					float oldDistance = *gridPixel;
					if (maxDistance < oldDistance) {
						*gridPixel = maxDistance;
					}
					gridPixel += 1;
					right += cellSize;
				}
				// Go to the next grid row
				depthRow.increaseBytes(depthStride * cellSize);
				gridRow.increaseBytes(gridStride);
			}
		} else {
			// Orthogonal case where linear depth is used for both grid and depth buffer.
			// TODO: Create test cases for many ways to use occlusion, even these strange cases like isometric occlusion where plain culling does not leave many occluded models.
			for (int32_t y = 0; y < this->height; y += cellSize) {
				SafePointer<float> gridPixel = gridRow;
				SafePointer<float> depthPixel = depthRow;
				int32_t x = 0;
				int32_t right = cellSize - 1;
				float maxDistance;
				// Scan bottom row of whole cell width
				for (int32_t gridX = 0; gridX < this->gridWidth; gridX++) {
					maxDistance = 0.0f;
					if (right >= this->width) { right = this->width; }
					while (x < right) {
						float newDistance = *depthPixel;
						if (newDistance > maxDistance) { maxDistance = newDistance; }
						depthPixel += 1;
						x += 1;
					}
					float oldDistance = *gridPixel;
					if (maxDistance < oldDistance) {
						*gridPixel = maxDistance;
					}
					gridPixel += 1;
					right += cellSize;
				}
				// Go to the next grid row
				depthRow.increaseBytes(depthStride * cellSize);
				gridRow.increaseBytes(gridStride);
			}
		}
	}
};

ImageRgbaU8 renderer_getColorBuffer(const Renderer& renderer) {
	MUST_EXIST(renderer, renderer_getColorBuffer);
	return renderer->receiving ? renderer->colorBuffer : ImageRgbaU8();
}

ImageF32 renderer_getDepthBuffer(const Renderer& renderer) {
	MUST_EXIST(renderer, renderer_getDepthBuffer);
	return renderer->receiving ? renderer->depthBuffer : ImageF32();
}

Renderer renderer_create() {
	return handle_create<RendererImpl>().setName("Renderer");
}

bool renderer_exists(const Renderer& renderer) {
	return renderer.isNotNull();
}

void renderer_begin(Renderer& renderer, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer) {
	MUST_EXIST(renderer, renderer_begin);
	renderer->beginFrame(colorBuffer, depthBuffer);
}

void renderer_giveTask_triangle(Renderer& renderer,
  const ProjectedPoint &posA, const ProjectedPoint &posB, const ProjectedPoint &posC,
  const FVector4D &colorA, const FVector4D &colorB, const FVector4D &colorC,
  const FVector4D &texCoordA, const FVector4D &texCoordB, const FVector4D &texCoordC,
  const TextureRgbaU8& diffuseMap, const TextureRgbaU8& lightMap,
  Filter filter, const Camera &camera) {
	#ifndef NDEBUG
		MUST_EXIST(renderer, renderer_addTriangle);
	#endif
	renderTriangleFromData(
	  &(renderer->commandQueue), renderer->colorBuffer, renderer->depthBuffer, camera,
	  posA, posB, posC,
	  filter, diffuseMap, lightMap,
	  TriangleTexCoords(texCoordA, texCoordB, texCoordC),
	  TriangleColors(colorA, colorB, colorC)
	);
}

void renderer_occludeFromBox(Renderer& renderer, const FVector3D& minimum, const FVector3D& maximum, const Transform3D &modelToWorldTransform, const Camera &camera, bool debugSilhouette) {
	#ifndef NDEBUG
		MUST_EXIST(renderer, renderer_occludeFromBox);
	#endif
	renderer->occludeFromBox(minimum, maximum, modelToWorldTransform, camera, debugSilhouette);
}

void renderer_occludeFromExistingTriangles(Renderer& renderer) {
	MUST_EXIST(renderer, renderer_optimize);
	renderer->occludeFromExistingTriangles();
}

void renderer_occludeFromTopRows(Renderer& renderer, const Camera &camera) {
	MUST_EXIST(renderer, renderer_occludeFromTopRows);
	renderer->occludeFromTopRows(camera);
}

bool renderer_isBoxVisible(const Renderer& renderer, const FVector3D &minimum, const FVector3D &maximum, const Transform3D &modelToWorldTransform, const Camera &camera) {
	#ifndef NDEBUG
		MUST_EXIST(renderer, renderer_isBoxVisible);
	#endif
	return !(renderer->isBoxOccluded(minimum, maximum, modelToWorldTransform, camera));
}

void renderer_end(Renderer& renderer, bool debugWireframe) {
	MUST_EXIST(renderer, renderer_end);
	renderer->endFrame(debugWireframe);
}

bool renderer_takesTriangles(const Renderer& renderer) {
	#ifndef NDEBUG
		MUST_EXIST(renderer, renderer_isReceivingTriangles);
	#endif
	return renderer->receiving;
}

bool renderer_hasOccluders(const Renderer& renderer) {
	#ifndef NDEBUG
		MUST_EXIST(renderer, renderer_hasOccluders);
	#endif
	return renderer->occluded;
}

}
