
#ifndef DFPSR_API_IMAGE_STB_WRAPPER
#define DFPSR_API_IMAGE_STB_WRAPPER

#include "../../api/imageAPI.h"
#include "../../api/stringAPI.h"
#include "../../api/types.h"

namespace dsr {

OrderedImageRgbaU8 image_stb_decode_RgbaU8(const SafePointer<uint8_t> data, int size);

// Pre-conditions:
// * The image must be packed in RGBA order at runtime, but can't be in the OrderedImageRgbaU8 format because Ordered inherits from Aligned.
// * Only the PNG format may use padding in this call.
Buffer image_stb_encode(const ImageRgbaU8 &image, ImageFileFormat format, int quality);

}

#endif
