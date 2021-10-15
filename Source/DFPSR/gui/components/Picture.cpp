// zlib open source license
//
// Copyright (c) 2021 David Forsgren Piuva
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

#include "Picture.h"
#include "../../api/filterAPI.h"

using namespace dsr;

PERSISTENT_DEFINITION(Picture)

void Picture::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"Image");
	target.declareAttribute(U"ImagePressed");
	target.declareAttribute(U"Interpolation");
}

Persistent* Picture::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Image")) {
		return &(this->image);
	} else if (string_caseInsensitiveMatch(name, U"ImagePressed")) {
		return &(this->imagePressed);
	} else if (string_caseInsensitiveMatch(name, U"Interpolation")) {
		return &(this->interpolation);
	} else if (string_caseInsensitiveMatch(name, U"Clickable")) {
		return &(this->clickable);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

Picture::Picture() {}

bool Picture::isContainer() const {
	return false;
}

void Picture::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	if (image_exists(this->image.value)) {
		this->generateGraphics();
		draw_alphaFilter(targetImage, (this->pressed && this->inside && image_exists(this->finalImagePressed)) ? this->finalImagePressed : this->finalImage, relativeLocation.left(), relativeLocation.top());
	}
}

bool Picture::pointIsInside(const IVector2D& pixelPosition) {
	if (this->clickable.value) {
		this->generateGraphics();
		// Get the point relative to the component instead of its direct container
		IVector2D localPoint = pixelPosition - this->location.upperLeft();
		// Sample opacity at the location
		return dsr::image_readPixel_border(this->finalImage, localPoint.x, localPoint.y).alpha > 127;
	} else {
		return false;
	}
}

void Picture::receiveMouseEvent(const MouseEvent& event) {
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

void Picture::generateGraphics() {
	int width = this->location.width();
	int height = this->location.height();
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	if (!this->hasImages) {
		this->finalImage = filter_resize(this->image.value, this->interpolation.value ? Sampler::Linear : Sampler::Nearest, width, height);
		this->finalImagePressed = filter_resize(this->imagePressed.value, this->interpolation.value ? Sampler::Linear : Sampler::Nearest, width, height);
		this->hasImages = true;
	}
}

void Picture::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
	}
}

void Picture::changedAttribute(const ReadableString &name) {
	// If an attribute has changed then force the image to be redrawn
	this->hasImages = false;
}
