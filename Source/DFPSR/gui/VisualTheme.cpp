// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
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

#include <stdint.h>
#include "VisualTheme.h"
#include "../api/imageAPI.h"
#include "../api/drawAPI.h"

namespace dsr {

// The default theme
//   Copy and modify and compile with theme_create to get a custom theme
static const ReadableString defaultThemeCode =
UR"QUOTE(
# Helper methods
BEGIN: generate_rounded_rectangle
	# Dimensions of the result image
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	# The whole pixel radius from center points to the end of the image
	INPUT: FixedPoint, corner
	# The subtracted offset from the radius to create a border on certain channels
	INPUT: FixedPoint, border
	# Create the result image
	OUTPUT: ImageU8, resultImage
	CREATE: resultImage, width, height
	# Limit outer radius to half of the image's minimum dimension
	MIN: radius<FixedPoint>, width, height
	MUL: radius, radius, 0.5
	MIN: radius, radius, corner
	ROUND: radius, radius
	# Place the inner radius for drawing
	SUB: innerRadius<FixedPoint>, corner, border
	# Use +- 0.5 pixel offsets for fake anti-aliasing
	ADD: radiusOut<FixedPoint>, innerRadius, 0.5
	ADD: radiusIn<FixedPoint>, innerRadius, -0.5
	# Calculate dimensions for drawing
	SUB: w2<FixedPoint>, width, radius
	SUB: w3<FixedPoint>, w2, radius
	SUB: w4<FixedPoint>, width, border
	SUB: w4, w4, border
	SUB: h2<FixedPoint>, height, radius
	SUB: h3<FixedPoint>, h2, radius
	SUB: r2<FixedPoint>, radius, border
	# Draw
	FADE_REGION_RADIAL: resultImage,   0,  0,  radius, radius,  radius, radius,  radiusIn, 255,  radiusOut, 0
	FADE_REGION_RADIAL: resultImage,  w2,  0,  radius, radius,       0, radius,  radiusIn, 255,  radiusOut, 0
	FADE_REGION_RADIAL: resultImage,   0, h2,  radius, radius,  radius,      0,  radiusIn, 255,  radiusOut, 0
	FADE_REGION_RADIAL: resultImage,  w2, h2,  radius, radius,       0,      0,  radiusIn, 255,  radiusOut, 0
	RECTANGLE: resultImage, radius, border, w3, r2, 255
	RECTANGLE: resultImage, radius, h2, w3, r2, 255
	RECTANGLE: resultImage, border, radius, w4, h3, 255
END:

BEGIN: generate_rounded_button
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, pressed
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	INPUT: FixedPoint, radius
	INPUT: FixedPoint, border
	OUTPUT: ImageRgbaU8, resultImage
	# Scale by 2 / 255 so that 127.5 represents full intensity in patternImage
	MUL: normRed<FixedPoint>, red, 0.007843138
	MUL: normGreen<FixedPoint>, green, 0.007843138
	MUL: normBlue<FixedPoint>, blue, 0.007843138
	CREATE: patternImage<ImageU8>, width, height
	MUL: pressDarknessHigh<FixedPoint>, pressed, 80
	MUL: pressDarknessLow<FixedPoint>, pressed, 10
	SUB: highLuma<FixedPoint>, 150, pressDarknessHigh
	SUB: lowLuma<FixedPoint>, 100, pressDarknessLow
	FADE_LINEAR: patternImage,  0, 0, highLuma,  0, height, lowLuma
	CALL: generate_rounded_rectangle, lumaImage<ImageU8>, width, height, radius, border
	MUL: lumaImage, lumaImage, patternImage, 0.003921569
	CALL: generate_rounded_rectangle, visImage<ImageU8>, width, height, radius, 0
	MUL: redImage<ImageU8>, lumaImage, normRed
	MUL: greenImage<ImageU8>, lumaImage, normGreen
	MUL: blueImage<ImageU8>, lumaImage, normBlue
	PACK_RGBA: resultImage, redImage, greenImage, blueImage, visImage
END:

BEGIN: Button
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, pressed
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	OUTPUT: ImageRgbaU8, colorImage
	CALL: generate_rounded_button, colorImage, width, height, pressed, red, green, blue, 12, 2
END:

BEGIN: ScrollButton
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, pressed
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	OUTPUT: ImageRgbaU8, colorImage
	CALL: generate_rounded_button, colorImage, width, height, pressed, red, green, blue, 5, 2
END:

BEGIN: VerticalScrollBar
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	OUTPUT: ImageRgbaU8, colorImage
	CREATE: visImage<ImageU8>, width, height
	CREATE: lumaImage<ImageU8>, width, height
	FADE_LINEAR: visImage, 0, 0, 128, width, 0, 0
	PACK_RGBA: colorImage, 0, 0, 0, visImage
END:

BEGIN: Panel
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	OUTPUT: ImageRgbaU8, colorImage
	CREATE: colorImage, width, height
	SUB: w2<FixedPoint>, width, 2
	SUB: h2<FixedPoint>, height, 2
	RECTANGLE: colorImage, 1, 1, w2, h2, red, green, blue, 255
END:

BEGIN: ListBox
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	OUTPUT: ImageRgbaU8, colorImage
	CREATE: colorImage, width, height
	SUB: w2<FixedPoint>, width, 4
	SUB: h2<FixedPoint>, height, 4
	RECTANGLE: colorImage, 2, 2, w2, h2, red, green, blue, 255
END:
)QUOTE";

class VisualThemeImpl {
public:
	MediaMachine machine;
	// Constructor
	explicit VisualThemeImpl(const ReadableString& mediaCode) : machine(machine_create(mediaCode)) {}
	// Destructor
	virtual ~VisualThemeImpl() {}
};

static VisualTheme defaultTheme;
VisualTheme theme_getDefault() {
	if (!(defaultTheme.get())) {
		defaultTheme = theme_create(defaultThemeCode);
	}
	return defaultTheme;
}

VisualTheme theme_create(const ReadableString& mediaCode) {
	return std::make_shared<VisualThemeImpl>(mediaCode);
}

MediaMethod theme_getScalableImage(const VisualTheme& theme, const ReadableString &name) {
	return machine_getMethod(theme->machine, name);
}

}
