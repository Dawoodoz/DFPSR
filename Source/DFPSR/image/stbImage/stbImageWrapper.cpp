
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "stbImageWrapper.h"

namespace dsr {

OrderedImageRgbaU8 image_stb_decode_RgbaU8(const SafePointer<uint8_t> data, int size, bool mustParse) {
	#ifdef SAFE_POINTER_CHECKS
		// If the safe pointer has debug information, use it to assert that size is within bound.
		target.assertInside("image_stb_decode_RgbaU8 (data)", data.getUnsafe(), (size_t)size);
	#endif
	int width, height, bpp;
	uint8_t *rawPixelData = stbi_load_from_memory(data.getUnsafe(), size, &width, &height, &bpp, 4);
	if (rawPixelData == nullptr) {
		if (mustParse) {
			throwError("An image could not be parsed!\n");
		}
		return OrderedImageRgbaU8(); // Return null
	}
	// Create a padded buffer
	OrderedImageRgbaU8 result = image_create_RgbaU8(width, height);
	// Copy the data
	int rowSize = width * 4;
	int32_t targetStride = image_getStride(result);
	const uint8_t *sourceRow = rawPixelData;
	uint8_t* targetRow = image_dangerous_getData(result);
	for (int32_t y = 0; y < height; y++) {
		// Copy a row without touching the padding
		memcpy(targetRow, sourceRow, rowSize);
		// Add stride using single byte elements
		targetRow += targetStride;
		sourceRow += rowSize;
	}
	// Free the unpadded image
	free((void*)rawPixelData);
	return result;
}

bool image_stb_save(const ImageRgbaU8 &image, const String& filename) {
	// Remove all padding before saving to avoid crashing
	ImageRgbaU8 unpadded = ImageRgbaU8(image_removePadding(image));
	return stbi_write_png(filename.toStdString().c_str(), image_getWidth(unpadded), image_getHeight(unpadded), 4, image_dangerous_getData(unpadded), image_getStride(unpadded)) != 0;
}

}
