
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

#include "imageAPI.h"
#include "../implementation/math/scalar.h"
#include "../implementation/image/PackOrder.h"
#include <limits>

namespace dsr {

// Preconditions:
//   0 <= a <= 255
//   0 <= b <= 255
// Postconditions:
//   Returns the normalized multiplication of a and b, where the 0..255 range represents decimal values from 0.0 to 1.0.
//   The result may not be less than zero or larger than any of the inputs.
// Examples:
//   normalizedByteMultiplication(0, 0) = 0
//   normalizedByteMultiplication(x, 0) = 0
//   normalizedByteMultiplication(0, x) = 0
//   normalizedByteMultiplication(x, 255) = x
//   normalizedByteMultiplication(255, x) = x
//   normalizedByteMultiplication(255, 255) = 255
inline uint32_t normalizedByteMultiplication(uint32_t a, uint32_t b) {
	// Approximate the reciprocal of an unsigned byte's maximum value 255 for normalization
	//   256³ / 255 ≈ 65793
	// Truncation goes down, so add half a unit before rounding to get the closest value
	//   2^24 / 2 = 8388608
	// No overflow for unsigned 32-bit integers
	//   255² * 65793 + 8388608 = 4286578433 < 2^32
	return (a * b * 65793 + 8388608) >> 24;
}

inline bool isUniformByte(uint16_t value) {
	return (value & 0xFF) == ((value & 0xFF00) >> 8);
}

inline bool isUniformByte(uint32_t value) {
	uint32_t least =    value & 0x000000FF;
	return   least == ((value & 0x0000FF00) >> 8)
	      && least == ((value & 0x00FF0000) >> 16)
		  && least == ((value & 0xFF000000) >> 24);
}

// -------------------------------- Drawing shapes --------------------------------

// TODO: Use the longest available SIMD vector to assign a color and overwrite padding when the image is not a sub-image.
//       Create a safe and reusable 32-bit memset function in SafePointer.h.

template <typename IMAGE_TYPE, typename COLOR_TYPE>
void drawSolidRectangleAssign(const IMAGE_TYPE &target, int32_t left, int32_t top, int32_t right, int32_t bottom, COLOR_TYPE color) {
	int32_t leftBound = max(0, left);
	int32_t topBound = max(0, top);
	int32_t rightBound = min(right, image_getWidth(target));
	int32_t bottomBound = min(bottom, image_getHeight(target));
	int32_t stride = image_getStride(target);
	SafePointer<COLOR_TYPE> rowData = image_getSafePointer<COLOR_TYPE>(target, topBound);
	rowData += leftBound;
	for (int32_t y = topBound; y < bottomBound; y++) {
		SafePointer<COLOR_TYPE> pixelData = rowData;
		for (int32_t x = leftBound; x < rightBound; x++) {
			pixelData.get() = color;
			pixelData += 1;
		}
		rowData.increaseBytes(stride);
	}
}

template <typename IMAGE_TYPE, typename COLOR_TYPE>
void drawSolidRectangleMemset(const IMAGE_TYPE &target, int32_t left, int32_t top, int32_t right, int32_t bottom, uint8_t uniformByte) {
	int32_t leftBound = max(0, left);
	int32_t topBound = max(0, top);
	int32_t rightBound = min(right, image_getWidth(target));
	int32_t bottomBound = min(bottom, image_getHeight(target));
	if (rightBound > leftBound && bottomBound > topBound) {
		int32_t stride = image_getStride(target);
		SafePointer<COLOR_TYPE> rowData = image_getSafePointer<COLOR_TYPE>(target, topBound);
		rowData += leftBound;
		int32_t filledWidth = rightBound - leftBound;
		int32_t rowSize = filledWidth * sizeof(COLOR_TYPE);
		int32_t rowCount = bottomBound - topBound;
		if ((!target.impl_dimensions.isSubImage()) && filledWidth == image_getWidth(target)) {
			// Write over any padding for parent images owning the whole buffer.
			// Including parent images with sub-images using the same data
			//   because no child image may display the parent-image's padding bytes.
			safeMemorySet(rowData, uniformByte, (stride * (rowCount - 1)) + rowSize);
		} else if (rowSize == stride) {
			// When the filled row stretches all the way from left to right in the main allocation
			//   there's no unseen pixels being overwritten in other images sharing the buffer.
			// This case handles sub-images that uses the full width of
			//   the parent image which doesn't have any padding.
			safeMemorySet(rowData, uniformByte, rowSize * rowCount);
		} else {
			// Fall back on using one memset operation per row.
			// This case is for sub-images that must preserve interleaved pixel rows belonging
			//   to other images that aren't visible and therefore not owned by this image.
			for (int32_t y = topBound; y < bottomBound; y++) {
				safeMemorySet(rowData, uniformByte, rowSize);
				rowData.increaseBytes(stride);
			}
		}
	}
}

void draw_rectangle(const ImageU8& image, const IRect& bound, int32_t color) {
	if (image_exists(image)) {
		if (color < 0) { color = 0; }
		if (color > 255) { color = 255; }
		drawSolidRectangleMemset<ImageU8, uint8_t>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), color);
	}
}
void draw_rectangle(const ImageU16& image, const IRect& bound, int32_t color) {
	if (image_exists(image)) {
		if (color < 0) { color = 0; }
		if (color > 65535) { color = 65535; }
		uint16_t uColor = color;
		if (isUniformByte(uColor)) {
			drawSolidRectangleMemset<ImageU16, uint16_t>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), 0);
		} else {
			drawSolidRectangleAssign<ImageU16, uint16_t>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), uColor);
		}
	}
}
void draw_rectangle(const ImageF32& image, const IRect& bound, float color) {
	if (image_exists(image)) {
		// Floating-point zero is a special value where all bits are assigned zeroes to allow fast initialization.
		if (color == 0.0f) {
			drawSolidRectangleMemset<ImageF32, float>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), 0);
		} else {
			drawSolidRectangleAssign<ImageF32, float>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), color);
		}
	}
}
void draw_rectangle(const ImageRgbaU8& image, const IRect& bound, uint32_t packedColor) {
	if (image_exists(image)) {
		if (isUniformByte(packedColor)) {
			drawSolidRectangleMemset<ImageRgbaU8, uint32_t>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), packedColor & 0xFF);
		} else {
			drawSolidRectangleAssign<ImageRgbaU8, uint32_t>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), packedColor);
		}
	}
}
void draw_rectangle(const ImageRgbaU8& image, const IRect& bound, const ColorRgbaI32& color) {
	if (image_exists(image)) {
		uint32_t packedColor = image_saturateAndPack(image, color);
		if (isUniformByte(packedColor)) {
			drawSolidRectangleMemset<ImageRgbaU8, uint32_t>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), packedColor & 0xFF);
		} else {
			drawSolidRectangleAssign<ImageRgbaU8, uint32_t>(image, bound.left(), bound.top(), bound.right(), bound.bottom(), packedColor);
		}
	}
}

