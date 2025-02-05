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

#ifndef DFPSR_RENDER_MODEL_POLYGONMODEL
#define DFPSR_RENDER_MODEL_POLYGONMODEL

#include <cstdint>
#include "../../../api/stringAPI.h"
#include "../../image/Texture.h"
#include "../shader/Shader.h"
#include "../Camera.h"
#include "../ResourcePool.h"
#include "../renderCore.h"
#include "../../../math/FVector.h"

namespace dsr {

// Only used when constructing new polygons
struct VertexData {
	FVector4D texCoord; // Two 2D coordinates or one 3D coordinate
	FVector4D color; // RGBA
	VertexData() : texCoord(FVector4D(0.0f, 0.0f, 0.0f, 0.0f)), color(FVector4D(1.0f, 1.0f, 1.0f, 1.0f)) {}
	VertexData(const FVector4D &texCoord, const FVector4D &color) : texCoord(texCoord), color(color) {}
};

// Only used when constructing new polygons
struct Vertex {
	int32_t pointIndex = -1; // Empty
	VertexData data;
	Vertex() {}
	Vertex(int32_t pointIndex, const VertexData &data) : pointIndex(pointIndex), data(data) {}
};

struct Polygon {
	static const int maxCorners = 4;
	int32_t pointIndices[maxCorners]; // pointIndices[3] equals -1 for triangles
	FVector4D texCoords[maxCorners];
	FVector4D colors[maxCorners];
	Polygon(const Vertex &vertA, const Vertex &vertB, const Vertex &vertC);
	Polygon(const Vertex &vertA, const Vertex &vertB, const Vertex &vertC, const Vertex &vertD);
	Polygon(int indexA, int indexB, int indexC);
	Polygon(int indexA, int indexB, int indexC, int indexD);
	int getVertexCount() const;
};

struct Part {
	TextureRgbaU8 diffuseMap, lightMap;
	List<Polygon> polygonBuffer;
	String name;
	explicit Part(const ReadableString &name);
	Part(const TextureRgbaU8 &diffuseMap, const TextureRgbaU8 &lightMap, const List<Polygon> &polygonBuffer, const String &name);
	Part clone() const;
	void render(CommandQueue *commandQueue, ImageRgbaU8* targetImage, ImageF32* depthBuffer, const Transform3D &modelToWorldTransform, const Camera &camera, Filter filter, const ProjectedPoint* projected) const;
	void renderDepth(ImageF32* depthBuffer, const Transform3D &modelToWorldTransform, const Camera &camera, const ProjectedPoint* projected) const;
	int getPolygonCount() const;
	int getPolygonVertexCount(int polygonIndex) const;
};

class ModelImpl {
public:
	Filter filter = Filter::Solid;
	List<FVector3D> positionBuffer; // Also called points
	List<Part> partBuffer;
	FVector3D minBound, maxBound;
private:
	// TODO: A method for recalculating a possibly tighter bounding box
	void expandBound(const FVector3D& point);
public:
	ModelImpl();
	ModelImpl(Filter filter, const List<Part> &partBuffer, const List<FVector3D> &positionBuffer);
	ModelImpl(const ModelImpl &old);
	// Geometry interface
	//   TODO: Make a sweep-and-clean garbage collection of unused points so that indices are remapped once for all changes
	//   TODO: Add empty quads and triangles and fill them later using setters
	//   TODO: Make a texture slot index instead of having getters and setters for each texture slot
	// Part interface
	int addEmptyPart(const String& name);
	int getNumberOfParts() const;
	void setPartName(int partIndex, const String &name);
	String getPartName(int partIndex) const;

	// TODO: Make an array of texture slots using a class enum for index
	TextureRgbaU8 getDiffuseMap(int partIndex) const;
	void setDiffuseMap(const TextureRgbaU8 &diffuseMap, int partIndex);
	void setDiffuseMapByName(ResourcePool &pool, const String &filename, int partIndex);

	TextureRgbaU8 getLightMap(int partIndex) const;
	void setLightMap(const TextureRgbaU8 &lightMap, int partIndex);
	void setLightMapByName(ResourcePool &pool, const String &filename, int partIndex);

	// Polygon interface
	int addPolygon(Polygon polygon, int partIndex);
	int getNumberOfPolygons(int partIndex) const;
	int getPolygonVertexCount(int partIndex, int polygonIndex) const;
	// Point interface
	int getNumberOfPoints() const;
	FVector3D getPoint(int pointIndex) const;
	void setPoint(int pointIndex, const FVector3D& position);
	int findPoint(const FVector3D &position, float threshold) const;
	int addPoint(const FVector3D &position);
	int addPointIfNeeded(const FVector3D &position, float threshold); // Returns the index of a new point or the first existing within threshold in euclidean 3D distance
	// Vertex interface
	int getVertexPointIndex(int partIndex, int polygonIndex, int vertexIndex) const;
	void setVertexPointIndex(int partIndex, int polygonIndex, int vertexIndex, int pointIndex);
	FVector3D getVertexPosition(int partIndex, int polygonIndex, int vertexIndex) const; // Returning getPoint using the point index shared by other polygons
	FVector4D getVertexColor(int partIndex, int polygonIndex, int vertexIndex) const;
	void setVertexColor(int partIndex, int polygonIndex, int vertexIndex, const FVector4D& color);
	FVector4D getTexCoord(int partIndex, int polygonIndex, int vertexIndex) const;
	void setTexCoord(int partIndex, int polygonIndex, int vertexIndex, const FVector4D& texCoord);
	// Rendering
	void render(CommandQueue *commandQueue, ImageRgbaU8* targetImage, ImageF32* depthBuffer, const Transform3D &modelToWorldTransform, const Camera &camera) const;
	void renderDepth(ImageF32* depthBuffer, const Transform3D &modelToWorldTransform, const Camera &camera) const;
};

}

#endif
