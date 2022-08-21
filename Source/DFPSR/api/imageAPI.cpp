
// zlib open source license
//
// Copyright (c) 2017 to 2022 David Forsgren Piuva
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

#define DFPSR_INTERNAL_ACCESS

#include <limits>
#include <cassert>
#include "imageAPI.h"
#include "drawAPI.h"
#include "fileAPI.h"
#include "../image/draw.h"
#include "../image/internal/imageInternal.h"
#include "../image/stbImage/stbImageWrapper.h"
#include "../math/scalar.h"

using namespace dsr;

// Constructors
AlignedImageU8 dsr::image_create_U8(int32_t width, int32_t height) {
	return AlignedImageU8(std::make_shared<ImageU8Impl>(width, height));
}
AlignedImageU16 dsr::image_create_U16(int32_t width, int32_t height) {
	return AlignedImageU16(std::make_shared<ImageU16Impl>(width, height));
}
AlignedImageF32 dsr::image_create_F32(int32_t width, int32_t height) {
	return AlignedImageF32(std::make_shared<ImageF32Impl>(width, height));
}
OrderedImageRgbaU8 dsr::image_create_RgbaU8(int32_t width, int32_t height) {
	return OrderedImageRgbaU8(std::make_shared<ImageRgbaU8Impl>(width, height));
}
AlignedImageRgbaU8 dsr::image_create_RgbaU8_native(int32_t width, int32_t height, PackOrderIndex packOrderIndex) {
	return AlignedImageRgbaU8(std::make_shared<ImageRgbaU8Impl>(width, height, packOrderIndex));
}

