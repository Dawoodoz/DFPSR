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

#include "Toolbar.h"

using namespace dsr;

PERSISTENT_DEFINITION(Toolbar)

void Toolbar::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"Solid");
	target.declareAttribute(U"Plain");
	target.declareAttribute(U"Color");
	target.declareAttribute(U"Padding");
	target.declareAttribute(U"Spacing");
}

Persistent* Toolbar::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Solid")) {
		return &(this->solid);
	} else if (string_caseInsensitiveMatch(name, U"Plain")) {
		return &(this->plain);
	} else if (string_caseInsensitiveMatch(name, U"Color") || string_caseInsensitiveMatch(name, U"BackColor")) {
		// Both color and backcolor is accepted as names for the only color.
		return &(this->color);
	} else if (string_caseInsensitiveMatch(name, U"Padding")) {
		return &(this->padding);
	} else if (string_caseInsensitiveMatch(name, U"Spacing")) {
		return &(this->spacing);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

Toolbar::Toolbar() {}

bool Toolbar::isContainer() const {
	return true;
}

void Toolbar::generateGraphics() {
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
void Toolbar::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	if (this->solid.value) {
		if (this->plain.value) {
			draw_rectangle(targetImage, relativeLocation, ColorRgbaI32(this->color.value, 255));
		} else {
			this->generateGraphics();
			draw_copy(targetImage, this->imageBackground, relativeLocation.left(), relativeLocation.top());
		}
	}
}

void Toolbar::changedTheme(VisualTheme newTheme) {
	this->background = theme_getScalableImage(newTheme, U"Toolbar");
	this->hasImages = false;
}

void Toolbar::completeAssets() {
	if (this->background.methodIndex == -1) {
		this->background = theme_getScalableImage(theme_getDefault(), U"Toolbar");
	}
}

void Toolbar::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
	}
}

void Toolbar::changedAttribute(const ReadableString &name) {
	if (!string_caseInsensitiveMatch(name, U"Visible")) {
		this->hasImages = false;
	}
	VisualComponent::changedAttribute(name);
}

void Toolbar::updateLocationEvent(const IRect& oldLocation, const IRect& newLocation) {
	int widthStretch = this->region.right.getRatio() - this->region.left.getRatio();
	int heightStretch = this->region.bottom.getRatio() - this->region.top.getRatio();
	// Place members vertically if the toolbar mostly stretches vertically or if it has uniform stretch and is taller than wide.
	bool vertical = (widthStretch < heightStretch) || ((widthStretch == heightStretch) && (newLocation.width() < newLocation.height()));
	if (vertical) {
		// TODO: Add scroll buttons on the sides if there is not enough space for all child components.
		// Place each child component in order.
		//   Each child is created within a segmented region, but can choose to add more padding or limit its height for fine adjustments.
		int left = this->padding.value;
		int top = newLocation.top() + this->padding.value;
		int width = newLocation.width() - (this->padding.value * 2);
		for (int i = 0; i < this->getChildCount(); i++) {
			int height = this->children[i]->getDesiredDimensions().y;
			this->children[i]->applyLayout(IRect(left, top, width, height));
			top += height + this->spacing.value;
		}
	} else {
		// TODO: Add scroll buttons on the sides if there is not enough space for all child components.
		// Place each child component in order.
		//   Each child is created within a segmented region, but can choose to add more padding or limit its height for fine adjustments.
		int left = newLocation.left() + this->padding.value;
		int top = this->padding.value;
		int height = newLocation.height() - (this->padding.value * 2);
		for (int i = 0; i < this->getChildCount(); i++) {
			int width = this->children[i]->getDesiredDimensions().x;
			this->children[i]->applyLayout(IRect(left, top, width, height));
			left += width + this->spacing.value;
		}
	}
}
