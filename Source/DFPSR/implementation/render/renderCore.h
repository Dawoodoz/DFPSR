// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_RENDER_MODEL_RENDERCORE
#define DFPSR_RENDER_MODEL_RENDERCORE

#include <cstdint>
#include "Camera.h"
#include "shader/Shader.h"
#include "../../base/threading.h"
#include "../../collection/List.h"

namespace dsr {

struct TriangleDrawData {
	// Color target
	ImageRgbaU8 targetImage;
	// Depth target
	ImageF32 depthBuffer;
	// When perspective is used, the depth buffer stores 1 / depth instead of linear depth.
	bool perspective;
	// The target blending method
	Filter filter;
	// Unprocessed triangle data in the standard layout
	TriangleInput triangleInput;
	// Function pointer to the method that will process the command
	DRAW_CALLBACK_TYPE processTriangle;
	TriangleDrawData(const ImageRgbaU8 &targetImage, const ImageF32 &depthBuffer, bool perspective, Filter filter, const TriangleInput &triangleInput, DRAW_CALLBACK_TYPE processTriangle)
	: targetImage(targetImage), depthBuffer(depthBuffer), perspective(perspective), filter(filter), triangleInput(triangleInput), processTriangle(processTriangle) {}
};

struct TriangleDrawCommand : public TriangleDrawData {
	// Triangle corners and projection
	//   Not a part of TriangleDrawData, because the draw command is made after clipping into multiple smaller triangles
	ITriangle2D triangle;
	// The vertex interpolation weights for each corner to allow clipping triangles without
	// looping the same vertex colors and texture coordinates on every sub-triangle
	//   Corner A's weight = (subB.x, subC.x)
	//   Corner B's weight = (subB.y, subC.y)
	//   Corner C's weight = (subB.z, subC.z)
	//   The final vertex weight of a corner becomes a linear interpolation of the three original vertex weights
	//     (A * (1 - subB - subC)) + (B * subB) + (C * subC)
	FVector3D subB, subC;
	// Extra clipping in case that the receiver of the command goes out of bound
	IRect clipBound;
	// Late removal of triangles without having to shuffle around any data
	bool occluded;
	TriangleDrawCommand(const TriangleDrawData &triangleDrawData, const ITriangle2D &triangle, const FVector3D &subB, const FVector3D &subC, const IRect &clipBound)
	: TriangleDrawData(triangleDrawData), triangle(triangle), subB(subB), subC(subC), clipBound(clipBound), occluded(false) {}
};

// Get the visibility state for the triangle as seen by the camera.
// If clipFrustum is false, the culling test will be done with the actual bounds of the target image.
//   This is used to know when a triangle needs to be drawn.
// If clipFrustum is true, the culling test will be done with extended clip bounds outside of the target image.
//   This is used to know when a triangle needs lossy clipping in floating-point coordinates
//   before it can be converted to integer coordinates without causing an overflow in rasterization.
Visibility getTriangleVisibility(const ITriangle2D &triangle, const Camera &camera, bool clipFrustum);

// Draws according to a draw command.
void executeTriangleDrawing(const TriangleDrawCommand &command, const IRect &clipBound);

// A queue of draw commands
class CommandQueue {
public:
	List<TriangleDrawCommand> buffer;
	void add(const TriangleDrawCommand &command);
	// Multi-threading will be disabled if jobCount equals 1.
	void execute(const IRect &clipBound, int32_t jobCount = 12) const;
	void clear();
};

// Given a triangle and a shader that holds the additional vertex data, this method can be called to draw it.
// Preconditions:
//   * triangle should have passed the triangle visibility test for the actual image bound.
//     Only construct the shader and make this call if "getTriangleVisibility(triangle, camera, false) != Visibility::Hidden" passed.
//     Otherwise, it will waste a lot of time on rasterizing triangles that are not even visible.

// Given a set of triangle data, this method can automatically draw it using the fastest default shader.
// Triangle culling is handled automatically but you might want to apply culling per model or something before drawing many triangles.
// commandQueue can be null to render directly using a single thread.
// targetImage can be null to avoid using the pixel shader.
// depthBuffer can be null to render without depth buffering.
void renderTriangleFromData(
  CommandQueue *commandQueue, const ImageRgbaU8 &targetImage, const ImageF32 &depthBuffer,
  const Camera &camera, const ProjectedPoint &posA, const ProjectedPoint &posB, const ProjectedPoint &posC,
  Filter filter, const TextureRgbaU8 &diffuse, const TextureRgbaU8 &light,
  const TriangleTexCoords &texCoords, const TriangleColors &colors
);
void renderTriangleFromDataDepth(const ImageF32 &depthBuffer, const Camera &camera, const ProjectedPoint &posA, const ProjectedPoint &posB, const ProjectedPoint &posC);

}

#endif

