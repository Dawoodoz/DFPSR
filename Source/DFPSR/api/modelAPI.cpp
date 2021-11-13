﻿// zlib open source license
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

// Context for rendering multiple models at the same time for improved speed
class RendererImpl {
private:
	bool receiving = false; // Preventing version dependency by only allowing calls in the expected order
	ImageRgbaU8 colorBuffer; // The color image being rendered to
	ImageF32 depthBuffer; // Linear depth for isometric cameras, 1 / depth for perspective cameras
	ImageF32 depthGrid; // An occlusion grid of cellSize² cells representing the longest linear depth where something might be visible
	CommandQueue commandQueue;
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
	void giveTask(const Model& model, const Transform3D &modelToWorldTransform, const Camera &camera) {
		if (!this->receiving) {
			throwError("Cannot call renderer_giveTask before renderer_begin!\n");
		}
		// TODO: Make an algorithm for selecting if the model should be queued as an instance or triangulated at once
		//       An extra argument may choose to force an instance directly into the command queue
		//           Because the model is being borrowed for vertex animation
		//           To prevent the command queue from getting full hold as much as possible in a sorted list of instances
		//           When the command queue is full, the solid instances will be drawn front to back before filtered is drawn back to front
		model->render(&this->commandQueue, this->colorBuffer, this->depthBuffer, modelToWorldTransform, camera);
	}
	bool pointInsideOfEdge(const LVector2D &edgeA, const LVector2D &edgeB, const LVector2D &point) {
		LVector2D edgeDirection = LVector2D(edgeB.y - edgeA.y, edgeA.x - edgeB.x);
		LVector2D relativePosition = point - edgeA;
		int64_t dotProduct = (edgeDirection.x * relativePosition.x) + (edgeDirection.y * relativePosition.y);
		return dotProduct <= 0;
	}
	// Returns true iff the point is inside of the triangle
	bool pointInsideOfTriangle(const ITriangle2D &triangle, const LVector2D &point) {
		return pointInsideOfEdge(triangle.position[0].flat, triangle.position[1].flat, point)
		    && pointInsideOfEdge(triangle.position[1].flat, triangle.position[2].flat, point)
		    && pointInsideOfEdge(triangle.position[2].flat, triangle.position[0].flat, point);
	}
	// Returns true iff all cornets of the rectangle are inside of the triangle
	bool rectangleInsideOfTriangle(const ITriangle2D &triangle, const IRect &rectangle) {
		return pointInsideOfTriangle(triangle, LVector2D(rectangle.left(), rectangle.top()))
		    && pointInsideOfTriangle(triangle, LVector2D(rectangle.right(), rectangle.top()))
		    && pointInsideOfTriangle(triangle, LVector2D(rectangle.left(), rectangle.bottom()))
		    && pointInsideOfTriangle(triangle, LVector2D(rectangle.right(), rectangle.bottom()));
	}
	IRect getOuterCellBound(const ITriangle2D &triangle) {
		int minCellX = triangle.wholeBound.left() / cellSize;
		int maxCellX = triangle.wholeBound.right() / cellSize + 1;
		int minCellY = triangle.wholeBound.top() / cellSize;
		int maxCellY = triangle.wholeBound.bottom() / cellSize + 1;
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
				IRect outerBound = getOuterCellBound(triangle);
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
	void occludeFromExistingTriangles() {
		prepareForOcclusion();
		// Generate a depth grid to remove many small triangles behind larger triangles
		//   This will leave triangles along seams but at least begin to remove the worst unwanted drawing
		for (int t = 0; t < this->commandQueue.buffer.length(); t++) {
			// Get the current triangle from the queue
			Filter filter = this->commandQueue.buffer[t].filter;
			if (filter == Filter::Solid) {
				ITriangle2D triangle = this->commandQueue.buffer[t].triangle;
				// Loop over all cells within the bound
				IRect outerBound = getOuterCellBound(triangle);
				// Loop over the outer bound while excluding the outmost rows and columns that are unlikely to be fully occluded by the triangle
				if (triangle.wholeBound.width() > cellSize && triangle.wholeBound.height() > cellSize) {
					for (int cellY = outerBound.top(); cellY < outerBound.bottom(); cellY++) {
						for (int cellX = outerBound.left(); cellX < outerBound.right(); cellX++) {
							IRect pixelRegion = IRect(cellX * cellSize, cellY * cellSize, cellSize, cellSize);
							IRect subPixelRegion = pixelRegion * constants::unitsPerPixel;
							if (rectangleInsideOfTriangle(triangle, subPixelRegion)) {
								float oldDepth = image_readPixel_clamp(this->depthGrid, cellX, cellY);
								float newDepth = triangle.position[0].cs.z;
								replaceWithLarger(newDepth, triangle.position[1].cs.z);
								replaceWithLarger(newDepth, triangle.position[2].cs.z);
								if (newDepth < oldDepth) {
									image_writePixel(this->depthGrid, cellX, cellY, newDepth);
								}
							}
						}
					}
				}
			}
		}
	}
	void debugDrawTriangles() {
		if (image_exists(this->colorBuffer)) {
			if (image_exists(this->depthGrid)) {
				for (int cellY = 0; cellY < this->gridHeight; cellY++) {
					for (int cellX = 0; cellX < this->gridWidth; cellX++) {
						float depth = image_readPixel_clamp(this->depthGrid, cellX, cellY);
						if (depth < std::numeric_limits<float>::infinity()) {
							int intensity = depth;
							draw_rectangle(this->colorBuffer, IRect(cellX * cellSize + 4, cellY * cellSize + 4, cellSize - 8, cellSize - 8), ColorRgbaI32(intensity, intensity, 0, 255));
						}
					}
				}
			}
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
	}
	void endFrame(bool debugWireframe) {
		if (!this->receiving) {
			throwError("Called renderer_end without renderer_begin!\n");
		}
		this->receiving = false;
		completeOcclusion();
		this->commandQueue.execute(IRect::FromSize(this->width, this->height));
		if (debugWireframe) {
			this->debugDrawTriangles();
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

void renderer_occludeFromExistingTriangles(Renderer& renderer) {
	MUST_EXIST(renderer,renderer_optimize);
	renderer->occludeFromExistingTriangles();
}

void renderer_end(Renderer& renderer, bool debugWireframe) {
	MUST_EXIST(renderer,renderer_end);
	renderer->endFrame(debugWireframe);
}

}

