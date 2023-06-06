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

#include "Shader.h"
#include <stdio.h>
#include <algorithm>
#include "../../image/internal/imageInternal.h"
#include "../../image/ImageRgbaU8.h"
#include "../../image/ImageF32.h"

using namespace dsr;

inline static const uint32_t roundUpEven(uint32_t x) {
	return (x + 1u) & ~1u;
}

inline static const uint32_t roundDownEven(uint32_t x) {
	return x & ~1u;
}

template<bool CLIP_SIDES>
static inline U32x4 clippedRead(SafePointer<uint32_t> upperLeft, SafePointer<uint32_t> lowerLeft, bool vis0, bool vis1, bool vis2, bool vis3) {
	if (CLIP_SIDES) {
		return U32x4(vis0 ? upperLeft[0] : 0, vis1 ? upperLeft[1] : 0, vis2 ? lowerLeft[0] : 0, vis3 ? lowerLeft[1] : 0);
	} else {
		return U32x4(upperLeft[0], upperLeft[1], lowerLeft[0], lowerLeft[1]);
	}
}

static inline void clippedWrite(SafePointer<uint32_t> upperLeft, SafePointer<uint32_t> lowerLeft, bool vis0, bool vis1, bool vis2, bool vis3, U32x4 vColor) {
	// Read back SIMD vector to scalar type
	UVector4D color = vColor.get();
	// Write colors for visible pixels
	if (vis0) { upperLeft[0] = color.x; }
	if (vis1) { upperLeft[1] = color.y; }
	if (vis2) { lowerLeft[0] = color.z; }
	if (vis3) { lowerLeft[1] = color.w; }
}

static inline void clippedWrite(SafePointer<float> upperLeft, SafePointer<float> lowerLeft, bool vis0, bool vis1, bool vis2, bool vis3, FVector4D depth) {
	// Write colors for visible pixels
	if (vis0) { upperLeft[0] = depth.x; }
	if (vis1) { upperLeft[1] = depth.y; }
	if (vis2) { lowerLeft[0] = depth.z; }
	if (vis3) { lowerLeft[1] = depth.w; }
}

template<bool CLIP_SIDES>
static inline void clipPixels(int x, const RowInterval &upperRow, const RowInterval &lowerRow, bool &clip0, bool &clip1, bool &clip2, bool &clip3) {
	if (CLIP_SIDES) {
		int x2 = x + 1;
		clip0 = x >= upperRow.left && x < upperRow.right;
		clip1 = x2 >= upperRow.left && x2 < upperRow.right;
		clip2 = x >= lowerRow.left && x < lowerRow.right;
		clip3 = x2 >= lowerRow.left && x2 < lowerRow.right;
	} else {
		clip0 = true;
		clip1 = true;
		clip2 = true;
		clip3 = true;
	}
}

template<bool CLIP_SIDES, bool DEPTH_READ, bool AFFINE>
static inline void getVisibility(int x, const RowInterval &upperRow, const RowInterval &lowerRow, const FVector4D &depth, const SafePointer<float> depthDataUpper, const SafePointer<float> depthDataLower, bool &vis0, bool &vis1, bool &vis2, bool &vis3) {
	// Clip pixels
	bool clip0, clip1, clip2, clip3;
	clipPixels<CLIP_SIDES>(x, upperRow, lowerRow, clip0, clip1, clip2, clip3);
	// Compare to depth buffer
	bool front0, front1, front2, front3;
	if (DEPTH_READ) {
		if (AFFINE) {
			if (CLIP_SIDES) {
				front0 = clip0 ? depth.x < depthDataUpper[0] : false;
				front1 = clip1 ? depth.y < depthDataUpper[1] : false;
				front2 = clip2 ? depth.z < depthDataLower[0] : false;
				front3 = clip3 ? depth.w < depthDataLower[1] : false;
			} else {
				front0 = depth.x < depthDataUpper[0];
				front1 = depth.y < depthDataUpper[1];
				front2 = depth.z < depthDataLower[0];
				front3 = depth.w < depthDataLower[1];
			}
		} else {
			if (CLIP_SIDES) {
				front0 = clip0 ? depth.x > depthDataUpper[0] : false;
				front1 = clip1 ? depth.y > depthDataUpper[1] : false;
				front2 = clip2 ? depth.z > depthDataLower[0] : false;
				front3 = clip3 ? depth.w > depthDataLower[1] : false;
			} else {
				front0 = depth.x > depthDataUpper[0];
				front1 = depth.y > depthDataUpper[1];
				front2 = depth.z > depthDataLower[0];
				front3 = depth.w > depthDataLower[1];
			}
		}
	} else {
		front0 = true;
		front1 = true;
		front2 = true;
		front3 = true;
	}
	// Decide visibility
	vis0 = clip0 && front0;
	vis1 = clip1 && front1;
	vis2 = clip2 && front2;
	vis3 = clip3 && front3;
}

