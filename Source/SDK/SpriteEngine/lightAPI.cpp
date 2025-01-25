
#include "lightAPI.h"
#include "../../DFPSR/base/simd3D.h"
#include "../../DFPSR/base/threading.h" // TODO: Make an official "dangerous" API for multi-threading

namespace dsr {

// Precondition: The packed color must be in the standard RGBA order, meaning no native packing
inline F32xXx3 unpackRgb_U32xX_to_F32xXx3(const U32xX& color) {
	return F32xXx3(floatFromU32(packOrder_getRed(color)), floatFromU32(packOrder_getGreen(color)), floatFromU32(packOrder_getBlue(color)));
}

static inline void setLight(SafePointer<uint8_t> lightPixel, U8xX newlight) {
	newlight.writeAligned(lightPixel, "setLight: writing light");
}

static inline void addLight(SafePointer<uint8_t> lightPixel, U8xX addedlight) {
	U8xX oldLight = U8xX::readAligned(lightPixel, "addLight: reading light");
	U8xX newlight = saturatedAddition(oldLight, addedlight);
	newlight.writeAligned(lightPixel, "addLight: writing light");
}

template <bool ADD_LIGHT>
void directedLight(const FMatrix3x3& normalToWorldSpace, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const FVector3D& lightDirection, float lightIntensity, const ColorRgbI32& lightColor) {
	// Normals in range 0..255 - 128 have lengths of 127 and 128, so if we double the reverse light direction we'll end up near 0..255 again for colors
	F32xXx3 reverseLightDirection = F32xXx3(-normalize(normalToWorldSpace.transformTransposed(lightDirection)) * lightIntensity * 2.0f);
	IRect rectangleBound = image_getBound(lightBuffer);
	float colorR = std::max(0.0f, (float)lightColor.red / 255.0f);
	float colorG = std::max(0.0f, (float)lightColor.green / 255.0f);
	float colorB = std::max(0.0f, (float)lightColor.blue / 255.0f);
	threadedSplit(rectangleBound, [
	  lightBuffer, normalBuffer, reverseLightDirection, colorR, colorG, colorB](const IRect& bound) mutable {
		SafePointer<uint8_t> lightRow = image_getSafePointer_channels(lightBuffer, bound.top());
		SafePointer<uint32_t> normalRow = image_getSafePointer(normalBuffer, bound.top());
		int lightStride = image_getStride(lightBuffer);
		int normalStride = image_getStride(normalBuffer);
		for (int y = bound.top(); y < bound.bottom(); y++) {
			SafePointer<uint8_t> lightPixel = lightRow;
			SafePointer<uint32_t> normalPixel = normalRow;
			for (int x = bound.left(); x < bound.right(); x += laneCountX_32Bit) {
				// Read surface normals
				U32xX normalColor = U32xX::readAligned(normalPixel, "directedLight: reading normal");
				// TODO: Port SIMD3D to handle arbitrary vector lengths.
				F32xXx3 negativeSurfaceNormal = unpackRgb_U32xX_to_F32xXx3(normalColor) - 128.0f;
				// Calculate light intensity
				//   Normalization and negation is already pre-multiplied into reverseLightDirection
				F32xX intensity = dotProduct(negativeSurfaceNormal, reverseLightDirection).clampLower(0.0f);
				F32xX red = intensity * colorR;
				F32xX green = intensity * colorG;
				F32xX blue = intensity * colorB;
				red = red.clampUpper(255.1f);
				green = green.clampUpper(255.1f);
				blue = blue.clampUpper(255.1f);
				// TODO: Let color packing handle arbitrary vector lengths.
				U8xX light = reinterpret_U8FromU32(packOrder_packBytes(truncateToU32(red), truncateToU32(green), truncateToU32(blue)));
				if (ADD_LIGHT) {
					addLight(lightPixel, light);
				} else {
					setLight(lightPixel, light);
				}
				lightPixel += laneCountX_8Bit;
				normalPixel += laneCountX_32Bit;
			}
			lightRow.increaseBytes(lightStride);
			normalRow.increaseBytes(normalStride);
		}
	});
}
void setDirectedLight(const OrthoView& camera, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const FVector3D& lightDirection, float lightIntensity, const ColorRgbI32& lightColor) {
	directedLight<false>(camera.normalToWorldSpace, lightBuffer, normalBuffer, lightDirection, lightIntensity, lightColor);
}
void addDirectedLight(const OrthoView& camera, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const FVector3D& lightDirection, float lightIntensity, const ColorRgbI32& lightColor) {
	directedLight<true>(camera.normalToWorldSpace, lightBuffer, normalBuffer, lightDirection, lightIntensity, lightColor);
}

static IRect calculateBound(const OrthoView& camera, const IVector2D& worldCenter, OrderedImageRgbaU8& lightBuffer, const FVector3D& lightSpacePosition, float lightRadius, int alignmentPixels) {
	// Get the light's 2D position in pixels
	FVector3D rotatedPosition = camera.lightSpaceToScreenDepth.transform(lightSpacePosition);
	IVector2D pixelCenter = IVector2D(rotatedPosition.x, rotatedPosition.y) + worldCenter;
	// Use the light-space X axis to convert the sphere's radius into pixels
	int pixelRadius = lightRadius * camera.lightSpaceToScreenDepth.xAxis.x;
	// Check if the location can be seen
	IRect imageBound = image_getBound(lightBuffer);
	if (pixelCenter.x < -pixelRadius
	 || pixelCenter.x > imageBound.right() + pixelRadius
	 || pixelCenter.y < -pixelRadius
	 || pixelCenter.y > imageBound.bottom() + pixelRadius) {
		// The light source cannot be seen at all
		return IRect();
	}
	// Calculate the bound
	IRect result = IRect::cut(imageBound, IRect(pixelCenter.x - pixelRadius, pixelCenter.y - pixelRadius, pixelRadius * 2.0f, pixelRadius * 2.0f));
	// Round out to multiples of SIMD vectors
	if (result.hasArea() && alignmentPixels > 1) {
		int left = roundDown(result.left(), alignmentPixels);
		int right = roundUp(result.right(), alignmentPixels);
		result = IRect(left, result.top(), right - left, result.height());
	}
	return result;
}

// Returns:
//   0.0 for blocked
//   1.0 for passing
//   Values between 0.0 and 1.0 for fuzzy thresholding
// Precondition: pixelData Does not contain any padding by using widths in multiples of 4 pixels
static float getShadowTransparency(SafePointer<float> pixelData, int32_t width, float halfWidth, const FVector3D& lightOffset) {
	// Get lengths
	float absX = lightOffset.x; if (absX < 0.0f) { absX = -absX; }
	float absY = lightOffset.y; if (absY < 0.0f) { absY = -absY; }
	float absZ = lightOffset.z; if (absZ < 0.0f) { absZ = -absZ; }
	// Compare dimensions
	bool xIsLongest = absX > absY && absX > absZ;
	bool yIsLongerThanZ = absY > absZ;
	// Transform
	float depth = xIsLongest ? lightOffset.x : (yIsLongerThanZ ? lightOffset.y : lightOffset.z);
	float slopeUp = (yIsLongerThanZ && !xIsLongest) ? lightOffset.z : lightOffset.y;
	float slopeSide = xIsLongest ? -lightOffset.z : (yIsLongerThanZ ? -lightOffset.x : lightOffset.x);
	int32_t viewOffset = width * (xIsLongest ? 0 : (yIsLongerThanZ ? 2 : 4));
	bool negativeSide = depth < 0.0f;
	if (negativeSide) { depth = -depth; }
	if (negativeSide) { slopeSide = -slopeSide; }
	if (negativeSide) { viewOffset = viewOffset + width; }
	// Project and round to pixels
	float reciDepth = 1.0f / depth;
	float scale = halfWidth * reciDepth;
	int32_t sampleX = (int)(halfWidth + (slopeSide * scale));
	int32_t sampleY = (int)(halfWidth - (slopeUp * scale));
	// Clamp to local view coordinates
	int32_t maxPixel = width - 1;
	if (sampleX < 0) { sampleX = 0; }
	if (sampleX > maxPixel) { sampleX = maxPixel; }
	if (sampleY < 0) { sampleY = 0; }
	if (sampleY > maxPixel) { sampleY = maxPixel; }
	// Read the depth pixel
	float shadowReciDepth = pixelData[((sampleY + viewOffset) * width) + sampleX];
	// Apply biased thresholding
	return reciDepth * 1.02f > shadowReciDepth ? 1.0f : 0.0f;
}

static inline F32xX getShadowTransparency(SafePointer<float> pixelData, int32_t width, float halfWidth, const F32xXx3& lightOffset) {
	// TODO: Create a way to quickly iterate over elements in a SIMD vector for interfacing with scalar operations.
	ALIGN_BYTES(DSR_DEFAULT_ALIGNMENT) float offsetX[DSR_DEFAULT_VECTOR_SIZE];
	ALIGN_BYTES(DSR_DEFAULT_ALIGNMENT) float offsetY[DSR_DEFAULT_VECTOR_SIZE];
	ALIGN_BYTES(DSR_DEFAULT_ALIGNMENT) float offsetZ[DSR_DEFAULT_VECTOR_SIZE];
	lightOffset.v1.writeAlignedUnsafe(offsetX);
	lightOffset.v2.writeAlignedUnsafe(offsetY);
	lightOffset.v3.writeAlignedUnsafe(offsetZ);
	#if DSR_DEFAULT_VECTOR_SIZE == 16
		return F32x4(
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[0], offsetY[0], offsetZ[0])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[1], offsetY[1], offsetZ[1])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[2], offsetY[2], offsetZ[2])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[3], offsetY[3], offsetZ[3]))
		);
	#elif DSR_DEFAULT_VECTOR_SIZE == 32
		return F32x8(
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[0], offsetY[0], offsetZ[0])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[1], offsetY[1], offsetZ[1])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[2], offsetY[2], offsetZ[2])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[3], offsetY[3], offsetZ[3])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[4], offsetY[4], offsetZ[4])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[5], offsetY[5], offsetZ[5])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[6], offsetY[6], offsetZ[6])),
			getShadowTransparency(pixelData, width, halfWidth, FVector3D(offsetX[7], offsetY[7], offsetZ[7]))
		);
	#endif
}

