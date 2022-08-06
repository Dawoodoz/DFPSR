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
#include "../render/model/format/dmf1.h"

namespace dsr {
	// Normalized texture coordinates:
	//   (0.0f, 0.0f) is the texture coordinate for the upper left corner of the upper left pixel in the 2D texture.
	//   (1.0f, 0.0f) is the texture coordinate for the upper right corner of the upper right pixel in the 2D texture.
	//   (0.0f, 1.0f) is the texture coordinate for the lower left corner of the lower left pixel in the 2D texture.
	//   (1.0f, 1.0f) is the texture coordinate for the lower right corner of the lower right pixel in the 2D texture.
	//   (0.5f, 0.5f) is the texture coordinate for the center of the 2D texture.

	// Texture sampling:
	//   By default, texture sampling is looped around the edges with bilinear with mip-maps for diffuse textures when available.
	//   In bi-linear interpolation, the center of the pixel has the full weight of the color while the sides may show pixels from the other end of the texture.
	//   When getting further away or viewing from the side, a lower resolution version of the texture will be taken from the pyramid if it was generated.
	//   Seams between resolutions will be hard seams, so avoid using hard lines in textures if you want it to look natural.
	//   Loading textures from a ResourcePool will automatically call image_generatePyramid.
	//   Images without the power of two dimensions needed to generate a pyramid can not be used as textures in models.
	//   image_isTexture can be used to know if an image has the supported dimensions for the current version of the renderer.

	// Side-effect: Creates a new empty model.
	// Post-condition: Returns a reference counted handle to the new model.
	// The model will be deleted automatically when all handles are gone.
	Model model_create();
	// Clones the geometry but refers to the same textures to save memory.
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns a reference counted handle to the clone of model.
	Model model_clone(const Model& model);

	// Assign a filter to the whole model.
	//   Assigning filters per model makes it easier to draw solid models before filtered models.
	//   Two separate models can be used if you need both solid and filtered geometry.
	// Filters:
	//   Filter::Alpha uses the alpha channel from the shader as opacity.
	//   Filter::Solid is the default setting for newly created models.
	// Pre-condition: model must refer to an existing model.
	// Side-effect: Sets the given model's filter to the given filter argument.
	void model_setFilter(const Model& model, Filter filter);
	// Get back the filter enumeration, which was assigned to the model using model_setFilter.
	// This is useful for knowing in which pass to render your model.
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the model's filter enumeration
	Filter model_getFilter(const Model& model);
	// Post-condition: Returns true iff the model exists.
	bool model_exists(const Model& model);

	// Each part contains material settings and a list of polygons.
	//   Each polygon contains 3 or 4 vertices. (triangles and quads)
	//     Each vertex has with its own color, texture coordinates and position index.
	//     Position indices refers to the model's list of points,
	//     which is shared across multiple parts to avoid gaps between materials.
	// Pre-condition: model must refer to an existing model.
	// Side-effect: Adds an empty part without any polygons and returns its new local part index.
	// The returned part index is relative to the model and goes from 0 to model_getNumberOfParts(model) - 1.
	int model_addEmptyPart(Model& model, const String &name);
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the number of parts in model.
	int model_getNumberOfParts(const Model& model);
	// Pre-condition: model must refer to an existing model.
	// Side-effect: Sets the part at partIndex in model to the new name.
	void model_setPartName(Model& model, int partIndex, const String &name);
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the name of the part at partIndex in model.
	String model_getPartName(const Model& model, int partIndex);

	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the number of points in model.
	int model_getNumberOfPoints(const Model& model);
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the 3D position of the point at pointIndex in model.
	FVector3D model_getPoint(const Model& model, int pointIndex);
	// Pre-condition: model must refer to an existing model.
	void model_setPoint(Model& model, int pointIndex, const FVector3D& position);
	// Pre-condition: model must refer to an existing model.
	// Post-condition:
	//   Returns an index to the closest point in model relative to position in euclidean distance.
	//   Returns -1 if none was inside of threshold.
	// A point p is inside of threshold iff |p - position| < threshold.
	// If multiple points have the same distance approximated, the point with the lowest index will be preferred.
	int model_findPoint(const Model& model, const FVector3D &position, float threshold);
	// Add a point even if it overlaps an existing point.
	// Can be used for animation where the initial position might not always be the same.
	// Pre-condition: model must refer to an existing model.
	// Side-effect: Adds a new point to model at position.
	// Post-condition: Returns a local index to the new point.
	int model_addPoint(const Model& model, const FVector3D &position);
	// Add a point, only if it does not overlap.
	// Can be used to seal small gaps and reduce the time needed to transform vertex positions.
	// Pre-condition: model must refer to an existing model.
	// Side-effect: Adds a new point to model at position unless another point already exists within threshold so that model_findPoint(model, position, threshold) > -1.
	// Post-condition:
	//   If a new point was created then its new index is returned.
	//   Otherwise, if any existing point was within threshold,
	//   then the index of the closest existing point in euclidean distance is returned.
	//   If multiple existing points are within the same distance,
	//   then the point with the lowest index is preferred, just like in model_findPoint.
	int model_addPointIfNeeded(Model& model, const FVector3D &position, float threshold);
	// Get the bounding box, which expands automatically when adding or moving points in the model.
	// Side-effect: Writes model's bounding box to minimum and maximum by reference.
	void model_getBoundingBox(const Model& model, FVector3D& minimum, FVector3D& maximum);

