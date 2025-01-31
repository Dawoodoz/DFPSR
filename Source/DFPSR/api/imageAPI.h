
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

// Everything stored directly in the image types is immutable to allow value types to behave like reference types using the data that they point to.
// Image types can not be dynamically casted, because the inheritance is entirely static without any virtual functions.

#ifndef DFPSR_API_IMAGE
#define DFPSR_API_IMAGE

#include "../implementation/image/Image.h"
#include "../implementation/image/Color.h"
#include "../implementation/math/scalar.h"
#include "../base/heap.h"

namespace dsr {

// Constructors
	// Pre-conditions:
	//   1 <= width <= 65536
	//   1 <= height <= 65536
	// Post-condition:
	//   Returns a new image of width x height pixels, or an empty image on failure.
	AlignedImageU8 image_create_U8(int32_t width, int32_t height);
	AlignedImageU16 image_create_U16(int32_t width, int32_t height);
	AlignedImageF32 image_create_F32(int32_t width, int32_t height);
	OrderedImageRgbaU8 image_create_RgbaU8(int32_t width, int32_t height);
	AlignedImageRgbaU8 image_create_RgbaU8_native(int32_t width, int32_t height, PackOrderIndex packOrderIndex);

// Properties
	// Returns image's width in pixels, or 0 from an empty image
	inline int32_t image_getWidth(const Image& image) { return image.impl_dimensions.getWidth(); }
	// Returns image's height in pixels, or 0 from an empty image
	inline int32_t image_getHeight(const Image& image) { return image.impl_dimensions.getHeight(); }

	// Stride is the offset from the beginning of one row to another.
	//   May be larger than the image's width to align with cache lines or share pixel data with a wider image.
	// When you add a variable offset to a pointer in C++, the added offset is multiplied by the element size because the address is always stored in bytes.
	//   Because all pixels have a power of two size, the multiplication can be optimized into a bit shift.
	//   On ARM, adding whole elements to a pointer is just as fast as adding bytes, by shifting and adding in the same instruction.
	//   On Intel/AMD, shifting and adding needs two instructions, so then it makes sense to pre-calculate the stride as bytes and cast to uint8_t* when adding.

	// Returns image's stride in whole pixels, or 0 from an empty image
	//   Used when incrementing indices instead of pointers.
	inline int32_t image_getPixelStride(const Image& image) { return (intptr_t)image.impl_dimensions.getPixelStride(); }
	// Returns image's stride in bytes, or 0 from an empty image.
	inline int32_t image_getStride(const Image&       image) { return (image_getPixelStride(image) << (uintptr_t)image.impl_dimensions.getLog2PixelSize()); }
	inline int32_t image_getStride(const ImageU8&     image) { return image_getPixelStride(image);      } // pixelStride * sizeof(uint8_t )
	inline int32_t image_getStride(const ImageU16&    image) { return image_getPixelStride(image) << 1; } // pixelStride * sizeof(uint16_t)
	inline int32_t image_getStride(const ImageF32&    image) { return image_getPixelStride(image) << 2; } // pixelStride * sizeof(float   )
	inline int32_t image_getStride(const ImageRgbaU8& image) { return image_getPixelStride(image) << 2; } // pixelStride * sizeof(uint32_t)
	// Returns image's offset from the allocation start in whole pixels, or 0 from an empty image
	inline int64_t image_getPixelStartOffset(const Image& image) { return (int64_t)image.impl_dimensions.getPixelStartOffset(); }
	// Returns image's offset from the allocation start in bytes, or 0 from an empty image
	inline int64_t image_getStartOffset(const ImageU8&     image) { return image_getPixelStartOffset(image);      } // pixelStartOffset * sizeof(uint8_t )
	inline int64_t image_getStartOffset(const ImageU16&    image) { return image_getPixelStartOffset(image) << 1; } // pixelStartOffset * sizeof(uint16_t)
	inline int64_t image_getStartOffset(const ImageF32&    image) { return image_getPixelStartOffset(image) << 2; } // pixelStartOffset * sizeof(float   )
	inline int64_t image_getStartOffset(const ImageRgbaU8& image) { return image_getPixelStartOffset(image) << 2; } // pixelStartOffset * sizeof(uint32_t)

