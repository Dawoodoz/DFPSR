
// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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

#define DFPSR_INTERNAL_ACCESS

#include "types.h"
#include "../image/Image.h"
#include "../image/ImageU8.h"
#include "../image/ImageU16.h"
#include "../image/ImageF32.h"
#include "../image/ImageRgbaU8.h"
#include "../image/PackOrder.h"

using namespace dsr;

// Null
ImageU8::ImageU8() {}
ImageU16::ImageU16() {}
ImageF32::ImageF32() {}
ImageRgbaU8::ImageRgbaU8() {}
MediaMachine::MediaMachine() {}

// Existing shared pointer
ImageU8::ImageU8(const std::shared_ptr<ImageU8Impl>& image) : std::shared_ptr<ImageU8Impl>(image) {}
ImageU16::ImageU16(const std::shared_ptr<ImageU16Impl>& image) : std::shared_ptr<ImageU16Impl>(image) {}
ImageF32::ImageF32(const std::shared_ptr<ImageF32Impl>& image) : std::shared_ptr<ImageF32Impl>(image) {}
ImageRgbaU8::ImageRgbaU8(const std::shared_ptr<ImageRgbaU8Impl>& image) : std::shared_ptr<ImageRgbaU8Impl>(image) {}
MediaMachine::MediaMachine(const std::shared_ptr<VirtualMachine>& machine) : std::shared_ptr<VirtualMachine>(machine) {}

// Shallow copy
ImageU8::ImageU8(const ImageU8Impl& image) : std::shared_ptr<ImageU8Impl>(std::make_shared<ImageU8Impl>(image)) {}
ImageU16::ImageU16(const ImageU16Impl& image) : std::shared_ptr<ImageU16Impl>(std::make_shared<ImageU16Impl>(image)) {}
ImageF32::ImageF32(const ImageF32Impl& image) : std::shared_ptr<ImageF32Impl>(std::make_shared<ImageF32Impl>(image)) {}
ImageRgbaU8::ImageRgbaU8(const ImageRgbaU8Impl& image) : std::shared_ptr<ImageRgbaU8Impl>(std::make_shared<ImageRgbaU8Impl>(image)) {}