template <bool SHADOW_CASTING>
static void addPointLightSuper(const OrthoView& camera, const IVector2D& worldCenter, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const AlignedImageF32& heightBuffer, const FVector3D& lightPosition, float lightRadius, float lightIntensity, const ColorRgbI32& lightColor, const AlignedImageF32& shadowCubeMap) {
	// Rotate the light position from relative space to light space
	//   Normal-space defines the rotation for light-space
	FVector3D lightSpaceSourcePosition = camera.normalToWorldSpace.transformTransposed(lightPosition);
	// Align the rectangle with 8 pixels, because that's the widest read to align in the 16-bit height buffer
	IRect rectangleBound = calculateBound(camera, worldCenter, lightBuffer, lightSpaceSourcePosition, lightRadius, laneCountX_32Bit);
	if (rectangleBound.hasArea()) {
		// Uniform values
		// How much closer to your face in light-space does the pixel go per depth unit
		F32xXx3 inYourFaceAxis = F32xXx3(camera.screenDepthToLightSpace.zAxis);
		// Light color
		float colorR = std::max(0.0f, (float)lightColor.red * lightIntensity);
		float colorG = std::max(0.0f, (float)lightColor.green * lightIntensity);
		float colorB = std::max(0.0f, (float)lightColor.blue * lightIntensity);
		float reciprocalRadius = 1.0f / lightRadius;
		threadedSplit(rectangleBound, [
		  lightBuffer, normalBuffer, heightBuffer, camera, worldCenter, inYourFaceAxis, lightSpaceSourcePosition,
		  reciprocalRadius, colorR, colorG, colorB, shadowCubeMap](const IRect& bound) mutable {
			// Initiate the local light-space sweep along base height
			//   The local light space is rotated like normal-space but has the origin at the light source
			FVector3D lightBaseRow = camera.screenDepthToLightSpace.transform(FVector3D(0.5f - (float)worldCenter.x + bound.left(), 0.5f - (float)worldCenter.y + bound.top(), 0.0f)) - lightSpaceSourcePosition;
			FVector3D dx = camera.screenDepthToLightSpace.xAxis;
			FVector3D dy = camera.screenDepthToLightSpace.yAxis;
			// Pack the offset for each of the 4 first pixels into a transposing constructor
			F32xXx3 lightBaseRowX = F32xXx3::createGradient(lightBaseRow, dx);
			// Derivatives for moving four pixels to the right in parallel
			//    (n+0, y0), (n+1, y0), (n+2, y0), (n+3, y0) -> (n+4, y0), (n+5, y0), (n+6, y0), (n+7, y0)
			F32xXx3 dxX = F32xXx3(dx * (float)laneCountX_32Bit);
			// Derivatives for moving one pixel down in parallel
			//    (x0, n+0), (x1, n+0), (x2, n+0), (x3, n+0)
			// -> (x0, n+1), (x1, n+1), (x2, n+1), (x3, n+1)
			F32xXx3 dy1 = F32xXx3(dy);
			// Get strides
			int lightStride = image_getStride(lightBuffer);
			int normalStride = image_getStride(normalBuffer);
			int heightStride = image_getStride(heightBuffer);
			// Get pointers
			SafePointer<uint8_t> lightRow = image_getSafePointer_channels(lightBuffer, bound.top()) + bound.left() * 4;
			SafePointer<uint32_t> normalRow = image_getSafePointer(normalBuffer, bound.top()) + bound.left();
			SafePointer<float> heightRow = image_getSafePointer(heightBuffer, bound.top()) + bound.left();
			// Get cube map for casting shadows
			int32_t shadowCubeWidth;
			SafePointer<float> shadowCubeData;
			float shadowCubeCenter;
			if (SHADOW_CASTING) {
				shadowCubeWidth = image_getWidth(shadowCubeMap); assert(shadowCubeWidth % laneCountX_32Bit == 0);
				shadowCubeData = image_getSafePointer(shadowCubeMap);
				shadowCubeCenter = (float)shadowCubeWidth * 0.5f;
			}
			// Loop over the pixels to add light
			for (int y = bound.top(); y < bound.bottom(); y++) {
				// Initiate the leftmost pixels before iterating to the right
				F32xXx3 lightBasePixelxX = lightBaseRowX;
				SafePointer<uint8_t> lightPixel = lightRow;
				SafePointer<uint32_t> normalPixel = normalRow;
				SafePointer<float> heightPixel = heightRow;
				// Iterate over 16-bit pixels 8 at a time
				for (int x = bound.left(); x < bound.right(); x += laneCountX_32Bit) {
					// Read pixel height
					F32xX depthOffset = F32xX::readAligned(heightPixel, "addPointLight: reading height");
					// Extrude the pixel using positive values towards the camera to represent another height
					//   This will solve X and Z positions based on the height Y
					F32xXx3 lightOffset = lightBasePixelxX + (inYourFaceAxis * depthOffset);
					// Get the linear distance, divide by sphere radius and limit to length 1 at intensity 0
					F32xX lightRatio = min(F32xX(1.0f), length(lightOffset) * reciprocalRadius);
					// Read surface normal
					U32xX normalColor = U32xX::readAligned(normalPixel, "addPointLight: reading normal");
					// normalScale is used to negate the normals in advance so that opposing directions get positive values
					F32xXx3 negativeSurfaceNormal = (unpackRgb_U32xX_to_F32xXx3(normalColor) - 128.0f) * (-1.0f / 128.0f);
					// Fade from 0 to 1 using 1 - 2x + x²
					F32xX distanceIntensity = 1.0f - 2.0f * lightRatio + lightRatio * lightRatio;
					F32xX angleIntensity = max(F32xX(0.0f), dotProduct(normalize(lightOffset), negativeSurfaceNormal));
					F32xX intensity = angleIntensity * distanceIntensity;
					if (SHADOW_CASTING) {
						intensity = intensity * getShadowTransparency(shadowCubeData, shadowCubeWidth, shadowCubeCenter, lightOffset);
					}
					// TODO: Make an optimized version for white light replacing red, green and blue with a single LUMA
					F32xX red = intensity * colorR;
					F32xX green = intensity * colorG;
					F32xX blue = intensity * colorB;
					red = red.clampUpper(255.1f);
					green = green.clampUpper(255.1f);
					blue = blue.clampUpper(255.1f);
					// Add light to the image
					U8xX morelight = reinterpret_U8FromU32(packOrder_packBytes(truncateToU32(red), truncateToU32(green), truncateToU32(blue)));
					addLight(lightPixel, morelight);
					// Go to the next four pixels in light-space
					lightBasePixelxX += dxX;
					// Go to the next 4 pixels of image data
					lightPixel += laneCountX_8Bit;
					normalPixel += laneCountX_32Bit;
					heightPixel += laneCountX_32Bit;
				}
				// Go to the next row in light-space
				lightBaseRowX += dy1;
				// Go to the next row of image data
				lightRow.increaseBytes(lightStride);
				normalRow.increaseBytes(normalStride);
				heightRow.increaseBytes(heightStride);
			}
		});
	}
}

