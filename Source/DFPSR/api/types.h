
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

#ifndef DFPSR_API_TYPES
#define DFPSR_API_TYPES

#include <stdint.h>
#include <memory>
#include "../image/Color.h"
#include "../math/IRect.h"
#include "../base/text.h"

// Define DFPSR_INTERNAL_ACCESS before any include to get internal access to exposed types
#ifdef DFPSR_INTERNAL_ACCESS
	#define IMPL_ACCESS public
#else
	#define IMPL_ACCESS protected
#endif

namespace dsr {

enum class PackOrderIndex {
	RGBA, // Windows
	BGRA, // Ubuntu
	ARGB,
	ABGR
};

enum class Sampler {
	Nearest,
	Linear
};

enum class ReturnCode {
	Good,
	KeyNotFound,
	ParsingFailure
};

// A handle to a model.
class ModelImpl;
using Model = std::shared_ptr<ModelImpl>;

// A handle to a multi-threaded rendering context.
class RendererImpl;
using Renderer = std::shared_ptr<RendererImpl>;

// A handle to a window.
//  The Window wraps itself around native window backends to abstract away platform specific details.
//  It also makes it easy to load and use a graphical interface using the optional component system.
class DsrWindow;
using Window = std::shared_ptr<DsrWindow>;

// A handle to a GUI component.
//   Components are an abstraction for graphical user interfaces, which might not always be powerful enough.
//   * If you're making something advanced that components cannot do,
//     you can also use draw calls and input events directly against the window without using Component.
class VisualComponent;
using Component = std::shared_ptr<VisualComponent>;

// A handle to a GUI theme.
//   Themes describes the visual appearance of an interface.
//   By having more than one theme for your interface, you can let the user select one.
class VisualThemeImpl;
using VisualTheme = std::shared_ptr<VisualThemeImpl>;

// A handle to a raster font
class RasterFontImpl;
using RasterFont = std::shared_ptr<RasterFontImpl>;

// A handle to a media machine.
//   Media machines can be used to generate, filter and analyze images.
//   Everything running in a media machine is guaranteed to be 100% deterministic to the last bit.
//     This reduces the amount of code where maintenance has to be performed during porting.
//     It also means that any use of float or double is forbidden.
struct VirtualMachine;
struct MediaMachine : IMPL_ACCESS std::shared_ptr<VirtualMachine> {
	MediaMachine(); // Defaults to null
IMPL_ACCESS:
	explicit MediaMachine(const std::shared_ptr<VirtualMachine>& machine);
};

// Images
// Points to a buffer region holding at least height * stride bytes.
// Each row contains:
//   * A number of visible pixels
//   * A number of unused bytes
//     New or cloned images have their stride aligned to 16-bytes
//       Stride is the number of bytes from the start of one row to the next
//     Sub-images have the same stride and buffer as their parent
//       Some unused pixels may be visible somewhere else

// 8-bit unsigned integer grayscale image
class ImageU8Impl;
struct ImageU8 : IMPL_ACCESS std::shared_ptr<ImageU8Impl> {
	ImageU8(); // Defaults to null
IMPL_ACCESS:
	explicit ImageU8(const std::shared_ptr<ImageU8Impl>& image);
	explicit ImageU8(const ImageU8Impl& image);
};
// Invariant:
//    * Each row's start and stride is aligned with 16-bytes in memory (16-byte = 16 pixels)
//      This allow reading a full SIMD vector at each row's end without violating memory bounds
//    * No other image can displays pixels from its padding
//      This allow writing a full SIMD vector at each row's end without making visible changes outside of the bound
struct AlignedImageU8 : public ImageU8 {
	AlignedImageU8() {} // Defaults to null
IMPL_ACCESS:
	explicit AlignedImageU8(const std::shared_ptr<ImageU8Impl>& image) : ImageU8(image) {}
	explicit AlignedImageU8(const ImageU8Impl& image) : ImageU8(image) {}
};

// 16-bit unsigned integer grayscale image
class ImageU16Impl;
struct ImageU16 : IMPL_ACCESS std::shared_ptr<ImageU16Impl> {
	ImageU16(); // Defaults to null
IMPL_ACCESS:
	explicit ImageU16(const std::shared_ptr<ImageU16Impl>& image);
	explicit ImageU16(const ImageU16Impl& image);
};
// Invariant:
//    * Each row's start and stride is aligned with 16-bytes in memory (16-byte = 16 pixels)
//      This allow reading a full SIMD vector at each row's end without violating memory bounds
//    * No other image can displays pixels from its padding
//      This allow writing a full SIMD vector at each row's end without making visible changes outside of the bound
struct AlignedImageU16 : public ImageU16 {
	AlignedImageU16() {} // Defaults to null
IMPL_ACCESS:
	explicit AlignedImageU16(const std::shared_ptr<ImageU16Impl>& image) : ImageU16(image) {}
	explicit AlignedImageU16(const ImageU16Impl& image) : ImageU16(image) {}
};

// 32-bit floating-point grayscale image
class ImageF32Impl;
struct ImageF32 : IMPL_ACCESS std::shared_ptr<ImageF32Impl> {
	ImageF32(); // Defaults to null
IMPL_ACCESS:
	explicit ImageF32(const std::shared_ptr<ImageF32Impl>& image);
	explicit ImageF32(const ImageF32Impl& image);
};
// Invariant:
//    * Each row's start and stride is aligned with 16-bytes in memory (16-byte = 4 pixels)
//      This allow reading a full SIMD vector at each row's end without violating memory bounds
//    * No other image can displays pixels from its padding
//      This allow writing a full SIMD vector at each row's end without making visible changes outside of the bound
struct AlignedImageF32 : public ImageF32 {
	AlignedImageF32() {} // Defaults to null
IMPL_ACCESS:
	explicit AlignedImageF32(const std::shared_ptr<ImageF32Impl>& image) : ImageF32(image) {}
	explicit AlignedImageF32(const ImageF32Impl& image) : ImageF32(image) {}
};

// 4x8-bit unsigned integer RGBA color image
class ImageRgbaU8Impl;
struct ImageRgbaU8 : IMPL_ACCESS std::shared_ptr<ImageRgbaU8Impl> {
	ImageRgbaU8(); // Defaults to null
IMPL_ACCESS:
	explicit ImageRgbaU8(const std::shared_ptr<ImageRgbaU8Impl>& image);
	explicit ImageRgbaU8(const ImageRgbaU8Impl& image);
};
// Invariant:
//    * Each row's start and stride is aligned with 16-bytes in memory (16-byte = 4 pixels)
//      This allow reading a full SIMD vector at each row's end without violating memory bounds
//    * No other image can displays pixels from its padding
//      This allow writing a full SIMD vector at each row's end without making visible changes outside of the bound
struct AlignedImageRgbaU8 : public ImageRgbaU8 {
	AlignedImageRgbaU8() {} // Defaults to null
IMPL_ACCESS:
	explicit AlignedImageRgbaU8(const std::shared_ptr<ImageRgbaU8Impl>& image) : ImageRgbaU8(image) {}
	explicit AlignedImageRgbaU8(const ImageRgbaU8Impl& image) : ImageRgbaU8(image) {}
};
// Invariant:
//    * Using the default RGBA pack order
//      This removes the need to implement filters for different pack orders when RGBA can be safely assumed
//      Just use AlignedImageRgbaU8 if channels don't have to be aligned
struct OrderedImageRgbaU8 : public AlignedImageRgbaU8 {
	OrderedImageRgbaU8() {} // Defaults to null
IMPL_ACCESS:
	explicit OrderedImageRgbaU8(const std::shared_ptr<ImageRgbaU8Impl>& image) : AlignedImageRgbaU8(image) {}
	explicit OrderedImageRgbaU8(const ImageRgbaU8Impl& image) : AlignedImageRgbaU8(image) {}
};

}

#endif
