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

#include <cassert>
#include "renderCore.h"
#include "../image/internal/imageInternal.h"
#include "shader/Shader.h"
#include "shader/RgbaMultiply.h"
#include "constants.h"

using namespace dsr;

//#define DISABLE_VERTEX_COLOR
//#define DISABLE_DIFFUSE_MAP
//#define DISABLE_LIGHT_MAP

class SubVertex {
public:
	FVector3D cs; // Camera space position based on the weights
	float subB, subC; // Weights for second and third vertices in the parent triangle
	int state = 0; // Used by algorithms
	float value = 0.0f; // Used by algorithms
	SubVertex() : cs(FVector3D()), subB(0.0f), subC(0.0f) {}
	SubVertex(FVector3D cs, float subB, float subC) : cs(cs), subB(subB), subC(subC) {}
	SubVertex(SubVertex vertexA, SubVertex vertexB, float ratio) {
		float invRatio = 1.0f - ratio;
		this->cs = vertexA.cs * invRatio + vertexB.cs * ratio;
		this->subB = vertexA.subB * invRatio + vertexB.subB * ratio;
		this->subC = vertexA.subC * invRatio + vertexB.subC * ratio;
	}
};

class ClippedTriangle {
private:
	static const int maxPoints = 9; // If a triangle starts with 3 points and each of 6 planes in the view frustum can add one point each then the maximum is 9 points
	int vertexCount = 0;
public:
	int getVertexCount() {
		return this->vertexCount;
	}
	SubVertex vertices[maxPoints] = {};
	ClippedTriangle(const ITriangle2D &triangle) {
		this->vertices[0] = SubVertex(triangle.position[0].cs, 0.0f, 0.0f);
		this->vertices[1] = SubVertex(triangle.position[1].cs, 1.0f, 0.0f);
		this->vertices[2] = SubVertex(triangle.position[2].cs, 0.0f, 1.0f);
		this->vertexCount = 3;
	}
	void deleteVertex(int removeIndex) {
		assert(removeIndex >= 0);
		assert(removeIndex < this->vertexCount);
		if (this->vertexCount > 0 && this->vertexCount <= maxPoints) {
			for (int v = removeIndex; v < this->vertexCount - 1; v++) {
				assert(v >= 0 && v + 1 < maxPoints);
				this->vertices[v] = this->vertices[v + 1];
			}
			this->vertexCount--;
		}
	}
	void insertVertex(int newIndex, const SubVertex &newVertex) {
		// Check against buffer overflow in case of bugs from rounding errors
		assert(newIndex >= 0);
		assert(newIndex <= this->vertexCount);
		if (this->vertexCount < maxPoints) {
			for (int v = this->vertexCount - 1; v >= newIndex; v--) {
				assert(v >= 0 && v + 1 < maxPoints);
				this->vertices[v + 1] = this->vertices[v];
			}
			this->vertices[newIndex] = newVertex;
			this->vertexCount++;
		}
	}
	void deleteAll() {
		this->vertexCount = 0;
	}
	// Returns 0 when value = a
	// Returns 0.5 when value = (a + b) / 2
	// Returns 1 when value = b
	static float inverseLerp(float a, float b, float value) {
		float c = b - a;
		if (c == 0.0f) {
			return 0.5f;
		} else {
			return (value - a) / c;
		}
	}
	// Cut away parts of the triangle that are on the positive side of the plane
	void clip(const FPlane3D &plane) {
		static const int state_use = 0;
		static const int state_delete = 1;
		static const int state_modified = 2;
		if (this->vertexCount >= 3 && this->vertexCount < maxPoints) {
			int outsideCount = 0;
			int lastOutside = 0;
			for (int v = 0; v < this->vertexCount; v++) {
				assert(v >= 0 && v < maxPoints);
				float distance = plane.signedDistance(this->vertices[v].cs);
				this->vertices[v].value = distance;
				if (distance > 0.0f) {
					outsideCount++;
					lastOutside = v;
					this->vertices[v].state = state_delete;
				} else {
					this->vertices[v].state = state_use;
				}
			}
			if (outsideCount > 0) {
				if (outsideCount >= this->vertexCount) {
					this->deleteAll();
				} else if (outsideCount == 1) {
					// Split a single vertex into two corners by interpolating with the previous and next corners						
					int currentVertex = lastOutside;
					int previousVertex = (lastOutside - 1 + this->vertexCount) % this->vertexCount;
					int nextVertex = (lastOutside + 1) % this->vertexCount;
					float previousToCurrentRatio = inverseLerp(this->vertices[previousVertex].value, this->vertices[currentVertex].value, 0.0f);
					float currentToNextRatio = inverseLerp(this->vertices[currentVertex].value, this->vertices[nextVertex].value, 0.0f);
					SubVertex cutStart(this->vertices[previousVertex], this->vertices[currentVertex], previousToCurrentRatio);
					SubVertex cutEnd(this->vertices[currentVertex], this->vertices[nextVertex], currentToNextRatio);
					this->vertices[lastOutside] = cutStart;
					insertVertex(nextVertex, cutEnd);
				} else {
					// Start and end of the cut
					for (int currentVertex = 0; currentVertex < this->vertexCount; currentVertex++) {
						int previousVertex = (currentVertex - 1 + this->vertexCount) % this->vertexCount;
						int nextVertex = (currentVertex + 1) % this->vertexCount;
						if (this->vertices[currentVertex].state == state_delete) {
							if (this->vertices[previousVertex].state == state_use) {
								// Begin the cut
								float previousToCurrentRatio = inverseLerp(this->vertices[previousVertex].value, this->vertices[currentVertex].value, 0.0f);
								this->vertices[currentVertex] = SubVertex(this->vertices[previousVertex], this->vertices[currentVertex], previousToCurrentRatio);
								this->vertices[currentVertex].state = state_modified;
							} else if (this->vertices[nextVertex].state == state_use) {
								// End the cut
								float currentToNextRatio = inverseLerp(this->vertices[currentVertex].value, this->vertices[nextVertex].value, 0.0f);
								this->vertices[currentVertex] = SubVertex(this->vertices[currentVertex], this->vertices[nextVertex], currentToNextRatio);
								this->vertices[currentVertex].state = state_modified;
							}
						}
					}
					// Delete every vertex that is marked for removal
					// Looping backwards will avoid using altered indices while deleting
					if (outsideCount > 2) {
						for (int v = this->vertexCount - 1; v >= 0; v--) {
							assert(v >= 0 && v < maxPoints);
							if (this->vertices[v].state == state_delete) {
								this->deleteVertex(v);
							}
						}
					}
				}
			}
		}
	}
};

