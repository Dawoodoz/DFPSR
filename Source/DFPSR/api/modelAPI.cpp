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

#include "modelAPI.h"
#include "imageAPI.h"
#include "drawAPI.h"
// TODO: Inline as much as possible from Model.h to modelAPI.cpp, to reduce call depth and make it easy to copy and modify the model implementation.
#include "../implementation/render/model/Model.h"
#include "../base/virtualStack.h"
#include <limits>

#define MUST_EXIST(OBJECT, METHOD) if (OBJECT.isNull()) { throwError(U"The " #OBJECT U" handle was null in " #METHOD U"\n"); }

namespace dsr {

Model model_create() {
	return handle_create<ModelImpl>().setName("Model");
}

Model model_clone(const Model& model) {
	MUST_EXIST(model,model_clone);
	return handle_create<ModelImpl>(model->filter, model->partBuffer, model->positionBuffer).setName("Cloned Model");
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
	return model.isNotNull();
}

int32_t model_addEmptyPart(Model& model, const String &name) {
	MUST_EXIST(model,model_addEmptyPart);
	return model->addEmptyPart(name);
}

int32_t model_getNumberOfParts(const Model& model) {
	MUST_EXIST(model,model_getNumberOfParts);
	return model->getNumberOfParts();
}

void model_setPartName(Model& model, int32_t partIndex, const String &name) {
	MUST_EXIST(model,model_setPartName);
	model->setPartName(partIndex, name);
}

String model_getPartName(const Model& model, int32_t partIndex) {
	MUST_EXIST(model,model_getPartName);
	return model->getPartName(partIndex);
}

int32_t model_getNumberOfPoints(const Model& model) {
	MUST_EXIST(model,model_getNumberOfPoints);
	return model->getNumberOfPoints();
}

FVector3D model_getPoint(const Model& model, int32_t pointIndex) {
	MUST_EXIST(model,model_getPoint);
	return model->getPoint(pointIndex);
}

void model_setPoint(Model& model, int32_t pointIndex, const FVector3D& position) {
	MUST_EXIST(model,model_setPoint);
	model->setPoint(pointIndex, position);
}

int32_t model_findPoint(const Model& model, const FVector3D &position, float threshold) {
	MUST_EXIST(model,model_findPoint);
	return model->findPoint(position, threshold);
}

int32_t model_addPoint(const Model& model, const FVector3D &position) {
	MUST_EXIST(model,model_addPoint);
	return model->addPoint(position);
}

int32_t model_addPointIfNeeded(Model& model, const FVector3D &position, float threshold) {
	MUST_EXIST(model,model_addPointIfNeeded);
	return model->addPointIfNeeded(position, threshold);
}

int32_t model_getVertexPointIndex(const Model& model, int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex) {
	MUST_EXIST(model,model_getVertexPointIndex);
	return model->getVertexPointIndex(partIndex, polygonIndex, vertexIndex);
}

void model_setVertexPointIndex(Model& model, int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex, int32_t pointIndex) {
	MUST_EXIST(model,model_setVertexPointIndex);
	model->setVertexPointIndex(partIndex, polygonIndex, vertexIndex, pointIndex);
}

FVector3D model_getVertexPosition(const Model& model, int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex) {
	MUST_EXIST(model,model_getVertexPosition);
	return model->getVertexPosition(partIndex, polygonIndex, vertexIndex);
}

FVector4D model_getVertexColor(const Model& model, int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex) {
	MUST_EXIST(model,model_getVertexColor);
	return model->getVertexColor(partIndex, polygonIndex, vertexIndex);
}

void model_setVertexColor(Model& model, int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex, const FVector4D& color) {
	MUST_EXIST(model,model_setVertexColor);
	model->setVertexColor(partIndex, polygonIndex, vertexIndex, color);
}

FVector4D model_getTexCoord(const Model& model, int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex) {
	MUST_EXIST(model,model_getTexCoord);
	return model->getTexCoord(partIndex, polygonIndex, vertexIndex);
}

void model_setTexCoord(Model& model, int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex, const FVector4D& texCoord) {
	MUST_EXIST(model,model_setTexCoord);
	model->setTexCoord(partIndex, polygonIndex, vertexIndex, texCoord);
}

int32_t model_addTriangle(Model& model, int32_t partIndex, int32_t pointA, int32_t pointB, int32_t pointC) {
	MUST_EXIST(model,model_addTriangle);
	return model->addPolygon(Polygon(pointA, pointB, pointC), partIndex);
}

int32_t model_addQuad(Model& model, int32_t partIndex, int32_t pointA, int32_t pointB, int32_t pointC, int32_t pointD) {
	MUST_EXIST(model,model_addQuad);
	return model->addPolygon(Polygon(pointA, pointB, pointC, pointD), partIndex);
}

int32_t model_getNumberOfPolygons(const Model& model, int32_t partIndex) {
	MUST_EXIST(model,model_getNumberOfPolygons);
	return model->getNumberOfPolygons(partIndex);
}

int32_t model_getPolygonVertexCount(const Model& model, int32_t partIndex, int32_t polygonIndex) {
	MUST_EXIST(model,model_getPolygonVertexCount);
	return model->getPolygonVertexCount(partIndex, polygonIndex);
}

TextureRgbaU8 model_getDiffuseMap(const Model& model, int32_t partIndex) {
	MUST_EXIST(model,model_getDiffuseMap);
	return model->getDiffuseMap(partIndex);
}

void model_setDiffuseMap(Model& model, int32_t partIndex, const TextureRgbaU8 &diffuseMap) {
	MUST_EXIST(model,model_setDiffuseMap);
	model->setDiffuseMap(diffuseMap, partIndex);
}

void model_setDiffuseMapByName(Model& model, int32_t partIndex, ResourcePool &pool, const String &filename) {
	MUST_EXIST(model,model_setDiffuseMapByName);
	model->setDiffuseMapByName(pool, filename, partIndex);
}

TextureRgbaU8 model_getLightMap(Model& model, int32_t partIndex) {
	MUST_EXIST(model,model_getLightMap);
	return model->getLightMap(partIndex);
}

void model_setLightMap(Model& model, int32_t partIndex, const TextureRgbaU8 &lightMap) {
	MUST_EXIST(model,model_setLightMap);
	model->setLightMap(lightMap, partIndex);
}

void model_setLightMapByName(Model& model, int32_t partIndex, ResourcePool &pool, const String &filename) {
	MUST_EXIST(model,model_setLightMapByName);
	model->setLightMapByName(pool, filename, partIndex);
}

// Single-threaded rendering for the simple cases where you just want it to work
void model_render(const Model& model, const Transform3D &modelToWorldTransform, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer, const Camera &camera) {
	if (model.isNotNull()) {
		model->render((CommandQueue*)nullptr, colorBuffer, depthBuffer, modelToWorldTransform, camera);
	}
}
void model_renderDepth(const Model& model, const Transform3D &modelToWorldTransform, ImageF32& depthBuffer, const Camera &camera) {
	if (model.isNotNull()) {
		model->renderDepth(depthBuffer, modelToWorldTransform, camera);
	}
}

void model_getBoundingBox(const Model& model, FVector3D& minimum, FVector3D& maximum) {
	MUST_EXIST(model,model_getBoundingBox);
	minimum = model->minBound;
	maximum = model->maxBound;
}

void model_render_threaded(const Model& model, const Transform3D &modelToWorldTransform, Renderer& renderer, const Camera &camera) {
	MUST_EXIST(renderer, renderer_giveTask);
	// Skip rendering if we do not have any model.
	if (model.isNull()) {
		return;
	}
	// Check the renderer's state.
	#ifndef NDEBUG
		if (!renderer_takesTriangles(renderer)) {
			throwError(U"Cannot call renderer_giveTask before renderer_begin!\n");
		}
	#endif
	// Culling.
	if (!camera.isBoxSeen(model->minBound, model->maxBound, modelToWorldTransform)) return;
	// Occlusion.
	if (renderer_hasOccluders(renderer)) {
		FVector3D minimum, maximum;
		model_getBoundingBox(model, minimum, maximum);
		if (!renderer_isBoxVisible(renderer, minimum, maximum, modelToWorldTransform, camera)) return;
	}
	// Render the model by calling the renderer API for each triangle.
	// Get the filter.
	Filter filter = model->filter;
	// Transform and project all vertices.
	int32_t positionCount = model->positionBuffer.length();
	VirtualStackAllocation<ProjectedPoint> projected(positionCount, "Projected points in renderer_giveTask");
	for (int32_t vert = 0; vert < positionCount; vert++) {
		projected[vert] = camera.worldToScreen(modelToWorldTransform.transformPoint(model->positionBuffer[vert]));
	}
	for (int32_t partIndex = 0; partIndex < model->partBuffer.length(); partIndex++) {
		// Get a pointer to the current part.
		Part *part = &(model->partBuffer[partIndex]);
		// Get textures.
		const TextureRgbaU8 diffuse = part->diffuseMap;
		const TextureRgbaU8 light = part->lightMap;
		for (int32_t p = 0; p < part->polygonBuffer.length(); p++) {
			Polygon polygon = part->polygonBuffer[p];
			// Render first triangle in the polygon of indices 0, 1, 2.
			renderer_giveTask_triangle(renderer,
			  projected[polygon.pointIndices[0]],
			  projected[polygon.pointIndices[1]],
			  projected[polygon.pointIndices[2]],
			  polygon.colors[0],
			  polygon.colors[1],
			  polygon.colors[2],
			  polygon.texCoords[0],
			  polygon.texCoords[1],
			  polygon.texCoords[2],
			  diffuse, light, filter, camera
			);
			if (polygon.pointIndices[3] != -1) {
				// Render second triangle in the polygon of indices 0, 2, 3 to form a quad polygon.
				renderer_giveTask_triangle(renderer,
				  projected[polygon.pointIndices[0]],
				  projected[polygon.pointIndices[2]],
				  projected[polygon.pointIndices[3]],
				  polygon.colors[0],
				  polygon.colors[2],
				  polygon.colors[3],
				  polygon.texCoords[0],
				  polygon.texCoords[2],
				  polygon.texCoords[3],
				  diffuse, light, filter, camera
				);
			}
		}
	}
}

}