template <typename IMAGE_TYPE, typename COLOR_TYPE>
inline void drawLineSuper(const IMAGE_TYPE &target, int32_t x1, int32_t y1, int32_t x2, int32_t y2, COLOR_TYPE color) {
	// Culling test to reduce wasted pixels outside of the image.
	int32_t width = image_getWidth(target);
	int32_t height = image_getHeight(target);	
	if ((x1 < 0 && x1 < 0) || (y1 < 0 && y1 < 0) || (x1 >= width && x1 >= width) || (y1 >= height && y1 >= height)) {
		// Skip drawing because both points are outside of the same edge.
		return;
	}
	if (y1 == y2) {
		// Sideways
		int32_t left = min(x1, x2);
		int32_t right = max(x1, x2);
		for (int32_t x = left; x <= right; x++) {
			image_writePixel(target, x, y1, color);
		}
	} else if (x1 == x2) {
		// Down
		int32_t top = min(y1, y2);
		int32_t bottom = max(y1, y2);
		for (int32_t y = top; y <= bottom; y++) {
			image_writePixel(target, x1, y, color);
		}
	} else {
		if (std::abs(y2 - y1) >= std::abs(x2 - x1)) {
			if (y2 < y1) {
				swap(x1, x2);
				swap(y1, y2);
			}
			assert(y2 > y1);
			if (x2 > x1) {
				// Down right
				int32_t x = x1;
				int32_t y = y1;
				int32_t tilt = (x2 - x1) * 2;
				int32_t maxError = y2 - y1;
				int32_t error = 0;
				while (y <= y2) {
					image_writePixel(target, x, y, color);
					error += tilt;
					if (error >= maxError) {
						x++;
						error -= maxError * 2;
					}
					y++;
				}
			} else {
				// Down left
				int32_t x = x1;
				int32_t y = y1;
				int32_t tilt = (x1 - x2) * 2;
				int32_t maxError = y2 - y1;
				int32_t error = 0;
				while (y <= y2) {
					image_writePixel(target, x, y, color);
					error += tilt;
					if (error >= maxError) {
						x--;
						error -= maxError * 2;
					}
					y++;
				}
			}
		} else {
			if (x2 < x1) {
				swap(x1, x2);
				swap(y1, y2);
			}
			assert(x2 > x1);
			if (y2 > y1) {
				// Down right
				int32_t x = x1;
				int32_t y = y1;
				int32_t tilt = (y2 - y1) * 2;
				int32_t maxError = x2 - x1;
				int32_t error = 0;
				while (x <= x2) {
					image_writePixel(target, x, y, color);
					error += tilt;
					if (error >= maxError) {
						y++;
						error -= maxError * 2;
					}
					x++;
				}
			} else {
				// Up right
				int32_t x = x1;
				int32_t y = y1;
				int32_t tilt = (y1 - y2) * 2;
				int32_t maxError = x2 - x1;
				int32_t error = 0;
				while (x <= x2) {
					image_writePixel(target, x, y, color);
					error += tilt;
					if (error >= maxError) {
						y--;
						error -= maxError * 2;
					}
					x++;
				}
			}
		}
	}
}

void draw_line(const ImageU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color) {
	if (image_exists(image)) {
		if (color < 0) { color = 0; }
		if (color > 255) { color = 255; }
		drawLineSuper<ImageU8, uint8_t>(image, x1, y1, x2, y2, color);
	}
}
void draw_line(const ImageU16& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color) {
	if (image_exists(image)) {
		if (color < 0) { color = 0; }
		if (color > 65535) { color = 65535; }
		drawLineSuper<ImageU16, uint16_t>(image, x1, y1, x2, y2, color);
	}
}
void draw_line(const ImageF32& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, float color) {
	if (image_exists(image)) {
		drawLineSuper<ImageF32, float>(image, x1, y1, x2, y2, color);
	}
}
void draw_line(const ImageRgbaU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t packedColor) {
	if (image_exists(image)) {
		drawLineSuper<ImageRgbaU8, uint32_t>(image, x1, y1, x2, y2, packedColor);
	}
}
void draw_line(const ImageRgbaU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, const ColorRgbaI32& color) {
	uint32_t packedColor = image_saturateAndPack(image, color);
	draw_line(image, x1, y1, x2, y2, packedColor);
}


// -------------------------------- Drawing images --------------------------------


// Unpacked image dimensions.
struct UnpackedDimensions {
	// width is the number of used pixels on each row.
	// height is the number of rows.
	// stride is the byte offset from one row to another including any padding.
	// pixelSize is the byte offset from one pixel to another from left to right.
	int32_t width, height, stride, pixelSize;
	UnpackedDimensions() : width(0), height(0), stride(0), pixelSize(0) {}
	UnpackedDimensions(const Image& image) :
	  width(image_getWidth(image)), height(image_getHeight(image)), stride(image_getStride(image)), pixelSize(image_getPixelSize(image)) {}
};

