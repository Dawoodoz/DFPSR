// zlib open source license
//
// Copyright (c) 2018 to 2023 David Forsgren Piuva
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
#include "../api/fileAPI.h"
#include "../api/imageAPI.h"
#include "../api/drawAPI.h"
#include "../api/mediaMachineAPI.h"
#include "../api/configAPI.h"
#include "../persistent/atomic/PersistentImage.h"

namespace dsr {

// The default theme
//   Copy, modify and compile with theme_create to get a custom theme
static const ReadableString defaultMediaMachineCode =
UR"QUOTE(
# Drawing a rounded rectangle to the alpha channel and a smaller rectangle reduced by the border argument for RGB channels.
#   This method for drawing edges works with alpha filtering enabled.
BEGIN: generate_rounded_rectangle
	# Dimensions of the result image.
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	# The subtracted offset from the radius to create a border on certain channels.
	INPUT: FixedPoint, border
	# The whole pixel radius from center points to the end of the image.
	INPUT: FixedPoint, rounding
	# Create the result image.
	OUTPUT: ImageU8, resultImage
	CREATE: resultImage, width, height
	# Limit outer radius to half of the image's minimum dimension.
	MIN: radius<FixedPoint>, width, height
	MUL: radius, radius, 0.5
	MIN: radius, radius, rounding
	ROUND: radius, radius
	# Place the inner radius for drawing.
	MIN: innerRadius<FixedPoint>, radius, rounding
	SUB: innerRadius, innerRadius, border
	# Use +- 0.5 pixel offsets for fake anti-aliasing.
	ADD: radiusOut<FixedPoint>, innerRadius, 0.5
	ADD: radiusIn<FixedPoint>, innerRadius, -0.5
	# Calculate dimensions for drawing.
	SUB: w2<FixedPoint>, width, radius
	SUB: w3<FixedPoint>, w2, radius
	SUB: w4<FixedPoint>, width, border
	SUB: w4, w4, border
	SUB: h2<FixedPoint>, height, radius
	SUB: h3<FixedPoint>, h2, radius
	SUB: r2<FixedPoint>, radius, border
	# Draw.
	FADE_REGION_RADIAL: resultImage,   0,  0,  radius, radius,  radius, radius,  radiusIn, 255,  radiusOut, 0
	FADE_REGION_RADIAL: resultImage,  w2,  0,  radius, radius,       0, radius,  radiusIn, 255,  radiusOut, 0
	FADE_REGION_RADIAL: resultImage,   0, h2,  radius, radius,  radius,      0,  radiusIn, 255,  radiusOut, 0
	FADE_REGION_RADIAL: resultImage,  w2, h2,  radius, radius,       0,      0,  radiusIn, 255,  radiusOut, 0
	RECTANGLE: resultImage, radius, border, w3, r2, 255
	RECTANGLE: resultImage, radius, h2, w3, r2, 255
	RECTANGLE: resultImage, border, radius, w4, h3, 255
END:

# Can be call directly to draw a component, or internally to add more effects.
# Black edges are created by default initializing the background to zeroes and drawing the inside smaller.
#   This method for drawing edges does not work if the resulting colorImage is drawn with alpha filtering, because setting alpha to zero means transparent.
BEGIN: HardRectangle
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
	# Scale by 2 / 255 so that 127.5 represents full intensity in patternImage.
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

BEGIN: VerticalScrollList
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

BEGIN: HorizontalScrollList
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	OUTPUT: ImageRgbaU8, colorImage
	CREATE: visImage<ImageU8>, width, height
	CREATE: lumaImage<ImageU8>, width, height
	FADE_LINEAR: visImage, 0, 0, 128, 0, height, 0
	PACK_RGBA: colorImage, 0, 0, 0, visImage
END:

BEGIN: TextBox
	INPUT: FixedPoint, width
	INPUT: FixedPoint, height
	INPUT: FixedPoint, red
	INPUT: FixedPoint, green
	INPUT: FixedPoint, blue
	INPUT: FixedPoint, border
	INPUT: FixedPoint, focused
	OUTPUT: ImageRgbaU8, colorImage
	ADD: intensity<FixedPoint>, 4, focused
	MUL: intensity, intensity, 0.2
	MUL: red, red, intensity
	MUL: green, green, intensity
	MUL: blue, blue, intensity
	CALL: HardRectangle, colorImage, width, height, red, green, blue, border
END:
)QUOTE";

// Using *.ini files for storing style settings as a simple start.
//   A more advanced system will be used later.
static const ReadableString defaultStyleSettings =
UR"QUOTE(
	border = 2
	method = "Button"
	; Fall back on the Button method if a component's class could not be recognized.
	[Button]
		rounding = 12
		filter = 1
		method = "Button"
	[ListBox]
		method = "HardRectangle"
	[TextBox]
		method = "TextBox"
	[VerticalScrollKnob]
		rounding = 8
	[HorizontalScrollKnob]
		rounding = 8
	[VerticalScrollList]
		method = "VerticalScrollList"
	[HorizontalScrollList]
		method = "HorizontalScrollList"
	[ScrollUp]
		rounding = 5
	[ScrollDown]
		rounding = 5
	[ScrollLeft]
		rounding = 5
	[ScrollRight]
		rounding = 5
	[Panel]
		border = 1
		method = "HardRectangle"
	[Toolbar]
		border = 1
		method = "HardRectangle"
	[MenuTop]
		border = 1
		method = "HardRectangle"
	[MenuSub]
		border = 1
		method = "HardRectangle"
	[MenuList]
		border = 1
		method = "HardRectangle"
)QUOTE";