Visibility dsr::getTriangleVisibility(const ITriangle2D &triangle, const Camera &camera, bool clipFrustum) {
	static const int cornerCount = 3;
	int planeCount = camera.getFrustumPlaneCount(clipFrustum);
	bool outside[cornerCount * planeCount];
	// Check which corners are outside of the different planes
	int offset = 0;
	for (int c = 0; c < cornerCount; c++) {
		FVector3D triangleCorner = triangle.position[c].cs;
		for (int s = 0; s < planeCount; s++) {
			outside[offset + s] = !(camera.getFrustumPlane(s, clipFrustum).inside(triangleCorner));
		}
		offset += planeCount;
	}
	// Do not render if all corners are outside of the same side
	for (int s = 0; s < planeCount; s++) {
		if (outside[s] && outside[s + planeCount] && outside[s + 2 * planeCount]) {
			return Visibility::Hidden; // All corners outside of the same plane
		}
	}
	// Partial visibility if any corner is outside of a side
	for (int i = 0; i < planeCount * cornerCount; i++) {
		if (outside[i]) {
			return Visibility::Partial; // Any corner outside of a plane
		}
	}
	return Visibility::Full;
}

static bool almostZero(float value) {
	return value > -0.001f && value < 0.001f;
}

static bool almostZero(const FVector3D &channel) {
	return almostZero(channel.x) && almostZero(channel.y) && almostZero(channel.z);
}

static bool almostOne(float value) {
	return value > 0.999f && value < 1.001f;
}

static bool almostOne(const FVector3D &channel) {
	return almostOne(channel.x) && almostOne(channel.y) && almostOne(channel.z);
}