struct ImageWriter : public UnpackedDimensions {
	uint8_t *data;
	ImageWriter(const UnpackedDimensions &dimensions, uint8_t *data) :
	  UnpackedDimensions(dimensions), data(data) {}
};

struct ImageReader : public UnpackedDimensions {
	const uint8_t *data;
	ImageReader(const UnpackedDimensions &dimensions, const uint8_t *data) :
	  UnpackedDimensions(dimensions), data(data) {}
};

static ImageWriter getWriter(const Image &image) {
	return ImageWriter(UnpackedDimensions(image), buffer_dangerous_getUnsafeData(image.impl_buffer) + image.impl_dimensions.getByteStartOffset());
}

static ImageReader getReader(const Image &image) {
	return ImageReader(UnpackedDimensions(image), buffer_dangerous_getUnsafeData(image.impl_buffer) + image.impl_dimensions.getByteStartOffset());
}

static Image getGenericSubImage(const Image &image, int32_t left, int32_t top, int32_t width, int32_t height) {
	return Image(image, IRect(left, top, width, height));
}

struct ImageIntersection {
	ImageWriter subTarget;
	ImageReader subSource;
	ImageIntersection(const ImageWriter &subTarget, const ImageReader &subSource) :
	  subTarget(subTarget), subSource(subSource) {}
	static bool canCreate(const Image &target, const Image &source, int32_t left, int32_t top) {
		int32_t targetRegionRight = left + image_getWidth(source);
		int32_t targetRegionBottom = top + image_getHeight(source);
		return left < image_getWidth(target) && top < image_getHeight(target) && targetRegionRight > 0 && targetRegionBottom > 0;
	}
	// Only call if canCreate passed with the same arguments
	static ImageIntersection create(const Image &target, const Image &source, int32_t left, int32_t top) {
		int32_t targetRegionRight = left + image_getWidth(source);
		int32_t targetRegionBottom = top + image_getHeight(source);
		assert(ImageIntersection::canCreate(target, source, left, top));
		// Check if the source has to be clipped
		if (left < 0 || top < 0 || targetRegionRight > image_getWidth(target) || targetRegionBottom > image_getHeight(target)) {
			int32_t clipLeft = max(0, -left);
			int32_t clipTop = max(0, -top);
			int32_t clipRight = max(0, targetRegionRight - image_getWidth(target));
			int32_t clipBottom = max(0, targetRegionBottom - image_getHeight(target));
			int32_t newWidth = image_getWidth(source) - (clipLeft + clipRight);
			int32_t newHeight = image_getHeight(source) - (clipTop + clipBottom);
			assert(newWidth > 0 && newHeight > 0);
			// Partial drawing
			Image subTarget = getGenericSubImage(target, left + clipLeft, top + clipTop, newWidth, newHeight);
			Image subSource = getGenericSubImage(source, clipLeft, clipTop, newWidth, newHeight);
			return ImageIntersection(getWriter(subTarget), getReader(subSource));
		} else {
			// Full drawing
			Image subTarget = getGenericSubImage(target, left, top, image_getWidth(source), image_getHeight(source));
			return ImageIntersection(getWriter(subTarget), getReader(source));
		}
	}
};

#define ITERATE_ROWS(WRITER, READER, OPERATION) \
{ \
	uint8_t *targetRow = WRITER.data; \
	const uint8_t *sourceRow = READER.data; \
	for (int32_t y = 0; y < READER.height; y++) { \
		OPERATION; \
		targetRow += WRITER.stride; \
		sourceRow += READER.stride; \
	} \
}

#define ITERATE_PIXELS(WRITER, READER, OPERATION) \
{ \
	uint8_t *targetRow = WRITER.data; \
	const uint8_t *sourceRow = READER.data; \
	for (int32_t y = 0; y < READER.height; y++) { \
		uint8_t *targetPixel = targetRow; \
		const uint8_t *sourcePixel = sourceRow; \
		for (int32_t x = 0; x < READER.width; x++) { \
			{OPERATION;} \
			targetPixel += WRITER.pixelSize; \
			sourcePixel += READER.pixelSize; \
		} \
		targetRow += WRITER.stride; \
		sourceRow += READER.stride; \
	} \
}

#define ITERATE_PIXELS_2(WRITER1, READER1, WRITER2, READER2, OPERATION) \
{ \
	uint8_t *targetRow1 = WRITER1.data; \
	uint8_t *targetRow2 = WRITER2.data; \
	const uint8_t *sourceRow1 = READER1.data; \
	const uint8_t *sourceRow2 = READER2.data; \
	int32_t minWidth = min(READER1.width, READER2.width); \
	int32_t minHeight = min(READER1.height, READER2.height); \
	for (int32_t y = 0; y < minHeight; y++) { \
		uint8_t *targetPixel1 = targetRow1; \
		uint8_t *targetPixel2 = targetRow2; \
		const uint8_t *sourcePixel1 = sourceRow1; \
		const uint8_t *sourcePixel2 = sourceRow2; \
		for (int32_t x = 0; x < minWidth; x++) { \
			{OPERATION;} \
			targetPixel1 += WRITER1.pixelSize; \
			targetPixel2 += WRITER2.pixelSize; \
			sourcePixel1 += READER1.pixelSize; \
			sourcePixel2 += READER2.pixelSize; \
		} \
		targetRow1 += WRITER1.stride; \
		targetRow2 += WRITER2.stride; \
		sourceRow1 += READER1.stride; \
		sourceRow2 += READER2.stride; \
	} \
}