	// Get the vertex position's index, which refers to a shared point in the model.
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the position index of the vertex. (At vertexIndex in the polygon at polygonIndex in the part at partIndex in model.)
	int model_getVertexPointIndex(const Model& model, int partIndex, int polygonIndex, int vertexIndex);
	// Pre-condition: model must refer to an existing model.
	// Side-effect: Sets the position index of the vertex to pointIndex. (At vertexIndex in the polygon at polygonIndex in the part at partIndex in model.)
	void model_setVertexPointIndex(Model& model, int partIndex, int polygonIndex, int vertexIndex, int pointIndex);
	// Get the vertex position directly, without having to look it up by index using model_getPoint.
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the position of the vertex. (At vertexIndex in the polygon at polygonIndex in the part at partIndex in model.)
	FVector3D model_getVertexPosition(const Model& model, int partIndex, int polygonIndex, int vertexIndex);
	// Get the vertex color, which is not shared with any other polygons. (Red, green, blue, alpha) channels are packed as (x, y, z, w) in FVector4D.
	// Vertex colors use a normalized scale from 0.0f to 1.0f.
	//   Transparent black is FVector4D(0.0f, 0.0f, 0.0f, 0.0f).
	//   Solid red is FVector4D(1.0f, 0.0f, 0.0f, 1.0f).
	//   Solid green is FVector4D(0.0f, 1.0f, 0.0f, 1.0f).
	//   Solid blue is FVector4D(0.0f, 0.0f, 1.0f, 1.0f).
	//   Half opaque orange is FVector4D(1.0f, 0.5f, 0.0f, 0.5f).
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the color of the vertex. (At vertexIndex in the polygon at polygonIndex in the part at partIndex in model.)
	FVector4D model_getVertexColor(const Model& model, int partIndex, int polygonIndex, int vertexIndex);
	// Set the vertex color using the same system as model_getVertexColor.
	// Pre-condition: model must refer to an existing model.
	// Side-effect: Sets the color of the vertex to color. (At vertexIndex in the polygon at polygonIndex in the part at partIndex in model.)
	void model_setVertexColor(Model& model, int partIndex, int polygonIndex, int vertexIndex, const FVector4D& color);
	// Get (U1, V1, U2, V2) texture coordinates packed as (x, y, z, w) in FVector4D.
	// UV1 coordinates (x, y) refers to normalized texture sampling coordinates for the diffuse-map.
	// UV2 coordinates (z, w) refers to normalized texture sampling coordinates for the light-map.
	//   Light-maps do not use mip-map layers, which allow generating light-maps dynamically.
	// Pre-condition: model must refer to an existing model.
	FVector4D model_getTexCoord(const Model& model, int partIndex, int polygonIndex, int vertexIndex);
	// Pre-condition: model must refer to an existing model.
	// Side-effect: Sets the texture coordinates of the vertex to texCoord for both UV1 and UV2.
	//              (At vertexIndex in the polygon at polygonIndex in the part at partIndex in model.)
	void model_setTexCoord(Model& model, int partIndex, int polygonIndex, int vertexIndex, const FVector4D& texCoord);

