
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

#include <limits>
#include <cassert>
#include "imageAPI.h"
#include "drawAPI.h"
#include "fileAPI.h"
#include "../implementation/image/stbImage/stbImageWrapper.h"
#include "../implementation/math/scalar.h"
#include "../settings.h"

namespace dsr {

static const int32_t maximumImageWidth = 65536;
static const int32_t maximumImageHeight = 65536;

template <typename IMAGE_TYPE>
IMAGE_TYPE image_create_template(const char * name, int32_t width, int32_t height, PackOrderIndex packOrderIndex, bool zeroed) {
	if (width < 1 || width > maximumImageWidth || height < 1 || height > maximumImageHeight) {
		sendWarning(U"");
		// Return an empty image on failure.
		return IMAGE_TYPE();
	} else {
		static const int32_t pixelSize = image_getPixelSize<IMAGE_TYPE>();
		// Calculate the stride.
		uintptr_t byteStride = memory_getPaddedSize(width * pixelSize, heap_getHeapAlignment());
		uint32_t pixelStride = byteStride / pixelSize;
		// Create the image.
		return IMAGE_TYPE(buffer_create(byteStride * height, 1, zeroed).setName(name), 0, width, height, pixelStride, packOrderIndex);
	}
}

// Take the dimensions as signed integers to avoid getting extreme dimensions on underflow.
AlignedImageU8 image_create_U8(int32_t width, int32_t height, bool zeroed) {
	return image_create_template<AlignedImageU8>("U8 pixel buffer", width, height, PackOrderIndex::RGBA, zeroed);
}

AlignedImageU16 image_create_U16(int32_t width, int32_t height, bool zeroed) {
	return image_create_template<AlignedImageU16>("U16 pixel buffer", width, height, PackOrderIndex::RGBA, zeroed);
}

AlignedImageF32 image_create_F32(int32_t width, int32_t height, bool zeroed) {
	return image_create_template<AlignedImageF32>("F32 pixel buffer", width, height, PackOrderIndex::RGBA, zeroed);
}

OrderedImageRgbaU8 image_create_RgbaU8(int32_t width, int32_t height, bool zeroed) {
	return image_create_template<OrderedImageRgbaU8>("RgbaU8 pixel buffer", width, height, PackOrderIndex::RGBA, zeroed);
}

AlignedImageRgbaU8 image_create_RgbaU8_native(int32_t width, int32_t height, PackOrderIndex packOrderIndex, bool zeroed) {
	return image_create_template<OrderedImageRgbaU8>("Native pixel buffer", width, height, packOrderIndex, zeroed);
}

// Pre-condition: image exists.
// Post-condition: Returns true if the stride is larger than the image's width.
template <typename IMAGE_TYPE>
inline bool imageIsPadded(const IMAGE_TYPE &image) {
	return image_getWidth(image) * image_getPixelSize(image) < image_getStride(image);
}

// Loading from data pointer
OrderedImageRgbaU8 image_decode_RgbaU8(SafePointer<const uint8_t> data, int size) {
	if (data.isNotNull()) {
		return image_stb_decode_RgbaU8(data, size);
	} else {
		return OrderedImageRgbaU8();
	}
}
// Loading from buffer
OrderedImageRgbaU8 image_decode_RgbaU8(const Buffer& fileContent) {
	return image_decode_RgbaU8(buffer_getSafeData<uint8_t>(fileContent, "image file buffer"), buffer_getSize(fileContent));
}
// Loading from file
OrderedImageRgbaU8 image_load_RgbaU8(const ReadableString& filename, bool mustExist) {
	OrderedImageRgbaU8 result;
	Buffer fileContent = file_loadBuffer(filename, mustExist);
	if (buffer_exists(fileContent)) {
		result = image_decode_RgbaU8(fileContent);
		if (mustExist && !image_exists(result)) {
			throwError(U"image_load_RgbaU8: Failed to load the image at ", filename, U".\n");
		}
	}
	return result;
}

Buffer image_encode(const ImageRgbaU8 &image, ImageFileFormat format, int quality) {
	if (image_exists(image)) {
		ImageRgbaU8 orderedImage;
		if (image_getPackOrderIndex(image) != PackOrderIndex::RGBA) {
			// Repack into RGBA.
			orderedImage = image_clone(image);
		} else {
			// Take the image handle as is.
			orderedImage = image;
		}
		if (imageIsPadded(orderedImage) && format != ImageFileFormat::PNG) {
			// If orderedImage is padded and it's not requested as PNG, the padding has to be removed first.
			return image_stb_encode(image_removePadding(orderedImage), format, quality);
		} else {
			// Send orderedImage directly to encoding.
			return image_stb_encode(orderedImage, format, quality);
		}
	} else {
		return Buffer();
	}
}

static ImageFileFormat detectImageFileExtension(const ReadableString& filename) {
	ImageFileFormat result = ImageFileFormat::Unknown;
	int lastDotIndex = string_findLast(filename, U'.');
	if (lastDotIndex != -1) {
		String extension = string_upperCase(file_getExtension(filename));
		if (string_match(extension, U"JPG") || string_match(extension, U"JPEG")) {
			result = ImageFileFormat::JPG;
		} else if (string_match(extension, U"PNG")) {
			result = ImageFileFormat::PNG;
		} else if (string_match(extension, U"TARGA") || string_match(extension, U"TGA")) {
			result = ImageFileFormat::TGA;
		} else if (string_match(extension, U"BMP")) {
			result = ImageFileFormat::BMP;
		}
	}
	return result;
}

bool image_save(const ImageRgbaU8 &image, const ReadableString& filename, bool mustWork, int quality) {
	ImageFileFormat extension = detectImageFileExtension(filename);
	Buffer buffer;
	if (extension == ImageFileFormat::Unknown) {
		if (mustWork) { throwError(U"The extension *.", file_getExtension(filename), " in ", filename, " is not a supported image format.\n"); }
		return false;
	} else {
		buffer = image_encode(image, extension, quality);
	}
	if (buffer_exists(buffer)) {
		return file_saveBuffer(filename, buffer, mustWork);
	} else {
		if (mustWork) { throwError(U"Failed to encode an image that was going to be saved as ", filename, "\n"); }
		return false;
	}
}

void image_fill(const ImageU8& image, int32_t color) {
	if (image_exists(image)) {
		draw_rectangle(image, image_getBound(image), color);
	}
}
void image_fill(const ImageU16& image, int32_t color) {
	if (image_exists(image)) {
		draw_rectangle(image, image_getBound(image), color);
	}
}
void image_fill(const ImageF32& image, float color) {
	if (image_exists(image)) {
		draw_rectangle(image, image_getBound(image), color);
	}
}
void image_fill(const ImageRgbaU8& image, const ColorRgbaI32& color) {
	if (image_exists(image)) {
		draw_rectangle(image, image_getBound(image), color);
	}
}

AlignedImageU8 image_clone(const ImageU8& image) {
	if (image_exists(image)) {
		AlignedImageU8 result = image_create_U8(image_getWidth(image), image_getHeight(image));
		draw_copy(result, image);
		return result;
	} else {
		return AlignedImageU8(); // Null gives null
	}
}
AlignedImageU16 image_clone(const ImageU16& image) {
	if (image_exists(image)) {
		AlignedImageU16 result = image_create_U16(image_getWidth(image), image_getHeight(image));
		draw_copy(result, image);
		return result;
	} else {
		return AlignedImageU16(); // Null gives null
	}
}
AlignedImageF32 image_clone(const ImageF32& image) {
	if (image_exists(image)) {
		AlignedImageF32 result = image_create_F32(image_getWidth(image), image_getHeight(image));
		draw_copy(result, image);
		return result;
	} else {
		return AlignedImageF32(); // Null gives null
	}
}
OrderedImageRgbaU8 image_clone(const ImageRgbaU8& image) {
	if (image_exists(image)) {
		OrderedImageRgbaU8 result = image_create_RgbaU8(image_getWidth(image), image_getHeight(image));
		draw_copy(result, image);
		return result;
	} else {
		return OrderedImageRgbaU8(); // Null gives null
	}
}

ImageRgbaU8 image_removePadding(const ImageRgbaU8& image) {
	if (!image_exists(image)) {
		return ImageRgbaU8(); // Null gives null
	} else if (imageIsPadded(image)) {
		return image;
	} else {
		uint32_t targetStride = image_getWidth(image) * image_getPixelSize(image);
		int32_t sourceStride = image_getStride(image);
		Buffer newBuffer = buffer_create(targetStride * image_getHeight(image));
		SafePointer<const uint8_t> sourceRow = image_getSafePointer<uint8_t>(image);
		SafePointer<uint8_t> targetRow = buffer_getSafeData<uint8_t>(newBuffer, "RgbaU8 padding removal target");
		for (int32_t y = 0; y < image_getHeight(image); y++) {
			safeMemoryCopy(targetRow, sourceRow, targetStride);
			sourceRow.increaseBytes(sourceStride);
			targetRow.increaseBytes(targetStride);
		}
		return ImageRgbaU8(newBuffer, 0, image_getWidth(image), image_getHeight(image), targetStride * image_getPixelSize<ImageRgbaU8>(), image_getPackOrderIndex(image));
	}
}

static void extractChannel(SafePointer<uint8_t> targetData, int targetStride, SafePointer<const uint8_t> sourceData, int sourceStride, int sourceChannels, int channelIndex, int width, int height) {
	SafePointer<const uint8_t> sourceRow = sourceData + channelIndex;
	SafePointer<uint8_t> targetRow = targetData;
	for (int y = 0; y < height; y++) {
		SafePointer<const uint8_t> sourceElement = sourceRow;
		SafePointer<uint8_t> targetElement = targetRow;
		for (int x = 0; x < width; x++) {
			*targetElement = *sourceElement; // Copy one channel from the soruce
			sourceElement += sourceChannels; // Jump to the same channel in the next source pixel
			targetElement += 1; // Jump to the next monochrome target pixel
		}
		sourceRow.increaseBytes(sourceStride);
		targetRow.increaseBytes(targetStride);
	}
}

static AlignedImageU8 getChannel(const ImageRgbaU8 image, int32_t channelIndex) {
	// Warning for debug mode
	static int channelCount = 4;
	assert(0 <= channelIndex && channelIndex < channelCount);
	AlignedImageU8 result = image_create_U8(image_getWidth(image), image_getHeight(image));
	extractChannel(image_getSafePointer<uint8_t>(result), image_getStride(result), image_getSafePointer<uint8_t>(image), image_getStride(image), channelCount, channelIndex, image_getWidth(image), image_getHeight(image));
	return result;
}

AlignedImageU8 image_get_red(const ImageRgbaU8& image) {
	if (image_exists(image)) {
		return getChannel(image, image_getPackOrder(image).redIndex);
	} else {
		return AlignedImageU8();
	}
}
AlignedImageU8 image_get_green(const ImageRgbaU8& image) {
	if (image_exists(image)) {
		return getChannel(image, image_getPackOrder(image).greenIndex);
	} else {
		return AlignedImageU8(); // Null gives null
	}
}
AlignedImageU8 image_get_blue(const ImageRgbaU8& image) {
	if (image_exists(image)) {
		return getChannel(image, image_getPackOrder(image).blueIndex);
	} else {
		return AlignedImageU8(); // Null gives null
	}
}
AlignedImageU8 image_get_alpha(const ImageRgbaU8& image) {
	if (image_exists(image)) {
		return getChannel(image, image_getPackOrder(image).alphaIndex);
	} else {
		return AlignedImageU8(); // Null gives null
	}
}

static inline int32_t readColor(const ImageU8& channel, int x, int y) {
	return image_accessPixel(channel, x, y);
}
static inline int32_t readColor(int32_t color, int x, int y) {
	return color;
}
template <typename R, typename G, typename B, typename A>
static OrderedImageRgbaU8 pack_template(int32_t width, int32_t height, R red, G green, B blue, A alpha) {
	OrderedImageRgbaU8 result = image_create_RgbaU8(width, height);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			ColorRgbaI32 color = ColorRgbaI32(readColor(red, x, y), readColor(green, x, y), readColor(blue, x, y), readColor(alpha, x, y));
			image_writePixel(result, x, y, color);
		}
	}
	return result;
}