#define ITERATE_PIXELS_3(WRITER1, READER1, WRITER2, READER2, WRITER3, READER3, OPERATION) \
{ \
	uint8_t *targetRow1 = WRITER1.data; \
	uint8_t *targetRow2 = WRITER2.data; \
	uint8_t *targetRow3 = WRITER3.data; \
	const uint8_t *sourceRow1 = READER1.data; \
	const uint8_t *sourceRow2 = READER2.data; \
	const uint8_t *sourceRow3 = READER3.data; \
	int32_t minWidth = min(min(READER1.width, READER2.width), READER3.width); \
	int32_t minHeight = min(min(READER1.height, READER2.height), READER3.height); \
	for (int32_t y = 0; y < minHeight; y++) { \
		uint8_t *targetPixel1 = targetRow1; \
		uint8_t *targetPixel2 = targetRow2; \
		uint8_t *targetPixel3 = targetRow3; \
		const uint8_t *sourcePixel1 = sourceRow1; \
		const uint8_t *sourcePixel2 = sourceRow2; \
		const uint8_t *sourcePixel3 = sourceRow3; \
		for (int32_t x = 0; x < minWidth; x++) { \
			{OPERATION;} \
			targetPixel1 += WRITER1.pixelSize; \
			targetPixel2 += WRITER2.pixelSize; \
			targetPixel3 += WRITER3.pixelSize; \
			sourcePixel1 += READER1.pixelSize; \
			sourcePixel2 += READER2.pixelSize; \
			sourcePixel3 += READER3.pixelSize; \
		} \
		targetRow1 += WRITER1.stride; \
		targetRow2 += WRITER2.stride; \
		targetRow3 += WRITER3.stride; \
		sourceRow1 += READER1.stride; \
		sourceRow2 += READER2.stride; \
		sourceRow3 += READER3.stride; \
	} \
}

inline int32_t saturateFloat(float value) {
	if (!(value >= 0.5f)) {
		// NaN or negative
		return 0;
	} else if (value > 254.5f) {
		// Too large
		return 255;
	} else {
		// Round to closest
		return (uint8_t)(value + 0.5f);
	}
}

// Copy data from one image region to another of the same size.
//   Packing order is reinterpreted without conversion.
static void copyImageData(ImageWriter writer, ImageReader reader) {
	assert(writer.width == reader.width && writer.height == reader.height && writer.pixelSize == reader.pixelSize);
	ITERATE_ROWS(writer, reader, std::memcpy(targetRow, sourceRow, reader.width * reader.pixelSize));
}

// TODO: Can SIMD be used for specific platforms where vector extract accepts a variable offset?
static void imageImpl_drawCopy(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top) {
	PackOrderIndex targetPackOrderIndex = image_getPackOrderIndex(target);
	PackOrderIndex sourcePackOrderIndex = image_getPackOrderIndex(source);
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		if (targetPackOrderIndex == sourcePackOrderIndex) {
			// No conversion needed
			copyImageData(intersection.subTarget, intersection.subSource);
		} else {
			PackOrder targetPackOrder = PackOrder::getPackOrder(targetPackOrderIndex);
			PackOrder sourcePackOrder = PackOrder::getPackOrder(sourcePackOrderIndex);
			// Read and repack to convert between different color formats
			// TODO: Pre-compute conversions for each combination of source and target pack order.
			//       We do not need to store the data in RGBA order, just pack from one format to another.
			ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
				targetPixel[targetPackOrder.redIndex]   = sourcePixel[sourcePackOrder.redIndex];
				targetPixel[targetPackOrder.greenIndex] = sourcePixel[sourcePackOrder.greenIndex];
				targetPixel[targetPackOrder.blueIndex]  = sourcePixel[sourcePackOrder.blueIndex];
				targetPixel[targetPackOrder.alphaIndex] = sourcePixel[sourcePackOrder.alphaIndex];
			);
		}
	}
}
static void imageImpl_drawCopy(const ImageU8& target, const ImageU8& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		copyImageData(intersection.subTarget, intersection.subSource);
	}
}
static void imageImpl_drawCopy(const ImageU16& target, const ImageU16& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		copyImageData(intersection.subTarget, intersection.subSource);
	}
}
static void imageImpl_drawCopy(const ImageF32& target, const ImageF32& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		copyImageData(intersection.subTarget, intersection.subSource);
	}
}
static void imageImpl_drawCopy(const ImageRgbaU8& target, const ImageU8& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		PackOrder targetPackOrder = image_getPackOrder(target);
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			uint8_t luma = *sourcePixel;
			targetPixel[targetPackOrder.redIndex]   = luma;
			targetPixel[targetPackOrder.greenIndex] = luma;
			targetPixel[targetPackOrder.blueIndex]  = luma;
			targetPixel[targetPackOrder.alphaIndex] = 255;
		);
	}
}
static void imageImpl_drawCopy(const ImageRgbaU8& target, const ImageU16& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		PackOrder targetPackOrder = image_getPackOrder(target);
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			int32_t luma = *((const uint16_t*)sourcePixel);
			if (luma > 255) { luma = 255; }
			targetPixel[targetPackOrder.redIndex]   = luma;
			targetPixel[targetPackOrder.greenIndex] = luma;
			targetPixel[targetPackOrder.blueIndex]  = luma;
			targetPixel[targetPackOrder.alphaIndex] = 255;
		);
	}
}
static void imageImpl_drawCopy(const ImageRgbaU8& target, const ImageF32& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		PackOrder targetPackOrder = image_getPackOrder(target);
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			int32_t luma = saturateFloat(*((const float*)sourcePixel));
			targetPixel[targetPackOrder.redIndex]   = luma;
			targetPixel[targetPackOrder.greenIndex] = luma;
			targetPixel[targetPackOrder.blueIndex]  = luma;
			targetPixel[targetPackOrder.alphaIndex] = 255;
		);
	}
}
static void imageImpl_drawCopy(const ImageU8& target, const ImageF32& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			*targetPixel = saturateFloat(*((const float*)sourcePixel));
		);
	}
}
static void imageImpl_drawCopy(const ImageU8& target, const ImageU16& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			int32_t luma = *((const uint16_t*)sourcePixel);
			if (luma > 255) { luma = 255; }
			*targetPixel = luma;
		);
	}
}
static void imageImpl_drawCopy(const ImageU16& target, const ImageU8& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			*((uint16_t*)targetPixel) = *sourcePixel;
		);
	}
}
static void imageImpl_drawCopy(const ImageU16& target, const ImageF32& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			int32_t luma = *((const float*)sourcePixel);
			if (luma < 0) { luma = 0; }
			if (luma > 65535) { luma = 65535; }
			*((uint16_t*)targetPixel) = *sourcePixel;
		);
	}
}
static void imageImpl_drawCopy(const ImageF32& target, const ImageU8& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			*((float*)targetPixel) = (float)(*sourcePixel);
		);
	}
}
static void imageImpl_drawCopy(const ImageF32& target, const ImageU16& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			int32_t luma = *((const uint16_t*)sourcePixel);
			if (luma > 255) { luma = 255; }
			*((float*)targetPixel) = (float)luma;
		);
	}
}