template<bool CLIP_SIDES, bool COLOR_WRITE, bool DEPTH_READ, bool DEPTH_WRITE, Filter FILTER, bool AFFINE>
inline static void fillQuadSuper(const Shader& shader, int x, SafePointer<uint32_t> pixelDataUpper, SafePointer<uint32_t> pixelDataLower, SafePointer<float> depthDataUpper, SafePointer<float> depthDataLower, const RowInterval &upperRow, const RowInterval &lowerRow, const PackOrder &targetPackingOrder, const FVector4D &depth, const F32x4x3 &weights) {
	// Get visibility
	bool vis0, vis1, vis2, vis3;
	getVisibility<CLIP_SIDES, DEPTH_READ, AFFINE>(x, upperRow, lowerRow, depth, depthDataUpper, depthDataLower, vis0, vis1, vis2, vis3);
	// Draw if something is visible
	if (vis0 || vis1 || vis2 || vis3) {
		if (COLOR_WRITE) {
			// Get the color
			U32x4 packedColor(0u); // Allow uninitialized memory?
			// Execute the shader
			Rgba_F32 planarSourceColor = shader.getPixels_2x2(weights);
			// Apply alpha filtering
			if (FILTER == Filter::Alpha) {
				// Get opacity from the source color
				F32x4 opacity = planarSourceColor.alpha * (1.0f / 255.0f);
				// Read the packed colors for alpha blending
				U32x4 packedTargetColor = clippedRead<CLIP_SIDES>(pixelDataUpper, pixelDataLower, vis0, vis1, vis2, vis3);
				// Unpack the target color into planar RGBA format so that it can be mixed with the source color
				Rgba_F32 planarTargetColor(packedTargetColor, targetPackingOrder);
				// Blend linearly using floats
				planarSourceColor = (planarSourceColor * opacity) + (planarTargetColor * (1.0f - opacity));
			}
			// Apply channel swapping while packing to bytes
			packedColor = planarSourceColor.toSaturatedByte(targetPackingOrder);
			// Write colors
			clippedWrite(pixelDataUpper, pixelDataLower, vis0, vis1, vis2, vis3, packedColor);
		}
		// Write depth for visible pixels
		if (DEPTH_WRITE) {
			clippedWrite(depthDataUpper, depthDataLower, vis0, vis1, vis2, vis3, depth);
		}
	}
}

