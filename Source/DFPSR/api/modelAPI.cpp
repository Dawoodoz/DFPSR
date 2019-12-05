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
#include "../render/model/Model.h"

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

int model_findPoint(const Model& model, const FVector3D &position, float treshold) {
	MUST_EXIST(model,model_findPoint);
	return model->findPoint(position, treshold);
}

int model_addPoint(const Model& model, const FVector3D &position) {
	MUST_EXIST(model,model_addPoint);
	return model->addPoint(position);
}

int model_addPointIfNeeded(Model& model, const FVector3D &position, float treshold) {
	MUST_EXIST(model,model_addPointIfNeeded);
	return model->addPointIfNeeded(position, treshold);
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

// TODO: Change the backend's argument order for partIndex or simply inline all of it's functionality
void model_setDiffuseMap(Model& model, int partIndex, const ImageRgbaU8 &diffuseMap) {
	MUST_EXIST(model,model_setDiffuseMap);
	model->setDiffuseMap(diffuseMap, partIndex);
}

// TODO: Change the backend's argument order for partIndex or simply inline all of it's functionality
void model_setDiffuseMapByName(Model& model, int partIndex, ResourcePool &pool, const String &filename) {
	MUST_EXIST(model,model_setDiffuseMapByName);
	model->setDiffuseMapByName(pool, filename, partIndex);
}

ImageRgbaU8 model_getLightMap(Model& model, int partIndex) {
	MUST_EXIST(model,model_getLightMap);
	return model->getLightMap(partIndex);
}

// TODO: Change the backend's argument order for partIndex or simply inline all of it's functionality
void model_setLightMap(Model& model, int partIndex, const ImageRgbaU8 &lightMap) {
	MUST_EXIST(model,model_setLightMap);
	model->setLightMap(lightMap, partIndex);
}

// TODO: Change the backend's argument order for partIndex or simply inline all of it's functionality
void model_setLightMapByName(Model& model, int partIndex, ResourcePool &pool, const String &filename) {
	MUST_EXIST(model,model_setLightMapByName);
	model->setLightMapByName(pool, filename, partIndex);
}

// Single-threaded rendering for the simple cases where you just want it to work
void model_render(const Model& model, const Transform3D &modelToWorldTransform, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer, const Camera &camera) {
	MUST_EXIST(model,model_render);
	model->render((CommandQueue*)nullptr, colorBuffer, depthBuffer, modelToWorldTransform, camera);
}
void model_renderDepth(const Model& model, const Transform3D &modelToWorldTransform, ImageF32& depthBuffer, const Camera &camera) {
	MUST_EXIST(model,model_renderDepth);
	model->renderDepth(depthBuffer, modelToWorldTransform, camera);
}

// Context for rendering multiple models at the same time for improved speed
class RendererImpl {
private:
	bool receiving = false; // Preventing version dependency by only allowing calls in the expected order
	ImageRgbaU8 colorBuffer;
	ImageF32 depthBuffer;
	CommandQueue commandQueue;
public:
	RendererImpl() {}
	void beginFrame(ImageRgbaU8& colorBuffer, ImageF32& depthBuffer) {
		if (this->receiving) {
			throwError("Called renderer_begin on the same renderer twice without ending the previous batch!\n");
		}
		this->receiving = true;
		this->colorBuffer = colorBuffer;
		this->depthBuffer = depthBuffer;
	}
	void giveTask(const Model& model, const Transform3D &modelToWorldTransform, const Camera &camera) {
		if (!this->receiving) {
			throwError("Cannot call renderer_giveTask before renderer_begin!\n");
		}
		// TODO: Make an algorithm for selecting if the model should be queued as an instance or triangulated at once
		//       An extra argument may choose to force an instance directly into the command queue
		//           Because the model is being borrowed for vertex animation
		//           To prevent the command queue from getting full hold as much as possible in a sorted list of instances
		//           When the command queue is full, the solid
		model->render(&this->commandQueue, this->colorBuffer, this->depthBuffer, modelToWorldTransform, camera);
	}
	void endFrame() {
		if (!this->receiving) {
			throwError("Called renderer_end without renderer_begin!\n");
		}
		this->receiving = false;
		if (image_exists(this->colorBuffer)) {
			this->commandQueue.execute(IRect::FromSize(image_getWidth(this->colorBuffer), image_getHeight(this->colorBuffer)));
		} else if (image_exists(this->depthBuffer)) {
			this->commandQueue.execute(IRect::FromSize(image_getWidth(this->depthBuffer), image_getHeight(this->depthBuffer)));
		}
		this->commandQueue.clear();
	}
};

Renderer renderer_create() {
	return std::make_shared<RendererImpl>();
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
	renderer->giveTask(model, modelToWorldTransform, camera);
}

void renderer_end(Renderer& renderer) {
	MUST_EXIST(renderer,renderer_end);
	renderer->endFrame();
}

}

