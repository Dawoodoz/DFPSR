// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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

#include "modelAPI.h"
#include "imageAPI.h"
#include "drawAPI.h"
#include "../render/model/Model.h"
#include <limits>

#define MUST_EXIST(OBJECT, METHOD) if (OBJECT.get() == nullptr) { throwError("The " #OBJECT " handle was null in " #METHOD "\n"); }

namespace dsr {

Model model_create() {
	return std::make_shared<ModelImpl>();
}

Model model_clone(const Model& model) {
	MUST_EXIST(model,model_clone);
	return std::make_shared<ModelImpl>(model->filter, model->partBuffer, model->positionBuffer);
}

void model_setFilter(const Model& model, Filter filter) {
	MUST_EXIST(model,model_setFilter);
	model->filter = filter;
}

Filter model_getFilter(const Model& model) {
	MUST_EXIST(model,model_getFilter);
	return model->filter;
}

bool model_exists(const Model& model) {
	return model.get() != nullptr;
}

int model_addEmptyPart(Model& model, const String &name) {
	MUST_EXIST(model,model_addEmptyPart);
	return model->addEmptyPart(name);
}

int model_getNumberOfParts(const Model& model) {
	MUST_EXIST(model,model_getNumberOfParts);
	return model->getNumberOfParts();
}

void model_setPartName(Model& model, int partIndex, const String &name) {
	MUST_EXIST(model,model_setPartName);
	model->setPartName(partIndex, name);
}

String model_getPartName(const Model& model, int partIndex) {
	MUST_EXIST(model,model_getPartName);
	return model->getPartName(partIndex);
}

int model_getNumberOfPoints(const Model& model) {
	MUST_EXIST(model,model_getNumberOfPoints);
	return model->getNumberOfPoints();
}

FVector3D model_getPoint(const Model& model, int pointIndex) {
	MUST_EXIST(model,model_getPoint);
	return model->getPoint(pointIndex);
}

void model_setPoint(Model& model, int pointIndex, const FVector3D& position) {
	MUST_EXIST(model,model_setPoint);
	model->setPoint(pointIndex, position);
}

int model_findPoint(const Model& model, const FVector3D &position, float threshold) {
	MUST_EXIST(model,model_findPoint);
	return model->findPoint(position, threshold);
}

int model_addPoint(const Model& model, const FVector3D &position) {
	MUST_EXIST(model,model_addPoint);
	return model->addPoint(position);
}

int model_addPointIfNeeded(Model& model, const FVector3D &position, float threshold) {
	MUST_EXIST(model,model_addPointIfNeeded);
	return model->addPointIfNeeded(position, threshold);
}

int model_getVertexPointIndex(const Model& model, int partIndex, int polygonIndex, int vertexIndex) {
	MUST_EXIST(model,model_getVertexPointIndex);
	return model->getVertexPointIndex(partIndex, polygonIndex, vertexIndex);
}

void model_setVertexPointIndex(Model& model, int partIndex, int polygonIndex, int vertexIndex, int pointIndex) {
	MUST_EXIST(model,model_setVertexPointIndex);
	model->setVertexPointIndex(partIndex, polygonIndex, vertexIndex, pointIndex);
}

FVector3D model_getVertexPosition(const Model& model, int partIndex, int polygonIndex, int vertexIndex) {
	MUST_EXIST(model,model_getVertexPosition);
	return model->getVertexPosition(partIndex, polygonIndex, vertexIndex);
}

FVector4D model_getVertexColor(const Model& model, int partIndex, int polygonIndex, int vertexIndex) {
	MUST_EXIST(model,model_getVertexColor);
	return model->getVertexColor(partIndex, polygonIndex, vertexIndex);
}

void model_setVertexColor(Model& model, int partIndex, int polygonIndex, int vertexIndex, const FVector4D& color) {
	MUST_EXIST(model,model_setVertexColor);
	model->setVertexColor(partIndex, polygonIndex, vertexIndex, color);
}

FVector4D model_getTexCoord(const Model& model, int partIndex, int polygonIndex, int vertexIndex) {
	MUST_EXIST(model,model_getTexCoord);
	return model->getTexCoord(partIndex, polygonIndex, vertexIndex);
}

void model_setTexCoord(Model& model, int partIndex, int polygonIndex, int vertexIndex, const FVector4D& texCoord) {
	MUST_EXIST(model,model_setTexCoord);
	model->setTexCoord(partIndex, polygonIndex, vertexIndex, texCoord);
}

int model_addTriangle(Model& model, int partIndex, int pointA, int pointB, int pointC) {
	MUST_EXIST(model,model_addTriangle);
	return model->addPolygon(Polygon(pointA, pointB, pointC), partIndex);
}

int model_addQuad(Model& model, int partIndex, int pointA, int pointB, int pointC, int pointD) {
	MUST_EXIST(model,model_addQuad);
	return model->addPolygon(Polygon(pointA, pointB, pointC, pointD), partIndex);
}

int model_getNumberOfPolygons(const Model& model, int partIndex) {
	MUST_EXIST(model,model_getNumberOfPolygons);
	return model->getNumberOfPolygons(partIndex);
}

int model_getPolygonVertexCount(const Model& model, int partIndex, int polygonIndex) {
	MUST_EXIST(model,model_getPolygonVertexCount);
	return model->getPolygonVertexCount(partIndex, polygonIndex);
}

ImageRgbaU8 model_getDiffuseMap(const Model& model, int partIndex) {
	MUST_EXIST(model,model_getDiffuseMap);
	return model->getDiffuseMap(partIndex);
}

void model_setDiffuseMap(Model& model, int partIndex, const ImageRgbaU8 &diffuseMap) {
	MUST_EXIST(model,model_setDiffuseMap);
	model->setDiffuseMap(diffuseMap, partIndex);
}

void model_setDiffuseMapByName(Model& model, int partIndex, ResourcePool &pool, const String &filename) {
	MUST_EXIST(model,model_setDiffuseMapByName);
	model->setDiffuseMapByName(pool, filename, partIndex);
}

ImageRgbaU8 model_getLightMap(Model& model, int partIndex) {
	MUST_EXIST(model,model_getLightMap);
	return model->getLightMap(partIndex);
}

void model_setLightMap(Model& model, int partIndex, const ImageRgbaU8 &lightMap) {
	MUST_EXIST(model,model_setLightMap);
	model->setLightMap(lightMap, partIndex);
}

void model_setLightMapByName(Model& model, int partIndex, ResourcePool &pool, const String &filename) {
	MUST_EXIST(model,model_setLightMapByName);
	model->setLightMapByName(pool, filename, partIndex);
}

// Single-threaded rendering for the simple cases where you just want it to work
void model_render(const Model& model, const Transform3D &modelToWorldTransform, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer, const Camera &camera) {
	if (model.get() != nullptr) {
		model->render((CommandQueue*)nullptr, colorBuffer, depthBuffer, modelToWorldTransform, camera);
	}
}
void model_renderDepth(const Model& model, const Transform3D &modelToWorldTransform, ImageF32& depthBuffer, const Camera &camera) {
	if (model.get() != nullptr) {
		model->renderDepth(depthBuffer, modelToWorldTransform, camera);
	}
}

void model_getBoundingBox(const Model& model, FVector3D& minimum, FVector3D& maximum) {
	MUST_EXIST(model,model_getBoundingBox);
	minimum = model->minBound;
	maximum = model->maxBound;
}

static const int cellSize = 16;

struct DebugLine {
	int64_t x1, y1, x2, y2;
	ColorRgbaI32 color;
	DebugLine(int64_t x1, int64_t y1, int64_t x2, int64_t y2, const ColorRgbaI32& color)
	: x1(x1), y1(y1), x2(x2), y2(y2), color(color) {}
};

// Context for rendering multiple models at the same time for improved speed
class RendererImpl {
private:
	bool receiving = false; // Preventing version dependency by only allowing calls in the expected order
	ImageRgbaU8 colorBuffer; // The color image being rendered to
	ImageF32 depthBuffer; // Linear depth for isometric cameras, 1 / depth for perspective cameras
	ImageF32 depthGrid; // An occlusion grid of cellSize² cells representing the longest linear depth where something might be visible
	CommandQueue commandQueue; // Triangles to be drawn
	List<DebugLine> debugLines; // Additional lines to be drawn as an overlay for debugging occlusion
	int width = 0, height = 0, gridWidth = 0, gridHeight = 0;
	bool occluded = false;
public:
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
	bool pointInsideOfEdge(const LVector2D &edgeA, const LVector2D &edgeB, const LVector2D &point) {
		LVector2D edgeDirection = LVector2D(edgeB.y - edgeA.y, edgeA.x - edgeB.x);
		LVector2D relativePosition = point - edgeA;
		int64_t dotProduct = (edgeDirection.x * relativePosition.x) + (edgeDirection.y * relativePosition.y);
		return dotProduct <= 0;
	}
	// Returns true iff the point is inside of the hull
	// convexHullCorners from 0 to cornerCount-1 must be sorted clockwise and may not include any concave corners
	bool pointInsideOfHull(const ProjectedPoint* convexHullCorners, int cornerCount, const LVector2D &point) {
		for (int c = 0; c < cornerCount; c++) {
			int nc = c + 1;
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
	// Returns true iff all cornets of the rectangle are inside of the hull
	bool rectangleInsideOfHull(const ProjectedPoint* convexHullCorners, int cornerCount, const IRect &rectangle) {
		return pointInsideOfHull(convexHullCorners, cornerCount, LVector2D(rectangle.left(), rectangle.top()))
		    && pointInsideOfHull(convexHullCorners, cornerCount, LVector2D(rectangle.right(), rectangle.top()))
		    && pointInsideOfHull(convexHullCorners, cornerCount, LVector2D(rectangle.left(), rectangle.bottom()))
		    && pointInsideOfHull(convexHullCorners, cornerCount, LVector2D(rectangle.right(), rectangle.bottom()));
	}
	IRect getOuterCellBound(const IRect &pixelBound) {
		int minCellX = pixelBound.left() / cellSize;
		int maxCellX = pixelBound.right() / cellSize + 1;
		int minCellY = pixelBound.top() / cellSize;
		int maxCellY = pixelBound.bottom() / cellSize + 1;
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
			for (int t = this->commandQueue.buffer.length() - 1; t >= 0; t--) {
				bool anyVisible = false;
				ITriangle2D triangle = this->commandQueue.buffer[t].triangle;
				IRect outerBound = getOuterCellBound(triangle.wholeBound);
				for (int cellY = outerBound.top(); cellY < outerBound.bottom(); cellY++) {
					for (int cellX = outerBound.left(); cellX < outerBound.right(); cellX++) {
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
	void occludeFromSortedHull(const ProjectedPoint* convexHullCorners, int cornerCount, const IRect& pixelBound) {
		// Loop over the outer bound
		if (pixelBound.width() > cellSize && pixelBound.height() > cellSize) {
			float distance = 0.0f;
			for (int c = 0; c < cornerCount; c++) {
				replaceWithLarger(distance, convexHullCorners[c].cs.z);
			}
			// Loop over all cells within the bound
			IRect outerBound = getOuterCellBound(pixelBound);
			for (int cellY = outerBound.top(); cellY < outerBound.bottom(); cellY++) {
				for (int cellX = outerBound.left(); cellX < outerBound.right(); cellX++) {
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
	IRect getPixelBoundFromProjection(const ProjectedPoint* convexHullCorners, int cornerCount) {
		IRect result = IRect(convexHullCorners[0].flat.x / constants::unitsPerPixel, convexHullCorners[0].flat.y / constants::unitsPerPixel, 1, 1);
		for (int p = 1; p < cornerCount; p++) {
			result = IRect::merge(result, IRect(convexHullCorners[p].flat.x / constants::unitsPerPixel, convexHullCorners[p].flat.y / constants::unitsPerPixel, 1, 1));
		}
		return result;
	}
	void occludeFromSortedHull(const ProjectedPoint* convexHullCorners, int cornerCount) {
		occludeFromSortedHull(convexHullCorners, cornerCount, getPixelBoundFromProjection(convexHullCorners, cornerCount));
	}
	void occludeFromExistingTriangles() {
		prepareForOcclusion();
		// Generate a depth grid to remove many small triangles behind larger triangles
		//   This will leave triangles along seams but at least begin to remove the worst unwanted drawing
		for (int t = 0; t < this->commandQueue.buffer.length(); t++) {
			// Get the current triangle from the queue
			Filter filter = this->commandQueue.buffer[t].filter;
			if (filter == Filter::Solid) {
				ITriangle2D triangle = this->commandQueue.buffer[t].triangle;
				occludeFromSortedHull(triangle.position, 3, triangle.wholeBound);
			}
		}
	}
	bool counterClockwise(const ProjectedPoint& p, const ProjectedPoint& q, const ProjectedPoint& r) {
		return (q.flat.y - p.flat.y) * (r.flat.x - q.flat.x) - (q.flat.x - p.flat.x) * (r.flat.y - q.flat.y) < 0;
	}
	// outputHullCorners must be at least as big as inputHullCorners, so that it can hold the worst case output size.
	// Instead of not allowing less than three points, it copies the input as output when it happens to reduce pre-conditions.
	void jarvisConvexHullAlgorithm(ProjectedPoint* outputHullCorners, int& outputCornerCount, const ProjectedPoint* inputHullCorners, int inputCornerCount) {
		if (inputCornerCount < 3) {
			outputCornerCount = inputCornerCount;
			for (int p = 0; p < inputCornerCount; p++) {
				outputHullCorners[p] = inputHullCorners[p];
			}
		} else {
			int l = 0;
			outputCornerCount = 0;
			for (int i = 1; i < inputCornerCount; i++) {
				if (inputHullCorners[i].flat.x < inputHullCorners[l].flat.x) {
					l = i;
				}
			}
			int p = l;
			do {
				if (outputCornerCount >= inputCornerCount) {
					// Prevent getting stuck in an infinite loop from overflow
					return;
				}
				outputHullCorners[outputCornerCount] = inputHullCorners[p]; outputCornerCount++;
				int q = (p + 1) % inputCornerCount;
				for (int i = 0; i < inputCornerCount; i++) {
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
	bool projectHull(ProjectedPoint* outputHullCorners, const FVector3D* inputHullCorners, int cornerCount, const Transform3D &modelToWorldTransform, const Camera &camera) {
		for (int p = 0; p < cornerCount; p++) {
			FVector3D worldPoint = modelToWorldTransform.transformPoint(inputHullCorners[p]);
			FVector3D cameraPoint = camera.worldToCamera(worldPoint);
			FVector3D narrowPoint = cameraPoint * FVector3D(0.5f, 0.5f, 1.0f);
			for (int s = 0; s < camera.cullFrustum.getPlaneCount(); s++) {
				FPlane3D plane = camera.cullFrustum.getPlane(s);
				if (!plane.inside(narrowPoint)) {
					return false;
				}
			}
			outputHullCorners[p] = camera.cameraToScreen(cameraPoint);
		}
		return true;
	}
	void occludeFromBox(const FVector3D& minimum, const FVector3D& maximum, const Transform3D &modelToWorldTransform, const Camera &camera, bool debugSilhouette) {
		prepareForOcclusion();
		static const int pointCount = 8;
		FVector3D localPoints[pointCount];
		ProjectedPoint projections[pointCount];
		ProjectedPoint edgeCorners[pointCount];
		localPoints[0] = FVector3D(minimum.x, minimum.y, minimum.z);
		localPoints[1] = FVector3D(minimum.x, minimum.y, maximum.z);
		localPoints[2] = FVector3D(minimum.x, maximum.y, minimum.z);
		localPoints[3] = FVector3D(minimum.x, maximum.y, maximum.z);
		localPoints[4] = FVector3D(maximum.x, minimum.y, minimum.z);
		localPoints[5] = FVector3D(maximum.x, minimum.y, maximum.z);
		localPoints[6] = FVector3D(maximum.x, maximum.y, minimum.z);
		localPoints[7] = FVector3D(maximum.x, maximum.y, maximum.z);
		if (projectHull(projections, localPoints, 8, modelToWorldTransform, camera)) {
			// Get a 2D convex hull from the projected corners
			int edgeCornerCount = 0;
			jarvisConvexHullAlgorithm(edgeCorners, edgeCornerCount, projections, 8);
			occludeFromSortedHull(edgeCorners, edgeCornerCount);
			// Allow saving the 2D silhouette for debugging
			if (debugSilhouette) {
				for (int p = 0; p < edgeCornerCount; p++) {
					int q = (p + 1) % edgeCornerCount;
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
	// Occlusion test for whole model bounds
	bool isHullVisible(ProjectedPoint* outputHullCorners, const FVector3D* inputHullCorners, int cornerCount, const Transform3D &modelToWorldTransform, const Camera &camera) {
		for (int p = 0; p < cornerCount; p++) {
			FVector3D worldPoint = modelToWorldTransform.transformPoint(inputHullCorners[p]);
			FVector3D cameraPoint = camera.worldToCamera(worldPoint);
			outputHullCorners[p] = camera.cameraToScreen(cameraPoint);
		}
		IRect pixelBound = getPixelBoundFromProjection(outputHullCorners, cornerCount);
		
		float closestDistance = std::numeric_limits<float>::infinity();
		for (int c = 0; c < cornerCount; c++) {
			replaceWithSmaller(closestDistance, outputHullCorners[c].cs.z);
		}
		// Loop over all cells within the bound
		IRect outerBound = getOuterCellBound(pixelBound);
		for (int cellY = outerBound.top(); cellY < outerBound.bottom(); cellY++) {
			for (int cellX = outerBound.left(); cellX < outerBound.right(); cellX++) {
				if (closestDistance < image_readPixel_clamp(this->depthGrid, cellX, cellY)) {
					return true;
				}
			}
		}
		return false;
	}
	void giveTask(const Model& model, const Transform3D &modelToWorldTransform, const Camera &camera) {
		if (!this->receiving) {
			throwError("Cannot call renderer_giveTask before renderer_begin!\n");
		}
		// If occluders are present, check if the model's bound is visible
		if (this->occluded) {
			FVector3D minimum, maximum;
			model_getBoundingBox(model, minimum, maximum);
			FVector3D corners[8];
			ProjectedPoint projections[8];
			corners[0] = FVector3D(minimum.x, minimum.y, minimum.z);
			corners[1] = FVector3D(minimum.x, minimum.y, maximum.z);
			corners[2] = FVector3D(minimum.x, maximum.y, minimum.z);
			corners[3] = FVector3D(minimum.x, maximum.y, maximum.z);
			corners[4] = FVector3D(maximum.x, minimum.y, minimum.z);
			corners[5] = FVector3D(maximum.x, minimum.y, maximum.z);
			corners[6] = FVector3D(maximum.x, maximum.y, minimum.z);
			corners[7] = FVector3D(maximum.x, maximum.y, maximum.z);
			if (!isHullVisible(projections, corners, 8, modelToWorldTransform, camera)) {
				// Skip projection of triangles if the whole bounding box is already behind occluders
				return;
			}
		}
		// TODO: Make an algorithm for selecting if the model should be queued as an instance or triangulated at once
		//       An extra argument may choose to force an instance directly into the command queue
		//           Because the model is being borrowed for vertex animation
		//           To prevent the command queue from getting full hold as much as possible in a sorted list of instances
		//           When the command queue is full, the solid instances will be drawn front to back before filtered is drawn back to front
		model->render(&this->commandQueue, this->colorBuffer, this->depthBuffer, modelToWorldTransform, camera);
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
					for (int cellY = 0; cellY < this->gridHeight; cellY++) {
						for (int cellX = 0; cellX < this->gridWidth; cellX++) {
							float depth = image_readPixel_clamp(this->depthGrid, cellX, cellY);
							if (depth < std::numeric_limits<float>::infinity()) {
								int intensity = depth;
								draw_rectangle(this->colorBuffer, IRect(cellX * cellSize + 4, cellY * cellSize + 4, cellSize - 8, cellSize - 8), ColorRgbaI32(intensity, intensity, 0, 255));
							}
						}
					}
				}*/
				for (int t = 0; t < this->commandQueue.buffer.length(); t++) {
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
			for (int l = 0; l < this->debugLines.length(); l++) {
				draw_line(this->colorBuffer, this->debugLines[l].x1, this->debugLines[l].y1, this->debugLines[l].x2, this->debugLines[l].y2, this->debugLines[l].color);
			}
			this->debugLines.clear();
		}
		this->commandQueue.clear();
	}
};

Renderer renderer_create() {
	return std::make_shared<RendererImpl>();
}

bool renderer_exists(const Renderer& renderer) {
	return renderer.get() != nullptr;
}

void renderer_begin(Renderer& renderer, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer) {
	MUST_EXIST(renderer,renderer_begin);
	renderer->beginFrame(colorBuffer, depthBuffer);
}

// TODO: Synchronous setting
//       * Asynchronous (default)
//         Only works on models that are locked from further editing
//         Locked models can also be safely pooled for reuse then (ResourcePool)
//       * Synced (for animation)
//         Dispatch triangles directly to the command queue so that the current state of the model is captured
//         This allow rendering many instances using the same model at different times
//         Enabling vertex light, reflection maps and bone animation
void renderer_giveTask(Renderer& renderer, const Model& model, const Transform3D &modelToWorldTransform, const Camera &camera) {
	MUST_EXIST(renderer,renderer_giveTask);
	if (model.get() != nullptr) {
		renderer->giveTask(model, modelToWorldTransform, camera);
	}
}

void renderer_occludeFromBox(Renderer& renderer, const FVector3D& minimum, const FVector3D& maximum, const Transform3D &modelToWorldTransform, const Camera &camera, bool debugSilhouette) {
	MUST_EXIST(renderer,renderer_occludeFromBox);
	renderer->occludeFromBox(minimum, maximum, modelToWorldTransform, camera, debugSilhouette);
}

void renderer_occludeFromExistingTriangles(Renderer& renderer) {
	MUST_EXIST(renderer,renderer_optimize);
	renderer->occludeFromExistingTriangles();
}

void renderer_end(Renderer& renderer, bool debugWireframe) {
	MUST_EXIST(renderer,renderer_end);
	renderer->endFrame(debugWireframe);
}

}