// CLIP_SIDES will use upperRow and lowerRow to clip pixels based on the x value. Only x values inside the ranges can be drawn.
//   This is used along the triangle edges.
// COLOR_WRITE can be disabled to skip writing to the color buffer. Usually when none is given.
// DEPTH_READ can be disabled to draw without caring if there is something already closer in the depth buffer.
// DEPTH_WRITE can be disabled to skip writing to the depth buffer so that it does not occlude following draw calls.
// FILTER can be set to Filter::Alpha to use the output alpha as the opacity.
template<bool CLIP_SIDES, bool COLOR_WRITE, bool DEPTH_READ, bool DEPTH_WRITE, Filter FILTER, bool AFFINE>
static inline void fillRowSuper(const Shader& shader, SafePointer<uint32_t> pixelDataUpper, SafePointer<uint32_t> pixelDataLower, SafePointer<float> depthDataUpper, SafePointer<float> depthDataLower, FVector3D pWeightUpper, FVector3D pWeightLower, const FVector3D &pWeightDx, int startX, int endX, const RowInterval &upperRow, const RowInterval &lowerRow, const PackOrder &targetPackingOrder) {
	if (AFFINE) {
		FVector3D dx2 = pWeightDx * 2.0f;
		F32x4 vLinearDepth(pWeightUpper.x, pWeightUpper.x + pWeightDx.x, pWeightLower.x, pWeightLower.x + pWeightDx.x);
		F32x4 weightB(pWeightUpper.y, pWeightUpper.y + pWeightDx.y, pWeightLower.y, pWeightLower.y + pWeightDx.y);
		F32x4 weightC(pWeightUpper.z, pWeightUpper.z + pWeightDx.z, pWeightLower.z, pWeightLower.z + pWeightDx.z);
		for (int x = startX; x < endX; x += 2) {
			// Get the linear depth
			FVector4D depth = vLinearDepth.get();
			// Calculate the weight of the first vertex from the other two
			F32x4 weightA = 1.0f - (weightB + weightC);
			F32x4x3 weights(weightA, weightB, weightC);
			fillQuadSuper<CLIP_SIDES, COLOR_WRITE, DEPTH_READ, DEPTH_WRITE, FILTER, AFFINE>(shader, x, pixelDataUpper, pixelDataLower, depthDataUpper, depthDataLower, upperRow, lowerRow, targetPackingOrder, depth, weights);
			// Iterate projection
			vLinearDepth = vLinearDepth + dx2.x;
			weightB = weightB + dx2.y;
			weightC = weightC + dx2.z;
			// Iterate buffer pointers
			pixelDataUpper += 2; pixelDataLower += 2;
			depthDataUpper += 2; depthDataLower += 2;
		}
	} else {
		FVector3D dx2 = pWeightDx * 2.0f;
		F32x4 vRecDepth(pWeightUpper.x, pWeightUpper.x + pWeightDx.x, pWeightLower.x, pWeightLower.x + pWeightDx.x);
		F32x4 vRecU(pWeightUpper.y, pWeightUpper.y + pWeightDx.y, pWeightLower.y, pWeightLower.y + pWeightDx.y);
		F32x4 vRecV(pWeightUpper.z, pWeightUpper.z + pWeightDx.z, pWeightLower.z, pWeightLower.z + pWeightDx.z);
		for (int x = startX; x < endX; x += 2) {
			// Get the reciprocal depth
			FVector4D depth = vRecDepth.get();
			// After linearly interpolating (1 / W, U / W, V / W) based on the affine weights...
			// Divide 1 by 1 / W to get the linear depth W
			F32x4 vLinearDepth = vRecDepth.reciprocal();
			// Multiply the vertex weights to the second and third edges with the depth to compensate for that we divided them by depth before interpolating.
			F32x4 weightB = vRecU * vLinearDepth;
			F32x4 weightC = vRecV * vLinearDepth;
			// Calculate the weight of the first vertex from the other two
			F32x4 weightA = 1.0f - (weightB + weightC);
			F32x4x3 weights(weightA, weightB, weightC);
			fillQuadSuper<CLIP_SIDES, COLOR_WRITE, DEPTH_READ, DEPTH_WRITE, FILTER, AFFINE>(shader, x, pixelDataUpper, pixelDataLower, depthDataUpper, depthDataLower, upperRow, lowerRow, targetPackingOrder, depth, weights);
			// Iterate projection
			vRecDepth = vRecDepth + dx2.x;
			vRecU = vRecU + dx2.y;
			vRecV = vRecV + dx2.z;
			// Iterate buffer pointers
			pixelDataUpper += 2; pixelDataLower += 2;
			depthDataUpper += 2; depthDataLower += 2;
		}
	}
}