	// Create a triangle surface at given position indices.
	//   The fourth vertex is used as padding, so quads and triangles take the same amount of memory per polygon.
	//   Using two triangles instead of one quad would use twice as much memory.
	// Pre-condition: model must refer to an existing model.
	// Side-effect:
	//   Adds a new polygon in the model's part at partIndex.
	//   The new polygon contains three vertices.
	//   Each new vertex has texture coordinates set to the upper left corner using (0.0f, 0.0f, 0.0f, 0.0f).
	//   Each new vertex has the color set to solid white using (1.0f, 1.0f, 1.0f, 1.0f).
	// Post-condition:
	//   Returns the new polygon's local index within the part at partIndex in model.
	int model_addTriangle(Model& model, int partIndex, int pointA, int pointB, int pointC);
	// Create a quad surface at given position indices.
	// Pre-condition: model must refer to an existing model.
	// Side-effect:
	//   Adds a new polygon in the model's part at partIndex.
	//   The new polygon contains four vertices.
	//   Each new vertex has texture coordinates set to the upper left corner using (0.0f, 0.0f, 0.0f, 0.0f).
	//   Each new vertex has the color set to solid white using (1.0f, 1.0f, 1.0f, 1.0f).
	// Post-condition:
	//   Returns the new polygon's local index within the part at partIndex in model.
	int model_addQuad(Model& model, int partIndex, int pointA, int pointB, int pointC, int pointD);
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the number of polygons (triangles + quads) in the part at partIndex in model.
	int model_getNumberOfPolygons(const Model& model, int partIndex);
	// Pre-condition: model must refer to an existing model.
	// Post-condition: Returns the number of vertices in the polygon at polygonIndex in the part at partIndex in model.
	int model_getPolygonVertexCount(const Model& model, int partIndex, int polygonIndex);

	// Get the part's diffuse texture.
	// Pre-condition: model must refer to an existing model.
	// Post-condition:
	//   Returns an image handle to the diffuse texture in the part at partIndex in model.
	//   If the part has no diffuse image then an empth handle is returned.
	ImageRgbaU8 model_getDiffuseMap(const Model& model, int partIndex);
	// Set the part's diffuse texture.
	//   A texture is just an image fulfilling the criterias of image_isTexture to allow fast texture sampling and pyramid generation.
	// Pre-condition:
	//   model must refer to an existing model.
	//   diffuseMap must be either empty or have power-of-two dimensions accepted by image_isTexture.
	// Side-effect:
	//   Sets the diffuse texture in the part at partIndex in model to diffuseMap.
	//   If diffuseMap is an empty image handle, then the diffuse texture will be replaced by the default solid white color.
	void model_setDiffuseMap(Model& model, int partIndex, const ImageRgbaU8 &diffuseMap);
	// Automatically find the diffuse texture by name in the resource pool and assign it.
	// Pre-condition:
	//   model must refer to an existing model.
	//   pool must refer to an existing resource pool.
	//   filename must be the image's filename without any extension nor path.
	//     "Car" is accepted.
	//     "Car.png" is rejected for having an extension.
	//     "myFolder/Car" is rejected for having a path.
	//     "myFolder\\Car" is rejected for having a path.
	//     "Car_1.2" is rejected for using a dot in the actual name, just to catch more mistakes with file extensions.
	// Side-effect:
	//   Sets the diffuse texture in the part at partIndex in model to the image looked up by filename in pool.
	void model_setDiffuseMapByName(Model& model, int partIndex, ResourcePool &pool, const String &filename);
	// Get the part's light texture.
	// Pre-condition:
	//   model must refer to an existing model.
	// Post-condition:
	//   Returns an image handle to the light texture in the part at partIndex in model.
	//   If the part has no light image then an empth handle is returned.
	ImageRgbaU8 model_getLightMap(Model& model, int partIndex);
	// Set the part's light texture.
	//   A texture is just an image fulfilling the criterias of image_isTexture to allow fast texture sampling.
	//   Even though no texture-pyramid is used for light-maps, it still has to look up
	//   pixels quickly using bit-shifts with base two logarithms of power of two widths.
	// Pre-condition:
	//   model must refer to an existing model.
	//   lightMap must be either empty or have power-of-two dimensions accepted by image_isTexture.
	// Side-effect:
	//   Sets the diffuse texture in the part at partIndex in model to diffuseMap.
	//   If diffuseMap is an empty image handle, then the diffuse texture will be replaced by the default solid white color.
	void model_setLightMap(Model& model, int partIndex, const ImageRgbaU8 &lightMap);
	// Automatically find the light texture by name in the resource pool and assign it.
	// Pre-condition:
	//   model must refer to an existing model.
	//   pool must refer to an existing resource pool.
	//   filename must be the image's filename without any extension nor path.
	// Side-effect:
	//   Sets the light texture in the part at partIndex in model to the image looked up by filename in pool.
	void model_setLightMapByName(Model& model, int partIndex, ResourcePool &pool, const String &filename);

