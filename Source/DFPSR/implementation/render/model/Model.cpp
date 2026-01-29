// zlib open source license
//
// Copyright (c) 2017 to 2025 David Forsgren Piuva
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

#include "Model.h"
#include "../constants.h"
#include "../../../api/imageAPI.h"
#include "../../../api/textureAPI.h"
#include "../../../base/virtualStack.h"

using namespace dsr;

#define CHECK_PART_INDEX(PART_INDEX, EXIT_STMT) if (PART_INDEX < 0 || PART_INDEX >= this->partBuffer.length()) { printText(U"Part index ", PART_INDEX, U" is out of range 0..", this->partBuffer.length() - 1, U"!\n"); EXIT_STMT; }
#define CHECK_POLYGON_INDEX(PART_PTR, POLYGON_INDEX, EXIT_STMT) if (POLYGON_INDEX < 0 || POLYGON_INDEX >= PART_PTR->polygonBuffer.length()) { printText(U"Polygon index ", POLYGON_INDEX, U" is out of range 0..", PART_PTR->polygonBuffer.length() - 1, U"!\n"); EXIT_STMT; }
#define CHECK_POINT_INDEX(POINT_INDEX, EXIT_STMT) if (POINT_INDEX < 0 || POINT_INDEX >= this->positionBuffer.length()) { printText(U"Position index ", POINT_INDEX, U" is out of range 0..", this->positionBuffer.length() - 1, U"!\n"); EXIT_STMT; }
#define CHECK_PART_POLYGON_INDEX(PART_INDEX, POLYGON_INDEX, EXIT_STMT) { \
	CHECK_PART_INDEX(PART_INDEX, EXIT_STMT); \
	const Part *PartPtr = &(this->partBuffer[PART_INDEX]); \
	CHECK_POLYGON_INDEX(PartPtr, POLYGON_INDEX, EXIT_STMT); \
}
#define CHECK_VERTEX_INDEX(VERTEX_INDEX, EXIT_STMT) if (VERTEX_INDEX < 0 || VERTEX_INDEX > 3) { printText(U"Vertex index ", VERTEX_INDEX, U" is out of the fixed range 0..3 for triangles and quads!\n"); EXIT_STMT; }

Polygon::Polygon(const Vertex &vertA, const Vertex &vertB, const Vertex &vertC) {
	this->pointIndices[0] = vertA.pointIndex;
	this->pointIndices[1] = vertB.pointIndex;
	this->pointIndices[2] = vertC.pointIndex;
	this->pointIndices[3] = -1;
	this->texCoords[0] = vertA.data.texCoord;
	this->texCoords[1] = vertB.data.texCoord;
	this->texCoords[2] = vertC.data.texCoord;
	this->texCoords[3] = FVector4D();
	this->colors[0] = vertA.data.color;
	this->colors[1] = vertB.data.color;
	this->colors[2] = vertC.data.color;
	this->colors[3] = FVector4D();
}

Polygon::Polygon(const Vertex &vertA, const Vertex &vertB, const Vertex &vertC, const Vertex &vertD) {
	this->pointIndices[0] = vertA.pointIndex;
	this->pointIndices[1] = vertB.pointIndex;
	this->pointIndices[2] = vertC.pointIndex;
	this->pointIndices[3] = vertD.pointIndex;
	this->texCoords[0] = vertA.data.texCoord;
	this->texCoords[1] = vertB.data.texCoord;
	this->texCoords[2] = vertC.data.texCoord;
	this->texCoords[3] = vertD.data.texCoord;
	this->colors[0] = vertA.data.color;
	this->colors[1] = vertB.data.color;
	this->colors[2] = vertC.data.color;
	this->colors[3] = vertD.data.color;
}

