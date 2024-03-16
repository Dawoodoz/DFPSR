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

#include "Button.h"
#include <cmath>

using namespace dsr;

PERSISTENT_DEFINITION(Button)

void Button::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"BackColor");
	target.declareAttribute(U"ForeColor");
	target.declareAttribute(U"Text");
	target.declareAttribute(U"Padding");
	target.declareAttribute(U"BackgroundClass");
}

Persistent* Button::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Color") || string_caseInsensitiveMatch(name, U"BackColor")) {
		// The short Color alias refers to the back color in Buttons, because most buttons use black text.
		return &(this->backColor);
	} else if (string_caseInsensitiveMatch(name, U"ForeColor")) {
		return &(this->foreColor);
	} else if (string_caseInsensitiveMatch(name, U"Text")) {
		return &(this->text);
	} else if (string_caseInsensitiveMatch(name, U"Padding")) {
		return &(this->padding);
	} else if (string_caseInsensitiveMatch(name, U"Class") || string_caseInsensitiveMatch(name, U"BackgroundClass")) {
		return &(this->backgroundClass);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

Button::Button() {}

bool Button::isContainer() const {
	return false;
}

static OrderedImageRgbaU8 generateButtonImage(Button &button, MediaMethod imageGenerator, int pressed, int width, int height, ColorRgbI32 backColor, ColorRgbI32 foreColor, String text, RasterFont font) {
	// Create a scaled image
	OrderedImageRgbaU8 result;
 	component_generateImage(button.getTheme(), imageGenerator, width, height, backColor.red, backColor.green, backColor.blue, pressed)(result);
	if (string_length(text) > 0) {
		int left = (image_getWidth(result) - font_getLineWidth(font, text)) / 2;
		int top = (image_getHeight(result) - font_getSize(font)) / 2;
		if (pressed) {
			top += 1;
		}
		font_printLine(result, font, text, IVector2D(left, top), ColorRgbaI32(foreColor, 255));
	}
	return result;
}

void Button::generateGraphics() {
	int width = this->location.width();
	int height = this->location.height();
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	if (!this->hasImages) {
		completeAssets();
		this->imageUp = generateButtonImage(*this, this->button, 0, width, height, this->backColor.value, this->foreColor.value, this->text.value, this->font);
		this->imageDown = generateButtonImage(*this, this->button, 1, width, height, this->backColor.value, this->foreColor.value, this->text.value, this->font);
		this->hasImages = true;
	}
}

void Button::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	if (this->background_filter == 1) {
		draw_alphaFilter(targetImage, (this->pressed && this->inside) ? this->imageDown : this->imageUp, relativeLocation.left(), relativeLocation.top());
	} else {
		draw_copy(targetImage, (this->pressed && this->inside) ? this->imageDown : this->imageUp, relativeLocation.left(), relativeLocation.top());
	}
}

void Button::receiveMouseEvent(const MouseEvent& event) {
	if (event.mouseEventType == MouseEventType::MouseDown) {
		this->pressed = true;
	} else if (this->pressed && event.mouseEventType == MouseEventType::MouseUp) {
		this->pressed = false;
		if (this->inside) {
			this->callback_pressedEvent();
		}
	}
	this->inside = this->pointIsInside(event.position);
	VisualComponent::receiveMouseEvent(event);
}

bool Button::pointIsInside(const IVector2D& pixelPosition) {
	this->generateGraphics();
	// Get the point relative to the component instead of its direct container
	IVector2D localPoint = pixelPosition - this->location.upperLeft();
	// Sample opacity at the location
	return image_readPixel_border(this->imageUp, localPoint.x, localPoint.y).alpha > 127;
}

void Button::loadTheme(const VisualTheme &theme) {
	this->finalBackgroundClass = theme_selectClass(theme, this->backgroundClass.value, U"Button");
	this->button = theme_getScalableImage(theme, this->finalBackgroundClass);
	this->background_filter = theme_getInteger(theme, this->finalBackgroundClass, U"Filter", 0);
}

void Button::changedTheme(VisualTheme newTheme) {
	this->loadTheme(newTheme);
	this->hasImages = false;
}

void Button::completeAssets() {
	if (this->button.methodIndex == -1) {
		this->loadTheme(theme_getDefault());
	}
	if (this->font.get() == nullptr) {
		this->font = font_getDefault();
	}
}

void Button::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
	}
}

void Button::changedAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"BackgroundClass")) {
		// Update from the theme if the theme class has changed.
		this->changedTheme(this->getTheme());
	} else if (!string_caseInsensitiveMatch(name, U"Visible")) {
		this->hasImages = false;
	}
	VisualComponent::changedAttribute(name);
}

IVector2D Button::getDesiredDimensions() {
	this->completeAssets();
	int sizeAdder = this->padding.value * 2;
	return IVector2D(font_getLineWidth(this->font, this->text.value) + sizeAdder, font_getSize(this->font) + sizeAdder);
}