	// Get a rectangle from the image's dimensions with the top left corner set to (0, 0).
	//   Useful for clipping to an image's bounds or subdividing space for a graphical user interface.
	//   Returns IRect(0, 0, 0, 0) for empty images.
	inline IRect image_getBound(const Image& image) { return IRect(0, 0, image.impl_dimensions.getWidth(), image.impl_dimensions.getHeight()); }

	// Returns false on null, true otherwise.
	inline bool image_exists(const Image& image) { return image.impl_buffer.isNotNull(); }

	// TODO: Rename into image_getUseCount for easier use.
	// Returns the number of handles to the image.
	//   References to a handle doesn't count, only when a handle is stored by value.
	inline uintptr_t image_useCount(const Image& image) { return image.impl_buffer.getUseCount(); }

	// Returns the image's pack order index.
	inline PackOrderIndex image_getPackOrderIndex(const ImageRgbaU8& image) { return image.impl_dimensions.getPackOrderIndex(); }
	// Returns the image's pack order, containing bit masks and offsets needed to pack and unpack colors.
	inline PackOrder image_getPackOrder(const ImageRgbaU8& image) { return PackOrder::getPackOrder(image.impl_dimensions.getPackOrderIndex()); };

	// Returns true iff the pixel at (x, y) is inside of image.
	inline bool image_isPixelInside(const Image& image, int32_t x, int32_t y) {
		return x >= 0 && x < image_getWidth(image) && y >= 0 && y < image_getHeight(image);
	}

	// Returns the size of one pixel in bytes dynamically by looking it up.
	inline int32_t image_getPixelSize(const Image& image) { return image.impl_dimensions.getPixelSize(); }
	// Returns the size of one pixel in bytes statically from the type.
	template <typename T> int32_t image_getPixelSize() { return T::impl_pixelSize; }

// Channel packing
	// Extract one channel
	AlignedImageU8 image_get_red  (const ImageRgbaU8& image);
	AlignedImageU8 image_get_green(const ImageRgbaU8& image);
	AlignedImageU8 image_get_blue (const ImageRgbaU8& image);
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

	// Pack a color to draw with using the image's pack order, as it would be represented as a pixel in the buffer.
	inline uint32_t image_pack(const ImageRgbaU8& image, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
		return PackOrder::getPackOrder(image.impl_dimensions.getPackOrderIndex()).packRgba(red, green, blue, alpha);
	}
	// Saturate and pack a color for an image's pack order, as it would be represented as a pixel in the buffer.
	inline uint32_t image_saturateAndPack(const ImageRgbaU8& image, const ColorRgbaI32& color) {
		return PackOrder::getPackOrder(image.impl_dimensions.getPackOrderIndex()).saturateAndPackRgba(color);
	}
	// Truncate and pack a color for an image's pack order, as it would be represented as a pixel in the buffer.
	inline uint32_t image_truncateAndPack(const ImageRgbaU8& image, const ColorRgbaI32& color) {
		return PackOrder::getPackOrder(image.impl_dimensions.getPackOrderIndex()).truncateAndPackRgba(color);
	}
	// Unpack a color back into an expanded and ordered RGBA format.
	//  packedColor is expressed in image's pack order.
	inline ColorRgbaI32 image_unpack(const ImageRgbaU8& image, uint32_t packedColor) {
		return PackOrder::getPackOrder(image.impl_dimensions.getPackOrderIndex()).unpackRgba(packedColor);
	}

// Pixel access
	// Pre-condition:
	//   The pixel at (x, y) must exist within the image, or else the program may crash.
	//   image_isPixelInsize(image, x, y)
	// Post-condition:
	//   Returns a reference to the pixel at (x, y) in image.
	inline uint8_t &image_accessPixel(const ImageU8& image, int32_t x, int32_t y) {
		uintptr_t pixelOffset = image_getPixelStartOffset(image) + y * image_getPixelStride(image) + x;
		return *(buffer_getSafeData<uint8_t>(image.impl_buffer, "ImageU8 pixel access buffer") + pixelOffset);
	}
	inline uint16_t &image_accessPixel(const ImageU16& image, int32_t x, int32_t y) {
		uintptr_t pixelOffset = image_getPixelStartOffset(image) + y * image_getPixelStride(image) + x;
		return *(buffer_getSafeData<uint16_t>(image.impl_buffer, "ImageU16 pixel access buffer") + pixelOffset);
	}
	inline float &image_accessPixel(const ImageF32& image, int32_t x, int32_t y) {
		uintptr_t pixelOffset = image_getPixelStartOffset(image) + y * image_getPixelStride(image) + x;
		return *(buffer_getSafeData<float>(image.impl_buffer, "ImageF32 pixel access buffer") + pixelOffset);
	}
	inline uint32_t &image_accessPixel(const ImageRgbaU8& image, int32_t x, int32_t y) {
		uintptr_t pixelOffset = image_getPixelStartOffset(image) + y * image_getPixelStride(image) + x;
		return *(buffer_getSafeData<uint32_t>(image.impl_buffer, "ImageRgbaU8 pixel access buffer") + pixelOffset);
	}