static void imageImpl_drawAlphaFilter(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		PackOrder targetPackOrder = image_getPackOrder(target);
		PackOrder sourcePackOrder = image_getPackOrder(source);
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		// Read and repack to convert between different color formats
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			// Optimized for anti-aliasing, where most alpha values are 0 or 255
			uint32_t sourceRatio = sourcePixel[sourcePackOrder.alphaIndex];
			if (sourceRatio > 0) {
				if (sourceRatio == 255) {
					targetPixel[targetPackOrder.redIndex]   = sourcePixel[sourcePackOrder.redIndex];
					targetPixel[targetPackOrder.greenIndex] = sourcePixel[sourcePackOrder.greenIndex];
					targetPixel[targetPackOrder.blueIndex]  = sourcePixel[sourcePackOrder.blueIndex];
					targetPixel[targetPackOrder.alphaIndex] = 255;
				} else {
					uint32_t targetRatio = 255 - sourceRatio;
					targetPixel[targetPackOrder.redIndex]   = normalizedByteMultiplication(targetPixel[targetPackOrder.redIndex], targetRatio) + normalizedByteMultiplication(sourcePixel[sourcePackOrder.redIndex], sourceRatio);
					targetPixel[targetPackOrder.greenIndex] = normalizedByteMultiplication(targetPixel[targetPackOrder.greenIndex], targetRatio) + normalizedByteMultiplication(sourcePixel[sourcePackOrder.greenIndex], sourceRatio);
					targetPixel[targetPackOrder.blueIndex]  = normalizedByteMultiplication(targetPixel[targetPackOrder.blueIndex], targetRatio) + normalizedByteMultiplication(sourcePixel[sourcePackOrder.blueIndex], sourceRatio);
					targetPixel[targetPackOrder.alphaIndex] = normalizedByteMultiplication(targetPixel[targetPackOrder.alphaIndex], targetRatio) + sourceRatio;
				}
			}
		);
	}
}

static void imageImpl_drawMaxAlpha(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top, int32_t sourceAlphaOffset) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		PackOrder targetPackOrder = image_getPackOrder(target);
		PackOrder sourcePackOrder = image_getPackOrder(source);
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		// Read and repack to convert between different color formats
		if (sourceAlphaOffset == 0) {
			ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
				int32_t sourceAlpha = sourcePixel[sourcePackOrder.alphaIndex];
				if (sourceAlpha > targetPixel[targetPackOrder.alphaIndex]) {
					targetPixel[targetPackOrder.redIndex]   = sourcePixel[sourcePackOrder.redIndex];
					targetPixel[targetPackOrder.greenIndex] = sourcePixel[sourcePackOrder.greenIndex];
					targetPixel[targetPackOrder.blueIndex]  = sourcePixel[sourcePackOrder.blueIndex];
					targetPixel[targetPackOrder.alphaIndex] = sourceAlpha;
				}
			);
		} else {
			ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
				int32_t sourceAlpha = sourcePixel[sourcePackOrder.alphaIndex];
				if (sourceAlpha > 0) {
					sourceAlpha += sourceAlphaOffset;
					if (sourceAlpha > targetPixel[targetPackOrder.alphaIndex]) {
						targetPixel[targetPackOrder.redIndex]   = sourcePixel[sourcePackOrder.redIndex];
						targetPixel[targetPackOrder.greenIndex] = sourcePixel[sourcePackOrder.greenIndex];
						targetPixel[targetPackOrder.blueIndex]  = sourcePixel[sourcePackOrder.blueIndex];
						if (sourceAlpha < 0) { sourceAlpha = 0; }
						if (sourceAlpha > 255) { sourceAlpha = 255; }
						targetPixel[targetPackOrder.alphaIndex] = sourceAlpha;
					}
				}
			);
		}
	}
}

static void imageImpl_drawAlphaClip(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top, int32_t threshold) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		PackOrder targetPackOrder = image_getPackOrder(target);
		PackOrder sourcePackOrder = image_getPackOrder(source);
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		// Read and repack to convert between different color formats
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			if (sourcePixel[sourcePackOrder.alphaIndex] > threshold) {
				targetPixel[targetPackOrder.redIndex]   = sourcePixel[sourcePackOrder.redIndex];
				targetPixel[targetPackOrder.greenIndex] = sourcePixel[sourcePackOrder.greenIndex];
				targetPixel[targetPackOrder.blueIndex]  = sourcePixel[sourcePackOrder.blueIndex];
				targetPixel[targetPackOrder.alphaIndex] = 255;
			}
		);
	}
}

