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

#include "Panel.h"

using namespace dsr;

PERSISTENT_DEFINITION(Panel)

void Panel::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"Solid");
	target.declareAttribute(U"Plain");
	target.declareAttribute(U"Color");
	target.declareAttribute(U"BackgroundClass");
}

Persistent* Panel::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Solid")) {
		return &(this->solid);
	} else if (string_caseInsensitiveMatch(name, U"Plain")) {
		return &(this->plain);
	} else if (string_caseInsensitiveMatch(name, U"Color") || string_caseInsensitiveMatch(name, U"BackColor")) {
		// Both color and backcolor is accepted as names for the only color.
		return &(this->color);
	} else if (string_caseInsensitiveMatch(name, U"Class") || string_caseInsensitiveMatch(name, U"BackgroundClass")) {
		return &(this->backgroundClass);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

Panel::Panel() {}

bool Panel::isContainer() const {
	return true;
}

void Panel::generateGraphics() {
	int width = this->location.width();
	int height = this->location.height();
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	if (!this->hasImages) {
		completeAssets();
		component_generateImage(this->theme, this->background, width, height, this->color.value.red, this->color.value.green, this->color.value.blue)(this->imageBackground);
		this->hasImages = true;
	}
}

// Fill the background with a solid color
void Panel::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	if (this->solid.value) {
		if (this->plain.value) {
			draw_rectangle(targetImage, relativeLocation, ColorRgbaI32(this->color.value, 255));
		} else {
			this->generateGraphics();			
			if (this->background_filter == 1) {
				draw_alphaFilter(targetImage, this->imageBackground, relativeLocation.left(), relativeLocation.top());
			} else {
				draw_copy(targetImage, this->imageBackground, relativeLocation.left(), relativeLocation.top());
			}
		}
	}
}

void Panel::loadTheme(const VisualTheme &theme) {
	this->finalBackgroundClass = theme_selectClass(theme, this->backgroundClass.value, U"Panel");
	this->background = theme_getScalableImage(theme, this->finalBackgroundClass);
	this->background_filter = theme_getInteger(theme, this->finalBackgroundClass, U"Filter", 0);
}

void Panel::changedTheme(VisualTheme newTheme) {
	this->loadTheme(newTheme);
	this->hasImages = false;
}

void Panel::completeAssets() {
	if (this->background.methodIndex == -1) {
		this->loadTheme(theme_getDefault());
	}
}

void Panel::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
	}
}

void Panel::changedAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"BackgroundClass")) {
		// Update from the theme if the theme class has changed.
		this->changedTheme(this->getTheme());
	} else if (!string_caseInsensitiveMatch(name, U"Visible")) {
		this->hasImages = false;
	}
	VisualComponent::changedAttribute(name);
}