	// Write a pixel to an image.
	//   Out of bound is ignored silently without writing.
	//   Empty images will be ignored safely.
	//   Packed is faster if the color can be packed in advance for multiple pixels or comes directly from an image of the same rgba order.
	// Saturated to 0..255
	inline void image_writePixel(const ImageU8& image, int32_t x, int32_t y, int32_t color) {
		if (image_isPixelInside(image, x, y)) image_accessPixel(image, x, y) = clamp(0, color, 255);
	}
	// Saturated to 0..65535
	inline void image_writePixel(const ImageU16& image, int32_t x, int32_t y, int32_t color) {
		if (image_isPixelInside(image, x, y)) image_accessPixel(image, x, y) = clamp(0, color, 65535);
	}
	// No saturation needed
	inline void image_writePixel(const ImageF32& image, int32_t x, int32_t y, float color) {
		if (image_isPixelInside(image, x, y)) image_accessPixel(image, x, y) = color;
	}
	// Saturated to 0..255 in all channels
	inline void image_writePixel(const ImageRgbaU8& image, int32_t x, int32_t y, const ColorRgbaI32& color) {
		if (image_isPixelInside(image, x, y)) image_accessPixel(image, x, y) = image_saturateAndPack(image, color);
	}
	// Pre-packed color using image_saturateAndPack to create the pixel in advance.
	inline void image_writePixel(const ImageRgbaU8& image, int32_t x, int32_t y, uint32_t packedColor) {
		if (image_isPixelInside(image, x, y)) image_accessPixel(image, x, y) = packedColor;
	}
	// Read a pixel from an image with a solid border outside.
	//   Out of bound will return the border color.
	//   The border color does not have to be constrained to the limits of pixel storage.
	//   Empty images will return zero.
	inline int32_t image_readPixel_border(const ImageU8& image, int32_t x, int32_t y, int32_t border = 0) {
		if (!image_exists(image)) {
			return 0;
		} else if (image_isPixelInside(image, x, y)) {
			return image_accessPixel(image, x, y);
		} else {
			return border;
		}
	}
	// The packed version is identical to the unpacked version, so we make a wrapper for template functions to call.
	//inline int32_t image_readPixel_border_packed(const ImageU8& image, int32_t x, int32_t y, int32_t border = 0) { return image_readPixel_border(image, x, y, border); }
	inline int32_t image_readPixel_border(const ImageU16& image, int32_t x, int32_t y, int32_t border = 0) {
		if (!image_exists(image)) {
			return 0;
		} else if (image_isPixelInside(image, x, y)) {
			return image_accessPixel(image, x, y);
		} else {
			return border;
		}
	}
	//inline int32_t image_readPixel_border_packed(const ImageU16& image, int32_t x, int32_t y, int32_t border = 0) { return image_readPixel_border(image, x, y, border); }
	inline float image_readPixel_border(const ImageF32& image, int32_t x, int32_t y, float border = 0.0f) {
		if (!image_exists(image)) {
			return 0.0f;
		} else if (image_isPixelInside(image, x, y)) {
			return image_accessPixel(image, x, y);
		} else {
			return border;
		}
	}
	//inline float image_readPixel_border_packed(const ImageF32& image, int32_t x, int32_t y, int32_t border = 0) { return image_readPixel_border(image, x, y, border); }
	inline ColorRgbaI32 image_readPixel_border(const ImageRgbaU8& image, int32_t x, int32_t y, const ColorRgbaI32& border = ColorRgbaI32()) {
		if (!image_exists(image)) {
			return ColorRgbaI32(0, 0, 0, 0);
		} else if (image_isPixelInside(image, x, y)) {
			return image_unpack(image, image_accessPixel(image, x, y));
		} else {
			return border;
		}
	}
	// Read the color directly as it is packed in image's pack order.
	inline uint32_t image_readPixel_border_packed(const ImageRgbaU8& image, int32_t x, int32_t y, uint32_t border = 0) {
		if (!image_exists(image)) {
			return 0;
		} else if (image_isPixelInside(image, x, y)) {
			return image_accessPixel(image, x, y);
		} else {
			return border;
		}
	}
	// Read a pixel from an image stretched edges.
	//   Out of bound will return the closest pixel.
	//   Empty images will return zero.
	inline uint8_t image_readPixel_clamp(const ImageU8& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_accessPixel(image, clamp(0, x, image_getWidth(image) - 1), clamp(0, y, image_getHeight(image) - 1));
		} else {
			return 0;
		}
	}
	//inline uint8_t image_readPixel_clamp_packed(const ImageU8& image, int32_t x, int32_t y) { return image_readPixel_clamp(image, x, y); }
	inline uint16_t image_readPixel_clamp(const ImageU16& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_accessPixel(image, clamp(0, x, image_getWidth(image) - 1), clamp(0, y, image_getHeight(image) - 1));
		} else {
			return 0;
		}
	}
	//inline uint16_t image_readPixel_clamp_packed(const ImageU16& image, int32_t x, int32_t y) { return image_readPixel_clamp(image, x, y); }
	inline float image_readPixel_clamp(const ImageF32& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_accessPixel(image, clamp(0, x, image_getWidth(image) - 1), clamp(0, y, image_getHeight(image) - 1));
		} else {
			return 0.0f;
		}
	}
	//inline float image_readPixel_clamp_packed(const ImageF32& image, int32_t x, int32_t y) { return image_readPixel_clamp(image, x, y); }
	inline ColorRgbaI32 image_readPixel_clamp(const ImageRgbaU8& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_unpack(image, image_accessPixel(image, clamp(0, x, image_getWidth(image) - 1), clamp(0, y, image_getHeight(image) - 1)));
		} else {
			return ColorRgbaI32();
		}
	}
	// Read the color directly as it is packed in image's pack order.
	inline uint32_t image_readPixel_clamp_packed(const ImageRgbaU8& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_accessPixel(image, clamp(0, x, image_getWidth(image) - 1), clamp(0, y, image_getHeight(image) - 1));
		} else {
			return 0;
		}
	}

	// Read a pixel from an image with tiling.
	//   Out of bound will take the coordinates in modulo of the size.
	//   Empty images will return zero.
	inline uint8_t image_readPixel_tile(const ImageU8& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_accessPixel(image, signedModulo(x, image_getWidth(image)), signedModulo(y, image_getHeight(image)));
		} else {
			return 0;
		}
	}
	//inline uint8_t image_readPixel_tile_packed(const ImageU8& image, int32_t x, int32_t y) { return image_readPixel_tile(image, x, y); }
	inline uint16_t image_readPixel_tile(const ImageU16& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_accessPixel(image, signedModulo(x, image_getWidth(image)), signedModulo(y, image_getHeight(image)));
		} else {
			return 0;
		}
	}
	//inline uint16_t image_readPixel_tile_packed(const ImageU16& image, int32_t x, int32_t y) { return image_readPixel_tile(image, x, y); }
	inline float image_readPixel_tile(const ImageF32& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_accessPixel(image, signedModulo(x, image_getWidth(image)), signedModulo(y, image_getHeight(image)));
		} else {
			return 0.0f;
		}
	}
	//inline float image_readPixel_tile_packed(const ImageF32& image, int32_t x, int32_t y) { return image_readPixel_tile(image, x, y); }
	inline ColorRgbaI32 image_readPixel_tile(const ImageRgbaU8& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_unpack(image, image_accessPixel(image, signedModulo(x, image_getWidth(image)), signedModulo(y, image_getHeight(image))));
		} else {
			return ColorRgbaI32();
		}
	}
	// Read the color directly as it is packed in image's pack order.
	inline uint32_t image_readPixel_tile_packed(const ImageRgbaU8& image, int32_t x, int32_t y) {
		if (image_exists(image)) {
			return image_accessPixel(image, signedModulo(x, image_getWidth(image)), signedModulo(y, image_getHeight(image)));
		} else {
			return 0;
		}
	}

