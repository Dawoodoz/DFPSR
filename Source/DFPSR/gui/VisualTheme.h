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

// TODO: Move to the API folder once themes are easy to create.

// Construction
VisualTheme theme_create(const ReadableString& mediaCode, const ReadableString& themeCode);
VisualTheme theme_getDefault();

// Get a scalable image by name from the theme.
MediaMethod theme_getScalableImage(const VisualTheme& theme, const ReadableString &name);

// Post-condition: Returns true if argumentName was identified and assigned as input to inputIndex of methodIndex in machine.
bool theme_assignMediaMachineArguments(const VisualTheme& theme, MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName);

}

#endif