static bool almostSame(const FVector3D &channel) {
	return almostZero(channel.x - channel.y) && almostZero(channel.x - channel.z) && almostZero(channel.y - channel.z);
}

static const int alignX = 2;
static const int alignY = 2;

void dsr::executeTriangleDrawing(const TriangleDrawCommand &command, const IRect &clipBound) {
	IRect finalClipBound = IRect::cut(command.clipBound, clipBound);
	int32_t rowCount = command.triangle.getBufferSize(finalClipBound, alignX, alignY);
	if (rowCount > 0) {
		int startRow;
		RowInterval rows[rowCount];
		command.triangle.getShape(startRow, rows, finalClipBound, alignX, alignY);
		Projection projection = command.triangle.getProjection(command.subB, command.subC, command.perspective);
		command.processTriangle(command.triangleInput, command.targetImage, command.depthBuffer, command.triangle, projection, RowShape(startRow, rowCount, rows), command.filter);
		#ifdef SHOW_POST_CLIPPING_WIREFRAME
			drawWireframe(command.targetImage, command.triangle);
		#endif
	}
}

// Draw a linearly interpolated sub-triangle for clipping
static void drawSubTriangle(CommandQueue *commandQueue, const TriangleDrawData &triangleDrawData, const Camera &camera, const IRect &clipBound, const SubVertex &vertexA, const SubVertex &vertexB, const SubVertex &vertexC) {
	//Get the weight of the first corner from the other weights
	FVector3D subB(vertexA.subB, vertexB.subB, vertexC.subB);
	FVector3D subC(vertexA.subC, vertexB.subC, vertexC.subC);
	//FVector3D subA = FVector3D(1.0f, 1.0f, 1.0f) - (subB + subC);
	ProjectedPoint posA = camera.cameraToScreen(vertexA.cs);
	ProjectedPoint posB = camera.cameraToScreen(vertexB.cs);
	ProjectedPoint posC = camera.cameraToScreen(vertexC.cs);
	// Create the sub-triangle
	ITriangle2D triangle(posA, posB, posC);
	// Rounding sub-triangles to integer locations may reverse the direction of zero area triangles
	if (triangle.isFrontfacing()) {
		TriangleDrawCommand command(triangleDrawData, triangle, subB, subC, clipBound);
		if (commandQueue) {
			commandQueue->add(command);
		} else {
			executeTriangleDrawing(command, clipBound);
		}
		
	}
}

// Clip triangles against the clip bounds outside of the image
// Precondition: The triangle needs to be clipped
// TODO: Take drawSubTriangle as a lambda to drawClippedTriangle using vertex weights as arguments and vertex data as captured variables
static void drawClippedTriangle(CommandQueue *commandQueue, const TriangleDrawData &triangleDrawData, const Camera &camera, const ITriangle2D &triangle, const IRect &clipBound) {
	ClippedTriangle clipped(triangle);
	int planeCount = camera.getFrustumPlaneCount(true);
	for (int s = 0; s < planeCount; s++) {
		clipped.clip(camera.getFrustumPlane(s, true));
	}
	// Draw a convex triangle fan from the clipped triangle
	for (int triangleIndex = 0; triangleIndex < clipped.getVertexCount() - 2; triangleIndex++) {
		int indexA = 0;
		int indexB = 1 + triangleIndex;
		int indexC = 2 + triangleIndex;
		drawSubTriangle(commandQueue, triangleDrawData, camera, clipBound, clipped.vertices[indexA], clipped.vertices[indexB], clipped.vertices[indexC]);
	}
}

// Clipping is applied automatically if needed
void dsr::renderTriangleWithShader(CommandQueue *commandQueue, const TriangleDrawData &triangleDrawData, const Camera &camera, const ITriangle2D &triangle, const IRect &clipBound) {
	// Allow small triangles to be a bit outside of the view frustum without being clipped by increasing the width and height slopes in a second test
	// This reduces redundant clipping to improve both speed and quality
	Visibility paddedVisibility = getTriangleVisibility(triangle, camera, true);
	// Draw the triangle
	if (paddedVisibility == Visibility::Full) {
		// Only check if the triangle is front facing once we know that the projection is in positive depth
		if (triangle.isFrontfacing()) {
			// Draw the full triangle
			TriangleDrawCommand command(triangleDrawData, triangle, FVector3D(0.0f, 1.0f, 0.0f), FVector3D(0.0f, 0.0f, 1.0f), clipBound);
			if (commandQueue) {
				commandQueue->add(command);
			} else {
				executeTriangleDrawing(command, clipBound);
			}
		}
	} else {
		// Draw a clipped triangle
		drawClippedTriangle(commandQueue, triangleDrawData, camera, triangle, clipBound);
	}
}