#define PACK1(FIRST) \
if (image_exists(FIRST)) { \
	return pack_template(image_getWidth(FIRST), image_getHeight(FIRST), red, green, blue, alpha); \
} else { \
	return OrderedImageRgbaU8(); \
}
OrderedImageRgbaU8 image_pack(const ImageU8& red, int32_t green, int32_t blue, int32_t alpha) { PACK1(red); }
OrderedImageRgbaU8 image_pack(int32_t red, const ImageU8& green, int32_t blue, int32_t alpha) { PACK1(green); }
OrderedImageRgbaU8 image_pack(int32_t red, int32_t green, const ImageU8& blue, int32_t alpha) { PACK1(blue); }
OrderedImageRgbaU8 image_pack(int32_t red, int32_t green, int32_t blue, const ImageU8& alpha) { PACK1(alpha); }

#define PACK2(FIRST,SECOND) \
if (image_exists(FIRST) && image_exists(SECOND)) { \
	if (image_getWidth(FIRST) != image_getWidth(SECOND) || image_getHeight(FIRST) != image_getHeight(SECOND)) { \
		throwError("Cannot pack two channels of different size!\n"); \
	} \
	return pack_template(image_getWidth(FIRST), image_getHeight(FIRST), red, green, blue, alpha); \
} else { \
	return OrderedImageRgbaU8(); \
}
OrderedImageRgbaU8 image_pack(const ImageU8& red, const ImageU8& green, int32_t blue, int32_t alpha) { PACK2(red,green) }
OrderedImageRgbaU8 image_pack(const ImageU8& red, int32_t green, const ImageU8& blue, int32_t alpha) { PACK2(red,blue) }
OrderedImageRgbaU8 image_pack(const ImageU8& red, int32_t green, int32_t blue, const ImageU8& alpha) { PACK2(red,alpha) }
OrderedImageRgbaU8 image_pack(int32_t red, const ImageU8& green, const ImageU8& blue, int32_t alpha) { PACK2(green,blue) }
OrderedImageRgbaU8 image_pack(int32_t red, const ImageU8& green, int32_t blue, const ImageU8& alpha) { PACK2(green,alpha) }
OrderedImageRgbaU8 image_pack(int32_t red, int32_t green, const ImageU8& blue, const ImageU8& alpha) { PACK2(blue,alpha) }

