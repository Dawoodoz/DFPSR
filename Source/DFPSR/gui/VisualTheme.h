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

#ifndef DFPSR_GUI_VISUALTHEME
#define DFPSR_GUI_VISUALTHEME

#include "../api/mediaMachineAPI.h"

namespace dsr {



// ---------------------------------------- WARNING! ----------------------------------------
//
//    This API is not yet finished and may break backwards compatibility before completed.
//    It is not yet decided if the media machine will expose virtual assembly code,
//    which syntax to define themes using or if themes should be bundled together into archives.
//
// ------------------------------------------------------------------------------------------



// TODO: Move to the API folder once complete.

// Create a theme using a virtual machine with functions to call, style settings telling which functions to call with what arguments, and a path to load any non-embedded images from.
VisualTheme theme_createFromText(const MediaMachine &machine, const ReadableString &styleSettings, const ReadableString &fromPath);
// Create a theme using a virtual machine with functions to call, and a path to the style settings to load.
//   Any non-embedded images will be loaded relative to styleFilename's folder;
VisualTheme theme_createFromFile(const MediaMachine &machine, const ReadableString &styleFilename);
// Get a handle to the default theme.
VisualTheme theme_getDefault();

// Get a scalable image by name from the theme.
MediaMethod theme_getScalableImage(const VisualTheme &theme, const ReadableString &className);

// Called by VisualComponent to assign input arguments to functions in the media machine that were not given by the component itself.
// Post-condition: Returns true if argumentName was identified and assigned as input to inputIndex of methodIndex in machine.
bool theme_assignMediaMachineArguments(const VisualTheme &theme, int contextIndex, MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName);

}

#endif
