// zlib open source license
//
// Copyright (c) 2018 to 2022 David Forsgren Piuva
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
#include "../api/mediaMachineAPI.h"
#include "../api/configAPI.h"

namespace dsr {

// The default theme
//   Copy, modify and compile with theme_create to get a custom theme
static const ReadableString defaultMediaMachineCode =
UR"QUOTE(
# Helper methods
BEGIN: generate_rounded_rectangle
	# Dimensions of the result image
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	# The subtracted offset from the radius to create a border on certain channels
	INPUT: FixedPoint, border
	# The whole pixel radius from center points to the end of the image
	INPUT: FixedPoint, rounding
	# Create the result image
	OUTPUT: ImageU8, resultImage
	CREATE: resultImage, width, height
	# Limit outer radius to half of the image's minimum dimension
	MIN: radius<FixedPoint>, width, height
	MUL: radius, radius, 0.5
	MIN: radius, radius, rounding
	ROUND: radius, radius
	# Place the inner radius for drawing
	SUB: innerRadius<FixedPoint>, rounding, border
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
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	INPUT: FixedPoint, pressed
	INPUT: FixedPoint, border
	INPUT: FixedPoint, rounding
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
	CALL: generate_rounded_rectangle, lumaImage<ImageU8>, width, height, border, rounding
	MUL: lumaImage, lumaImage, patternImage, 0.003921569
	CALL: generate_rounded_rectangle, visImage<ImageU8>, width, height, 0, rounding
	MUL: redImage<ImageU8>, lumaImage, normRed
	MUL: greenImage<ImageU8>, lumaImage, normGreen
	MUL: blueImage<ImageU8>, lumaImage, normBlue
	PACK_RGBA: resultImage, redImage, greenImage, blueImage, visImage
END:

BEGIN: Button
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	INPUT: FixedPoint, pressed
	INPUT: FixedPoint, border
	INPUT: FixedPoint, rounding
	OUTPUT: ImageRgbaU8, colorImage
	CALL: generate_rounded_button, colorImage, width, height, red, green, blue, pressed, border, rounding
END:

BEGIN: ListBox
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	INPUT: FixedPoint, border
	OUTPUT: ImageRgbaU8, colorImage
	CREATE: colorImage, width, height
	ADD: b2<FixedPoint>, border, border
	SUB: w2<FixedPoint>, width, b2
	SUB: h2<FixedPoint>, height, b2
	RECTANGLE: colorImage, border, border, w2, h2, red, green, blue, 255
END:

BEGIN: VerticalScrollBackground
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
	INPUT: FixedPoint, border
	OUTPUT: ImageRgbaU8, colorImage
	CREATE: colorImage, width, height
	ADD: b2<FixedPoint>, border, border
	SUB: w2<FixedPoint>, width, b2
	SUB: h2<FixedPoint>, height, b2
	RECTANGLE: colorImage, border, border, w2, h2, red, green, blue, 255
END:
)QUOTE";

// Using *.ini files for storing style settings as a simple start.
//   A more advanced system will be used later.
static const ReadableString defaultStyleSettings =
UR"QUOTE(
	border = 2
	; Fall back on the Button method if a component's class could not be recognized.
	method = "Button"
	[Button]
		rounding = 12
	[ListBox]
		method = "ListBox"
	[VerticalScrollKnob]
		rounding = 8
	[VerticalScrollBackground]
		method = "VerticalScrollBackground"
	[ScrollTop]
		rounding = 5
	[ScrollBottom]
		rounding = 5
	[Panel]
		border = 1
		method = "Panel"
)QUOTE";

template <typename V>
struct KeywordEntry {
	String key;
	V value;
	KeywordEntry(const ReadableString &key, const V &value)
	: key(key), value(value) {}
};

#define FOR_EACH_COLLECTION(LOCATION, MACRO_NAME) \
	MACRO_NAME(LOCATION monochromeImages) \
	MACRO_NAME(LOCATION colorImages) \
	MACRO_NAME(LOCATION scalars) \
	MACRO_NAME(LOCATION strings)