// TODO: Move shader selection to Shader_RgbaMultiply and let models default to its shader factory function pointer as shader selection
void dsr::renderTriangleFromData(
  CommandQueue *commandQueue, ImageRgbaU8Impl *targetImage, ImageF32Impl *depthBuffer,
  const Camera &camera, const ProjectedPoint &posA, const ProjectedPoint &posB, const ProjectedPoint &posC,
  Filter filter, const ImageRgbaU8Impl *diffuse, const ImageRgbaU8Impl *light,
  TriangleTexCoords texCoords, TriangleColors colors) {
	// Get dimensions from both buffers
	int colorWidth = imageInternal::getWidth(targetImage);
	int colorHeight = imageInternal::getHeight(targetImage);
	int depthWidth = imageInternal::getWidth(depthBuffer);
	int depthHeight = imageInternal::getHeight(depthBuffer);
	// Combine dimensions
	int targetWidth, targetHeight;
	if (targetImage != nullptr) {
		targetWidth = colorWidth;
		targetHeight = colorHeight;
		if (depthBuffer != nullptr) {
			assert(targetWidth == depthWidth);
			assert(targetHeight == depthHeight);
		}
	} else {
		if (depthBuffer != nullptr) {
			targetWidth = depthWidth;
			targetHeight = depthHeight;
		} else {
			return; // No target buffer to draw on
		}
	}
	// Select a bound
	IRect clipBound = IRect::FromSize(targetWidth, targetHeight);
	// Create a triangle
	ITriangle2D triangle(posA, posB, posC);
	// Only draw visible triangles
	Visibility visibility = getTriangleVisibility(triangle, camera, false);
	if (visibility != Visibility::Hidden) {
		// Disable features when debugging
		#ifdef DISABLE_VERTEX_COLOR
			colors = TriangleColors(1.0f);
		#endif
		#ifdef DISABLE_DIFFUSE_MAP
			diffuse = nullptr;
		#endif
		#ifdef DISABLE_LIGHT_MAP
			light = nullptr;
		#endif
		// Select an instance of the default shader
		if (!(filter == Filter::Alpha && almostZero(colors.alpha))) {
			bool hasVertexFade = !(almostSame(colors.red) && almostSame(colors.green) && almostSame(colors.blue) && almostSame(colors.alpha));
			bool colorless = almostOne(colors.red) && almostOne(colors.green) && almostOne(colors.blue) && almostOne(colors.alpha);
			// Get the function pointer to the correct shader
			DRAW_CALLBACK_TYPE drawTask = &drawCallbackTemplate;
			if (diffuse) {
				bool hasDiffusePyramid = diffuse->texture.hasMipBuffer();
				if (light) {
					if (hasVertexFade) { // DiffuseLightVertex
						if (hasDiffusePyramid) { // With mipmap
							drawTask = &(Shader_RgbaMultiply<true, true, true, false, false>::processTriangle);
						} else { // Without mipmap
							drawTask = &(Shader_RgbaMultiply<true, true, true, false, true>::processTriangle);
						}
					} else { // DiffuseLight
						if (hasDiffusePyramid) { // With mipmap
							drawTask = &(Shader_RgbaMultiply<true, true, false, false, false>::processTriangle);
						} else { // Without mipmap
							drawTask = &(Shader_RgbaMultiply<true, true, false, false, true>::processTriangle);
						}
					}
				} else {
					if (hasVertexFade) { // DiffuseVertex
						if (hasDiffusePyramid) { // With mipmap
							drawTask = &(Shader_RgbaMultiply<true, false, true, false, false>::processTriangle);
						} else { // Without mipmap
							drawTask = &(Shader_RgbaMultiply<true, false, true, false, true>::processTriangle);
						}
					} else {
						if (colorless) { // Diffuse without normalization
							if (hasDiffusePyramid) { // With mipmap
								drawTask = &(Shader_RgbaMultiply<true, false, false, true, false>::processTriangle);
							} else { // Without mipmap
								drawTask = &(Shader_RgbaMultiply<true, false, false, true, true>::processTriangle);
							}
						} else { // Diffuse
							if (hasDiffusePyramid) { // With mipmap
								drawTask = &(Shader_RgbaMultiply<true, false, false, false, false>::processTriangle);
							} else { // Without mipmap
								drawTask = &(Shader_RgbaMultiply<true, false, false, false, true>::processTriangle);
							}
						}
					}
				}
			} else {
				if (light) {
					if (hasVertexFade) { // LightVertex
						drawTask = &(Shader_RgbaMultiply<false, true, true, false, false>::processTriangle);
					} else {
						if (colorless) { // Light without normalization
							drawTask = &(Shader_RgbaMultiply<false, true, false, true, false>::processTriangle);
						} else { // Light
							drawTask = &(Shader_RgbaMultiply<false, true, false, false, false>::processTriangle);
						}
					}
				} else {
					if (hasVertexFade) { // Vertex
						drawTask = &(Shader_RgbaMultiply<false, false, true, false, false>::processTriangle);
					} else { // Single color
						drawTask = &(Shader_RgbaMultiply<false, false, false, false, false>::processTriangle);
					}
				}
			}
			renderTriangleWithShader(commandQueue, TriangleDrawData(targetImage, depthBuffer, camera.perspective, filter, TriangleInput(diffuse, light, texCoords, colors), drawTask), camera, triangle, clipBound);
		}
	}
}

