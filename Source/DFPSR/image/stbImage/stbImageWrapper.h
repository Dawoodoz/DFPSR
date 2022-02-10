
#ifndef DFPSR_API_IMAGE_STB_WRAPPER
#define DFPSR_API_IMAGE_STB_WRAPPER

#include "../../api/imageAPI.h"
#include "../../api/stringAPI.h"

namespace dsr {

OrderedImageRgbaU8 image_stb_decode_RgbaU8(const SafePointer<uint8_t> data, int size, bool mustParse = true);
bool image_stb_save(const ImageRgbaU8 &image, const String& filename);

}

#endif