	// In order to draw two adjacent polygons without any missing pixels along the seam, they must:
	//   * Share two position indices in opposite directions.
	//     (Rounding the same value to integers twice can be rounded differently,
	//     even though it's highly unlikely to actually happen.)
	//   * Have each vertex position inside of the camera's clipping frustum.
	//     (Far outside of the view frustum, triangles must be clipped in
	//     floating-point 3D space to prevent integer overflows when converted
	//     to sub-pixel integer coordinates.)
	//   * Avoid colliding with near or far clip planes.
	//     (This would also cause clipping in floating-point 3D space, because a
	//     location behind the camera cannot be represented as a screen coordinate)
	// If your clipped polygons are fully outside of the view-frustum,
	// then you will not see the seam nor the polygons.
	// To solve this:
	//   * use model_addPointIfNeeded instead of model_addPoint when adding points.
	//   * Split polygons that are way too big and use them to produce more details.
	//     (This will also increase precision for texture coordinates by splitting up seemingly infinite planes.)
	// If this does not hold true then there is either an exception missing
	// or a bug in the renderer, which should be reported as soon as possible.

	// Single-threaded rendering (Slow but easy to use for small tasks)
	//   Can be executed on different threads if targetImage and depthBuffer doesn't have overlapping memory lines between the threads
	// Pre-condition: colorBuffer and depthBuffer must have the same dimensions.
	// Side-effect: Render any model transformed by modelToWorldTransform, seen from camera, to any colorBuffer using any depthBuffer.
	//   An empty model handle will be skipped silently, which can be used instead of an model with zero polygons.
	void model_render(const Model& model, const Transform3D &modelToWorldTransform, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer, const Camera &camera);
	// Simpler rendering without colorBuffer, for shadows and other depth effects
	//   Equivalent to model_render with a non-existing colorBuffer and filter forced to solid.
	//   Skip this call conditionally for filtered models (using model_getFilter) if you want full equivalence with model_render.
	// Side-effect: Render any model transformed by modelToWorldTransform, seen from camera, to any depthBuffer.
	//   An empty model handle will be skipped silently, which can be used instead of an model with zero polygons.
	void model_renderDepth(const Model& model, const Transform3D &modelToWorldTransform, ImageF32& depthBuffer, const Camera &camera);