// Loading
	// Load an image from a file by giving the filename including folder path and extension.
	// If mustExist is true, an exception will be raised on failure.
	// If mustExist is false, failure will return an empty handle.
	OrderedImageRgbaU8 image_load_RgbaU8(const String& filename, bool mustExist = true);
	// Load an image from a memory buffer, which can be loaded with file_loadBuffer to get the same result as loading directly from the file.
	// A convenient way of loading compressed images from larger files.
	// Failure will return an empty handle.
	OrderedImageRgbaU8 image_decode_RgbaU8(const Buffer& fileContent);
	// A faster and more flexible way to load compressed images from memory.
	// If you just want to point directly to a memory location to avoid allocating many small buffers, you can use a safe pointer and a size in bytes.
	// Failure will return an empty handle.
	OrderedImageRgbaU8 image_decode_RgbaU8(SafePointer<const uint8_t> data, int size);

// Saving
	// Save the image to the path specified by filename and return true iff the operation was successful.
	// The file extension is case insensitive after the last dot in filename.
	//   Accepted file extensions:
	//     *.jpg or *.jpeg
	//     *.png
	//     *.tga or *.targa
	//     *.bmp
	// If mustWork is true, an exception will be raised on failure.
	// If mustWork is false, failure will return false.
	// The optional quality setting goes from 1% to 100% and is at the maximum by default.
	bool image_save(const ImageRgbaU8 &image, const String& filename, bool mustWork = true, int quality = 100);
	// Save the image to a memory buffer.
	// Post-condition: Returns a buffer with the encoded image format as it would be saved to a file, or empty on failure.
	//                 No exceptions will be raised on failure, because an error message without a filename would not explain much.
	// The optional quality setting goes from 1% to 100% and is at the maximum by default.
	Buffer image_encode(const ImageRgbaU8 &image, ImageFileFormat format, int quality = 90);