Polygon::Polygon(int32_t indexA, int32_t indexB, int32_t indexC) {
	this->pointIndices[0] = indexA;
	this->pointIndices[1] = indexB;
	this->pointIndices[2] = indexC;
	this->pointIndices[3] = -1;
	this->texCoords[0] = FVector4D(0.0f, 0.0f, 0.0f, 0.0f);
	this->texCoords[1] = FVector4D(1.0f, 0.0f, 1.0f, 0.0f);
	this->texCoords[2] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
	this->texCoords[3] = FVector4D(0.0f, 1.0f, 0.0f, 1.0f);
	this->colors[0] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
	this->colors[1] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
	this->colors[2] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
	this->colors[3] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
}

Polygon::Polygon(int32_t indexA, int32_t indexB, int32_t indexC, int32_t indexD) {
	this->pointIndices[0] = indexA;
	this->pointIndices[1] = indexB;
	this->pointIndices[2] = indexC;
	this->pointIndices[3] = indexD;
	this->texCoords[0] = FVector4D(0.0f, 0.0f, 0.0f, 0.0f);
	this->texCoords[1] = FVector4D(1.0f, 0.0f, 1.0f, 0.0f);
	this->texCoords[2] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
	this->texCoords[3] = FVector4D(0.0f, 1.0f, 0.0f, 1.0f);
	this->colors[0] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
	this->colors[1] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
	this->colors[2] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
	this->colors[3] = FVector4D(1.0f, 1.0f, 1.0f, 1.0f);
}

int32_t Polygon::getVertexCount() const {
	if (this->pointIndices[0] < 0) {
		return 0;
	} else if (this->pointIndices[1] < 0) {
		return 1;
	} else if (this->pointIndices[2] < 0) {
		return 2;
	} else if (this->pointIndices[3] < 0) {
		return 3;
	} else {
		return 4;
	}
}

Part::Part(const ReadableString &name) : name(name) {}
Part::Part(const TextureRgbaU8 &diffuseMap, const TextureRgbaU8 &lightMap, const List<Polygon> &polygonBuffer, const String &name) :
  diffuseMap(diffuseMap), lightMap(lightMap), polygonBuffer(polygonBuffer), name(name) {}
Part Part::clone() const { return Part(this->diffuseMap, this->lightMap, this->polygonBuffer, this->name); }
int32_t Part::getPolygonCount() const {
	return this->polygonBuffer.length();
}
int32_t Part::getPolygonVertexCount(int32_t polygonIndex) const {
	CHECK_POLYGON_INDEX(this, polygonIndex, return -1);
	return this->polygonBuffer[polygonIndex].getVertexCount();
}

// Precondition:
//   TODO: Make a "validated" flag to check reference integrity before drawing models
//         Only decreasing the length of the point buffer, changing a position index or adding new polygons should set it to false
//         Only running validation before rendering should set it from false to true
//   point indices may not go outside of projected's array range
static void renderTriangleFromPolygon(CommandQueue *commandQueue, const ImageRgbaU8 &targetImage, const ImageF32 &depthBuffer, const Camera &camera, const Polygon &polygon, int32_t triangleIndex, const ProjectedPoint *projected, Filter filter, const TextureRgbaU8 &diffuse, const TextureRgbaU8 &light) {
	// Triangle fan starting from the first vertex of the polygon
	int32_t indexA = 0;
	int32_t indexB = 1 + triangleIndex;
	int32_t indexC = 2 + triangleIndex;
	ProjectedPoint posA = projected[polygon.pointIndices[indexA]];
	ProjectedPoint posB = projected[polygon.pointIndices[indexB]];
	ProjectedPoint posC = projected[polygon.pointIndices[indexC]];
	// Read texture coordinates and convert to planar format in the constructor
	TriangleTexCoords texCoords(polygon.texCoords[indexA], polygon.texCoords[indexB], polygon.texCoords[indexC]);
	// Read colors and convert to planar format in the constructor
	TriangleColors colors(polygon.colors[indexA], polygon.colors[indexB], polygon.colors[indexC]);
	renderTriangleFromData(commandQueue, targetImage, depthBuffer, camera, posA, posB, posC, filter, diffuse, light, texCoords, colors);
}

