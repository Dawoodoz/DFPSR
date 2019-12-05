
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "stbImageWrapper.h"

using namespace dsr;

OrderedImageRgbaU8 dsr::image_stb_load_RgbaU8(const String& filename, bool mustExist) {
	int width, height, bpp;
	uint8_t *data = stbi_load(filename.toStdString().c_str(), &width, &height, &bpp, 4);
	if (data == 0) {
		if (mustExist) {
			// TODO: Throw an optional runtime exception
			printText("The image ", filename, " could not be loaded!\n");
		}
		return OrderedImageRgbaU8(); // Return null
	}
	// Create a padded buffer
	OrderedImageRgbaU8 result = image_create_RgbaU8(width, height);
	// Copy the data
	int rowSize = width * 4;
	int32_t targetStride = image_getStride(result);
	const uint8_t *sourceRow = data;
	uint8_t* targetRow = image_dangerous_getData(result);
	for (int32_t y = 0; y < height; y++) {
		// Copy a row without touching the padding
		memcpy(targetRow, sourceRow, rowSize);
		// Add stride using single byte elements
		targetRow += targetStride;
		sourceRow += rowSize;
	}
	// Free the unpadded image
	free(data);
	return result;
}

bool dsr::image_stb_save(const ImageRgbaU8 &image, const String& filename) {
	// Remove all padding before saving to avoid crashing
	ImageRgbaU8 unpadded = ImageRgbaU8(image_removePadding(image));
	return stbi_write_png(filename.toStdString().c_str(), image_getWidth(unpadded), image_getHeight(unpadded), 4, image_dangerous_getData(unpadded), image_getStride(unpadded)) != 0;
}
