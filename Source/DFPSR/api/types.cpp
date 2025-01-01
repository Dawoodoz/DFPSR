
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
ImageU8::ImageU8(const Handle<ImageU8Impl>& image) : Handle<ImageU8Impl>(image) {}
ImageU16::ImageU16(const Handle<ImageU16Impl>& image) : Handle<ImageU16Impl>(image) {}
ImageF32::ImageF32(const Handle<ImageF32Impl>& image) : Handle<ImageF32Impl>(image) {}
ImageRgbaU8::ImageRgbaU8(const Handle<ImageRgbaU8Impl>& image) : Handle<ImageRgbaU8Impl>(image) {}
MediaMachine::MediaMachine(const Handle<VirtualMachine>& machine) : Handle<VirtualMachine>(machine) {}

// Shallow copy
ImageU8::ImageU8(const ImageU8Impl& image) : Handle<ImageU8Impl>(handle_create<ImageU8Impl>(image)) {}
ImageU16::ImageU16(const ImageU16Impl& image) : Handle<ImageU16Impl>(handle_create<ImageU16Impl>(image)) {}
ImageF32::ImageF32(const ImageF32Impl& image) : Handle<ImageF32Impl>(handle_create<ImageF32Impl>(image)) {}
ImageRgbaU8::ImageRgbaU8(const ImageRgbaU8Impl& image) : Handle<ImageRgbaU8Impl>(handle_create<ImageRgbaU8Impl>(image)) {}