void Part::render(CommandQueue *commandQueue, const ImageRgbaU8 &targetImage, const ImageF32 &depthBuffer, const Transform3D &modelToWorldTransform, const Camera &camera, Filter filter, const ProjectedPoint* projected) const {
	for (int32_t p = 0; p < this->polygonBuffer.length(); p++) {
		Polygon polygon = this->polygonBuffer[p];
		if (polygon.pointIndices[3] == -1) {
			// Render triangle
			renderTriangleFromPolygon(commandQueue, targetImage, depthBuffer, camera, polygon, 0, projected, filter, this->diffuseMap, this->lightMap);
		} else {
			// Render quad
			renderTriangleFromPolygon(commandQueue, targetImage, depthBuffer, camera, polygon, 0, projected, filter, this->diffuseMap, this->lightMap);
			renderTriangleFromPolygon(commandQueue, targetImage, depthBuffer, camera, polygon, 1, projected, filter, this->diffuseMap, this->lightMap);
		}
	}
}

void Part::renderDepth(const ImageF32 &depthBuffer, const Transform3D &modelToWorldTransform, const Camera &camera, const ProjectedPoint* projected) const {
	for (int32_t p = 0; p < this->polygonBuffer.length(); p++) {
		Polygon polygon = this->polygonBuffer[p];
		if (polygon.pointIndices[3] == -1) {
			// Render triangle
			ProjectedPoint posA = projected[polygon.pointIndices[0]];
			ProjectedPoint posB = projected[polygon.pointIndices[1]];
			ProjectedPoint posC = projected[polygon.pointIndices[2]];
			renderTriangleFromDataDepth(depthBuffer, camera, posA, posB, posC);
		} else {
			// Render quad
			ProjectedPoint posA = projected[polygon.pointIndices[0]];
			ProjectedPoint posB = projected[polygon.pointIndices[1]];
			ProjectedPoint posC = projected[polygon.pointIndices[2]];
			ProjectedPoint posD = projected[polygon.pointIndices[3]];
			renderTriangleFromDataDepth(depthBuffer, camera, posA, posB, posC);
			renderTriangleFromDataDepth(depthBuffer, camera, posA, posC, posD);
		}
	}
}

void ModelImpl::render(CommandQueue *commandQueue, const ImageRgbaU8 &targetImage, const ImageF32 &depthBuffer, const Transform3D &modelToWorldTransform, const Camera &camera) const {
	if (camera.isBoxSeen(this->minBound, this->maxBound, modelToWorldTransform)) {
		// Transform and project all vertices
		int32_t positionCount = positionBuffer.length();
		VirtualStackAllocation<ProjectedPoint> projected(positionCount, "Projected points in ModelImpl::render");
		for (int32_t vert = 0; vert < positionCount; vert++) {
			projected[vert] = camera.worldToScreen(modelToWorldTransform.transformPoint(positionBuffer[vert]));
		}
		for (int32_t partIndex = 0; partIndex < this->partBuffer.length(); partIndex++) {
			this->partBuffer[partIndex].render(commandQueue, targetImage, depthBuffer, modelToWorldTransform, camera, this->filter, projected.getUnsafe());
		}
	}
}

void ModelImpl::renderDepth(const ImageF32 &depthBuffer, const Transform3D &modelToWorldTransform, const Camera &camera) const {
	if (camera.isBoxSeen(this->minBound, this->maxBound, modelToWorldTransform)) {
		// Transform and project all vertices
		int32_t positionCount = positionBuffer.length();
		VirtualStackAllocation<ProjectedPoint> projected(positionCount, "Projected points in ModelImpl::renderDepth");
		for (int32_t vert = 0; vert < positionCount; vert++) {
			projected[vert] = camera.worldToScreen(modelToWorldTransform.transformPoint(positionBuffer[vert]));
		}
		for (int32_t partIndex = 0; partIndex < this->partBuffer.length(); partIndex++) {
			this->partBuffer[partIndex].renderDepth(depthBuffer, modelToWorldTransform, camera, projected.getUnsafe());
		}
	}
}

