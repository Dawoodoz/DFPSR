
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

#ifndef DFPSR_API_IMAGE
#define DFPSR_API_IMAGE

#include "types.h"
#include "../base/SafePointer.h"

namespace dsr {

// Constructors
	// Each row's start and stride is aligned to 16-bytes using padding at the end
	//   This allow using in-place writing with aligned 16-byte SIMD vectors
	AlignedImageU8 image_create_U8(int32_t width, int32_t height);
	AlignedImageU16 image_create_U16(int32_t width, int32_t height);
	AlignedImageF32 image_create_F32(int32_t width, int32_t height);
	OrderedImageRgbaU8 image_create_RgbaU8(int32_t width, int32_t height);
	AlignedImageRgbaU8 image_create_RgbaU8_native(int32_t width, int32_t height, PackOrderIndex packOrderIndex);

// Properties
	// Returns image's width in pixels or 0 on null image
	int32_t image_getWidth(const ImageU8& image);
	int32_t image_getWidth(const ImageU16& image);
	int32_t image_getWidth(const ImageF32& image);
	int32_t image_getWidth(const ImageRgbaU8& image);
	// Returns image's height in pixels or 0 on null image
	int32_t image_getHeight(const ImageU8& image);
	int32_t image_getHeight(const ImageU16& image);
	int32_t image_getHeight(const ImageF32& image);
	int32_t image_getHeight(const ImageRgbaU8& image);
	// Returns image's stride in bytes or 0 on null image
	//   Stride is the offset from the beginning of one row to another
	//   May be larger than width times pixel size
	//     * If padding is used to align with 16-bytes
	//     * Or the buffer is shared with a larger image
	int32_t image_getStride(const ImageU8& image);
	int32_t image_getStride(const ImageU16& image);
	int32_t image_getStride(const ImageF32& image);
	int32_t image_getStride(const ImageRgbaU8& image);
	// Get a rectangle from the image's dimensions with the top left corner set to (0, 0)
	//   Useful for clipping to an image's bounds or subdividing space for a graphical user interface
	IRect image_getBound(const ImageU8& image);
	IRect image_getBound(const ImageU16& image);
	IRect image_getBound(const ImageF32& image);
	IRect image_getBound(const ImageRgbaU8& image);
	// Returns false on null, true otherwise
	bool image_exists(const ImageU8& image);
	bool image_exists(const ImageU16& image);
	bool image_exists(const ImageF32& image);
	bool image_exists(const ImageRgbaU8& image);
	// Returns the number of handles to the image
	//   References to a handle doesn't count, only when a handle is stored by value
	int image_useCount(const ImageU8& image);
	int image_useCount(const ImageU16& image);
	int image_useCount(const ImageF32& image);
	int image_useCount(const ImageRgbaU8& image);
	// Returns the image's pack order index
	PackOrderIndex image_getPackOrderIndex(const ImageRgbaU8& image);

// Texture
	// TODO: A method for removing the pyramid
	void image_generatePyramid(ImageRgbaU8& image);
	bool image_hasPyramid(const ImageRgbaU8& image);
	bool image_isTexture(const ImageRgbaU8& image);

// Pixel access
	// Write a pixel to an image.
	//   Out of bound is ignored silently without writing.
	//   Empty images will be ignored safely.
	//   Packed is faster if the color can be packed in advance for multiple pixels or comes directly from an image of the same rgba order.
	void image_writePixel(ImageU8& image, int32_t x, int32_t y, int32_t color); // Saturated to 0..255
	void image_writePixel(ImageU16& image, int32_t x, int32_t y, int32_t color); // Saturated to 0..65535
	void image_writePixel(ImageF32& image, int32_t x, int32_t y, float color);
	void image_writePixel(ImageRgbaU8& image, int32_t x, int32_t y, const ColorRgbaI32& color); // Saturated to 0..255
	// Read a pixel from an image.
	//   Out of bound will return the border color.
	//   Empty images will return zero.
	int32_t image_readPixel_border(const ImageU8& image, int32_t x, int32_t y, int32_t border = 0); // Can have negative value as border
	int32_t image_readPixel_border(const ImageU16& image, int32_t x, int32_t y, int32_t border = 0); // Can have negative value as border
	float image_readPixel_border(const ImageF32& image, int32_t x, int32_t y, float border = 0.0f);
	ColorRgbaI32 image_readPixel_border(const ImageRgbaU8& image, int32_t x, int32_t y, const ColorRgbaI32& border = ColorRgbaI32()); // Can have negative value as border
	// Read a pixel from an image.
	//   Out of bound will return the closest pixel.
	//   Empty images will return zero.
	uint8_t image_readPixel_clamp(const ImageU8& image, int32_t x, int32_t y);
	uint16_t image_readPixel_clamp(const ImageU16& image, int32_t x, int32_t y);
	float image_readPixel_clamp(const ImageF32& image, int32_t x, int32_t y);
	ColorRgbaI32 image_readPixel_clamp(const ImageRgbaU8& image, int32_t x, int32_t y);
	// Read a pixel from an image.
	//   Out of bound will take the coordinates in modulo of the size.
	//   Empty images will return zero.
	uint8_t image_readPixel_tile(const ImageU8& image, int32_t x, int32_t y);
	uint16_t image_readPixel_tile(const ImageU16& image, int32_t x, int32_t y);
	float image_readPixel_tile(const ImageF32& image, int32_t x, int32_t y);
	ColorRgbaI32 image_readPixel_tile(const ImageRgbaU8& image, int32_t x, int32_t y);



// ------------------------ Below is untested! ------------------------ //



// Loading and saving
	OrderedImageRgbaU8 image_load_RgbaU8(const String& filename, bool mustExist = true);
	bool image_save(const ImageRgbaU8 &image, const String& filename);

// Fill all pixels with a uniform color
	void image_fill(ImageU8& image, int32_t color);
	void image_fill(ImageU16& image, int32_t color);
	void image_fill(ImageF32& image, float color);
	void image_fill(ImageRgbaU8& image, const ColorRgbaI32& color);

// Clone
	// Get a deep clone of an image's content while discarding any pack order, padding and texture pyramids
	AlignedImageU8 image_clone(const ImageU8& image);
	AlignedImageU16 image_clone(const ImageU16& image);
	AlignedImageF32 image_clone(const ImageF32& image);
	OrderedImageRgbaU8 image_clone(const ImageRgbaU8& image);
	// Returns a copy of the image without any padding, which means that alignment cannot be guaranteed
	//   Used when external image libraries don't allow it
	ImageRgbaU8 image_removePadding(const ImageRgbaU8& image);

// Channel packing
	// Extract one channel
	AlignedImageU8 image_get_red(const ImageRgbaU8& image);
	AlignedImageU8 image_get_green(const ImageRgbaU8& image);
	AlignedImageU8 image_get_blue(const ImageRgbaU8& image);
	AlignedImageU8 image_get_alpha(const ImageRgbaU8& image);
	// Pack one channel
	OrderedImageRgbaU8 image_pack(const ImageU8& red, int32_t green, int32_t blue, int32_t alpha);
	OrderedImageRgbaU8 image_pack(int32_t red, const ImageU8& green, int32_t blue, int32_t alpha);
	OrderedImageRgbaU8 image_pack(int32_t red, int32_t green, const ImageU8& blue, int32_t alpha);
	OrderedImageRgbaU8 image_pack(int32_t red, int32_t green, int32_t blue, const ImageU8& alpha);
	// Pack two channels
	OrderedImageRgbaU8 image_pack(const ImageU8& red, const ImageU8& green, int32_t blue, int32_t alpha);
	OrderedImageRgbaU8 image_pack(const ImageU8& red, int32_t green, const ImageU8& blue, int32_t alpha);
	OrderedImageRgbaU8 image_pack(const ImageU8& red, int32_t green, int32_t blue, const ImageU8& alpha);
	OrderedImageRgbaU8 image_pack(int32_t red, const ImageU8& green, const ImageU8& blue, int32_t alpha);
	OrderedImageRgbaU8 image_pack(int32_t red, const ImageU8& green, int32_t blue, const ImageU8& alpha);
	OrderedImageRgbaU8 image_pack(int32_t red, int32_t green, const ImageU8& blue, const ImageU8& alpha);
	// Pack three channels
	OrderedImageRgbaU8 image_pack(int32_t red, const ImageU8& green, const ImageU8& blue, const ImageU8& alpha);
	OrderedImageRgbaU8 image_pack(const ImageU8& red, int32_t green, const ImageU8& blue, const ImageU8& alpha);
	OrderedImageRgbaU8 image_pack(const ImageU8& red, const ImageU8& green, int32_t blue, const ImageU8& alpha);
	OrderedImageRgbaU8 image_pack(const ImageU8& red, const ImageU8& green, const ImageU8& blue, int32_t alpha);
	// Pack four channels
	OrderedImageRgbaU8 image_pack(const ImageU8& red, const ImageU8& green, const ImageU8& blue, const ImageU8& alpha);

// Ascii images
	String image_toAscii(const ImageU8& image, const String &alphabet);
	String image_toAscii(const ImageU8& image);
	AlignedImageU8 image_fromAscii(const String &content);

// Comparisons
	// Get the maximum pixelwise difference between two images of the same format, or the highest possible value on failure
	//   Useful for regression tests
	uint8_t image_maxDifference(const ImageU8& imageA, const ImageU8& imageB);
	uint16_t image_maxDifference(const ImageU16& imageA, const ImageU16& imageB);
	float image_maxDifference(const ImageF32& imageA, const ImageF32& imageB);
	uint8_t image_maxDifference(const ImageRgbaU8& imageA, const ImageRgbaU8& imageB);

// Sub-images are viewports to another image's data
// TODO: Aligned sub-images that only takes vertial sections using whole rows
// TODO: Aligned sub-images that terminates with an error if the input rectangle isn't aligned
//       Start must be 16-byte aligned, end must be same as the parent or also 16-byte aligned
// TODO: Make an optional warning for not returning the desired dimensions when out of bound
	// Get a sub-image sharing buffer and side-effects with the parent image
	// Returns the overlapping region if out of bound
	// Returns a null image if there are no overlapping pixels to return
	ImageU8 image_getSubImage(const ImageU8& image, const IRect& region);
	ImageU16 image_getSubImage(const ImageU16& image, const IRect& region);
	ImageF32 image_getSubImage(const ImageF32& image, const IRect& region);
	ImageRgbaU8 image_getSubImage(const ImageRgbaU8& image, const IRect& region);

// Bound-checked pointer access (relatively safe compared to a raw pointer)
	// Returns a bound-checked pointer to the first byte at rowIndex
	// Bound-checked safe-pointers are equally fast as raw pointers in release mode
	// Warning! Bound-checked pointers are not reference counted, because that would be too slow for real-time graphics
	SafePointer<uint8_t> image_getSafePointer(const ImageU8& image, int rowIndex = 0);
	SafePointer<uint16_t> image_getSafePointer(const ImageU16& image, int rowIndex = 0);
	SafePointer<float> image_getSafePointer(const ImageF32& image, int rowIndex = 0);
	SafePointer<uint32_t> image_getSafePointer(const ImageRgbaU8& image, int rowIndex = 0);
	// Get a pointer iterating over individual channels instead of whole pixels
	SafePointer<uint8_t> image_getSafePointer_channels(const ImageRgbaU8& image, int rowIndex = 0);

// The dangerous image API
// Use of these methods can be spotted using a search for "_dangerous_" in your code
	// Replaces the destructor in image's buffer.
	//   newDestructor is responsible for freeing the given data.
	//   Use when the buffer's pointer is being sent to a function that promises to free the memory
	//   For example: Creating buffers being wrapped as XLib images
	void image_dangerous_replaceDestructor(ImageU8& image, const std::function<void(uint8_t *)>& newDestructor);
	void image_dangerous_replaceDestructor(ImageU16& image, const std::function<void(uint8_t *)>& newDestructor);
	void image_dangerous_replaceDestructor(ImageF32& image, const std::function<void(uint8_t *)>& newDestructor);
	void image_dangerous_replaceDestructor(ImageRgbaU8& image, const std::function<void(uint8_t *)>& newDestructor);
	// Returns a pointer to the image's pixels
	// Warning! Reading elements larger than 8 bits will have lower and higher bytes stored based on local endianness
	// Warning! Using bytes outside of the [0 .. stride * height - 1] range may cause crashes and undefined behaviour
	// Warning! Using the pointer after the image's lifetime may cause crashes from trying to access freed memory
	uint8_t* image_dangerous_getData(const ImageU8& image);
	uint8_t* image_dangerous_getData(const ImageU16& image);
	uint8_t* image_dangerous_getData(const ImageF32& image);
	uint8_t* image_dangerous_getData(const ImageRgbaU8& image);
}

#endif
