// zlib open source license
//
// Copyright (c) 2018 to 2025 David Forsgren Piuva
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

// TODO: Also provide single-threaded rendering using a similar interface.

// An API for multi-threaded rendering of triangles.
// Optimized for a few large triangles with textures, which means high overhead per triangle but low overhead per pixel.
//   * Performs triangle clipping on triangles.
//     If slightly outside, the rasterizer will clip the triangle after projection without creating any holes between edges.
//     If far outside, the triangle will be subdivided into multiple triangles using floating-point operations to prevent integer overflow.
//     Avoid triangles that become very large after projection if you want to completely avoid floating-point triangle clipping.
//     Smaller triangles also have a higher chance of being occluded by your shapes.
//   * It perspective correction per pixel to make rendering accurate at the cost of performance.

#ifndef DFPSR_API_RENDERER
#define DFPSR_API_RENDERER

#include "../math/FVector.h"
#include "../implementation/image/Texture.h"
#include "../implementation/render/Camera.h"
#include "../implementation/render/ResourcePool.h"

namespace dsr {

	// A handle to a multi-threaded rendering context.
	struct RendererImpl;
	using Renderer = Handle<RendererImpl>;

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
	// Pre-condition: Renderer must exist.
	// Post-condition: Returns the color buffer given to renderer_begin, or an empty image handle if not rendering.
	ImageRgbaU8 renderer_getColorBuffer(const Renderer& renderer);
	// Pre-condition: Renderer must exist.
	// Post-condition: Returns the depth buffer given to renderer_begin, or an empty image handle if not rendering.
	ImageF32 renderer_getDepthBuffer(const Renderer& renderer);
	// Returns true between renderer_begin and renderer_end when triangles can be sent to the renderer.
	bool renderer_takesTriangles(const Renderer& renderer);
	// Project an occluding box against the occlusion grid so that triangles hidden behind it will not be drawn.
	// Occluders may only be placed within solid geometry, because otherwise it may affect the visual result.
	// Should ideally be used before giving render tasks, so that optimizations can take advantage of early occlusion checks.
	void renderer_occludeFromBox(Renderer& renderer, const FVector3D& minimum, const FVector3D& maximum, const Transform3D &modelToWorldTransform, const Camera &camera, bool debugSilhouette = false);
	// If you have drawn the ground in a separate pass and know that lower pixels along the current depth buffer are never further away from the camera,
	// you can fill the occlusion grid using the furthest distance in the top row of each cell sampled from the depth buffer and know the maximum distance of each cell for occluding models in the next pass.
	// Make sure to call it after renderer_begin (so that you don't clear your result on start), but before checking bounding box occlusion and sending triangles to draw.
	// Pre-condition:
	//   The renderer must have started a pass with a depth buffer using renderer_begin.
	void renderer_occludeFromTopRows(Renderer& renderer, const Camera &camera);
	// Returns true if the renderer contains any occluders.
	bool renderer_hasOccluders(const Renderer& renderer);
	// After having filled the occlusion grid (using renderer_occludeFromBox, renderer_occludeFromTopRows or renderer_occludeFromExistingTriangles), you can check if a bounding box is visible.
	//   For a single model, you can use model_getBoundingBox to get the local bound and then provide its model to world transform that would be used to render the specific instance.
	//   This is already applied automatically in renderer_giveTask, but you might want to know which model may potentially be visible ahead of time
	//   to bake effects into textures, procedurally generate geometry, skip whole groups of models in a broad-phase or use your own custom rasterizer.
	// Opposite to when filling the occlusion grid, the tested bound must include the whole drawn content.
	//   This makes sure that renderer_isBoxVisible will only return false if it cannot be seen, with exception for near clipping and abused occluders.
	//   False positives from having the bounding box seen is to be expected, because the purpose is to save time by doing less work.
	bool renderer_isBoxVisible(const Renderer& renderer, const FVector3D &minimum, const FVector3D &maximum, const Transform3D &modelToWorldTransform, const Camera &camera);
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
	  const TextureRgbaU8& diffuseMap, const TextureRgbaU8& lightMap,
	  Filter filter, const Camera &camera);
	// Use already given triangles as occluders.
	//   Used after calls to renderer_giveTask have filled the buffer with triangles, but before they are drawn using renderer_end.
	void renderer_occludeFromExistingTriangles(Renderer& renderer);
	// Side-effect: Finishes all the jobs in the rendering context so that triangles are rasterized to the targets given to renderer_begin.
	// Pre-condition: renderer must refer to an existing renderer.
	// If debugWireframe is true, each triangle's edges will be drawn on top of the drawn world to indicate how well the occlusion system is working
	void renderer_end(Renderer& renderer, bool debugWireframe = false);
}

#endif