// Loading from data pointer
OrderedImageRgbaU8 dsr::image_decode_RgbaU8(const SafePointer<uint8_t> data, int size) {
	if (data.isNotNull()) {
		return image_stb_decode_RgbaU8(data, size);
	} else {
		return OrderedImageRgbaU8();
	}
}
// Loading from buffer
OrderedImageRgbaU8 dsr::image_decode_RgbaU8(const Buffer& fileContent) {
	return image_decode_RgbaU8(buffer_getSafeData<uint8_t>(fileContent, "image file buffer"), buffer_getSize(fileContent));
}
// Loading from file
OrderedImageRgbaU8 dsr::image_load_RgbaU8(const String& filename, bool mustExist) {
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

// Pre-condition: image exists.
// Post-condition: Returns true if the stride is larger than the image's width.
static bool imageIsPadded(const ImageRgbaU8 &image) {
	return image_getWidth(image) * 4 < image_getStride(image);
}

Buffer dsr::image_encode(const ImageRgbaU8 &image, ImageFileFormat format, int quality) {
	if (buffer_exists) {
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

static ImageFileFormat detectImageFileExtension(const String& filename) {
	ImageFileFormat result = ImageFileFormat::Unknown;
	int lastDotIndex = string_findLast(filename, U'.');
	if (lastDotIndex != -1) {
		ReadableString extension = string_upperCase(file_getExtension(filename));
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

bool dsr::image_save(const ImageRgbaU8 &image, const String& filename, bool mustWork, int quality) {
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

#define GET_OPTIONAL(SOURCE,DEFAULT) \
	if (image) { \
		return SOURCE; \
	} else { \
		return DEFAULT; \
	}

// Properties
int32_t dsr::image_getWidth(const ImageU8& image)     { GET_OPTIONAL(image->width, 0); }
int32_t dsr::image_getWidth(const ImageU16& image)    { GET_OPTIONAL(image->width, 0); }
int32_t dsr::image_getWidth(const ImageF32& image)    { GET_OPTIONAL(image->width, 0); }
int32_t dsr::image_getWidth(const ImageRgbaU8& image) { GET_OPTIONAL(image->width, 0); }

int32_t dsr::image_getHeight(const ImageU8& image)     { GET_OPTIONAL(image->height, 0); }
int32_t dsr::image_getHeight(const ImageU16& image)    { GET_OPTIONAL(image->height, 0); }
int32_t dsr::image_getHeight(const ImageF32& image)    { GET_OPTIONAL(image->height, 0); }
int32_t dsr::image_getHeight(const ImageRgbaU8& image) { GET_OPTIONAL(image->height, 0); }

int32_t dsr::image_getStride(const ImageU8& image)     { GET_OPTIONAL(image->stride, 0); }
int32_t dsr::image_getStride(const ImageU16& image)    { GET_OPTIONAL(image->stride, 0); }
int32_t dsr::image_getStride(const ImageF32& image)    { GET_OPTIONAL(image->stride, 0); }
int32_t dsr::image_getStride(const ImageRgbaU8& image) { GET_OPTIONAL(image->stride, 0); }

IRect dsr::image_getBound(const ImageU8& image)     { GET_OPTIONAL(IRect(0, 0, image->width, image->height), IRect()); }
IRect dsr::image_getBound(const ImageU16& image)    { GET_OPTIONAL(IRect(0, 0, image->width, image->height), IRect()); }
IRect dsr::image_getBound(const ImageF32& image)    { GET_OPTIONAL(IRect(0, 0, image->width, image->height), IRect()); }
IRect dsr::image_getBound(const ImageRgbaU8& image) { GET_OPTIONAL(IRect(0, 0, image->width, image->height), IRect()); }

bool dsr::image_exists(const ImageU8& image)     { GET_OPTIONAL(true, false); }
bool dsr::image_exists(const ImageU16& image)    { GET_OPTIONAL(true, false); }
bool dsr::image_exists(const ImageF32& image)    { GET_OPTIONAL(true, false); }
bool dsr::image_exists(const ImageRgbaU8& image) { GET_OPTIONAL(true, false); }

int dsr::image_useCount(const ImageU8& image)     { return image.use_count(); }
int dsr::image_useCount(const ImageU16& image)    { return image.use_count(); }
int dsr::image_useCount(const ImageF32& image)    { return image.use_count(); }
int dsr::image_useCount(const ImageRgbaU8& image) { return image.use_count(); }

PackOrderIndex dsr::image_getPackOrderIndex(const ImageRgbaU8& image) {
	GET_OPTIONAL(image->packOrder.packOrderIndex, PackOrderIndex::RGBA);
}

// Texture
void dsr::image_generatePyramid(ImageRgbaU8& image) {
	if (image) {
		image->generatePyramid();
	}
}
void dsr::image_removePyramid(ImageRgbaU8& image) {
	if (image) {
		image->removePyramid();
	}
}
bool dsr::image_hasPyramid(const ImageRgbaU8& image) {
	GET_OPTIONAL(image->texture.hasMipBuffer(), false);
}
bool dsr::image_isTexture(const ImageRgbaU8& image) {
	GET_OPTIONAL(image->isTexture(), false);
}

// Pixel access
#define INSIDE_XY (x >= 0 && x < image->width && y >= 0 && y < image->height)
#define CLAMP_XY \
	if (x < 0) { x = 0; } \
	if (y < 0) { y = 0; } \
	if (x >= image->width) { x = image->width - 1; } \
	if (y >= image->height) { y = image->height - 1; }
#define TILE_XY \
	x = signedModulo(x, image->width); \
	y = signedModulo(y, image->height);
void dsr::image_writePixel(ImageU8& image, int32_t x, int32_t y, int32_t color) {
	if (image) {
		if (INSIDE_XY) {
			if (color < 0) { color = 0; }
			if (color > 255) { color = 255; }
			ImageU8Impl::writePixel_unsafe(*image, x, y, color);
		}
	}
}
void dsr::image_writePixel(ImageU16& image, int32_t x, int32_t y, int32_t color) {
	if (image) {
		if (INSIDE_XY) {
			if (color < 0) { color = 0; }
			if (color > 65535) { color = 65535; }
			ImageU16Impl::writePixel_unsafe(*image, x, y, color);
		}
	}
}
void dsr::image_writePixel(ImageF32& image, int32_t x, int32_t y, float color) {
	if (image) {
		if (INSIDE_XY) {
			ImageF32Impl::writePixel_unsafe(*image, x, y, color);
		}
	}
}
void dsr::image_writePixel(ImageRgbaU8& image, int32_t x, int32_t y, const ColorRgbaI32& color) {
	if (image) {
		if (INSIDE_XY) {
			ImageRgbaU8Impl::writePixel_unsafe(*image, x, y, image->packRgba(color.saturate()));
		}
	}
}
int32_t dsr::image_readPixel_border(const ImageU8& image, int32_t x, int32_t y, int32_t border) {
	if (image) {
		if (INSIDE_XY) {
			return ImageU8Impl::readPixel_unsafe(*image, x, y);
		} else {
			return border;
		}
	} else {
		return 0;
	}
}
int32_t dsr::image_readPixel_border(const ImageU16& image, int32_t x, int32_t y, int32_t border) {
	if (image) {
		if (INSIDE_XY) {
			return ImageU16Impl::readPixel_unsafe(*image, x, y);
		} else {
			return border;
		}
	} else {
		return 0;
	}
}
float dsr::image_readPixel_border(const ImageF32& image, int32_t x, int32_t y, float border) {
	if (image) {
		if (INSIDE_XY) {
			return ImageF32Impl::readPixel_unsafe(*image, x, y);
		} else {
			return border;
		}
	} else {
		return 0.0f;
	}
}
ColorRgbaI32 dsr::image_readPixel_border(const ImageRgbaU8& image, int32_t x, int32_t y, const ColorRgbaI32& border) {
	if (image) {
		if (INSIDE_XY) {
			return image->unpackRgba(ImageRgbaU8Impl::readPixel_unsafe(*image, x, y));
		} else {
			return border; // Can return unsaturated colors as error codes
		}
	} else {
		return ColorRgbaI32();
	}
}
uint8_t dsr::image_readPixel_clamp(const ImageU8& image, int32_t x, int32_t y) {
	if (image) {
		CLAMP_XY;
		return ImageU8Impl::readPixel_unsafe(*image, x, y);
	} else {
		return 0;
	}
}
uint16_t dsr::image_readPixel_clamp(const ImageU16& image, int32_t x, int32_t y) {
	if (image) {
		CLAMP_XY;
		return ImageU16Impl::readPixel_unsafe(*image, x, y);
	} else {
		return 0;
	}
}
float dsr::image_readPixel_clamp(const ImageF32& image, int32_t x, int32_t y) {
	if (image) {
		CLAMP_XY;
		return ImageF32Impl::readPixel_unsafe(*image, x, y);
	} else {
		return 0.0f;
	}
}
ColorRgbaI32 dsr::image_readPixel_clamp(const ImageRgbaU8& image, int32_t x, int32_t y) {
	if (image) {
		CLAMP_XY;
		return image->unpackRgba(ImageRgbaU8Impl::readPixel_unsafe(*image, x, y));
	} else {
		return ColorRgbaI32();
	}
}
uint8_t dsr::image_readPixel_tile(const ImageU8& image, int32_t x, int32_t y) {
	if (image) {
		TILE_XY;
		return ImageU8Impl::readPixel_unsafe(*image, x, y);
	} else {
		return 0;
	}
}
uint16_t dsr::image_readPixel_tile(const ImageU16& image, int32_t x, int32_t y) {
	if (image) {
		TILE_XY;
		return ImageU16Impl::readPixel_unsafe(*image, x, y);
	} else {
		return 0;
	}
}
float dsr::image_readPixel_tile(const ImageF32& image, int32_t x, int32_t y) {
	if (image) {
		TILE_XY;
		return ImageF32Impl::readPixel_unsafe(*image, x, y);
	} else {
		return 0.0f;
	}
}
ColorRgbaI32 dsr::image_readPixel_tile(const ImageRgbaU8& image, int32_t x, int32_t y) {
	if (image) {
		TILE_XY;
		return image->unpackRgba(ImageRgbaU8Impl::readPixel_unsafe(*image, x, y));
	} else {
		return ColorRgbaI32();
	}
}

void dsr::image_fill(ImageU8& image, int32_t color) {
	if (image) {
		imageImpl_draw_solidRectangle(*image, imageInternal::getBound(*image), color);
	}
}
void dsr::image_fill(ImageU16& image, int32_t color) {
	if (image) {
		imageImpl_draw_solidRectangle(*image, imageInternal::getBound(*image), color);
	}
}
void dsr::image_fill(ImageF32& image, float color) {
	if (image) {
		imageImpl_draw_solidRectangle(*image, imageInternal::getBound(*image), color);
	}
}
void dsr::image_fill(ImageRgbaU8& image, const ColorRgbaI32& color) {
	if (image) {
		imageImpl_draw_solidRectangle(*image, imageInternal::getBound(*image), color);
	}
}

AlignedImageU8 dsr::image_clone(const ImageU8& image) {
	if (image) {
		AlignedImageU8 result = image_create_U8(image->width, image->height);
		draw_copy(result, image);
		return result;
	} else {
		return AlignedImageU8(); // Null gives null
	}
}
AlignedImageU16 dsr::image_clone(const ImageU16& image) {
	if (image) {
		AlignedImageU16 result = image_create_U16(image->width, image->height);
		draw_copy(result, image);
		return result;
	} else {
		return AlignedImageU16(); // Null gives null
	}
}
AlignedImageF32 dsr::image_clone(const ImageF32& image) {
	if (image) {
		AlignedImageF32 result = image_create_F32(image->width, image->height);
		draw_copy(result, image);
		return result;
	} else {
		return AlignedImageF32(); // Null gives null
	}
}
OrderedImageRgbaU8 dsr::image_clone(const ImageRgbaU8& image) {
	if (image) {
		OrderedImageRgbaU8 result = image_create_RgbaU8(image->width, image->height);
		draw_copy(result, image);
		return result;
	} else {
		return OrderedImageRgbaU8(); // Null gives null
	}
}
ImageRgbaU8 dsr::image_removePadding(const ImageRgbaU8& image) {
	if (image) {
		// TODO: Copy the implementation of getWithoutPadding, to create ImageRgbaU8 directly
		return ImageRgbaU8(image->getWithoutPadding());
	} else {
		return ImageRgbaU8(); // Null gives null
	}
}

AlignedImageU8 dsr::image_get_red(const ImageRgbaU8& image) {
	if (image) {
		// TODO: Copy the implementation of getChannel, to create ImageU8 directly
		return AlignedImageU8(image->getChannel(image->packOrder.redIndex));
	} else {
		return AlignedImageU8(); // Null gives null
	}
}
AlignedImageU8 dsr::image_get_green(const ImageRgbaU8& image) {
	if (image) {
		// TODO: Copy the implementation of getChannel, to create ImageU8 directly
		return AlignedImageU8(image->getChannel(image->packOrder.greenIndex));
	} else {
		return AlignedImageU8(); // Null gives null
	}
}
AlignedImageU8 dsr::image_get_blue(const ImageRgbaU8& image) {
	if (image) {
		// TODO: Copy the implementation of getChannel, to create ImageU8 directly
		return AlignedImageU8(image->getChannel(image->packOrder.blueIndex));
	} else {
		return AlignedImageU8(); // Null gives null
	}
}
AlignedImageU8 dsr::image_get_alpha(const ImageRgbaU8& image) {
	if (image) {
		// TODO: Copy the implementation of getChannel, to create ImageU8 directly
		return AlignedImageU8(image->getChannel(image->packOrder.alphaIndex));
	} else {
		return AlignedImageU8(); // Null gives null
	}
}

static inline int32_t readColor(const ImageU8& channel, int x, int y) {
	return ImageU8Impl::readPixel_unsafe(*channel, x, y);
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
if (FIRST) { \
	return pack_template(FIRST->width, FIRST->height, red, green, blue, alpha); \
} else { \
	return OrderedImageRgbaU8(); \
}
OrderedImageRgbaU8 dsr::image_pack(const ImageU8& red, int32_t green, int32_t blue, int32_t alpha) { PACK1(red); }
OrderedImageRgbaU8 dsr::image_pack(int32_t red, const ImageU8& green, int32_t blue, int32_t alpha) { PACK1(green); }
OrderedImageRgbaU8 dsr::image_pack(int32_t red, int32_t green, const ImageU8& blue, int32_t alpha) { PACK1(blue); }
OrderedImageRgbaU8 dsr::image_pack(int32_t red, int32_t green, int32_t blue, const ImageU8& alpha) { PACK1(alpha); }

#define PACK2(FIRST,SECOND) \
if (FIRST && SECOND) { \
	if (FIRST->width != SECOND->width || FIRST->height != SECOND->height) { \
		throwError("Cannot pack two channels of different size!\n"); \
	} \
	return pack_template(FIRST->width, FIRST->height, red, green, blue, alpha); \
} else { \
	return OrderedImageRgbaU8(); \
}
OrderedImageRgbaU8 dsr::image_pack(const ImageU8& red, const ImageU8& green, int32_t blue, int32_t alpha) { PACK2(red,green) }
OrderedImageRgbaU8 dsr::image_pack(const ImageU8& red, int32_t green, const ImageU8& blue, int32_t alpha) { PACK2(red,blue) }
OrderedImageRgbaU8 dsr::image_pack(const ImageU8& red, int32_t green, int32_t blue, const ImageU8& alpha) { PACK2(red,alpha) }
OrderedImageRgbaU8 dsr::image_pack(int32_t red, const ImageU8& green, const ImageU8& blue, int32_t alpha) { PACK2(green,blue) }
OrderedImageRgbaU8 dsr::image_pack(int32_t red, const ImageU8& green, int32_t blue, const ImageU8& alpha) { PACK2(green,alpha) }
OrderedImageRgbaU8 dsr::image_pack(int32_t red, int32_t green, const ImageU8& blue, const ImageU8& alpha) { PACK2(blue,alpha) }

#define PACK3(FIRST,SECOND,THIRD) \
if (FIRST && SECOND && THIRD) { \
	if (FIRST->width != SECOND->width || FIRST->height != SECOND->height \
	 || FIRST->width != THIRD->width || FIRST->height != THIRD->height) { \
		throwError("Cannot pack three channels of different size!\n"); \
	} \
	return pack_template(FIRST->width, FIRST->height, red, green, blue, alpha); \
} else { \
	return OrderedImageRgbaU8(); \
}
OrderedImageRgbaU8 dsr::image_pack(int32_t red, const ImageU8& green, const ImageU8& blue, const ImageU8& alpha) { PACK3(green, blue, alpha) }
OrderedImageRgbaU8 dsr::image_pack(const ImageU8& red, int32_t green, const ImageU8& blue, const ImageU8& alpha) { PACK3(red, blue, alpha) }
OrderedImageRgbaU8 dsr::image_pack(const ImageU8& red, const ImageU8& green, int32_t blue, const ImageU8& alpha) { PACK3(red, green, alpha) }
OrderedImageRgbaU8 dsr::image_pack(const ImageU8& red, const ImageU8& green, const ImageU8& blue, int32_t alpha) { PACK3(red, green, blue) }

// TODO: Optimize using zip instructions
#define PACK4(FIRST,SECOND,THIRD,FOURTH) \
if (FIRST && SECOND && THIRD && FOURTH) { \
	if (FIRST->width != SECOND->width || FIRST->height != SECOND->height \
	 || FIRST->width != THIRD->width || FIRST->height != THIRD->height \
 	 || FIRST->width != FOURTH->width || FIRST->height != FOURTH->height) { \
		throwError("Cannot pack four channels of different size!\n"); \
	} \
	return pack_template(FIRST->width, FIRST->height, red, green, blue, alpha); \
} else { \
	return OrderedImageRgbaU8(); \
}
OrderedImageRgbaU8 dsr::image_pack(const ImageU8& red, const ImageU8& green, const ImageU8& blue, const ImageU8& alpha) { PACK4(red, green, blue, alpha) }

// Convert a grayscale image into an ascii image using the given alphabet.
//   Since all 256 characters cannot be in the alphabet, the encoding is lossy.
// Each line is stored within <> to prevent text editors from removing meaningful white space.
// The first line contains the given alphabet as a gradient from black to white.
// Preconditions:
//   alphabet may not have extended ascii, non printable, '\', '"', '>' or linebreak
//   width <= stride
//   size of monochromeImage = height * stride
// Example alphabet: " .,-_':;!+~=^?*abcdefghijklmnopqrstuvwxyz()[]{}|&@#0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
String dsr::image_toAscii(const ImageU8& image, const String& alphabet) {
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

String dsr::image_toAscii(const ImageU8& image) {
	return image_toAscii(image, U" .,-_':;!+~=^?*abcdefghijklmnopqrstuvwxyz()[]{}|&@#0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

// Create a monochrome image from the ascii image in content.
// String is used instead of ReadableString, so that the content can be decompressed from 8-bit strings in the binary.
AlignedImageU8 dsr::image_fromAscii(const String& content) {
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

// TODO: Try to recycle the memory to reduce overhead from heap allocating heads pointing to existing buffers
template <typename IMAGE_TYPE, typename VALUE_TYPE>
static inline IMAGE_TYPE subImage_template(const IMAGE_TYPE& image, const IRect& region) {
	if (image) {
		IRect cut = IRect::cut(imageInternal::getBound(*image), region);
		if (cut.hasArea()) {
			intptr_t newOffset = image->startOffset + (cut.left() * image->pixelSize) + (cut.top() * image->stride);
			return IMAGE_TYPE(std::make_shared<VALUE_TYPE>(cut.width(), cut.height(), image->stride, image->buffer, newOffset));
		}
	}
	return IMAGE_TYPE(); // Null if where are no overlapping pixels
}

template <typename IMAGE_TYPE, typename VALUE_TYPE>
static inline IMAGE_TYPE subImage_template_withPackOrder(const IMAGE_TYPE& image, const IRect& region) {
	if (image) {
		IRect cut = IRect::cut(imageInternal::getBound(*image), region);
		if (cut.hasArea()) {
			intptr_t newOffset = image->startOffset + (cut.left() * image->pixelSize) + (cut.top() * image->stride);
			return IMAGE_TYPE(std::make_shared<VALUE_TYPE>(cut.width(), cut.height(), image->stride, image->buffer, newOffset, image->packOrder));
		}
	}
	return IMAGE_TYPE(); // Null if where are no overlapping pixels
}

ImageU8 dsr::image_getSubImage(const ImageU8& image, const IRect& region) {
	return subImage_template<ImageU8, ImageU8Impl>(image, region);
}

ImageU16 dsr::image_getSubImage(const ImageU16& image, const IRect& region) {
	return subImage_template<ImageU16, ImageU16Impl>(image, region);
}

ImageF32 dsr::image_getSubImage(const ImageF32& image, const IRect& region) {
	return subImage_template<ImageF32, ImageF32Impl>(image, region);
}

ImageRgbaU8 dsr::image_getSubImage(const ImageRgbaU8& image, const IRect& region) {
	return subImage_template_withPackOrder<ImageRgbaU8, ImageRgbaU8Impl>(image, region);
}

template <typename IMAGE_TYPE, int CHANNELS, typename ELEMENT_TYPE>
ELEMENT_TYPE maxDifference_template(const IMAGE_TYPE& imageA, const IMAGE_TYPE& imageB) {
	if (imageA.width != imageB.width || imageA.height != imageB.height) {
		return std::numeric_limits<ELEMENT_TYPE>::max();
	} else {
		ELEMENT_TYPE maxDifference = 0;
		const SafePointer<ELEMENT_TYPE> rowDataA = imageInternal::getSafeData<ELEMENT_TYPE>(imageA);
		const SafePointer<ELEMENT_TYPE> rowDataB = imageInternal::getSafeData<ELEMENT_TYPE>(imageB);
		for (int y = 0; y < imageA.height; y++) {
			const SafePointer<ELEMENT_TYPE> pixelDataA = rowDataA;
			const SafePointer<ELEMENT_TYPE> pixelDataB = rowDataB;
			for (int x = 0; x < imageA.width; x++) {
				for (int c = 0; c < CHANNELS; c++) {
					ELEMENT_TYPE difference = absDiff(*pixelDataA, *pixelDataB);
					if (difference > maxDifference) {
						maxDifference = difference;
					}
					pixelDataA += 1;
					pixelDataB += 1;
				}
			}
			rowDataA.increaseBytes(imageA.stride);
			rowDataB.increaseBytes(imageB.stride);
		}
		return maxDifference;
	}
}
uint8_t dsr::image_maxDifference(const ImageU8& imageA, const ImageU8& imageB) {
	if (imageA && imageB) {
		return maxDifference_template<ImageU8Impl, 1, uint8_t>(*imageA, *imageB);
	} else {
		return std::numeric_limits<uint8_t>::infinity();
	}
}
uint16_t dsr::image_maxDifference(const ImageU16& imageA, const ImageU16& imageB) {
	if (imageA && imageB) {
		return maxDifference_template<ImageU16Impl, 1, uint16_t>(*imageA, *imageB);
	} else {
		return std::numeric_limits<uint16_t>::infinity();
	}
}
float dsr::image_maxDifference(const ImageF32& imageA, const ImageF32& imageB) {
	if (imageA && imageB) {
		return maxDifference_template<ImageF32Impl, 1, float>(*imageA, *imageB);
	} else {
		return std::numeric_limits<float>::infinity();
	}
}
uint8_t dsr::image_maxDifference(const ImageRgbaU8& imageA, const ImageRgbaU8& imageB) {
	if (imageA && imageB) {
		return maxDifference_template<ImageRgbaU8Impl, 4, uint8_t>(*imageA, *imageB);
	} else {
		return std::numeric_limits<uint8_t>::infinity();
	}
}

SafePointer<uint8_t> dsr::image_getSafePointer(const ImageU8& image, int rowIndex) {
	if (image) {
		return imageInternal::getSafeData<uint8_t>(image.get(), rowIndex);
	} else {
		return SafePointer<uint8_t>();
	}
}
SafePointer<uint16_t> dsr::image_getSafePointer(const ImageU16& image, int rowIndex) {
	if (image) {
		return imageInternal::getSafeData<uint16_t>(image.get(), rowIndex);
	} else {
		return SafePointer<uint16_t>();
	}
}
SafePointer<float> dsr::image_getSafePointer(const ImageF32& image, int rowIndex) {
	if (image) {
		return imageInternal::getSafeData<float>(image.get(), rowIndex);
	} else {
		return SafePointer<float>();
	}
}
SafePointer<uint32_t> dsr::image_getSafePointer(const ImageRgbaU8& image, int rowIndex) {
	if (image) {
		return imageInternal::getSafeData<uint32_t>(image.get(), rowIndex);
	} else {
		return SafePointer<uint32_t>();
	}
}
SafePointer<uint8_t> dsr::image_getSafePointer_channels(const ImageRgbaU8& image, int rowIndex) {
	if (image) {
		return imageInternal::getSafeData<uint8_t>(image.get(), rowIndex);
	} else {
		return SafePointer<uint8_t>();
	}
}

void dsr::image_dangerous_replaceDestructor(ImageU8& image, const std::function<void(uint8_t *)>& newDestructor) {
	if (image) { return buffer_replaceDestructor(image->buffer, newDestructor); }
}
void dsr::image_dangerous_replaceDestructor(ImageU16& image, const std::function<void(uint8_t *)>& newDestructor) {
	if (image) { return buffer_replaceDestructor(image->buffer, newDestructor); }
}
void dsr::image_dangerous_replaceDestructor(ImageF32& image, const std::function<void(uint8_t *)>& newDestructor) {
	if (image) { return buffer_replaceDestructor(image->buffer, newDestructor); }
}
void dsr::image_dangerous_replaceDestructor(ImageRgbaU8& image, const std::function<void(uint8_t *)>& newDestructor) {
	if (image) { return buffer_replaceDestructor(image->buffer, newDestructor); }
}

uint8_t* dsr::image_dangerous_getData(const ImageU8& image) {
	if (image) {
		return imageInternal::getSafeData<uint8_t>(*image).getUnsafe();
	} else {
		return nullptr;
	}
}
uint8_t* dsr::image_dangerous_getData(const ImageU16& image) {
	if (image) {
		return imageInternal::getSafeData<uint8_t>(*image).getUnsafe();
	} else {
		return nullptr;
	}
}
uint8_t* dsr::image_dangerous_getData(const ImageF32& image) {
	if (image) {
		return imageInternal::getSafeData<uint8_t>(*image).getUnsafe();
	} else {
		return nullptr;
	}
}
uint8_t* dsr::image_dangerous_getData(const ImageRgbaU8& image) {
	if (image) {
		return imageInternal::getSafeData<uint8_t>(*image).getUnsafe();
	} else {
		return nullptr;
	}
}