#define PACK3(FIRST,SECOND,THIRD) \
if (image_exists(FIRST) && image_exists(SECOND) && image_exists(THIRD)) { \
	if (image_getWidth(FIRST) != image_getWidth(SECOND) || image_getHeight(FIRST) != image_getHeight(SECOND) \
	 || image_getWidth(FIRST) != image_getWidth(THIRD) || image_getHeight(FIRST) != image_getHeight(THIRD)) { \
		throwError("Cannot pack three channels of different size!\n"); \
	} \
	return pack_template(image_getWidth(FIRST), image_getHeight(FIRST), red, green, blue, alpha); \
} else { \
	return OrderedImageRgbaU8(); \
}
OrderedImageRgbaU8 image_pack(int32_t red, const ImageU8& green, const ImageU8& blue, const ImageU8& alpha) { PACK3(green, blue, alpha) }
OrderedImageRgbaU8 image_pack(const ImageU8& red, int32_t green, const ImageU8& blue, const ImageU8& alpha) { PACK3(red, blue, alpha) }
OrderedImageRgbaU8 image_pack(const ImageU8& red, const ImageU8& green, int32_t blue, const ImageU8& alpha) { PACK3(red, green, alpha) }
OrderedImageRgbaU8 image_pack(const ImageU8& red, const ImageU8& green, const ImageU8& blue, int32_t alpha) { PACK3(red, green, blue) }