// Fill all pixels with a uniform color
	void image_fill(const ImageU8& image, int32_t color);
	void image_fill(const ImageU16& image, int32_t color);
	void image_fill(const ImageF32& image, float color);
	void image_fill(const ImageRgbaU8& image, const ColorRgbaI32& color);

// Clone
	// Get a deep clone of an image's content while discarding any pack order, padding and texture pyramids.
	// If the input image had a different pack order, it will automatically be converted into RGBA to preserve the colors.
	AlignedImageU8 image_clone(const ImageU8& image);
	AlignedImageU16 image_clone(const ImageU16& image);
	AlignedImageF32 image_clone(const ImageF32& image);
	OrderedImageRgbaU8 image_clone(const ImageRgbaU8& image);
	// Returns a copy of the image without any padding, which means that alignment cannot be guaranteed.
	// The pack order is the same as the input, becuase it just copies the memory one row at a time to be fast.
	// Used when external image libraries don't allow giving stride as a separate argument.
	ImageRgbaU8 image_removePadding(const ImageRgbaU8& image);

// Ascii images
	String image_toAscii(const ImageU8& image, const String &alphabet);
	String image_toAscii(const ImageU8& image);
	AlignedImageU8 image_fromAscii(const String &content);

// Comparisons
	// Get the maximum pixelwise difference between two images of the same format, or the highest possible value on failure
	//   Useful for regression tests
	uint8_t  image_maxDifference(const ImageU8&     imageA, const ImageU8&     imageB);
	uint16_t image_maxDifference(const ImageU16&    imageA, const ImageU16&    imageB);
	float    image_maxDifference(const ImageF32&    imageA, const ImageF32&    imageB);
	uint8_t  image_maxDifference(const ImageRgbaU8& imageA, const ImageRgbaU8& imageB);