template <bool FULL_ALPHA>
void drawSilhouette_template(const ImageRgbaU8& target, const ImageU8& source, const ColorRgbaI32& color, int32_t left, int32_t top) {
	if (ImageIntersection::canCreate(target, source, left, top)) {
		PackOrder targetPackOrder = image_getPackOrder(target);
		ImageIntersection intersection = ImageIntersection::create(target, source, left, top);
		// Read and repack to convert between different color formats
		ITERATE_PIXELS(intersection.subTarget, intersection.subSource,
			uint32_t sourceRatio;
			if (FULL_ALPHA) {
				sourceRatio = *sourcePixel;
			} else {
				sourceRatio = normalizedByteMultiplication(*sourcePixel, color.alpha);
			}
			if (sourceRatio > 0) {
				if (sourceRatio == 255) {
					targetPixel[targetPackOrder.redIndex]   = color.red;
					targetPixel[targetPackOrder.greenIndex] = color.green;
					targetPixel[targetPackOrder.blueIndex]  = color.blue;
					targetPixel[targetPackOrder.alphaIndex] = 255;
				} else {
					uint32_t targetRatio = 255 - sourceRatio;
					targetPixel[targetPackOrder.redIndex]   = normalizedByteMultiplication(targetPixel[targetPackOrder.redIndex], targetRatio) + normalizedByteMultiplication(color.red, sourceRatio);
					targetPixel[targetPackOrder.greenIndex] = normalizedByteMultiplication(targetPixel[targetPackOrder.greenIndex], targetRatio) + normalizedByteMultiplication(color.green, sourceRatio);
					targetPixel[targetPackOrder.blueIndex]  = normalizedByteMultiplication(targetPixel[targetPackOrder.blueIndex], targetRatio) + normalizedByteMultiplication(color.blue, sourceRatio);
					targetPixel[targetPackOrder.alphaIndex] = normalizedByteMultiplication(targetPixel[targetPackOrder.alphaIndex], targetRatio) + sourceRatio;
				}
			}
		);
	}
}
static void imageImpl_drawSilhouette(const ImageRgbaU8& target, const ImageU8& source, const ColorRgbaI32& color, int32_t left, int32_t top) {
	if (color.alpha > 0) {
		ColorRgbaI32 saturatedColor = color.saturate();
		if (color.alpha < 255) {
			drawSilhouette_template<false>(target, source, saturatedColor, left, top);
		} else {
			drawSilhouette_template<true>(target, source, saturatedColor, left, top);
		}
	}
}

static void imageImpl_drawHigher(const ImageU16& targetHeight, const ImageU16& sourceHeight, int32_t left, int32_t top, int32_t sourceHeightOffset) {
	if (ImageIntersection::canCreate(targetHeight, sourceHeight, left, top)) {
		ImageIntersection intersectionH = ImageIntersection::create(targetHeight, sourceHeight, left, top);
		ITERATE_PIXELS(intersectionH.subTarget, intersectionH.subSource,
			int32_t newHeight = *((const uint16_t*)sourcePixel);
			if (newHeight > 0) {
				newHeight += sourceHeightOffset;
				if (newHeight < 0) { newHeight = 0; }
				if (newHeight > 65535) { newHeight = 65535; }
				if (newHeight > 0 && newHeight > *((uint16_t*)targetPixel)) {
					*((uint16_t*)targetPixel) = newHeight;
				}
			}
		);
	}
}
static void imageImpl_drawHigher(const ImageU16& targetHeight, const ImageU16& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  int32_t left, int32_t top, int32_t sourceHeightOffset) {
	assert(image_getWidth(sourceA) == image_getWidth(sourceHeight));
	assert(image_getHeight(sourceA) == image_getHeight(sourceHeight));
	if (ImageIntersection::canCreate(targetHeight, sourceHeight, left, top)) {
		PackOrder targetAPackOrder = image_getPackOrder(targetA);
		PackOrder sourceAPackOrder = image_getPackOrder(sourceA);
		ImageIntersection intersectionH = ImageIntersection::create(targetHeight, sourceHeight, left, top);
		ImageIntersection intersectionA = ImageIntersection::create(targetA, sourceA, left, top);
		ITERATE_PIXELS_2(intersectionH.subTarget, intersectionH.subSource, intersectionA.subTarget, intersectionA.subSource,
			int32_t newHeight = *((const uint16_t*)sourcePixel1);
			if (newHeight > 0) {
				newHeight += sourceHeightOffset;
				if (newHeight < 0) { newHeight = 0; }
				if (newHeight > 65535) { newHeight = 65535; }
				if (newHeight > *((uint16_t*)targetPixel1)) {
					*((uint16_t*)targetPixel1) = newHeight;
					targetPixel2[targetAPackOrder.redIndex]   = sourcePixel2[sourceAPackOrder.redIndex];
					targetPixel2[targetAPackOrder.greenIndex] = sourcePixel2[sourceAPackOrder.greenIndex];
					targetPixel2[targetAPackOrder.blueIndex]  = sourcePixel2[sourceAPackOrder.blueIndex];
					targetPixel2[targetAPackOrder.alphaIndex] = sourcePixel2[sourceAPackOrder.alphaIndex];
				}
			}
		);
	}
}
static void imageImpl_drawHigher(const ImageU16& targetHeight, const ImageU16& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  ImageRgbaU8& targetB, const ImageRgbaU8& sourceB, int32_t left, int32_t top, int32_t sourceHeightOffset) {
	assert(image_getWidth(sourceA) == image_getWidth(sourceHeight));
	assert(image_getHeight(sourceA) == image_getHeight(sourceHeight));
	assert(image_getWidth(sourceB) == image_getWidth(sourceHeight));
	assert(image_getHeight(sourceB) == image_getHeight(sourceHeight));
	if (ImageIntersection::canCreate(targetHeight, sourceHeight, left, top)) {
		PackOrder targetAPackOrder = image_getPackOrder(targetA);
		PackOrder targetBPackOrder = image_getPackOrder(targetB);
		PackOrder sourceAPackOrder = image_getPackOrder(sourceA);
		PackOrder sourceBPackOrder = image_getPackOrder(sourceB);
		ImageIntersection intersectionH = ImageIntersection::create(targetHeight, sourceHeight, left, top);
		ImageIntersection intersectionA = ImageIntersection::create(targetA, sourceA, left, top);
		ImageIntersection intersectionB = ImageIntersection::create(targetB, sourceB, left, top);
		ITERATE_PIXELS_3(intersectionH.subTarget, intersectionH.subSource, intersectionA.subTarget, intersectionA.subSource, intersectionB.subTarget, intersectionB.subSource,
			int32_t newHeight = *((const uint16_t*)sourcePixel1);
			if (newHeight > 0) {
				newHeight += sourceHeightOffset;
				if (newHeight < 0) { newHeight = 0; }
				if (newHeight > 65535) { newHeight = 65535; }
				if (newHeight > *((uint16_t*)targetPixel1)) {
					*((uint16_t*)targetPixel1) = newHeight;
					targetPixel2[targetAPackOrder.redIndex]   = sourcePixel2[sourceAPackOrder.redIndex];
					targetPixel2[targetAPackOrder.greenIndex] = sourcePixel2[sourceAPackOrder.greenIndex];
					targetPixel2[targetAPackOrder.blueIndex]  = sourcePixel2[sourceAPackOrder.blueIndex];
					targetPixel2[targetAPackOrder.alphaIndex] = sourcePixel2[sourceAPackOrder.alphaIndex];
					targetPixel3[targetBPackOrder.redIndex]   = sourcePixel3[sourceBPackOrder.redIndex];
					targetPixel3[targetBPackOrder.greenIndex] = sourcePixel3[sourceBPackOrder.greenIndex];
					targetPixel3[targetBPackOrder.blueIndex]  = sourcePixel3[sourceBPackOrder.blueIndex];
					targetPixel3[targetBPackOrder.alphaIndex] = sourcePixel3[sourceBPackOrder.alphaIndex];
				}
			}
		);
	}
}

