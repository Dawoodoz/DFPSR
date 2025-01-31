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

#ifndef DFPSR_GUI_VISUALTHEME
#define DFPSR_GUI_VISUALTHEME

#include "../../api/mediaMachineAPI.h"
#include "componentStates.h"

namespace dsr {



// ---------------------------------------- WARNING! ----------------------------------------
//
//    This API is not yet finished and may break backwards compatibility before completed.
//    It is not yet decided if the media machine will expose virtual assembly code,
//    which syntax to define themes using or if themes should be bundled together into archives.
//
// ------------------------------------------------------------------------------------------



// TODO: Move to the API folder once complete.

// A handle to a GUI theme.
//   Themes describes the visual appearance of an interface.
//   By having more than one theme for your interface, you can let the user select one.
class VisualThemeImpl;
using VisualTheme = Handle<VisualThemeImpl>;

// Create a theme using a virtual machine with functions to call, style settings telling which functions to call with what arguments, and a path to load any non-embedded images from.
VisualTheme theme_createFromText(const MediaMachine &machine, const ReadableString &styleSettings, const ReadableString &fromPath);
// Create a theme using a virtual machine with functions to call, and a path to the style settings to load.
//   Any non-embedded images will be loaded relative to styleFilename's folder;
VisualTheme theme_createFromFile(const MediaMachine &machine, const ReadableString &styleFilename);
// Get a handle to the default theme.
VisualTheme theme_getDefault();

// Returns true iff theme exists, otherwise false.
bool theme_exists(const VisualTheme &theme);
// Returns the index of className in theme, 0 if it did not exist in theme or -1 if theme does not exist.
int theme_getClassIndex(const VisualTheme &theme, const ReadableString &className);
// Returns true iff className exists in theme.
bool theme_class_exists(const VisualTheme &theme, const ReadableString &className);
// Returns suggestedClassName if it exists in theme, fallbackClassName otherwise.
String theme_selectClass(const VisualTheme &theme, const ReadableString &suggestedClassName, const ReadableString &fallbackClassName);

// Getters for resources in the theme's *.ini configuration file.
//   If className is not found in the theme, the settings declared before the first class are used.
//     This allow unhandled components to have a generic look, get a pattern that signals unfinished work or leave it blank to terminate with an error when a class is not handled.
//   Settings after the class name within [] until the end of the file or next class belong to that class.

// Get the scalable image created using the media machine method name stored in the string setting "method" within className in theme.
//   If there is no method name assigned to "method" after [className], the "method" before the first [] block is used instead.
//   If no method is found at all, it means that the theme wants to throw an exception to alert the developer about a class that needs to be handled.
//   The matches for className and "method" are both case insensitive.
MediaMethod theme_getScalableImage(const VisualTheme &theme, const ReadableString &className);
// TODO: Create syntax for naming sub-image regions within the atlas to be used as icons without needing to load separate files.
// Get a fixed size image from className in theme using settingName, an empty handle if not found in theme or theme does not exist.
//   Looks for image assignments to settingName after [className] first, then before the first [] block, and then returns an empty handle if not found at all.
//   The matches for className and settingName are both case insensitive.
//   Use theme_getClassIndex if you are unsure of why an empty handle was returned, because it will not throw exceptions.
OrderedImageRgbaU8 theme_getImage(const VisualTheme &theme, const ReadableString &className, const ReadableString &settingName);
// Get a fixed-point value from className in theme using settingName, the default value if not found, or throw an exception if theme does not exist.
//   Looks for scalar assignments to settingName after [className] first, then before the first [] block, and then returns defaultValue if not found at all.
//   The matches for className and settingName are both case insensitive.
//   Use theme_getClassIndex if you are unsure of why defaultValue was returned, because it will not throw exceptions.
FixedPoint theme_getFixedPoint(const VisualTheme &theme, const ReadableString &className, const ReadableString &settingName, const FixedPoint &defaultValue);
// Get FixedPoint and truncate to an integer, which should leave 16 bits of useful range.
int theme_getInteger(const VisualTheme &theme, const ReadableString &className, const ReadableString &settingName, const int &defaultValue);
// Get a string from className in theme using settingName, the default value if not found, or throw an exception if theme does not exist.
//   Looks for string assignments to settingName after [className] first, then before the first [] block, and then returns defaultValue if not found at all.
//   The matches for className and settingName are both case insensitive.
//   Use theme_getClassIndex if you are unsure of why defaultValue was returned, because it will not throw exceptions.
ReadableString theme_getString(const VisualTheme &theme, const ReadableString &className, const ReadableString &settingName, const FixedPoint &defaultValue);

// Called by VisualComponent to assign input arguments to functions in the media machine that were not given by the component itself.
// Post-condition: Returns true if argumentName was identified and assigned as input to inputIndex of methodIndex in machine.
bool theme_assignMediaMachineArguments(const VisualTheme &theme, int contextIndex, MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName);

// Post-condition:
//   Returns a bit-mask for the direct states that have corresponding input arguments in the method.
//     Includes the componentState_focusDirect bit if the method has an input argument named "focused".
//     Includes the componentState_hoverDirect bit if the method has an input argument named "hover".
ComponentState theme_getStateListenerMask(const MediaMethod &scalableImage);

}

#endif