	// Multi-threaded rendering (Huge performance boost with more CPU cores!)
	// Post-condition: Returns the handle to a new multi-threaded rendering context.
	//   It is basically a list of triangles to be drawn in parallel using a single call.
	//   After creating a renderer, you may execute a number of batches using it.
	//   Each batch may execute a number of tasks in parallel.
	//   Call pattern:
	//     renderer_create (renderer_begin renderer_giveTask* renderer_end)*
	Renderer renderer_create();
	// Post-condition: Returns true iff the renderer exists.
	bool renderer_exists(const Renderer& renderer);
	// Prepares for rendering by giving the target images to draw pixels on.
	// This step makes sure that nobody changes the target dimensions while rendering,
	// which could otherwise happen if someone requests a new canvas too often.
	// Pre-condition:
	//   renderer must refer to an existing renderer.
	//   colorBuffer and depthBuffer must have the same dimensions.
	void renderer_begin(Renderer& renderer, ImageRgbaU8& colorBuffer, ImageF32& depthBuffer);
	// Project an occluding box against the occlusion grid so that triangles hidden behind it will not be drawn.
	// Occluders may only be placed within solid geometry, because otherwise it may affect the visual result.
	// Should ideally be used before giving render tasks, so that optimizations can take advantage of early occlusion checks.
	void renderer_occludeFromBox(Renderer& renderer, const FVector3D& minimum, const FVector3D& maximum, const Transform3D &modelToWorldTransform, const Camera &camera, bool debugSilhouette = false);
	// If you have drawn the ground in a separate pass and know that lower pixels along the current depth buffer are never further away from the camera,
	// you can fill the occlusion grid using the furthest distance in the top row of each cell sampled from the depth buffer and know the maximum distance of each cell for occluding models in the next pass.
	// Make sure to call it after renderer_begin (so that you don't clear your result on start), but before renderer_giveTask (so that whole models can be occluded without filling the buffer with projected triangles).
	// Pre-condition:
	//   The renderer must have started a pass with a depth buffer using renderer_begin.
	void renderer_occludeFromTopRows(Renderer& renderer, const Camera &camera);
	// After having filled the occlusion grid (using renderer_occludeFromBox, renderer_occludeFromTopRows or renderer_occludeFromExistingTriangles), you can check if a bounding box is visible.
	//   For a single model, you can use model_getBoundingBox to get the local bound and then provide its model to world transform that would be used to render the specific instance.
	//   This is already applied automatically in renderer_giveTask, but you might want to know which model may potentially be visible ahead of time
	//   to bake effects into textures, procedurally generate geometry, skip whole groups of models in a broad-phase or use your own custom rasterizer.
	// Opposite to when filling the occlusion grid, the tested bound must include the whole drawn content.
	//   This makes sure that renderer_isBoxVisible will only return false if it cannot be seen, with exception for near clipping and abused occluders.
	//   False positives from having the bounding box seen is to be expected, because the purpose is to save time by doing less work.
	bool renderer_isBoxVisible(Renderer& renderer, const FVector3D &minimum, const FVector3D &maximum, const Transform3D &modelToWorldTransform, const Camera &camera);
	// Once an object passed game-specific occlusion tests, give it to the renderer using renderer_giveTask.
	// The render job will be performed during the next call to renderer_end.
	// Pre-condition: renderer must refer to an existing renderer.
	// An empty model handle will be skipped silently, which can be used instead of an model with zero polygons.
	// Side-effect: The visible triangles are queued up in the renderer.
	void renderer_giveTask(Renderer& renderer, const Model& model, const Transform3D &modelToWorldTransform, const Camera &camera);
	// A move powerful alternative to renderer_giveTask, sending one triangle at a time without occlusion tests.
	//   Call renderer_isBoxVisible for the whole model's bounding box to check if the triangles in your own representation should be drawn.
	// Useful for engine specific model formats allowing vertex animation, vertex shading and texture shading.
	//   Positions can be transformed to implement bone animation, or interpolated from key frames for vertex animation.
	//   Vertex colors can be modified to implement dynamic vertex light, which is useful for animated geometry.
	//   Having one texture per instances using the same geometry, makes it easy to apply shading in texture space for sub-surface scattering and soft shadows.
	//     Simply transform each light source into object space and generate a normal map in object space instead of tangent space, to make fast texture space shading of rigid models.
	// Side-effect:
	//   Adds the triangle to the renderer's list of things to do when multi-threaded rasterization starts.
	//   Vertex data is cloned by value and you may therefore generate vertex data dynamically and reuse buffers for multiple instances.
	//   Textures are however taken as raw pointers.
	//     Reference counting shared resources with multi-threading would be super slow and most textures are loaded from pools anyway.
	//     Cloning whole textures would always be slower and take more memory than just storing one texture for each variation of shading.
	//     Just don't delete the last handle to a texture while it is being rendered using multiple threads, and you get decent performance without crashes.
	// Inputs:
	//   The renderer must exist, because otherwise it does not know where to draw the result.
	//     Safety checks are only performed in debug mode, so that rendering of triangles will not be slowed down too much in the final release.
	//   posA, posB and posC are pre-projected screen coordinates containing camera space coordinates for clipping.
	//     These are supposed to be projected once per position using the camera and then reused for all vertices that share the position.
	//   By rounding projected coordinates to sub-pixel integers in advance, the rasterization algorithm can perform exact comparisons along the line between two triangles.
	//     This guarantees that triangles that are not clipped against the view frustum will not leak pixels between triangles who had two share two projected positions.
	//   colorA, colorB and colorC are the vertex colors.
	//     If assigned to nearly identical values, a faster shader will be used to fill everything in a solid color.
	//   texCoordA, texCoordB and texCoordC are the texture coordinates.
	//     x and y elements contain UV1 for the diffuse map.
	//     z and w elements contain UV2 for the light map.
	//   Both diffuseMap and lightMap must be a valid texture or not exist.
	//   See model_setFilter for an explanation of the available filters.
	//   The camera should be the same that was used for projecting posA, posB and posC, so that new vertices from clipping can be projected again.
	void renderer_giveTask_triangle(Renderer& renderer,
	  const ProjectedPoint &posA, const ProjectedPoint &posB, const ProjectedPoint &posC,
	  const FVector4D &colorA, const FVector4D &colorB, const FVector4D &colorC,
	  const FVector4D &texCoordA, const FVector4D &texCoordB, const FVector4D &texCoordC,
	  const ImageRgbaU8& diffuseMap, const ImageRgbaU8& lightMap,
	  Filter filter, const Camera &camera);
	// Use already given triangles as occluders.
	//   Used after calls to renderer_giveTask have filled the buffer with triangles, but before they are drawn using renderer_end.
	void renderer_occludeFromExistingTriangles(Renderer& renderer);
	// Side-effect: Finishes all the jobs in the rendering context so that triangles are rasterized to the targets given to renderer_begin.
	// Pre-condition: renderer must refer to an existing renderer.
	// If debugWireframe is true, each triangle's edges will be drawn on top of the drawn world to indicate how well the occlusion system is working
	void renderer_end(Renderer& renderer, bool debugWireframe = false);

	// Imports a DMF model from file content.
	//   Use in combination with string_load or your own system for storing files.
	//   Example:
	//     Model crateModel = importFromContent_DMF1(string_load(mediaPath + U"Model_Crate.dmf"), pool);
	// Pre-condition:
	//   fileContent must be the content of a DMF 1.0 model file.
	//   pool must refer to an existing resource pool.
	//   0 <= detailLevel <= 2 (0 = low, 1 = medium, 2 = high)
	// Post-condition:
	//   Returns a handle to a model imported from fileContent, using pool to access resources, with parts not visible in detailLevel excluded.
	// How to import from the DMF1 format:
	//   * Only use M_Diffuse_0Tex, M_Diffuse_1Tex or M_Diffuse_2Tex as shaders.
	//       Place any diffuse texture in texture slot 0 and any lightmap in slot 1.
	//       Remove any textures that are not used by the shaders.
	//       The fixed pipeline only checks which textures are used.
	//   * Make sure that texture names are spelled case sensitive or they might not be found on some operating systems like Linux.
	Model importFromContent_DMF1(const String &fileContent, ResourcePool &pool, int detailLevel = 2);

}

#endif