template<bool AFFINE>
static void executeTriangleDrawingDepth(ImageF32Impl *depthBuffer, const ITriangle2D& triangle, const IRect &clipBound) {
	int32_t rowCount = triangle.getBufferSize(clipBound, 1, 1);
	if (rowCount > 0) {
		int startRow;
		RowInterval rows[rowCount];
		triangle.getShape(startRow, rows, clipBound, 1, 1);
		Projection projection = triangle.getProjection(FVector3D(), FVector3D(), !AFFINE); // TODO: Create a weight using only depth to save time
		RowShape shape = RowShape(startRow, rowCount, rows);
		// Draw the triangle
		const int depthBufferStride = imageInternal::getStride(depthBuffer);
		SafePointer<float> depthDataRow = imageInternal::getSafeData<float>(depthBuffer, shape.startRow);
		for (int32_t y = shape.startRow; y < shape.startRow + shape.rowCount; y++) {
			RowInterval row = shape.rows[y - shape.startRow];
			SafePointer<float> depthData = depthDataRow + row.left;
			// Initialize depth iteration
			float depthValue;
			if (AFFINE) {
				depthValue = projection.getWeight_affine(IVector2D(row.left, y)).x;
			} else {
				depthValue = projection.getDepthDividedWeight_perspective(IVector2D(row.left, y)).x;
			}
			float depthDx = projection.pWeightDx.x;
			// Loop over a row of depth pixels
			for (int32_t x = row.left; x < row.right; x++) {
				float oldValue = *depthData;
				if (AFFINE) {
					// Write lower depthValue for orthogonal cameras
					if (depthValue < oldValue) {
						*depthData = depthValue;
					}
				} else {
					// Write higher depthValue for perspective cameras
					if (depthValue > oldValue) {
						*depthData = depthValue;
					}
				}
				depthValue += depthDx;
				depthData += 1;
			}
			// Iterate to the next row
			depthDataRow.increaseBytes(depthBufferStride);
		}
	}
}

static void drawTriangleDepth(ImageF32Impl *depthBuffer, const Camera &camera, const IRect &clipBound, const ITriangle2D& triangle) {
	// Rounding sub-triangles to integer locations may reverse the direction of zero area triangles
	if (triangle.isFrontfacing()) {
		if (camera.perspective) {
			executeTriangleDrawingDepth<false>(depthBuffer, triangle, clipBound);
		} else {
			executeTriangleDrawingDepth<true>(depthBuffer, triangle, clipBound);
		}
	}
}