// TODO: Create sub-image constructors in the image types.

// Sub-images are read/write views to a smaller region of the same pixel data.
	// Get a sub-image sharing buffer and side-effects with the parent image
	// Returns the overlapping region if out of bound
	// Returns a null image if there are no overlapping pixels to return
	inline ImageU8 image_getSubImage(const ImageU8& image, const IRect& region) {
		static_assert(sizeof(ImageU8) == sizeof(Image), "ImageU8 must have the same size as Image, to prevent slicing in assignments!");
		return ImageU8(image, region);
	}
	inline ImageU16 image_getSubImage(const ImageU16& image, const IRect& region) {
		static_assert(sizeof(ImageU16) == sizeof(Image), "ImageU16 must have the same size as Image, to prevent slicing in assignments!");
		return ImageU16(image, region);
	}
	inline ImageF32 image_getSubImage(const ImageF32& image, const IRect& region) {
		static_assert(sizeof(ImageF32) == sizeof(Image), "ImageF32 must have the same size as Image, to prevent slicing in assignments!");
		return ImageF32(image, region);
	}
	inline ImageRgbaU8 image_getSubImage(const ImageRgbaU8& image, const IRect& region) {
		static_assert(sizeof(ImageRgbaU8) == sizeof(Image), "ImageRgbaU8 must have the same size as Image, to prevent slicing in assignments!");
		return ImageRgbaU8(image, region);
	}
	// Check dynamically if the image was created as a sub-image.
	// Returns true if the image is a sub-image, created using image_getSubImage.
	// Returns false if the image is not a sub-image, created using image_create or default constructed as an empty image.
	inline bool image_isSubImage(const Image& image) {
		return image.impl_dimensions.isSubImage();
	}

