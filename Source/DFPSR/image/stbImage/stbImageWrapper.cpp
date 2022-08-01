
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "stbImageWrapper.h"

namespace dsr {

OrderedImageRgbaU8 image_stb_decode_RgbaU8(const SafePointer<uint8_t> data, int size) {
	#ifdef SAFE_POINTER_CHECKS
		// If the safe pointer has debug information, use it to assert that size is within bound.
		data.assertInside("image_stb_decode_RgbaU8 (data)", data.getUnsafe(), (size_t)size);
	#endif
	int width, height, bpp;
	uint8_t *rawPixelData = stbi_load_from_memory(data.getUnsafe(), size, &width, &height, &bpp, 4);
	if (rawPixelData == nullptr) {
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

// Pre-condition: Images that STB image don't have stride implementations for must be given unpadded images.
Buffer image_stb_encode(const ImageRgbaU8 &image, ImageFileFormat format, int quality) {
	int width = image_getWidth(image);
	int height = image_getHeight(image);
	List<uint8_t> targetList;
	// Reserve enough memory for an uncompressed file to reduce the need for reallcation.
	targetList.reserve(width * height * 4 + 2048);
	stbi_write_func* writer = [](void* context, void* data, int size) {
		List<uint8_t>* target = (List<uint8_t>*)context;
		for (int i = 0; i < size; i++) {
			target->push(((uint8_t*)data)[i]);
		}
	};
	bool success = false;
	if (format == ImageFileFormat::JPG) {
		success = stbi_write_jpg_to_func(writer, &targetList, width, height, 4, image_dangerous_getData(image), quality);
	} else if (format == ImageFileFormat::PNG) {
		success = stbi_write_png_to_func(writer, &targetList, width, height, 4, image_dangerous_getData(image), image_getStride(image));
	} else if (format == ImageFileFormat::TGA) {
		success = stbi_write_tga_to_func(writer, &targetList, width, height, 4, image_dangerous_getData(image));
	} else if (format == ImageFileFormat::BMP) {
		success = stbi_write_bmp_to_func(writer, &targetList, width, height, 4, image_dangerous_getData(image));
	}
	if (success) {
		// Copy data to a new buffer once the total size is known.
		Buffer result = buffer_create(targetList.length());
		uint8_t* targetData = buffer_dangerous_getUnsafeData(result);
		memcpy(targetData, &targetList[0], targetList.length());
		return result; // Return the buffer on success.
	} else {
		return Buffer(); // Return a null handle on failure.
	}
}

}