static void drawSubTriangleDepth(ImageF32Impl *depthBuffer, const Camera &camera, const IRect &clipBound, const SubVertex &vertexA, const SubVertex &vertexB, const SubVertex &vertexC) {
	ProjectedPoint posA = camera.cameraToScreen(vertexA.cs);
	ProjectedPoint posB = camera.cameraToScreen(vertexB.cs);
	ProjectedPoint posC = camera.cameraToScreen(vertexC.cs);
	drawTriangleDepth(depthBuffer, camera, clipBound, ITriangle2D(posA, posB, posC));
}

void dsr::renderTriangleFromDataDepth(ImageF32Impl *depthBuffer, const Camera &camera, const ProjectedPoint &posA, const ProjectedPoint &posB, const ProjectedPoint &posC) {
	// Skip rendering if there's no target buffer
	if (depthBuffer == nullptr) { return; }
	// Select a bound
	IRect clipBound = IRect::FromSize(imageInternal::getWidth(depthBuffer), imageInternal::getHeight(depthBuffer));
	// Create a triangle
	ITriangle2D triangle(posA, posB, posC);
	// Only draw visible triangles
	Visibility visibility = getTriangleVisibility(triangle, camera, false);
	if (visibility != Visibility::Hidden) {
		// Allow small triangles to be a bit outside of the view frustum without being clipped by increasing the width and height slopes in a second test
		// This reduces redundant clipping to improve both speed and quality
		Visibility paddedVisibility = getTriangleVisibility(triangle, camera, true);
		// Draw the triangle
		if (paddedVisibility == Visibility::Full) {
			// Only check if the triangle is front facing once we know that the projection is in positive depth
			if (triangle.isFrontfacing()) {
				// Draw the full triangle
				drawTriangleDepth(depthBuffer, camera, clipBound, triangle);
			}
		} else {
			// Draw a clipped triangle
			ClippedTriangle clipped(triangle); // TODO: Simpler vertex clipping using only positions
			int planeCount = camera.getFrustumPlaneCount(true);
			for (int s = 0; s < planeCount; s++) {
				clipped.clip(camera.getFrustumPlane(s, true));
			}
			// Draw a convex triangle fan from the clipped triangle
			for (int triangleIndex = 0; triangleIndex < clipped.getVertexCount() - 2; triangleIndex++) {
				int indexA = 0;
				int indexB = 1 + triangleIndex;
				int indexC = 2 + triangleIndex;
				drawSubTriangleDepth(depthBuffer, camera, clipBound, clipped.vertices[indexA], clipped.vertices[indexB], clipped.vertices[indexC]);
			}
		}
	}
}

void CommandQueue::add(const TriangleDrawCommand &command) {
	this->buffer.push(command);
}

void CommandQueue::execute(const IRect &clipBound, int jobCount) const {
	if (jobCount <= 1) {
		// TODO: Make a setting for sorting triangles using indices within each job
		for (int i = 0; i < this->buffer.length(); i++) {
			if (!this->buffer[i].occluded) {
				executeTriangleDrawing(this->buffer[i], clipBound);
			}
		}
	} else {
		std::function<void()> jobs[jobCount];
		int y1 = clipBound.top();
		for (int j = 0; j < jobCount; j++) {
			int y2 = clipBound.top() + ((clipBound.bottom() * (j + 1)) / jobCount);
			// Align to multiples of two lines if it's not at the bottom
			if (j < jobCount - 1) {
				y2 = (y2 / 2) * 2;
			}
			int height = y2 - y1;
			IRect subBound = IRect(clipBound.left(), y1, clipBound.width(), height);
			jobs[j] = [this, subBound]() {
				//this->execute(subBound, 1);
				for (int i = 0; i < this->buffer.length(); i++) {
					if (!this->buffer[i].occluded) {
						executeTriangleDrawing(this->buffer[i], subBound);
					}
				}
			};
			y1 = y2;
		}
		threadedWorkFromArray(jobs, jobCount);
	}
}

void CommandQueue::clear() {
	this->buffer.clear();
}