// Bound-checked pointer access (relatively safe compared to a raw pointer)
	// Returns a bound-checked pointer to the first pixel.
	template <typename T = uint8_t>
	inline SafePointer<uint8_t> image_getSafePointer(const ImageU8& image) {
		return image.impl_buffer.getSafe<uint8_t>("Pointer to ImageU8 pixels").increaseBytes(image_getStartOffset(image));
	}
	// Returns a bound-checked pointer to the first pixel at rowIndex.
	template <typename T = uint8_t>
	inline SafePointer<uint8_t> image_getSafePointer(const ImageU8& image, int32_t rowIndex) {
		return image_getSafePointer(image).increaseBytes(image_getStride(image) * rowIndex);
	}
	// Returns a bound-checked pointer to the first pixel.
	template <typename T = uint16_t>
	inline SafePointer<T> image_getSafePointer(const ImageU16& image) {
		return image.impl_buffer.getSafe<T>("Pointer to ImageU16 pixels").increaseBytes(image_getStartOffset(image));
	}
	// Returns a bound-checked pointer to the first pixel at rowIndex.
	template <typename T = uint16_t>
	inline SafePointer<T> image_getSafePointer(const ImageU16& image, int32_t rowIndex) {
		return image_getSafePointer<T>(image).increaseBytes(image_getStride(image) * rowIndex);
	}
	// Returns a bound-checked pointer to the first pixel.
	template <typename T = float>
	inline SafePointer<T> image_getSafePointer(const ImageF32& image) {
		return image.impl_buffer.getSafe<T>("Pointer to ImageF32 pixels").increaseBytes(image_getStartOffset(image));
	}
	// Returns a bound-checked pointer to the first pixel at rowIndex.
	template <typename T = float>
	inline SafePointer<T> image_getSafePointer(const ImageF32& image, int32_t rowIndex) {
		return image_getSafePointer<T>(image).increaseBytes(image_getStride(image) * rowIndex);
	}
	// Returns a bound-checked pointer to the first pixel.
	template <typename T = uint32_t>
	inline SafePointer<T> image_getSafePointer(const ImageRgbaU8& image) {
		return image.impl_buffer.getSafe<T>("Pointer to ImageRgbaU8 pixels").increaseBytes(image_getStartOffset(image));
	}
	// Returns a bound-checked pointer to the first pixel at rowIndex.
	template <typename T = uint32_t>
	inline SafePointer<T> image_getSafePointer(const ImageRgbaU8& image, int32_t rowIndex) {
		return image_getSafePointer<T>(image).increaseBytes(image_getStride(image) * rowIndex);
	}
	// Returns a bound-checked pointer to the first channel in the first pixel.
	inline SafePointer<uint8_t> image_getSafePointer_channels(const ImageRgbaU8& image) {
		return image.impl_buffer.getSafe<uint8_t>("Pointer to ImageRgbaU8 channels").increaseBytes(image_getStartOffset(image));
	}
	// Returns a bound-checked pointer to the first channel in the first pixel at rowIndex.
	inline SafePointer<uint8_t> image_getSafePointer_channels(const ImageRgbaU8& image, int32_t rowIndex) {
		return image.impl_buffer.getSafe<uint8_t>("Pointer to ImageRgbaU8 channels").increaseBytes(image_getStartOffset(image)).increaseBytes(image_getStride(image) * rowIndex);
	}

// The dangerous image API
// Use of these methods can be spotted using a search for "_dangerous_" in your code
	// Replaces the destructor in image's buffer, which.
	//   newDestructor should not free the given data, only invoke destruction of any external resources that may depend on it before the data is freed automatically.
	inline void image_dangerous_replaceDestructor(ImageU8& image, const HeapDestructor &newDestructor) {
		if (image_exists(image)) { return buffer_replaceDestructor(image.impl_buffer, newDestructor); }
	}
	inline void image_dangerous_replaceDestructor(ImageU16& image, const HeapDestructor &newDestructor) {
		if (image_exists(image)) { return buffer_replaceDestructor(image.impl_buffer, newDestructor); }
	}
	inline void image_dangerous_replaceDestructor(ImageF32& image, const HeapDestructor &newDestructor) {
		if (image_exists(image)) { return buffer_replaceDestructor(image.impl_buffer, newDestructor); }
	}
	inline void image_dangerous_replaceDestructor(ImageRgbaU8& image, const HeapDestructor &newDestructor) {
		if (image_exists(image)) { return buffer_replaceDestructor(image.impl_buffer, newDestructor); }
	}

	// Returns a pointer to the image's pixels
	// Warning! Reading elements larger than 8 bits will have lower and higher bytes stored based on local endianness
	// Warning! Using bytes outside of the [0 .. stride * height - 1] range may cause crashes and undefined behaviour
	// Warning! Using the pointer after the image's lifetime may cause crashes from trying to access freed memory
	inline uint8_t* image_dangerous_getData(const ImageU8&     image) { return image.impl_buffer.getUnsafe() + image_getStartOffset(image); }
	inline uint8_t* image_dangerous_getData(const ImageU16&    image) { return image.impl_buffer.getUnsafe() + image_getStartOffset(image); }
	inline uint8_t* image_dangerous_getData(const ImageF32&    image) { return image.impl_buffer.getUnsafe() + image_getStartOffset(image); }
	inline uint8_t* image_dangerous_getData(const ImageRgbaU8& image) { return image.impl_buffer.getUnsafe() + image_getStartOffset(image); }
}

#endif
