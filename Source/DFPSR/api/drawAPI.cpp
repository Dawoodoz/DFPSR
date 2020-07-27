
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

#define DFPSR_INTERNAL_ACCESS

#include "imageAPI.h"
#include "drawAPI.h"
#include "../image/draw.h"
#include "../image/PackOrder.h"
#include "../image/internal/imageTemplate.h"
#include "../image/internal/imageInternal.h"

using namespace dsr;


// -------------------------------- Drawing shapes --------------------------------


void dsr::draw_rectangle(ImageU8& image, const IRect& bound, int color) {
	if (image) {
		imageImpl_draw_solidRectangle(*image, bound, color);
	}
}
void dsr::draw_rectangle(ImageF32& image, const IRect& bound, float color) {
	if (image) {
		imageImpl_draw_solidRectangle(*image, bound, color);
	}
}
void dsr::draw_rectangle(ImageRgbaU8& image, const IRect& bound, const ColorRgbaI32& color) {
	if (image) {
		imageImpl_draw_solidRectangle(*image, bound, color);
	}
}

void dsr::draw_line(ImageU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int color) {
	if (image) {
		imageImpl_draw_line(*image, x1, y1, x2, y2, color);
	}
}
void dsr::draw_line(ImageF32& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, float color) {
	if (image) {
		imageImpl_draw_line(*image, x1, y1, x2, y2, color);
	}
}
void dsr::draw_line(ImageRgbaU8& image, int32_t x1, int32_t y1, int32_t x2, int32_t y2, const ColorRgbaI32& color) {
	if (image) {
		imageImpl_draw_line(*image, x1, y1, x2, y2, color);
	}
}


// -------------------------------- Drawing images --------------------------------


#define DRAW_COPY_WRAPPER(TARGET_TYPE, SOURCE_TYPE) \
	void dsr::draw_copy(TARGET_TYPE& target, const SOURCE_TYPE& source, int32_t left, int32_t top) { \
		if (target && source) { \
			imageImpl_drawCopy(*target, *source, left, top); \
		} \
	}
DRAW_COPY_WRAPPER(ImageRgbaU8, ImageRgbaU8);
DRAW_COPY_WRAPPER(ImageU8, ImageU8);
DRAW_COPY_WRAPPER(ImageU16, ImageU16);
DRAW_COPY_WRAPPER(ImageF32, ImageF32);
DRAW_COPY_WRAPPER(ImageRgbaU8, ImageU8);
DRAW_COPY_WRAPPER(ImageRgbaU8, ImageU16);
DRAW_COPY_WRAPPER(ImageRgbaU8, ImageF32);
DRAW_COPY_WRAPPER(ImageU8, ImageU16);
DRAW_COPY_WRAPPER(ImageU8, ImageF32);
DRAW_COPY_WRAPPER(ImageU16, ImageU8);
DRAW_COPY_WRAPPER(ImageU16, ImageF32);
DRAW_COPY_WRAPPER(ImageF32, ImageU8);
DRAW_COPY_WRAPPER(ImageF32, ImageU16);

void dsr::draw_alphaFilter(ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top) {
	if (target && source) {
		imageImpl_drawAlphaFilter(*target, *source, left, top);
	}
}
void dsr::draw_maxAlpha(ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top, int32_t sourceAlphaOffset) {
	if (target && source) {
		imageImpl_drawMaxAlpha(*target, *source, left, top, sourceAlphaOffset);
	}
}
void dsr::draw_alphaClip(ImageRgbaU8& target, const ImageRgbaU8& source, int32_t left, int32_t top, int32_t threshold) {
	if (target && source) {
		imageImpl_drawAlphaClip(*target, *source, left, top, threshold);
	}
}
void dsr::draw_silhouette(ImageRgbaU8& target, const ImageU8& source, const ColorRgbaI32& color, int32_t left, int32_t top) {
	if (target && source) {
		imageImpl_drawSilhouette(*target, *source, color, left, top);
	}
}
void dsr::draw_higher(ImageU16& targetHeight, const ImageU16& sourceHeight, int32_t left, int32_t top, int32_t sourceHeightOffset) {
	if (targetHeight && sourceHeight) {
		imageImpl_drawHigher(*targetHeight, *sourceHeight, left, top, sourceHeightOffset);
	}
}
void dsr::draw_higher(ImageU16& targetHeight, const ImageU16& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  int32_t left, int32_t top, int32_t sourceHeightOffset) {
	if (targetHeight && sourceHeight && targetA && sourceA) {
		imageImpl_drawHigher(*targetHeight, *sourceHeight, *targetA, *sourceA, left, top, sourceHeightOffset);
	}
}
void dsr::draw_higher(ImageU16& targetHeight, const ImageU16& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  ImageRgbaU8& targetB, const ImageRgbaU8& sourceB, int32_t left, int32_t top, int32_t sourceHeightOffset) {
	if (targetHeight && sourceHeight && targetA && sourceA && targetB && sourceB) {
		imageImpl_drawHigher(*targetHeight, *sourceHeight, *targetA, *sourceA, *targetB, *sourceB, left, top, sourceHeightOffset);
	}
}
void dsr::draw_higher(ImageF32& targetHeight, const ImageF32& sourceHeight, int32_t left, int32_t top, float sourceHeightOffset) {
	if (targetHeight && sourceHeight) {
		imageImpl_drawHigher(*targetHeight, *sourceHeight, left, top, sourceHeightOffset);
	}
}
void dsr::draw_higher(ImageF32& targetHeight, const ImageF32& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  int32_t left, int32_t top, float sourceHeightOffset) {
	if (targetHeight && sourceHeight && targetA && sourceA) {
		imageImpl_drawHigher(*targetHeight, *sourceHeight, *targetA, *sourceA, left, top, sourceHeightOffset);
	}
}
void dsr::draw_higher(ImageF32& targetHeight, const ImageF32& sourceHeight, ImageRgbaU8& targetA, const ImageRgbaU8& sourceA,
  ImageRgbaU8& targetB, const ImageRgbaU8& sourceB, int32_t left, int32_t top, float sourceHeightOffset) {
	if (targetHeight && sourceHeight && targetA && sourceA && targetB && sourceB) {
		imageImpl_drawHigher(*targetHeight, *sourceHeight, *targetA, *sourceA, *targetB, *sourceB, left, top, sourceHeightOffset);
	}
}