#define PACK4(FIRST,SECOND,THIRD,FOURTH) \
if (image_exists(FIRST) && image_exists(SECOND) && image_exists(THIRD) && image_exists(FOURTH)) { \
	if (image_getWidth(FIRST) != image_getWidth(SECOND) || image_getHeight(FIRST) != image_getHeight(SECOND) \
	 || image_getWidth(FIRST) != image_getWidth(THIRD) || image_getHeight(FIRST) != image_getHeight(THIRD) \
 	 || image_getWidth(FIRST) != image_getWidth(FOURTH) || image_getHeight(FIRST) != image_getHeight(FOURTH)) { \
		throwError("Cannot pack four channels of different size!\n"); \
	} \
	return pack_template(image_getWidth(FIRST), image_getHeight(FIRST), red, green, blue, alpha); \
} else { \
	return OrderedImageRgbaU8(); \
}
OrderedImageRgbaU8 image_pack(const ImageU8& red, const ImageU8& green, const ImageU8& blue, const ImageU8& alpha) { PACK4(red, green, blue, alpha) }

// Convert a grayscale image into an ascii image using the given alphabet.
//   Since all 256 characters cannot be in the alphabet, the encoding is lossy.
// Each line is stored within <> to prevent text editors from removing meaningful white space.
// The first line contains the given alphabet as a gradient from black to white.
// Preconditions:
//   alphabet may not have extended ascii, non printable, '\', '"', '>' or linebreak
//   width <= stride
//   size of monochromeImage = height * stride
// Example alphabet: " .,-_':;!+~=^?*abcdefghijklmnopqrstuvwxyz()[]{}|&@#0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
String image_toAscii(const ImageU8& image, const String& alphabet) {
	if (!image_exists(image)) {
		return U"null";
	}
	String result;
	char alphabetMap[256];
	int alphabetSize = string_length(alphabet);
	int width = image_getWidth(image);
	int height = image_getHeight(image);
	string_reserve(result, ((width + 4) * height) + alphabetSize + 5);
	double scale = (double)(alphabetSize - 1) / 255.0;
	double output = 0.49;
	for (int rawValue = 0; rawValue < 256; rawValue++) {
		int charIndex = (int)output;
		if (charIndex < 0) charIndex = 0;
		if (charIndex > alphabetSize - 1) charIndex = alphabetSize - 1;
		alphabetMap[rawValue] = alphabet[charIndex];
		output += scale;
	}
	string_appendChar(result, U'<');
	for (int charIndex = 0; charIndex < alphabetSize; charIndex++) {
		string_appendChar(result, alphabet[charIndex]);
	}
	string_append(result, U">\n");
	for (int y = 0; y < height; y++) {
		string_appendChar(result, U'<');
		for (int x = 0; x < width; x++) {
			string_appendChar(result, alphabetMap[image_readPixel_clamp(image, x, y)]);
		}
		string_append(result, U">\n");
	}
	return result;
}