template<bool COLOR_WRITE, bool DEPTH_READ, bool DEPTH_WRITE, Filter FILTER, bool AFFINE>
inline static void fillShapeSuper(const Shader& shader, ImageRgbaU8Impl *colorBuffer, ImageF32Impl *depthBuffer, const ITriangle2D &triangle, const Projection &projection, const RowShape &shape) {
	// Prepare constants
	const int targetStride = imageInternal::getStride(colorBuffer);
	const int depthBufferStride = imageInternal::getStride(depthBuffer);
	const FVector3D doublePWeightDx = projection.pWeightDx * 2.0f;
	const int colorRowSize = imageInternal::getRowSize(colorBuffer);
	const int depthRowSize = imageInternal::getRowSize(depthBuffer);
	const PackOrder& targetPackingOrder = imageInternal::getPackOrder(colorBuffer);
	const int colorHeight = imageInternal::getHeight(colorBuffer);
	const int depthHeight = imageInternal::getHeight(depthBuffer);
	const int maxHeight = colorHeight > depthHeight ? colorHeight : depthHeight;

	// Initialize row pointers for color buffer
	SafePointer<uint32_t> pixelDataUpper, pixelDataLower, pixelDataUpperRow, pixelDataLowerRow;
	if (COLOR_WRITE) {
		SafePointer<uint32_t> targetData = imageInternal::getSafeData<uint32_t>(colorBuffer);
		pixelDataUpperRow = targetData;
		pixelDataUpperRow.increaseBytes(shape.startRow * targetStride);
		pixelDataLowerRow = targetData;
		pixelDataLowerRow.increaseBytes((shape.startRow + 1) * targetStride);
	} else {
		pixelDataUpperRow = SafePointer<uint32_t>();
		pixelDataLowerRow = SafePointer<uint32_t>();
	}

	// Initialize row pointers for depth buffer
	SafePointer<float> depthDataUpper, depthDataLower, depthDataUpperRow, depthDataLowerRow;
	if (DEPTH_READ || DEPTH_WRITE) {
		SafePointer<float> depthBufferData = imageInternal::getSafeData<float>(depthBuffer);
		depthDataUpperRow = depthBufferData;
		depthDataUpperRow.increaseBytes(shape.startRow * depthBufferStride);
		depthDataLowerRow = depthBufferData;
		depthDataLowerRow.increaseBytes((shape.startRow + 1) * depthBufferStride);
	} else {
		depthDataUpperRow = SafePointer<float>();
		depthDataLowerRow = SafePointer<float>();
	}
	for (int32_t y1 = shape.startRow; y1 < shape.startRow + shape.rowCount; y1 += 2) {
		int y2 = y1 + 1;
		RowInterval upperRow = shape.rows[y1 - shape.startRow];
		RowInterval lowerRow = shape.rows[y2 - shape.startRow];
		int outerStart = min(upperRow.left, lowerRow.left);
		int outerEnd = max(upperRow.right, lowerRow.right);
		int innerStart = max(upperRow.left, lowerRow.left);
		int innerEnd = min(upperRow.right, lowerRow.right);
		// Round exclusive intervals to multiples of two pixels
		int outerBlockStart = roundDownEven(outerStart);
		int outerBlockEnd = roundUpEven(outerEnd);
		int innerBlockStart = roundUpEven(innerStart);
		int innerBlockEnd = roundDownEven(innerEnd);
		// Clip last row if outside on odd height
		if (y2 >= maxHeight) {
			lowerRow.right = lowerRow.left;
		}
		// Avoid reading outside of the given bound
		bool hasTop = upperRow.right > upperRow.left;
		bool hasBottom = lowerRow.right > lowerRow.left;
		if (hasTop || hasBottom) {
			// Initialize pointers
			if (COLOR_WRITE) {
				if (hasTop) {
					pixelDataUpper = pixelDataUpperRow.slice("pixelDataUpper", 0, colorRowSize);
				} else {
					// Repeat the lower row to avoid reading outside
					pixelDataUpper = pixelDataLowerRow.slice("pixelDataUpper (from lower)", 0, colorRowSize);
				}
				if (hasBottom) {
					pixelDataLower = pixelDataLowerRow.slice("pixelDataLower", 0, colorRowSize);
				} else {
					// Repeat the upper row to avoid reading outside
					pixelDataLower = pixelDataUpperRow.slice("pixelDataLower (from upper)", 0, colorRowSize);
				}
				int startColorOffset = outerBlockStart * sizeof(uint32_t);
				pixelDataUpper.increaseBytes(startColorOffset);
				pixelDataLower.increaseBytes(startColorOffset);
			}
			if (DEPTH_READ || DEPTH_WRITE) {
				if (hasTop) {
					depthDataUpper = depthDataUpperRow.slice("depthDataUpper", 0, depthRowSize);
				} else {
					// Repeat the upper row to avoid reading outside
					depthDataUpper = depthDataLowerRow.slice("depthDataUpper (from lower)", 0, depthRowSize);
				}
				if (hasBottom) {
					depthDataLower = depthDataLowerRow.slice("depthDataLower", 0, depthRowSize);
				} else {
					// Repeat the upper row to avoid reading outside
					depthDataLower = depthDataUpperRow.slice("depthDataLower (from upper)", 0, depthRowSize);
				}
				depthDataUpper += outerBlockStart;
				depthDataLower += outerBlockStart;
			} else {
				depthDataUpper = SafePointer<float>();
				depthDataLower = SafePointer<float>();
			}
			// Initialize projection
			FVector3D pWeightUpperRow;
			if (AFFINE) {
				pWeightUpperRow = projection.getWeight_affine(IVector2D(outerBlockStart, y1));
			} else {
				pWeightUpperRow = projection.getDepthDividedWeight_perspective(IVector2D(outerBlockStart, y1));
			}
			FVector3D pWeightUpper = pWeightUpperRow;
			FVector3D pWeightLowerRow = pWeightUpperRow + projection.pWeightDy;
			FVector3D pWeightLower = pWeightLowerRow;
			// Render the pixels
			if (innerBlockEnd <= innerBlockStart) {
				// Clipped from left and right
				for (int32_t x = outerBlockStart; x < outerBlockEnd; x += 2) {
					fillRowSuper<true, COLOR_WRITE, DEPTH_READ, DEPTH_WRITE, FILTER, AFFINE>
					  (shader, pixelDataUpper, pixelDataLower, depthDataUpper, depthDataLower, pWeightUpper, pWeightLower, projection.pWeightDx, x, x + 2, upperRow, lowerRow, targetPackingOrder);
					if (COLOR_WRITE) { pixelDataUpper += 2; pixelDataLower += 2; }
					if (DEPTH_READ || DEPTH_WRITE) { depthDataUpper += 2; depthDataLower += 2; }
					pWeightUpper = pWeightUpper + doublePWeightDx; pWeightLower = pWeightLower + doublePWeightDx;
				}
			} else {
				// Left edge
				for (int32_t x = outerBlockStart; x < innerBlockStart; x += 2) {
					fillRowSuper<true, COLOR_WRITE, DEPTH_READ, DEPTH_WRITE, FILTER, AFFINE>
					  (shader, pixelDataUpper, pixelDataLower, depthDataUpper, depthDataLower, pWeightUpper, pWeightLower, projection.pWeightDx, x, x + 2, upperRow, lowerRow, targetPackingOrder);
					if (COLOR_WRITE) { pixelDataUpper += 2; pixelDataLower += 2; }
					if (DEPTH_READ || DEPTH_WRITE) { depthDataUpper += 2; depthDataLower += 2; }
					pWeightUpper = pWeightUpper + doublePWeightDx; pWeightLower = pWeightLower + doublePWeightDx;
				}
				// Full quads
				int width = innerBlockEnd - innerBlockStart;
				int quadCount = width / 2;
				fillRowSuper<false, COLOR_WRITE, DEPTH_READ, DEPTH_WRITE, FILTER, AFFINE>
				  (shader, pixelDataUpper, pixelDataLower, depthDataUpper, depthDataLower, pWeightUpper, pWeightLower, projection.pWeightDx, innerBlockStart, innerBlockEnd, RowInterval(), RowInterval(), targetPackingOrder);
				if (COLOR_WRITE) { pixelDataUpper += 2 * quadCount; pixelDataLower += 2 * quadCount; }
				if (DEPTH_READ || DEPTH_WRITE) { depthDataUpper += 2 * quadCount; depthDataLower += 2 * quadCount; }
				pWeightUpper = pWeightUpper + (doublePWeightDx * quadCount); pWeightLower = pWeightLower + (doublePWeightDx * quadCount);
				// Right edge
				for (int32_t x = innerBlockEnd; x < outerBlockEnd; x += 2) {
					fillRowSuper<true, COLOR_WRITE, DEPTH_READ, DEPTH_WRITE, FILTER, AFFINE>
					  (shader, pixelDataUpper, pixelDataLower, depthDataUpper, depthDataLower, pWeightUpper, pWeightLower, projection.pWeightDx, x, x + 2, upperRow, lowerRow, targetPackingOrder);
					if (COLOR_WRITE) { pixelDataUpper += 2; pixelDataLower += 2; }
					if (DEPTH_READ || DEPTH_WRITE) { depthDataUpper += 2; depthDataLower += 2; }
					pWeightUpper = pWeightUpper + doublePWeightDx; pWeightLower = pWeightLower + doublePWeightDx;
				}
			}
		}
		// Iterate to the next row
		if (COLOR_WRITE) {
			pixelDataUpperRow.increaseBytes(targetStride * 2);
			pixelDataLowerRow.increaseBytes(targetStride * 2);
		}
		if (DEPTH_READ || DEPTH_WRITE) {
			depthDataUpperRow.increaseBytes(depthBufferStride * 2);
			depthDataLowerRow.increaseBytes(depthBufferStride * 2);
		}
	}
}

