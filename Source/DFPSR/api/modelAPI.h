// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_API_MODEL
#define DFPSR_API_MODEL

#include "types.h"
#include "../math/FVector.h"

// TODO: How should these be exposed to the caller?
#include "../render/Camera.h"
#include "../render/ResourcePool.h"

namespace dsr {

	// TODO: Document the API

	// Construction
	Model model_create();
	Model model_clone(const Model& model);

	// Whole model
	void model_setFilter(const Model& model, Filter filter);
	Filter model_getFilter(const Model& model);
	bool model_exists(const Model& model);

	// Part
	int model_addEmptyPart(Model& model, const String &name);
	int model_getNumberOfParts(const Model& model);
	void model_setPartName(Model& model, int partIndex, const String &name);
	String model_getPartName(const Model& model, int partIndex);

	// Point
	int model_getNumberOfPoints(const Model& model);
	FVector3D model_getPoint(const Model& model, int pointIndex);
	void model_setPoint(Model& model, int pointIndex, const FVector3D& position);
	int model_findPoint(const Model& model, const FVector3D &position, float threshold);
	int model_addPoint(const Model& model, const FVector3D &position);
	int model_addPointIfNeeded(Model& model, const FVector3D &position, float threshold);

	// Vertex
	int model_getVertexPointIndex(const Model& model, int partIndex, int polygonIndex, int vertexIndex);
	void model_setVertexPointIndex(Model& model, int partIndex, int polygonIndex, int vertexIndex, int pointIndex);
	FVector3D model_getVertexPosition(const Model& model, int partIndex, int polygonIndex, int vertexIndex);
	FVector4D model_getVertexColor(const Model& model, int partIndex, int polygonIndex, int vertexIndex);
	void model_setVertexColor(Model& model, int partIndex, int polygonIndex, int vertexIndex, const FVector4D& color);
	FVector4D model_getTexCoord(const Model& model, int partIndex, int polygonIndex, int vertexIndex);
	void model_setTexCoord(Model& model, int partIndex, int polygonIndex, int vertexIndex, const FVector4D& texCoord);

	// Polygon
	int model_addTriangle(Model& model, int partIndex, int pointA, int pointB, int pointC);
	int model_addQuad(Model& model, int partIndex, int pointA, int pointB, int pointC, int pointD);
	int model_getNumberOfPolygons(const Model& model, int partIndex);
	int model_getPolygonVertexCount(const Model& model, int partIndex, int polygonIndex);

	// Texture
	ImageRgbaU8 model_getDiffuseMap(const Model& model, int partIndex);
	void model_setDiffuseMap(Model& model, int partIndex, const ImageRgbaU8 &diffuseMap);
	void model_setDiffuseMapByName(Model& model, int partIndex, ResourcePool &pool, const String &filename);
	ImageRgbaU8 model_getLightMap(Model& model, int partIndex);
	void model_setLightMap(Model& model, int partIndex, const ImageRgbaU8 &lightMap);
	void model_setLightMapByName(Model& model, int partIndex, ResourcePool &pool, const String &filename);

	// Single-threaded rendering
	//   Can be executed on different threads if targetImage and depthBuffer doesn't have overlapping memory lines between the threads
	void model_render(const Model& model, const Transform3D &modelToWorldTransform, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer, const Camera &camera);
	// Simpler rendering without colorBuffer, for shadows and other depth effects
	//   Equivalent to model_render with a non-existing colorBuffer and filter forced to solid.
	//   Skip this call conditionally for filtered models (using model_getFilter) if you want full equivalence with model_render.
	void model_renderDepth(const Model& model, const Transform3D &modelToWorldTransform, ImageF32& depthBuffer, const Camera &camera);

	// Returns a new rendering context
	//   After creating a renderer, you may execute a number of batches using it
	//   Each batch may execute a number of tasks in parallel
	//   Call pattern:
	//     create (begin giveTask* end)*
	Renderer renderer_create();
	// Begin rendering to target color and depth buffers of the same dimensions
	bool renderer_exists(const Renderer& renderer);
	void renderer_begin(Renderer& renderer, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer);
	// Once an object passed game-specific occlusion tests, give it to the renderer using renderer_giveTask
	// The render job will be performed during the next call to renderer_execute
	void renderer_giveTask(Renderer& renderer, const Model& model, const Transform3D &modelToWorldTransform, const Camera &camera);
	// Finish all the jobs in the rendering context
	void renderer_end(Renderer& renderer);

	// How to import from the DMF1 format:
	//   * Only use M_Diffuse_0Tex, M_Diffuse_1Tex or M_Diffuse_2Tex as shaders.
	//       Place any diffuse texture in texture slot 0 and any lightmap in slot 1.
	//       Remove any textures that are not used by the shaders.
	//       The fixed pipeline only checks which textures are used.
	//   * Make sure that texture names are spelled case sensitive or they might not be found on some operating systems like Linux.
	//   See dmf1.cpp for the implementation
	Model importFromContent_DMF1(const String &fileContent, ResourcePool &pool, int detailLevel = 2);

}

#endif

