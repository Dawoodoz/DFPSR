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
#include <math.h>

using namespace dsr;

PERSISTENT_DEFINITION(Button)

void Button::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"Color");
	target.declareAttribute(U"Text");
}

Persistent* Button::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Color")) {
		return &(this->color);
	} else if (string_caseInsensitiveMatch(name, U"Text")) {
		return &(this->text);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

Button::Button() {}

bool Button::isContainer() const {
	return true;
}

static OrderedImageRgbaU8 generateButtonImage(MediaMethod imageGenerator, int pressed, int width, int height, ColorRgbI32 backColor, String text, const std::shared_ptr<RasterFont>& font) {
	// Create a scaled image
	OrderedImageRgbaU8 result;
 	imageGenerator(width, height, pressed, backColor.red, backColor.green, backColor.blue)(result);
	if (text.length() > 0) {
		int left = (image_getWidth(result) - font->getLineWidth(text)) / 2;
		int top = (image_getHeight(result) - font->size) / 2;
		if (pressed) {
			top += 1;
		}
		font->printLine(result, text, IVector2D(left, top), ColorRgbaI32(0, 0, 0, 255));
	}
	return result;
}

void Button::generateGraphics() {
	int width = this->location.width();
	int height = this->location.height();
	if (!this->hasImages || this->lastWidth != width || this->lastHeight != height || !string_match(this->text.value, this->lastText) || this->color.value != this->lastColor) {
		completeAssets();
		this->imageUp = generateButtonImage(this->button, 0, width, height, this->color.value, this->text.value, this->font);
		this->imageDown = generateButtonImage(this->button, 1, width, height, this->color.value, this->text.value, this->font);
		this->lastWidth = width;
		this->lastHeight = height;
		this->lastText = this->text.value;
		this->lastColor = this->color.value;
		this->hasImages = true;
	}
}

void Button::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	draw_alphaFilter(targetImage, (this->pressed && this->inside) ? this->imageDown : this->imageUp, relativeLocation.left(), relativeLocation.top());
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
	return dsr::image_readPixel_border(this->imageUp, localPoint.x, localPoint.y).alpha > 127;
}

void Button::changedTheme(VisualTheme newTheme) {
	this->button = theme_getScalableImage(newTheme, U"Button");
	this->hasImages = false;
}

void Button::completeAssets() {
	if (this->button.methodIndex == -1) {
		this->button = theme_getScalableImage(theme_getDefault(), U"Button");
	}
	if (this->font.get() == nullptr) {
		this->font = font_getDefault();
	}
}