static void imageImpl_drawHigher(const ImageF32& targetHeight, const ImageF32& sourceHeight, int32_t left, int32_t top, float sourceHeightOffset) {
	if (ImageIntersection::canCreate(targetHeight, sourceHeight, left, top)) {
		ImageIntersection intersectionH = ImageIntersection::create(targetHeight, sourceHeight, left, top);
		ITERATE_PIXELS(intersectionH.subTarget, intersectionH.subSource,
			float newHeight = *((const float*)sourcePixel);
			if (newHeight > -std::numeric_limits<float>::infinity()) {
				newHeight += sourceHeightOffset;
				if (newHeight > *((float*)targetPixel)) {
					*((float*)targetPixel) = newHeight;
				}
			}
		);
	}
}
static void imageImpl_drawHigher(const ImageF32& targetHeight, const ImageF32& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  int32_t left, int32_t top, float sourceHeightOffset) {
	assert(image_getWidth(sourceA) == image_getWidth(sourceHeight));
	assert(image_getHeight(sourceA) == image_getHeight(sourceHeight));
	if (ImageIntersection::canCreate(targetHeight, sourceHeight, left, top)) {
		PackOrder targetAPackOrder = image_getPackOrder(targetA);
		PackOrder sourceAPackOrder = image_getPackOrder(sourceA);
		ImageIntersection intersectionH = ImageIntersection::create(targetHeight, sourceHeight, left, top);
		ImageIntersection intersectionA = ImageIntersection::create(targetA, sourceA, left, top);
		ITERATE_PIXELS_2(intersectionH.subTarget, intersectionH.subSource, intersectionA.subTarget, intersectionA.subSource,
			float newHeight = *((const float*)sourcePixel1);
			if (newHeight > -std::numeric_limits<float>::infinity()) {
				newHeight += sourceHeightOffset;
				if (newHeight > *((float*)targetPixel1)) {
					*((float*)targetPixel1) = newHeight;
					targetPixel2[targetAPackOrder.redIndex]   = sourcePixel2[sourceAPackOrder.redIndex];
					targetPixel2[targetAPackOrder.greenIndex] = sourcePixel2[sourceAPackOrder.greenIndex];
					targetPixel2[targetAPackOrder.blueIndex]  = sourcePixel2[sourceAPackOrder.blueIndex];
					targetPixel2[targetAPackOrder.alphaIndex] = sourcePixel2[sourceAPackOrder.alphaIndex];
				}
			}
		);
	}
}
static void imageImpl_drawHigher(const ImageF32& targetHeight, const ImageF32& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  ImageRgbaU8& targetB, const ImageRgbaU8& sourceB, int32_t left, int32_t top, float sourceHeightOffset) {
	assert(image_getWidth(sourceA) == image_getWidth(sourceHeight));
	assert(image_getHeight(sourceA) == image_getHeight(sourceHeight));
	assert(image_getWidth(sourceB) == image_getWidth(sourceHeight));
	assert(image_getHeight(sourceB) == image_getHeight(sourceHeight));
	if (ImageIntersection::canCreate(targetHeight, sourceHeight, left, top)) {
		PackOrder targetAPackOrder = image_getPackOrder(targetA);
		PackOrder targetBPackOrder = image_getPackOrder(targetB);
		PackOrder sourceAPackOrder = image_getPackOrder(sourceA);
		PackOrder sourceBPackOrder = image_getPackOrder(sourceB);
		ImageIntersection intersectionH = ImageIntersection::create(targetHeight, sourceHeight, left, top);
		ImageIntersection intersectionA = ImageIntersection::create(targetA, sourceA, left, top);
		ImageIntersection intersectionB = ImageIntersection::create(targetB, sourceB, left, top);
		ITERATE_PIXELS_3(intersectionH.subTarget, intersectionH.subSource, intersectionA.subTarget, intersectionA.subSource, intersectionB.subTarget, intersectionB.subSource,
			float newHeight = *((const float*)sourcePixel1);
			if (newHeight > -std::numeric_limits<float>::infinity()) {
				newHeight += sourceHeightOffset;
				if (newHeight > *((float*)targetPixel1)) {
					*((float*)targetPixel1) = newHeight;
					targetPixel2[targetAPackOrder.redIndex]   = sourcePixel2[sourceAPackOrder.redIndex];
					targetPixel2[targetAPackOrder.greenIndex] = sourcePixel2[sourceAPackOrder.greenIndex];
					targetPixel2[targetAPackOrder.blueIndex]  = sourcePixel2[sourceAPackOrder.blueIndex];
					targetPixel2[targetAPackOrder.alphaIndex] = sourcePixel2[sourceAPackOrder.alphaIndex];
					targetPixel3[targetBPackOrder.redIndex]   = sourcePixel3[sourceBPackOrder.redIndex];
					targetPixel3[targetBPackOrder.greenIndex] = sourcePixel3[sourceBPackOrder.greenIndex];
					targetPixel3[targetBPackOrder.blueIndex]  = sourcePixel3[sourceBPackOrder.blueIndex];
					targetPixel3[targetBPackOrder.alphaIndex] = sourcePixel3[sourceBPackOrder.alphaIndex];
				}
			}
		);
	}
}

