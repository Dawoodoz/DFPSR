
#ifndef DFPSR_API_IMAGE_STB_WRAPPER
#define DFPSR_API_IMAGE_STB_WRAPPER

#include "../../api/imageAPI.h"
#include "../../base/text.h"

namespace dsr {

OrderedImageRgbaU8 image_stb_load_RgbaU8(const String& filename, bool mustExist = true);
bool image_stb_save(const ImageRgbaU8 &image, const String& filename);

}

#endif