#define RETURN_TRUE_IF_SETTING_EXISTS(COLLECTION) \
	for (int64_t i = 0; i < COLLECTION.length(); i++) { \
		if (string_caseInsensitiveMatch(COLLECTION[i].key, key)) { \
			return true; \
		} \
	}
struct ClassSettings {
	String className; // Following a line with [className] in the *.ini configuration file.
	List<KeywordEntry<AlignedImageU8>> monochromeImages;
	List<KeywordEntry<OrderedImageRgbaU8>> colorImages;
	List<KeywordEntry<FixedPoint>> scalars;
	List<KeywordEntry<String>> strings;
	// TODO: Store strings for paths and method names.
	ClassSettings(const ReadableString &className)
	: className(className) {}
	bool keyExists(const ReadableString &key) {
		FOR_EACH_COLLECTION(this->, RETURN_TRUE_IF_SETTING_EXISTS)
		return false;
	}
	void setVariable(const ReadableString &key, const ReadableString &value) {
		if (this->keyExists(key)) { throwError(U"The property ", key, U" was defined multiple times in ", className, U"\n"); }
		// TODO: Check what type the value has and select the correct list for it.
		//       Try to be consistent with how images are embedded into the layout files, but allow embedding and loading monochrome images as well.
		//       Let the parsing of settings have a temporary pool of RGBA images,
		//       so that one does not have to load the same image multiple times when extracting channels,
		//       and so that one can reuse memory when different components are using parts of the same atlas image.
		//this->monochromeImages.pushConstruct(key, value);
		//this->colorImages.pushConstruct(key, value);
		if (value[0] == U'\"') {
			this->strings.pushConstruct(key, string_unmangleQuote(value));
		} else {
			this->scalars.pushConstruct(key, FixedPoint::fromText(value));
		}
	}
	// Post-condition: Returns true iff the key was found for the expected type.
	// Side-effect: Writes the value of the found key iff found.
	bool getString(String &target, const ReadableString &key) {
		for (int64_t i = 0; i < this->strings.length(); i++) {
			if (string_caseInsensitiveMatch(this->strings[i].key, key)) {
				target = this->strings[i].value;
				return true;
			}
		}
		return false;
	}
};

// TODO: Make it easy for visual components to ask the theme for additional resources such as custom fonts, text offset from pressing buttons and fixed dimensions for scroll lists to match fixed-size images.
class VisualThemeImpl {
public:
	MediaMachine machine;
	List<ClassSettings> settings;
	int32_t getClassIndex(const ReadableString& className) {
		for (int64_t i = 0; i < this->settings.length(); i++) { if (string_caseInsensitiveMatch(this->settings[i].className, className)) { return i; } }
		settings.pushConstruct(className);
		return settings.length() - 1;
	}
	VisualThemeImpl(const ReadableString& mediaCode, const ReadableString &styleSettings) : machine(machine_create(mediaCode)) {
		this->settings.pushConstruct(U"default");
		config_parse_ini(styleSettings, [this](const ReadableString& block, const ReadableString& key, const ReadableString& value) {
			int32_t classIndex = (string_length(block) == 0) ? 0 : this->getClassIndex(block);
			this->settings[classIndex].setVariable(key, value);
		});
	}
	// Destructor
	virtual ~VisualThemeImpl() {}
};