template <typename V>
struct KeywordEntry {
	String key;
	V value;
	KeywordEntry(const ReadableString &key, const V &value)
	: key(key), value(value) {}
};

#define FOR_EACH_COLLECTION(LOCATION, MACRO_NAME) \
	MACRO_NAME(LOCATION colorImages) \
	MACRO_NAME(LOCATION scalars) \
	MACRO_NAME(LOCATION strings)

#define RETURN_TRUE_IF_SETTING_EXISTS(COLLECTION) \
	for (int i = 0; i < COLLECTION.length(); i++) { \
		if (string_caseInsensitiveMatch(COLLECTION[i].key, key)) { \
			return true; \
		} \
	}
struct ClassSettings {
	String className; // Following a line with [className] in the *.ini configuration file.
	List<KeywordEntry<PersistentImage>> colorImages;
	List<KeywordEntry<FixedPoint>> scalars;
	List<KeywordEntry<String>> strings;
	ClassSettings(const ReadableString &className)
	: className(className) {}
	bool keyExists(const ReadableString &key) {
		FOR_EACH_COLLECTION(this->, RETURN_TRUE_IF_SETTING_EXISTS)
		return false;
	}
	void setVariable(const ReadableString &key, const ReadableString &value, const ReadableString &fromPath) {
		if (this->keyExists(key)) { throwError(U"The property ", key, U" was defined multiple times in ", className, U"\n"); }
		DsrChar firstCharacter = value[0];
		if (firstCharacter == U'\"') {
			// Key = "text"
			this->strings.pushConstruct(key, string_unmangleQuote(value));
		} else {
			if (string_findFirst(value, U':') > -1) {
				// Key = File:Path
				// Key = WxH:Hexadecimals
				PersistentImage newImage;
				newImage.assignValue(value, fromPath);
				this->colorImages.pushConstruct(key, newImage);
			} else {
				// Key = Integer
				// Key = Integer.Decimals
				this->scalars.pushConstruct(key, FixedPoint::fromText(value));
			}
		}
	}
	// Post-condition: Returns true iff the key was found for the expected type.
	// Side-effect: Writes the value of the found key iff found.
	bool getString(String &target, const ReadableString &key) {
		for (int i = 0; i < this->strings.length(); i++) {
			if (string_caseInsensitiveMatch(this->strings[i].key, key)) {
				target = this->strings[i].value;
				return true;
			}
		}
		return false;
	}
	// Post-condition: Returns true iff the key was found for the expected type.
	// Side-effect: Writes the value of the found key iff found.
	bool getImage(PersistentImage &target, const ReadableString &key) {
		for (int i = 0; i < this->colorImages.length(); i++) {
			if (string_caseInsensitiveMatch(this->colorImages[i].key, key)) {
				target = this->colorImages[i].value;
				return true;
			}
		}
		return false;
	}
	// Post-condition: Returns true iff the key was found for the expected type.
	// Side-effect: Writes the value of the found key iff found.
	bool getScalar(FixedPoint &target, const ReadableString &key) {
		for (int i = 0; i < this->scalars.length(); i++) {
			if (string_caseInsensitiveMatch(this->scalars[i].key, key)) {
				target = this->scalars[i].value;
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
	int getClassIndex(const ReadableString& className) {
		for (int i = 0; i < this->settings.length(); i++) { if (string_caseInsensitiveMatch(this->settings[i].className, className)) { return i; } }
		return settings.pushConstructGetIndex(className);
	}
	VisualThemeImpl(const MediaMachine &machine, const ReadableString &styleSettings, const ReadableString &fromPath) : machine(machine) {
		this->settings.pushConstruct(U"default");
		config_parse_ini(styleSettings, [this, fromPath](const ReadableString& block, const ReadableString& key, const ReadableString& value) {
			int classIndex = (string_length(block) == 0) ? 0 : this->getClassIndex(block);
			this->settings[classIndex].setVariable(key, value, fromPath);
		});
	}
	// Destructor
	virtual ~VisualThemeImpl() {}
};

static VisualTheme defaultTheme;
VisualTheme theme_getDefault() {
	if (!(defaultTheme.get())) {
		defaultTheme = theme_createFromText(machine_create(defaultMediaMachineCode), defaultStyleSettings, file_getCurrentPath());
	}
	return defaultTheme;
}

VisualTheme theme_createFromText(const MediaMachine &machine, const ReadableString &styleSettings, const ReadableString &fromPath) {
	return std::make_shared<VisualThemeImpl>(machine, styleSettings, fromPath);
}

VisualTheme theme_createFromFile(const MediaMachine &machine, const ReadableString &styleFilename) {
	return theme_createFromText(machine, string_load(styleFilename), file_getRelativeParentFolder(styleFilename));
}

bool theme_exists(const VisualTheme &theme) {
	return theme.get() != nullptr;
}

int theme_getClassIndex(const VisualTheme &theme, const ReadableString &className) {
	if (!theme_exists(theme)) {
		return -1;
	} else if (string_length(className) == 0) {
		return 0;
	} else {
		int classIndex = theme->getClassIndex(className);
		return (classIndex == -1) ? 0 : classIndex;
	}
}

bool theme_class_exists(const VisualTheme &theme, const ReadableString &className) {
	return theme_getClassIndex(theme, className) > 0;
}

String theme_selectClass(const VisualTheme &theme, const ReadableString &suggestedClassName, const ReadableString &fallbackClassName) {
	return theme_class_exists(theme, suggestedClassName) ? suggestedClassName : fallbackClassName;
}

OrderedImageRgbaU8 theme_getImage(const VisualTheme &theme, const ReadableString &className, const ReadableString &settingName) {
	if (!theme.get()) {
		return OrderedImageRgbaU8();
	}
	int classIndex = theme->getClassIndex(className);
	PersistentImage result;
	if ((classIndex != -1 && theme->settings[classIndex].getImage(result, settingName))
	                     || (theme->settings[0].getImage(result, settingName))) {
		// If the class existed and it contained the setting or the setting could be found in the default class then return it.
		return result.value;
	} else {
		return OrderedImageRgbaU8();
	}
}

FixedPoint theme_getFixedPoint(const VisualTheme &theme, const ReadableString &className, const ReadableString &settingName, const FixedPoint &defaultValue) {
	if (!theme.get()) {
		return defaultValue;
	}
	int classIndex = theme->getClassIndex(className);
	FixedPoint result;
	if ((classIndex != -1 && theme->settings[classIndex].getScalar(result, settingName))
	                     || (theme->settings[0].getScalar(result, settingName))) {
		// If the class existed and it contained the setting or the setting could be found in the default class then return it.
		return result;
	} else {
		return defaultValue;
	}
}

int theme_getInteger(const VisualTheme &theme, const ReadableString &className, const ReadableString &settingName, const int &defaultValue) {
	return fixedPoint_round(theme_getFixedPoint(theme, className, settingName, FixedPoint::fromWhole(defaultValue)));
}

ReadableString theme_getString(const VisualTheme &theme, const ReadableString &className, const ReadableString &settingName, const ReadableString &defaultValue) {
	if (!theme.get()) {
		return defaultValue;
	}
	int classIndex = theme->getClassIndex(className);
	String result;
	if ((classIndex != -1 && theme->settings[classIndex].getString(result, settingName))
	                     || (theme->settings[0].getString(result, settingName))) {
		// If the class existed and it contained the setting or the setting could be found in the default class then return it.
		return result;
	} else {
		return defaultValue;
	}
}

MediaMethod theme_getScalableImage(const VisualTheme &theme, const ReadableString &className) {
	if (!theme.get()) {
		throwError(U"theme_getScalableImage: Can't get scalable image of class ", className, U" from a non-existing theme!\n");
	}
	int classIndex = theme->getClassIndex(className);
	String methodName;
	if ((classIndex != -1 && theme->settings[classIndex].getString(methodName, U"method"))
	                     || (theme->settings[0].getString(methodName, U"method"))) {
		// If the class existed and it contained the setting or the setting could be found in the default class then return it.
		return machine_getMethod(theme->machine, methodName, theme->getClassIndex(className));
	} else {
		throwError(U"theme_getScalableImage: Can't get scalable image of class ", className, U" because the setting did not exist in neither the class nor the default settings!\n");
		return MediaMethod();
	}
}

static bool assignMediaMachineArguments(ClassSettings settings, MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName) {
	// Search for argumentName in colorImages.
	for (int i = 0; i < settings.colorImages.length(); i++) {
		if (string_caseInsensitiveMatch(settings.colorImages[i].key, argumentName)) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, settings.colorImages[i].value.value);
			return true;
		}
	}
	// Search for argumentName in scalars.
	for (int i = 0; i < settings.scalars.length(); i++) {
		if (string_caseInsensitiveMatch(settings.scalars[i].key, argumentName)) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, settings.scalars[i].value);
			return true;
		}
	}
	// The media machine currently does not support strings.
	return false;
}

bool theme_assignMediaMachineArguments(const VisualTheme &theme, int contextIndex, MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName) {
	if (!theme.get()) { return false; }
	// Check in the context first, and then in the default settings.
	return (contextIndex > 0 && assignMediaMachineArguments(theme->settings[contextIndex], machine, methodIndex, inputIndex, argumentName))
	                         || assignMediaMachineArguments(theme->settings[0],            machine, methodIndex, inputIndex, argumentName);
}

}