void addPointLight(const OrthoView& camera, const IVector2D& worldCenter, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const AlignedImageF32& heightBuffer, const FVector3D& lightPosition, float lightRadius, float lightIntensity, const ColorRgbI32& lightColor, const AlignedImageF32& shadowCubeMap) {
	if (image_exists(shadowCubeMap)) {
		addPointLightSuper<true>(camera, worldCenter, lightBuffer, normalBuffer, heightBuffer, lightPosition, lightRadius, lightIntensity, lightColor, shadowCubeMap);
	} else {
		addPointLightSuper<false>(camera, worldCenter, lightBuffer, normalBuffer, heightBuffer, lightPosition, lightRadius, lightIntensity, lightColor, AlignedImageF32());
	}
}

void addPointLight(const OrthoView& camera, const IVector2D& worldCenter, OrderedImageRgbaU8& lightBuffer, const OrderedImageRgbaU8& normalBuffer, const AlignedImageF32& heightBuffer, const FVector3D& lightPosition, float lightRadius, float lightIntensity, const ColorRgbI32& lightColor) {
	addPointLightSuper<false>(camera, worldCenter, lightBuffer, normalBuffer, heightBuffer, lightPosition, lightRadius, lightIntensity, lightColor, AlignedImageF32());
}

void blendLight(AlignedImageRgbaU8& colorBuffer, const OrderedImageRgbaU8& diffuseBuffer, const OrderedImageRgbaU8& lightBuffer) {
	PackOrder targetOrder = PackOrder::getPackOrder(image_getPackOrderIndex(colorBuffer));
	int width = image_getWidth(colorBuffer);
	int height = image_getHeight(colorBuffer);
	threadedSplit(0, height, [colorBuffer, diffuseBuffer, lightBuffer, targetOrder, width](int startIndex, int stopIndex) mutable {
		SafePointer<uint32_t> targetRow = image_getSafePointer(colorBuffer, startIndex);
		SafePointer<uint32_t> diffuseRow = image_getSafePointer(diffuseBuffer, startIndex);
		SafePointer<uint32_t> lightRow = image_getSafePointer(lightBuffer, startIndex);
		int targetStride = image_getStride(colorBuffer);
		int diffuseStride = image_getStride(diffuseBuffer);
		int lightStride = image_getStride(lightBuffer);
		F32xX scale = F32xX(1.0 / 128.0f);
		for (int y = startIndex; y < stopIndex; y++) {
			SafePointer<uint32_t> targetPixel = targetRow;
			SafePointer<uint32_t> diffusePixel = diffuseRow;
			SafePointer<uint32_t> lightPixel = lightRow;
			for (int x = 0; x < width; x += laneCountX_32Bit) {
				U32xX diffuse = U32xX::readAligned(diffusePixel, "blendLight: reading diffuse");
				U32xX light = U32xX::readAligned(lightPixel, "blendLight: reading light");
				F32xX red = (floatFromU32(packOrder_getRed(diffuse)) * floatFromU32(packOrder_getRed(light))) * scale;
				F32xX green = (floatFromU32(packOrder_getGreen(diffuse)) * floatFromU32(packOrder_getGreen(light))) * scale;
				F32xX blue = (floatFromU32(packOrder_getBlue(diffuse)) * floatFromU32(packOrder_getBlue(light))) * scale;
				red = red.clampUpper(255.1f);
				green = green.clampUpper(255.1f);
				blue = blue.clampUpper(255.1f);
				U32xX color = packOrder_packBytes(truncateToU32(red), truncateToU32(green), truncateToU32(blue), targetOrder);
				color.writeAligned(targetPixel, "blendLight: writing color");
				targetPixel += laneCountX_32Bit;
				diffusePixel += laneCountX_32Bit;
				lightPixel += laneCountX_32Bit;
			}
			targetRow.increaseBytes(targetStride);
			diffuseRow.increaseBytes(diffuseStride);
			lightRow.increaseBytes(lightStride);
		}
	});
}

}