ModelImpl::ModelImpl() {}
ModelImpl::ModelImpl(Filter filter, const List<Part> &partBuffer, const List<FVector3D> &positionBuffer) :
  filter(filter),
  positionBuffer(positionBuffer),
  partBuffer(partBuffer) {}
ModelImpl::ModelImpl(const ModelImpl &old) :
  filter(old.filter),
  positionBuffer(old.positionBuffer),
  partBuffer(old.partBuffer) {}
int32_t ModelImpl::addEmptyPart(const String& name) {
	return this->partBuffer.pushConstructGetIndex(name);
}
int32_t ModelImpl::getNumberOfParts() const {
	return this->partBuffer.length();
}
void ModelImpl::setPartName(int32_t partIndex, const String &name) {
	CHECK_PART_INDEX(partIndex, return);
	this->partBuffer[partIndex].name = name;
}
String ModelImpl::getPartName(int32_t partIndex) const {
	CHECK_PART_INDEX(partIndex, return U"");
	return this->partBuffer[partIndex].name;
}
TextureRgbaU8 ModelImpl::getDiffuseMap(int32_t partIndex) const {
	CHECK_PART_INDEX(partIndex, return TextureRgbaU8());
	return this->partBuffer[partIndex].diffuseMap;
}
void ModelImpl::setDiffuseMap(const TextureRgbaU8 &diffuseMap, int32_t partIndex) {
	CHECK_PART_INDEX(partIndex, return);
	this->partBuffer[partIndex].diffuseMap = diffuseMap;
}
void ModelImpl::setDiffuseMapByName(ResourcePool &pool, const String &filename, int32_t partIndex) {
	CHECK_PART_INDEX(partIndex, return);
	const TextureRgbaU8 texture = pool.fetchTextureRgba(filename, 5);
	if (texture_exists(texture)) {
		this->setDiffuseMap(texture, partIndex);
	}
}
TextureRgbaU8 ModelImpl::getLightMap(int32_t partIndex) const {
	CHECK_PART_INDEX(partIndex, return TextureRgbaU8());
	return this->partBuffer[partIndex].lightMap;
}
void ModelImpl::setLightMap(const TextureRgbaU8 &lightMap, int32_t partIndex) {
	CHECK_PART_INDEX(partIndex, return);
	this->partBuffer[partIndex].lightMap = lightMap;
}
void ModelImpl::setLightMapByName(ResourcePool &pool, const String &filename, int32_t partIndex) {
	CHECK_PART_INDEX(partIndex, return);
	const TextureRgbaU8 texture = pool.fetchTextureRgba(filename, 1); // TODO: Allow configuring the number of mip levels and selecting a sampler somehow.
	if (texture_exists(texture)) {
		this->setLightMap(texture, partIndex);
	}
}
int32_t ModelImpl::addPolygon(Polygon polygon, int32_t partIndex) {
	CHECK_PART_INDEX(partIndex, return -1);
	return this->partBuffer[partIndex].polygonBuffer.pushGetIndex(polygon);
}
int32_t ModelImpl::getNumberOfPolygons(int32_t partIndex) const {
	CHECK_PART_INDEX(partIndex, return -1);
	return this->partBuffer[partIndex].getPolygonCount();
}
int32_t ModelImpl::getPolygonVertexCount(int32_t partIndex, int32_t polygonIndex) const {
	CHECK_PART_INDEX(partIndex, return -1);
	return this->partBuffer[partIndex].getPolygonVertexCount(polygonIndex);
}
int32_t ModelImpl::getNumberOfPoints() const {
	return this->positionBuffer.length();
}
void ModelImpl::expandBound(const FVector3D& point) {
	if (this->minBound.x > point.x) { this->minBound.x = point.x; }
	if (this->minBound.y > point.y) { this->minBound.y = point.y; }
	if (this->minBound.z > point.z) { this->minBound.z = point.z; }
	if (this->maxBound.x < point.x) { this->maxBound.x = point.x; }
	if (this->maxBound.y < point.y) { this->maxBound.y = point.y; }
	if (this->maxBound.z < point.z) { this->maxBound.z = point.z; }
}
int32_t ModelImpl::findPoint(const FVector3D &position, float threshold) const {
	float bestDistance = threshold;
	int32_t bestIndex = -1;
	for (int32_t index = 0; index < this->positionBuffer.length(); index++) {
		float distance = length(position - this->getPoint(index));
		if (distance < bestDistance) {
			bestDistance = distance;
			bestIndex = index;
		}
	}
	return bestIndex;
}
FVector3D ModelImpl::getPoint(int32_t pointIndex) const {
	CHECK_POINT_INDEX(pointIndex, return FVector3D());
	return this->positionBuffer[pointIndex];
}
void ModelImpl::setPoint(int32_t pointIndex, const FVector3D& position) {
	CHECK_POINT_INDEX(pointIndex, return);
	this->expandBound(position);
	this->positionBuffer[pointIndex] = position;
}
int32_t ModelImpl::addPoint(const FVector3D &position) {
	this->expandBound(position);
	return this->positionBuffer.pushGetIndex(position);
}
int32_t ModelImpl::addPointIfNeeded(const FVector3D &position, float threshold) {
	int32_t existingIndex = this->findPoint(position, threshold);
	if (existingIndex > -1) {
		return existingIndex;
	} else {
		return addPoint(position);
	}
}