#define DRAW_COPY_WRAPPER(TARGET_TYPE, SOURCE_TYPE) \
	void draw_copy(const TARGET_TYPE& target, const SOURCE_TYPE& source, int32_t left, int32_t top) { \
		if (image_exists(target) && image_exists(source)) { \
			imageImpl_drawCopy(target, source, left, top); \
		} \
	}
DRAW_COPY_WRAPPER(ImageU8, ImageU8);
DRAW_COPY_WRAPPER(ImageU8, ImageU16);
DRAW_COPY_WRAPPER(ImageU8, ImageF32);
DRAW_COPY_WRAPPER(ImageU16, ImageU8);
DRAW_COPY_WRAPPER(ImageU16, ImageU16);
DRAW_COPY_WRAPPER(ImageU16, ImageF32);
DRAW_COPY_WRAPPER(ImageF32, ImageU8);
DRAW_COPY_WRAPPER(ImageF32, ImageU16);
DRAW_COPY_WRAPPER(ImageF32, ImageF32);
DRAW_COPY_WRAPPER(ImageRgbaU8, ImageU8);
DRAW_COPY_WRAPPER(ImageRgbaU8, ImageU16);
DRAW_COPY_WRAPPER(ImageRgbaU8, ImageF32);
DRAW_COPY_WRAPPER(ImageRgbaU8, ImageRgbaU8);

void draw_alphaFilter(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top) {
	if (image_exists(target) && image_exists(source)) {
		imageImpl_drawAlphaFilter(target, source, left, top);
	}
}
void draw_maxAlpha(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top, int32_t sourceAlphaOffset) {
	if (image_exists(target) && image_exists(source)) {
		imageImpl_drawMaxAlpha(target, source, left, top, sourceAlphaOffset);
	}
}
void draw_alphaClip(const ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top, int32_t threshold) {
	if (image_exists(target) && image_exists(source)) {
		imageImpl_drawAlphaClip(target, source, left, top, threshold);
	}
}
void draw_silhouette(const ImageRgbaU8& target, const ImageU8& source, const ColorRgbaI32& color, int32_t left, int32_t top) {
	if (image_exists(target) && image_exists(source)) {
		imageImpl_drawSilhouette(target, source, color, left, top);
	}
}
void draw_higher(const ImageU16& targetHeight, const ImageU16& sourceHeight, int32_t left, int32_t top, int32_t sourceHeightOffset) {
	if (image_exists(targetHeight) && image_exists(sourceHeight)) {
		imageImpl_drawHigher(targetHeight, sourceHeight, left, top, sourceHeightOffset);
	}
}
void draw_higher(const ImageU16& targetHeight, const ImageU16& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  int32_t left, int32_t top, int32_t sourceHeightOffset) {
	if (image_exists(targetHeight) && image_exists(sourceHeight) && image_exists(targetA) && image_exists(sourceA)) {
		imageImpl_drawHigher(targetHeight, sourceHeight, targetA, sourceA, left, top, sourceHeightOffset);
	}
}
void draw_higher(const ImageU16& targetHeight, const ImageU16& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  ImageRgbaU8& targetB, const ImageRgbaU8& sourceB, int32_t left, int32_t top, int32_t sourceHeightOffset) {
	if (image_exists(targetHeight) && image_exists(sourceHeight) && image_exists(targetA) && image_exists(sourceA) && image_exists(targetB) && image_exists(sourceB)) {
		imageImpl_drawHigher(targetHeight, sourceHeight, targetA, sourceA, targetB, sourceB, left, top, sourceHeightOffset);
	}
}
void draw_higher(const ImageF32& targetHeight, const ImageF32& sourceHeight, int32_t left, int32_t top, float sourceHeightOffset) {
	if (image_exists(targetHeight) && image_exists(sourceHeight)) {
		imageImpl_drawHigher(targetHeight, sourceHeight, left, top, sourceHeightOffset);
	}
}
void draw_higher(const ImageF32& targetHeight, const ImageF32& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  int32_t left, int32_t top, float sourceHeightOffset) {
	if (image_exists(targetHeight) && image_exists(sourceHeight) && image_exists(targetA) && image_exists(sourceA)) {
		imageImpl_drawHigher(targetHeight, sourceHeight, targetA, sourceA, left, top, sourceHeightOffset);
	}
}
void draw_higher(const ImageF32& targetHeight, const ImageF32& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  ImageRgbaU8& targetB, const ImageRgbaU8& sourceB, int32_t left, int32_t top, float sourceHeightOffset) {
	if (image_exists(targetHeight) && image_exists(sourceHeight) && image_exists(targetA) && image_exists(sourceA) && image_exists(targetB) && image_exists(sourceB)) {
		imageImpl_drawHigher(targetHeight, sourceHeight, targetA, sourceA, targetB, sourceB, left, top, sourceHeightOffset);
	}
}

}
