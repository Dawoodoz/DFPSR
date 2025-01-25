
// zlib open source license
//
// Copyright (c) 2019 to 2025 David Forsgren Piuva
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

#ifndef DFPSR_IMAGE_TYPES
#define DFPSR_IMAGE_TYPES

#include "PackOrder.h"
#include "../math/IRect.h"
#include "../api/bufferAPI.h"

namespace dsr {

enum class ImageFileFormat {
	Unknown, // Used as an error code for unidentified formats.
	JPG, // Lossy compressed image format storing brightness separated from red and blue offsets using the discrete cosine transform of each block.
	PNG, // Lossless compressed image format. Some image editors don't save RGB values where alpha is zero, which will bleed through black edges in bi-linear interpolation when the interpolated alpha is not zero.
	TGA, // Lossless compressed format. Applications usually give Targa better control over the alpha channel than PNG, but it's more common that the Targa specification is interpreted in incompatible ways.
	BMP // Uncompressed image format for storing data that does not really represent an image and you just want it to be exact.
};

// Packed into 2 bits in ImageDimensions.
enum class PixelFormat : uint32_t {
	MonoU8, // Gray-scale image of 8 bits per pixel (0..255).
	MonoU16,
	MonoF32,
	RgbaU8 // RGBA colors in any order. 8 bits per channel (0..255). 32 bits per pixel.
};

// Start offset and stride is stored in pixels and the getters in imageAPI can automatically convert them into byte offsets as needed.
// Maximum image dimensions are 65536 x 65536, because that will precisely fit the worst case start offset into uint32_t.
//   maxPixelCount = 65536²             = 4294967296
//   maxStartOffset = maxPixelCount - 1 = 4294967295
//   largest uint32_t         = 2³² - 1 = 4294967295

// Because the computer will do bitwise operations to read and write small integers anyway,
//   there is usually no performance penalty for choosing an odd number of bits to pack more information.
class ImageDimensions {
private:
	// Bit masks and offsets for the properties that are packed into the same 64-bit integer.
	static const uint64_t  readMask_width         = 0b1111111111111111100000000000000000000000000000000000000000000000; //  0 zeroes, 17 ones, 47 zeroes
	static const uint32_t  inputMask_width        = 0b11111111111111111                                               ; //            17 ones
	static const int       bitOffset_width        =                    47                                             ;
	static const uint64_t  readMask_height        = 0b0000000000000000011111111111111111000000000000000000000000000000; // 17 zeroes, 17 ones, 30 zeroes
	static const uint32_t  inputMask_height       =                  0b11111111111111111                              ; //            17 ones
	static const int       bitOffset_height       =                                     30                            ;
	static const uint64_t  readMask_stride        = 0b0000000000000000000000000000000000111111111111111110000000000000; // 34 zeroes, 17 ones, 13 zeroes
	static const uint32_t  inputMask_stride       =                                   0b11111111111111111             ; //            17 ones
	static const int       bitOffset_stride       =                                                      13           ;
	static const uint64_t  readMask_packOrder     = 0b0000000000000000000000000000000000000000000000000001100000000000; // 51 zeroes,  2 ones, 11 zeroes
	static const uint32_t  inputMask_packOrder    =                                                    0b11           ; //             2 ones
	static const int       bitOffset_packOrder    =                                                        11         ;
	static const uint64_t  readMask_format        = 0b0000000000000000000000000000000000000000000000000000011000000000; // 52 zeroes,  2 ones,  9 zeroes
	static const uint32_t  inputMask_format       =                                                      0b11         ; //             2 ones
	static const int       bitOffset_format       =                                                          9        ;
	static const uint64_t  readMask_subImage      = 0b0000000000000000000000000000000000000000000000000000000100000000; // 55 zeroes,  1 ones,  8 zeroes
private:
	// Actual members.
	uint64_t data = 0;
	uint32_t pixelStartOffset = 0; // This one fits exactly into 32 bits, but we will have 32 more bits of padding.
private:
	// Helper functions.
	static inline uint64_t readFrom64(const uint64_t &source, uint64_t readMask, uint32_t bitOffset) {
		return (source & readMask) >> bitOffset;
	}
public:
	// Access to the data.
	inline uint32_t getWidth() const {
		return ImageDimensions::readFrom64(this->data, readMask_width, bitOffset_width);
	}
	inline uint32_t getHeight() const {
		return ImageDimensions::readFrom64(this->data, readMask_height, bitOffset_height);
	}
	inline uint32_t getPixelStride() const {
		return ImageDimensions::readFrom64(this->data, readMask_stride, bitOffset_stride);
	}
	inline PackOrderIndex getPackOrderIndex() const {
		return (PackOrderIndex)readFrom64(this->data, readMask_packOrder, bitOffset_packOrder);
	}
	inline PixelFormat getPixelFormat() const {
		return (PixelFormat)readFrom64(this->data, readMask_format, bitOffset_format);
	}
	inline bool isSubImage() const {
		// No need to shift the bit before normalizing, because anything else than zero becomes 1.
		return (this->data & readMask_subImage) != 0;
	}
	inline uint32_t getLog2PixelSize() const {
		// Shift the constants instead of the index to save a cycle.
		uint64_t shifterPixelFormatIndex = this->data & readMask_format;
		if        (shifterPixelFormatIndex == ((uint64_t)PixelFormat::MonoU8  << bitOffset_format)) {
			return 0;
		} else if (shifterPixelFormatIndex == ((uint64_t)PixelFormat::MonoU16 << bitOffset_format)) {
			return 1;
		} else if (shifterPixelFormatIndex == ((uint64_t)PixelFormat::MonoF32 << bitOffset_format)) {
			return 2;
		} else if (shifterPixelFormatIndex == ((uint64_t)PixelFormat::RgbaU8  << bitOffset_format)) {
			return 2;
		} else {
			return 0; // Unknown pixel format!
		}
	}
	inline uint32_t getPixelSize() const {
		// Shift the constants instead of the index to save a cycle.
		uint64_t shifterPixelFormatIndex = this->data & readMask_format;
		if        (shifterPixelFormatIndex == ((uint64_t)PixelFormat::MonoU8  << bitOffset_format)) {
			return 1;
		} else if (shifterPixelFormatIndex == ((uint64_t)PixelFormat::MonoU16 << bitOffset_format)) {
			return 2;
		} else if (shifterPixelFormatIndex == ((uint64_t)PixelFormat::MonoF32 << bitOffset_format)) {
			return 4;
		} else if (shifterPixelFormatIndex == ((uint64_t)PixelFormat::RgbaU8  << bitOffset_format)) {
			return 4;
		} else {
			return 0; // Unknown pixel format!
		}
	}
	inline uint32_t getPixelStartOffset() const {
		return this->pixelStartOffset;
	}
	inline uintptr_t getByteStartOffset() const {
		return uintptr_t(this->getPixelStartOffset()) << this->getLog2PixelSize();
	}
	inline uintptr_t getByteStride() const {
		return uintptr_t(this->getPixelStride()) << this->getLog2PixelSize();
	}
	// Constuction that truncates individual inputs in modulo, just to make sure that too large values do not affect other values and make debugging into a nightmare.
	ImageDimensions(uint32_t width, uint32_t height, uint32_t pixelStride, PackOrderIndex packOrderIndex, PixelFormat pixelFormat, uint32_t pixelStartOffset) noexcept
	: data(((uint64_t)(           width           & inputMask_width       ) << bitOffset_width)
	     | ((uint64_t)(           height          & inputMask_height      ) << bitOffset_height)
	     | ((uint64_t)(           pixelStride     & inputMask_stride      ) << bitOffset_stride)
	     | ((uint64_t)(((uint32_t)packOrderIndex) & inputMask_packOrder   ) << bitOffset_packOrder)
	     | ((uint64_t)(((uint32_t)pixelFormat)    & inputMask_format      ) << bitOffset_format)
	), pixelStartOffset(pixelStartOffset) {}
	ImageDimensions() {}
	void setWidthHeightStartSubImage(uint32_t width, uint32_t height, uint32_t pixelStartOffset) {
		this->data = (this->data & ~(readMask_width | readMask_height))
		           | ((uint64_t)(width  & inputMask_width  ) << bitOffset_width)
	               | ((uint64_t)(height & inputMask_height ) << bitOffset_height)
				   | readMask_subImage;
		this->pixelStartOffset = pixelStartOffset;
	}
};

#define IMPL_IMAGE_CONSTRUCTORS(NEW_TYPE, BASE_TYPE) \
	NEW_TYPE() {} \
	NEW_TYPE(const Buffer &buffer, ImageDimensions dimensions) : BASE_TYPE(buffer, dimensions) {} \
	NEW_TYPE(const NEW_TYPE &source) : BASE_TYPE(source.impl_buffer, source.impl_dimensions) {} \
	NEW_TYPE(const NEW_TYPE &source, const IRect &region) \
	: BASE_TYPE(source.impl_buffer, source.impl_dimensions) { \
		IRect cut = IRect::cut(IRect(0, 0, source.impl_dimensions.getWidth(), source.impl_dimensions.getHeight()), region); \
		if (cut.hasArea()) { \
			this->impl_dimensions.setWidthHeightStartSubImage(cut.width(), cut.height(), source.impl_dimensions.getPixelStartOffset() + cut.left() + cut.top() * source.impl_dimensions.getPixelStride()); \
		} else { \
			this->impl_buffer = Buffer(); \
			this->impl_dimensions = ImageDimensions(); \
		} \
	}

#define IMPL_IMAGE_HIGHER_CONSTRUCTORS(NEW_TYPE, BASE_TYPE) \
	IMPL_IMAGE_CONSTRUCTORS(NEW_TYPE, BASE_TYPE) \
	NEW_TYPE(const Buffer &buffer, uint32_t pixelStartOffset, uint32_t width, uint32_t height, uint32_t pixelStride, const PackOrderIndex &packOrderIndex) \
	: BASE_TYPE(buffer, pixelStartOffset, width, height, pixelStride, packOrderIndex) {}

// Use imageAPI.h to access the content of images!
//   The content may change between library versions but is public to simplify access for inlined getters.
struct Image {
// PRIVATE:
// To maintain encapsulation from version specific details, do not touch things starting with IMPL_ or impl_.
// Use the inlined getters in imageAPI.h instead.