int32_t ModelImpl::getVertexPointIndex(int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex) const {
	CHECK_PART_POLYGON_INDEX(partIndex, polygonIndex, return -1);
	CHECK_VERTEX_INDEX(vertexIndex, return -1);
	return partBuffer[partIndex].polygonBuffer[polygonIndex].pointIndices[vertexIndex];
}
void ModelImpl::setVertexPointIndex(int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex, int32_t pointIndex) {
	CHECK_PART_POLYGON_INDEX(partIndex, polygonIndex, return);
	CHECK_VERTEX_INDEX(vertexIndex, return);
	partBuffer[partIndex].polygonBuffer[polygonIndex].pointIndices[vertexIndex] = pointIndex;
}
FVector3D ModelImpl::getVertexPosition(int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex) const {
	int32_t pointIndex = getVertexPointIndex(partIndex, polygonIndex, vertexIndex);
	if (pointIndex > -1 && pointIndex < this->getNumberOfPoints()) {
		return this->getPoint(pointIndex);
	} else {
		return FVector3D();
	}
}
FVector4D ModelImpl::getVertexColor(int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex) const {
	CHECK_PART_POLYGON_INDEX(partIndex, polygonIndex, return FVector4D());
	CHECK_VERTEX_INDEX(vertexIndex, return FVector4D());
	return partBuffer[partIndex].polygonBuffer[polygonIndex].colors[vertexIndex];
}
void ModelImpl::setVertexColor(int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex, const FVector4D& color) {
	CHECK_PART_POLYGON_INDEX(partIndex, polygonIndex, return);
	CHECK_VERTEX_INDEX(vertexIndex, return);
	partBuffer[partIndex].polygonBuffer[polygonIndex].colors[vertexIndex] = color;
}
FVector4D ModelImpl::getTexCoord(int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex) const {
	CHECK_PART_POLYGON_INDEX(partIndex, polygonIndex, return FVector4D());
	CHECK_VERTEX_INDEX(vertexIndex, return FVector4D());
	return partBuffer[partIndex].polygonBuffer[polygonIndex].texCoords[vertexIndex];
}
void ModelImpl::setTexCoord(int32_t partIndex, int32_t polygonIndex, int32_t vertexIndex, const FVector4D& texCoord) {
	CHECK_PART_POLYGON_INDEX(partIndex, polygonIndex, return);
	CHECK_VERTEX_INDEX(vertexIndex, return);
	partBuffer[partIndex].polygonBuffer[polygonIndex].texCoords[vertexIndex] = texCoord;
}