/* TODO: Find a better way to debug the content of a theme that can actually show the images.
static String& string_toStreamIndented(String &target, const ImageU8 &image, const ReadableString &indentation) {
	string_append(target, indentation, U"ImageU8 of ", image_getWidth(image), U"x", image_getHeight(image), U" pixels\n");
	return target;
}

static String& string_toStreamIndented(String &target, const ImageRgbaU8 &image, const ReadableString &indentation) {
	string_append(target, indentation, U"ImageRgbaU8 of ", image_getWidth(image), U"x", image_getHeight(image), U" pixels\n");
	return target;
}

#define PRINT_SETTINGS(COLLECTION) \
	for (int64_t i = 0; i < COLLECTION.length(); i++) { \
		string_append(target, indentation, U"\t", COLLECTION[i].key, U" = ", COLLECTION[i].value, U"\n"); \
	}
static String& string_toStreamIndented(String &target, const ClassSettings &settings, const ReadableString &indentation) {
	string_append(target, indentation, U"[", settings.className, U"]\n");
	FOR_EACH_COLLECTION(settings., PRINT_SETTINGS)
	return target;
}

static String& string_toStreamIndented(String &target, const VisualTheme &theme, const ReadableString &indentation) {
	if (!theme.get()) {
		string_append(target, indentation, U"Non-existing visual theme");
	} else {
		string_append(target, indentation, U"Theme:\n");
		for (int c = 0; c < theme->settings.length(); c++) {
			string_toStreamIndented(target, theme->settings[c], indentation + "\t");
		}
	}
	return target;
}
*/

static VisualTheme defaultTheme;
VisualTheme theme_getDefault() {
	if (!(defaultTheme.get())) {
		defaultTheme = theme_create(defaultMediaMachineCode, defaultStyleSettings);
	}
	return defaultTheme;
}

VisualTheme theme_create(const ReadableString &mediaCode, const ReadableString &styleSettings) {
	return std::make_shared<VisualThemeImpl>(mediaCode, styleSettings);
}

MediaMethod theme_getScalableImage(const VisualTheme &theme, const ReadableString &className) {
	if (!theme.get()) {
		throwError(U"theme_getScalableImage: Can't get scalable image from a non-existing theme!\n");
	}
	int classIndex = theme->getClassIndex(className);
	if (classIndex == -1) {
		//printText(theme, U"\n");
		throwError(U"theme_getScalableImage: Can't find any style class named ", className, U" in the given theme!\n");
	}
	// Try to get the method's name from the component's class settings,
	// and fall back on the class name itself if not found in neither the class settings nor the common default settings.
	String methodName;
	if (!theme->settings[classIndex].getString(methodName, U"method")) {
		if (!theme->settings[0].getString(methodName, U"method")) {
			//printText(theme, U"\n");
			throwError(U"The property \"method\" could not be found from the style class ", className, U", nor in the default settings!\n");
		}
	}
	return machine_getMethod(theme->machine, methodName, classIndex);
}

static bool assignMediaMachineArguments(ClassSettings settings, MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName) {
	// Search for argumentName in monochromeImages.
	for (int64_t i = 0; i < settings.monochromeImages.length(); i++) {
		if (string_caseInsensitiveMatch(settings.monochromeImages[i].key, argumentName)) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, settings.monochromeImages[i].value);
			return true;
		}
	}
	// Search for argumentName in colorImages.
	for (int64_t i = 0; i < settings.colorImages.length(); i++) {
		if (string_caseInsensitiveMatch(settings.colorImages[i].key, argumentName)) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, settings.colorImages[i].value);
			return true;
		}
	}
	// Search for argumentName in scalars.
	for (int64_t i = 0; i < settings.scalars.length(); i++) {
		if (string_caseInsensitiveMatch(settings.scalars[i].key, argumentName)) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, settings.scalars[i].value);
			return true;
		}
	}
	return false;
}

bool theme_assignMediaMachineArguments(const VisualTheme &theme, int32_t contextIndex, MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName) {
	if (!theme.get()) { return false; }
	// Check in the context first.
	if (contextIndex > 0 && assignMediaMachineArguments(theme->settings[contextIndex], machine, methodIndex, inputIndex, argumentName)) {
		return true;
	} else {
		// If not found in the context, check in the default settings.
		return assignMediaMachineArguments(theme->settings[0], machine, methodIndex, inputIndex, argumentName);
	}
}

}