	// Reference counted pointer to the pixel data.
	Buffer impl_buffer;
	// Dimensions and pack order of the image.
	ImageDimensions impl_dimensions;
	// New.
	Image(const Buffer &buffer, ImageDimensions dimensions)
	: impl_buffer(buffer), impl_dimensions(dimensions) {}
	// Generic cut.
	Image(const Image &source, const IRect &region)
	: impl_buffer(source.impl_buffer), impl_dimensions(source.impl_dimensions) {
		IRect cut = IRect::cut(IRect(0, 0, source.impl_dimensions.getWidth(), source.impl_dimensions.getHeight()), region);
		if (cut.hasArea()) {
			this->impl_dimensions.setWidthHeightStartSubImage(cut.width(), cut.height(), source.impl_dimensions.getPixelStartOffset() + cut.left() + cut.top() * source.impl_dimensions.getPixelStride());
		} else {
			this->impl_buffer = Buffer();
			this->impl_dimensions = ImageDimensions();
		}
	}
	// Empty.
	Image() {}
};

// Can be unaligned.
//   Is not allowed to overwrite padding bytes, because it does not know the difference between padding and pixels belonging to a larger image sharing the same pixel buffer.
struct ImageU8
: public Image {
	static const int impl_pixelSize = 1;
	ImageU8(const Buffer &buffer, uint32_t pixelStartOffset, uint32_t width, uint32_t height, uint32_t pixelStride, const PackOrderIndex &packOrderIndex)
	: Image(buffer, ImageDimensions(width, height, pixelStride, packOrderIndex, PixelFormat::MonoU8, pixelStartOffset)) {}
	IMPL_IMAGE_CONSTRUCTORS(ImageU8, Image)
};

// The start of each row is aligned to DSR_MAXIMUM_ALIGNMENT for SIMD vectorization and thread safety.
//   Owns the padding bytes and may overwrite them during SIMD vectorization.
struct AlignedImageU8
: public ImageU8 {
	IMPL_IMAGE_HIGHER_CONSTRUCTORS(AlignedImageU8, ImageU8)
};

// Can be unaligned.
//   Is not allowed to overwrite padding bytes, because it does not know the difference between padding and pixels belonging to a larger image sharing the same pixel buffer.
struct ImageU16
: public Image {
	static const int impl_pixelSize = 2;
	ImageU16(const Buffer &buffer, uint32_t pixelStartOffset, uint32_t width, uint32_t height, uint32_t pixelStride, const PackOrderIndex &packOrderIndex)
	: Image(buffer, ImageDimensions(width, height, pixelStride, packOrderIndex, PixelFormat::MonoU16, pixelStartOffset)) {}
	IMPL_IMAGE_CONSTRUCTORS(ImageU16, Image)
};

// The start of each row is aligned to DSR_MAXIMUM_ALIGNMENT for SIMD vectorization and thread safety.
//   Owns the padding bytes and may overwrite them during SIMD vectorization.
struct AlignedImageU16
: public ImageU16 {
	IMPL_IMAGE_HIGHER_CONSTRUCTORS(AlignedImageU16, ImageU16)
};

// Can be unaligned.
//   Is not allowed to overwrite padding bytes, because it does not know the difference between padding and pixels belonging to a larger image sharing the same pixel buffer.
struct ImageF32
: public Image {
	static const int impl_pixelSize = 4;
	ImageF32(const Buffer &buffer, uint32_t pixelStartOffset, uint32_t width, uint32_t height, uint32_t pixelStride, const PackOrderIndex &packOrderIndex)
	: Image(buffer, ImageDimensions(width, height, pixelStride, packOrderIndex, PixelFormat::MonoF32, pixelStartOffset)) {}
	IMPL_IMAGE_CONSTRUCTORS(ImageF32, Image)
};

// The start of each row is aligned to DSR_MAXIMUM_ALIGNMENT for SIMD vectorization and thread safety.
//   Owns the padding bytes and may overwrite them during SIMD vectorization.
struct AlignedImageF32
: public ImageF32 {
	IMPL_IMAGE_HIGHER_CONSTRUCTORS(AlignedImageF32, ImageF32)
};

// Can be unaligned.
//   Is not allowed to overwrite padding bytes, because it does not know the difference between padding and pixels belonging to a larger image sharing the same pixel buffer.
// Can have any pack order.
struct ImageRgbaU8
: public Image {
	static const int impl_pixelSize = 4;
	ImageRgbaU8(const Buffer &buffer, uint32_t pixelStartOffset, uint32_t width, uint32_t height, uint32_t pixelStride, const PackOrderIndex &packOrderIndex)
	: Image(buffer, ImageDimensions(width, height, pixelStride, packOrderIndex, PixelFormat::RgbaU8, pixelStartOffset)) {}
	IMPL_IMAGE_CONSTRUCTORS(ImageRgbaU8, Image)
};

// The start of each row is aligned to DSR_MAXIMUM_ALIGNMENT for SIMD vectorization and thread safety.
//   Owns the padding bytes and may overwrite them during SIMD vectorization.
// Can have any pack order.
struct AlignedImageRgbaU8
: public ImageRgbaU8 {
	IMPL_IMAGE_HIGHER_CONSTRUCTORS(AlignedImageRgbaU8, ImageRgbaU8)
};

// The start of each row is aligned to DSR_MAXIMUM_ALIGNMENT for SIMD vectorization and thread safety.
//   Owns the padding bytes and may overwrite them during SIMD vectorization.
// Always in RGBA order.
struct OrderedImageRgbaU8
: public AlignedImageRgbaU8 {
	IMPL_IMAGE_HIGHER_CONSTRUCTORS(OrderedImageRgbaU8, AlignedImageRgbaU8)
};

#undef IMPL_IMAGE_CONSTRUCTORS
#undef IMPL_IMAGE_HIGHER_CONSTRUCTORS

}

#endif
