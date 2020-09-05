// zlib open source license
//
// Copyright (c) 2020 David Forsgren Piuva
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

#include "Label.h"
#include <math.h>

using namespace dsr;

PERSISTENT_DEFINITION(Label)

void Label::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"Color");
	target.declareAttribute(U"Opacity");
	target.declareAttribute(U"Text");
}

Persistent* Label::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Color")) {
		return &(this->color);
	} else if (string_caseInsensitiveMatch(name, U"Opacity")) {
		return &(this->opacity);
	} else if (string_caseInsensitiveMatch(name, U"Text")) {
		return &(this->text);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

Label::Label() {}

bool Label::isContainer() const {
	return true;
}

void Label::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	completeAssets();
	if (string_length(this->text.value) > 0) {
		// Uncomment to draw a white background for debugging
		//draw_rectangle(targetImage, relativeLocation, ColorRgbaI32(255, 255, 255, 255));
		// Print the text directly each time without buffering, because the biggest cost is to fill pixels
		font_printMultiLine(targetImage, this->font, this->text.value, relativeLocation, ColorRgbaI32(this->color.value, this->opacity.value));
	}
}

bool Label::pointIsInside(const IVector2D& pixelPosition) {
	// Labels are not clickable, because they have no clearly defined border drawn
	return false;
}

void Label::completeAssets() {
	if (this->font.get() == nullptr) {
		this->font = font_getDefault();
	}
}

