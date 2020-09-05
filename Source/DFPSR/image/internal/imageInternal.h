
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

#ifndef DFPSR_IMAGE_INTERNAL
#define DFPSR_IMAGE_INTERNAL

#include "../Image.h"
#include "../ImageRgbaU8.h"

namespace dsr {
namespace imageInternal {

//inline int32_t getWidth(const ImageImpl &image) { return image.width; }
inline int32_t getWidth(const ImageImpl *image) { return image ? image->width : 0; }
//inline int32_t getHeight(const ImageImpl &image) { return image.height; }
inline int32_t getHeight(const ImageImpl *image) { return image ? image->height : 0; }
//inline int32_t getStride(const ImageImpl &image) { return image.stride; }
inline int32_t getStride(const ImageImpl *image) { return image ? image->stride : 0; }
inline int32_t getRowSize(const ImageImpl &image) { return image.width * image.pixelSize; }
inline int32_t getRowSize(const ImageImpl *image) { return image ? getRowSize(*image) : 0; }
inline int32_t getUsedBytes(const ImageImpl &image) { return (image.stride * (image.height - 1)) + (image.width * image.pixelSize); }
inline int32_t getUsedBytes(const ImageImpl *image) { return image ? getUsedBytes(*image) : 0; }
//inline int32_t getPixelSize(const ImageImpl &image) { return image.pixelSize; }
inline int32_t getPixelSize(const ImageImpl *image) { return image ? image->pixelSize : 0; }
//inline int32_t getStartOffset(const ImageImpl &image) { return image.startOffset; }
inline int32_t getStartOffset(const ImageImpl *image) { return image ? image->startOffset : 0; }
inline Buffer getBuffer(const ImageImpl &image) { return image.buffer; }
inline Buffer getBuffer(const ImageImpl *image) { return image ? getBuffer(*image) : Buffer(); }
inline IRect getBound(const ImageImpl &image) { return IRect(0, 0, image.width, image.height); }
inline IRect getBound(const ImageImpl *image) { return image ? getBound(*image) : IRect(); }
inline PackOrder getPackOrder(const ImageRgbaU8Impl *image) { return image ? image->packOrder : PackOrder(); }

// Get data
//   The pointer has access to the whole parent buffer,
//   to allow aligning SIMD vectors outside of the used region.
template <typename T>
static inline const SafePointer<T> getSafeData(const ImageImpl &image, int rowIndex = 0) {
	auto result = buffer_getSafeData<T>(image.buffer, "Image buffer");
	result.increaseBytes(image.startOffset + image.stride * rowIndex);
	return result;
}
template <typename T>
inline const SafePointer<T> getSafeData(const ImageImpl *image, int rowIndex = 0) {
	return image ? getSafeData<T>(*image, rowIndex) : SafePointer<T>("Null image buffer");
}
template <typename T>
static inline SafePointer<T> getSafeData(ImageImpl &image, int rowIndex = 0) {
	auto result = buffer_getSafeData<T>(image.buffer, "Image buffer");
	result.increaseBytes(image.startOffset + image.stride * rowIndex);
	return result;
}
template <typename T>
inline SafePointer<T> getSafeData(ImageImpl *image, int rowIndex = 0) {
	return image ? getSafeData<T>(*image, rowIndex) : SafePointer<T>("Null image buffer");
}

}
}

#endif