String image_toAscii(const ImageU8& image) {
	return image_toAscii(image, U" .,-_':;!+~=^?*abcdefghijklmnopqrstuvwxyz()[]{}|&@#0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

// Create a monochrome image from the ascii image in content.
// String is used instead of ReadableString, so that the content can be decompressed from 8-bit strings in the binary.
AlignedImageU8 image_fromAscii(const String& content) {
	char alphabet[128];
	uint8_t alphabetMap[128];
	char current;
	int x = 0;
	int y = -1;
	int width = 0;
	int height = 0;
	int alphabetSize = 0;
	int contentSize = string_length(content);
	bool quoted = false;
	int i = 0;
	while (i < contentSize && ((current = content[i]) != '\0')) {
		if (quoted) {
			if (y < 0) {
				if (current == '>') {
					quoted = false;
					y = 0;
				} else if (alphabetSize < 128) {
					alphabet[alphabetSize] = current;
					alphabetSize++;
				}
			} else {
				if (current == '>') {
					quoted = false;
					if (width < x) width = x;
					y++;
					x = 0;
				} else {
					x++;
				}
			}
		} else if (current == '<') {
			quoted = true;
		}
		i++;
	}
	if (alphabetSize < 2) {
		throwError(U"The alphabet needs at least two characters!");
	}
	height = y;
	if (x > 0) {
		throwError(U"All ascii images must end with a linebreak!");
	}
	for (i = 0; i < 128; i++) {
		alphabetMap[i] = 0;
	}
	for (i = 0; i < alphabetSize; i++) {
		int code = (int)(alphabet[i]);
		if (code < 32 || code > 126) {
			throwError(U"Ascii image contained non-printable standard ascii! Use codes 32 to 126.");
		}
		if (alphabetMap[code] > 0) {
			throwError(U"A character in the alphabet was used more than once!");
		}
		int value = (int)(((double)i) * (255.0f / ((double)(alphabetSize - 1))));
		if (value < 0) value = 0;
		if (value > 255) value = 255;
		alphabetMap[code] = value;
	}
	if (width <= 0 || height <= 0) {
		throwError(U"An ascii image had zero dimensions!");
	}
	AlignedImageU8 result = image_create_U8(width, height);
	x = 0; y = -1;
	quoted = false;
	i = 0;
	while (i < contentSize && ((current = content[i]) != '\0')) {
		if (quoted) {
			if (current == '>') {
				quoted = false;
				if (y >= 0 && x != width) {
					throwError(U"Lines in the ascii image do not have the same lengths.");
				}
				y++;
				x = 0;
			} else if (y >= 0) {
				int code = (int)current;
				if (code < 0) code = 0;
				if (code > 127) code = 127;
				image_writePixel(result, x, y, alphabetMap[code]);
				x++;
			}
		} else if (current == '<') {
			quoted = true;
		}
		i++;
	}
	return result;
}

template <typename IMAGE_TYPE, int CHANNELS, typename ELEMENT_TYPE>
ELEMENT_TYPE maxDifference_template(const IMAGE_TYPE& imageA, const IMAGE_TYPE& imageB) {
	if (image_getWidth(imageA) != image_getWidth(imageB) || image_getHeight(imageA) != image_getHeight(imageB)) {
		return std::numeric_limits<ELEMENT_TYPE>::max();
	} else {
		intptr_t strideA = image_getStride(imageA);
		intptr_t strideB = image_getStride(imageB);
		ELEMENT_TYPE maxDifference = 0;
		SafePointer<const ELEMENT_TYPE> rowDataA = image_getSafePointer<ELEMENT_TYPE>(imageA);
		SafePointer<const ELEMENT_TYPE> rowDataB = image_getSafePointer<ELEMENT_TYPE>(imageB);
		for (int y = 0; y < image_getHeight(imageA); y++) {
			SafePointer<const ELEMENT_TYPE> pixelDataA = rowDataA;
			SafePointer<const ELEMENT_TYPE> pixelDataB = rowDataB;
			for (int x = 0; x < image_getWidth(imageA); x++) {
				for (int c = 0; c < CHANNELS; c++) {
					ELEMENT_TYPE difference = absDiff(*pixelDataA, *pixelDataB);
					if (difference > maxDifference) {
						maxDifference = difference;
					}
					pixelDataA += 1;
					pixelDataB += 1;
				}
			}
			rowDataA.increaseBytes(strideA);
			rowDataB.increaseBytes(strideB);
		}
		return maxDifference;
	}
}
uint8_t image_maxDifference(const ImageU8& imageA, const ImageU8& imageB) {
	if (image_exists(imageA) && image_exists(imageB)) {
		return maxDifference_template<ImageU8, 1, uint8_t>(imageA, imageB);
	} else {
		return std::numeric_limits<uint8_t>::infinity();
	}
}
uint16_t image_maxDifference(const ImageU16& imageA, const ImageU16& imageB) {
	if (image_exists(imageA) && image_exists(imageB)) {
		return maxDifference_template<ImageU16, 1, uint16_t>(imageA, imageB);
	} else {
		return std::numeric_limits<uint16_t>::infinity();
	}
}
float image_maxDifference(const ImageF32& imageA, const ImageF32& imageB) {
	if (image_exists(imageA) && image_exists(imageB)) {
		return maxDifference_template<ImageF32, 1, float>(imageA, imageB);
	} else {
		return std::numeric_limits<float>::infinity();
	}
}
uint8_t image_maxDifference(const ImageRgbaU8& imageA, const ImageRgbaU8& imageB) {
	if (image_exists(imageA) && image_exists(imageB)) {
		return maxDifference_template<ImageRgbaU8, 4, uint8_t>(imageA, imageB);
	} else {
		return std::numeric_limits<uint8_t>::infinity();
	}
}

}