void Shader::fillShape(ImageRgbaU8Impl *colorBuffer, ImageF32Impl *depthBuffer, const ITriangle2D &triangle, const Projection &projection, const RowShape &shape, Filter filter) {
	bool hasColorBuffer = colorBuffer != nullptr;
	bool hasDepthBuffer = depthBuffer != nullptr;
	if (projection.affine) {
		if (hasDepthBuffer) {
			if (hasColorBuffer) {
				if (filter != Filter::Solid) {
					// Alpha filtering with read only depth buffer
					fillShapeSuper<true, true, false, Filter::Alpha, true>(*this, colorBuffer, depthBuffer, triangle, projection, shape);
				} else {
					// Solid with depth buffer
					fillShapeSuper<true, true, true, Filter::Solid, true>(*this, colorBuffer, depthBuffer, triangle, projection, shape);
				}
			} else {
				// Solid depth
				// TODO: Use for orthogonal depth based shadows
				fillShapeSuper<false, true, true, Filter::Solid, true>(*this, nullptr, depthBuffer, triangle, projection, shape);
			}
		} else {
			if (hasColorBuffer) {
				if (filter != Filter::Solid) {
					// Alpha filtering without depth buffer
					fillShapeSuper<true, false, false, Filter::Alpha, true>(*this, colorBuffer, nullptr, triangle, projection, shape);
				} else {
					// Solid without depth buffer
					fillShapeSuper<true, false, false, Filter::Solid, true>(*this, colorBuffer, nullptr, triangle, projection, shape);
				}
			}
		}
	} else {
		if (hasDepthBuffer) {
			if (hasColorBuffer) {
				if (filter != Filter::Solid) {
					// Alpha filtering with read only depth buffer
					fillShapeSuper<true, true, false, Filter::Alpha, false>(*this, colorBuffer, depthBuffer, triangle, projection, shape);
				} else {
					// Solid with depth buffer
					fillShapeSuper<true, true, true, Filter::Solid, false>(*this, colorBuffer, depthBuffer, triangle, projection, shape);
				}
			} else {
				// Solid depth
				// TODO: Use for depth based shadows with perspective projection
				fillShapeSuper<false, true, true, Filter::Solid, false>(*this, nullptr, depthBuffer, triangle, projection, shape);
			}
		} else {
			if (hasColorBuffer) {
				if (filter != Filter::Solid) {
					// Alpha filtering without depth buffer
					fillShapeSuper<true, false, false, Filter::Alpha, false>(*this, colorBuffer, nullptr, triangle, projection, shape);
				} else {
					// Solid without depth buffer
					fillShapeSuper<true, false, false, Filter::Solid, false>(*this, colorBuffer, nullptr, triangle, projection, shape);
				}
			}
		}
	}
}

