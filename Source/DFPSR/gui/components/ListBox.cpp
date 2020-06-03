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

#include "ListBox.h"
#include <math.h>

using namespace dsr;

PERSISTENT_DEFINITION(ListBox)

void ListBox::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"Color");
	target.declareAttribute(U"List");
	target.declareAttribute(U"SelectedIndex");
}

Persistent* ListBox::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Color")) {
		return &(this->color);
	} else if (string_caseInsensitiveMatch(name, U"List")) {
		return &(this->list);
	} else if (string_caseInsensitiveMatch(name, U"SelectedIndex")) {
		return &(this->selectedIndex);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

ListBox::ListBox() {}

bool ListBox::isContainer() const {
	return true;
}

static const int textBorderLeft = 6;
static const int textBorderTop = 4;
static const int scrollWidth = 16; // The width of the scroll bar
static const int scrollEndHeight = 14; // The height of upper and lower scroll buttons
static const int border = 1; // Scroll-bar edge thickness
static const int knobErosion = border; // Scroll-bar knob erosion

void ListBox::generateGraphics() {
	int width = this->location.width();
	int height = this->location.height();
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	if (!this->hasImages) {
		completeAssets();
	 	this->listBox(width, height, this->color.value.red, this->color.value.green, this->color.value.blue)(this->image);
		int verticalStep = this->font->size;
		int left = textBorderLeft;
		int top = textBorderTop;
		for (int64_t i = this->firstVisible; i < this->list.value.length() && top < height; i++) {
			ColorRgbaI32 textColor;
			if (i == this->pressedIndex) {
				textColor = ColorRgbaI32(255, 255, 255, 255);
			} else if (i == this->selectedIndex.value) {
				textColor = ColorRgbaI32(255, 255, 255, 255);
			} else {
				textColor = ColorRgbaI32(0, 0, 0, 255);
			}
			if (i == this->selectedIndex.value) {
				draw_rectangle(this->image, IRect(left, top, width - (textBorderLeft * 2), verticalStep), ColorRgbaI32(0, 0, 0, 255));
			}
			this->font->printLine(this->image, this->list.value[i], IVector2D(left, top), textColor);
			top += verticalStep;
		}
		if (this->hasVerticalScroll) {
			ColorRgbaI32 buttonColor = ColorRgbaI32(this->color.value, 255);
			ColorRgbaI32 barColor = ColorRgbaI32(this->color.value.red / 2, this->color.value.green / 2, this->color.value.blue / 2, 255);
			ColorRgbaI32 borderColor = ColorRgbaI32(0, 0, 0, 255);
			IRect whole = IRect(this->location.width() - scrollWidth, 0, scrollWidth, this->location.height());
			IRect upper = IRect(whole.left() + border, whole.top() + border, whole.width() - border * 2, scrollEndHeight - border * 2);
			IRect middle = IRect(whole.left() + border, whole.top() + scrollEndHeight + border, whole.width() - border * 2, whole.height() - (border + scrollEndHeight) * 2);
			IRect lower = IRect(whole.left() + border, whole.bottom() - scrollEndHeight + border, whole.width() - border * 2, scrollEndHeight - border * 2);
			IRect knob = this->getKnobLocation();
			// Scroll-bar
			draw_rectangle(this->image, whole, borderColor);
			draw_rectangle(this->image, upper, buttonColor);
			draw_rectangle(this->image, middle, barColor);
			draw_rectangle(this->image, lower, buttonColor);
			draw_rectangle(this->image, knob, buttonColor);
		}
		this->hasImages = true;
	}
}

void ListBox::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	draw_copy(targetImage, this->image, relativeLocation.left(), relativeLocation.top());
}

void ListBox::pressScrollBar(int64_t localY) {
	int64_t maxScroll = this->list.value.length() - this->getVisibleScrollRange();
	int64_t knobHeight = this->getKnobLocation().height();
	int64_t endDistance = scrollEndHeight + knobHeight / 2;
	int64_t barHeight = this->location.height() - (endDistance * 2);
	this->firstVisible = ((localY - endDistance) * maxScroll) / barHeight;
	this->limitScrolling();
	this->hasImages = false; // Force redraw
}

void ListBox::receiveMouseEvent(const MouseEvent& event) {
	bool supressEvent = false;
	this->inside = this->pointIsInside(event.position);
	bool onScrollBar = this->hasVerticalScroll && event.position.x >= this->location.width() - scrollWidth;
	int64_t maxIndex = this->list.value.length() - 1;
	int64_t hoverIndex = this->firstVisible + ((event.position.y - this->location.top() - textBorderTop) / this->font->size);
	if (hoverIndex > maxIndex) {
		hoverIndex = -1;
	}
	if (event.mouseEventType == MouseEventType::MouseDown) {
		if (onScrollBar) {
			this->pressedIndex = -1;
			if (event.position.y < scrollEndHeight) {
				// Upper scroll button
				this->firstVisible--;
			} else if (event.position.y > this->location.height() - scrollEndHeight) {
				// Lower scroll button
				this->firstVisible++;
			} else {
				// Start scrolling with the mouse using the relative height on the scroll bar.
				this->pressScrollBar(event.position.y);
				// Repeat on mouse move
				this->holdingScrollBar = true;
			}
		} else {
			this->pressedIndex = hoverIndex;
		}
		this->limitScrolling();
		this->hasImages = false; // Force redraw
	} else if (event.mouseEventType == MouseEventType::MouseUp) {
		if (this->pressedIndex > -1 && this->inside && !onScrollBar && hoverIndex == this->pressedIndex) {
			this->selectedIndex.value = hoverIndex;
			this->limitScrolling(true);
			this->callback_pressedEvent();
		}
		this->pressedIndex = -1;
		this->holdingScrollBar = false;
		this->hasImages = false; // Force redraw
	} else if (event.mouseEventType == MouseEventType::Scroll) {
		if (event.key == MouseKeyEnum::ScrollUp) {
			this->firstVisible--;
		} else if (event.key == MouseKeyEnum::ScrollDown) {
			this->firstVisible++;
		}
		this->holdingScrollBar = false;
		this->limitScrolling();
		this->hasImages = false; // Force redraw
	} else if (event.mouseEventType == MouseEventType::MouseMove) {
		if (this->holdingScrollBar) {
			supressEvent = true;
			this->pressScrollBar(event.position.y);
		}
	}
	if (!supressEvent) {
		VisualComponent::receiveMouseEvent(event);
	}
}

void ListBox::changedTheme(VisualTheme newTheme) {
	this->listBox = theme_getScalableImage(newTheme, U"Panel");
	this->hasImages = false; // Force redraw
}

void ListBox::completeAssets() {
	if (this->listBox.methodIndex == -1) {
		this->listBox = theme_getScalableImage(theme_getDefault(), U"Panel");
	}
	if (this->font.get() == nullptr) {
		this->font = font_getDefault();
	}
}

void ListBox::changedLocation(IRect &oldLocation, IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
		this->limitScrolling();
	}
}

void ListBox::changedAttribute(const ReadableString &name) {
	// If an attribute has changed then force the image to be redrawn
	this->hasImages = false;
	if (string_caseInsensitiveMatch(name, U"List")) {
		// Reset selection on full list updates
		this->selectedIndex.value = 0;
	}
	this->limitSelection();
	this->limitScrolling();
}

int64_t ListBox::getSelectedIndex() {
	int64_t index = this->selectedIndex.value;
	if (this->selectedIndex.value < 0 || this->selectedIndex.value >= this->list.value.length()) {
		index = -1;
	}
	return index;
}

void ListBox::limitSelection() {
	// Get the maximum index
	int64_t maxIndex = this->list.value.length() - 1;
	if (maxIndex < 0) {
		maxIndex = 0;
	}
	// Reset selection on out of bound
	if (this->selectedIndex.value < 0 || this->selectedIndex.value > maxIndex) {
		this->selectedIndex.value = 0;
	}
}

int64_t ListBox::getVisibleScrollRange() {
	int64_t verticalStep = this->font->size;
	return (this->location.height() - textBorderTop * 2) / verticalStep;
}

IRect ListBox::getScrollBarLocation_includingButtons() {
	return IRect(this->location.width() - scrollWidth, 0, scrollWidth, this->location.height());
}

IRect ListBox::getScrollBarLocation_excludingButtons() {
	return IRect(this->location.width() - scrollWidth, scrollEndHeight, scrollWidth, this->location.height() - (scrollEndHeight * 2));
}

IRect ListBox::getKnobLocation() {
	// Eroded scroll-bar excluding buttons
	// The final knob is a sub-set of this region corresponding to the visibility
	IRect erodedBar = this->getScrollBarLocation_excludingButtons().expanded(-knobErosion);
	// Item ranges
	int64_t visibleRange = this->getVisibleScrollRange(); // 0..visibleRange-1
	int64_t itemCount = this->list.value.length(); // 0..itemCount-1
	int64_t maxScroll = itemCount - visibleRange; // 0..maxScroll
	// Dimensions
	int64_t knobHeight = (erodedBar.height() * visibleRange) / itemCount;
	// Visual range for center
	int64_t scrollStart = erodedBar.top() + knobHeight / 2;
	int64_t scrollDistance = erodedBar.height() - knobHeight;
	int64_t knobCenterY = scrollStart + ((this->firstVisible * scrollDistance) / maxScroll);
	return IRect(erodedBar.left(), knobCenterY - (knobHeight / 2), erodedBar.width(), knobHeight);
}

// Optional limit of scrolling, to be applied when the user don't explicitly scroll away from the selection
// limitSelection should be called before limitScrolling, because scrolling limits depend on selection
void ListBox::limitScrolling(bool keepSelectedVisible) {
	// Try to load the font before estimating how big the view is
	this->completeAssets();
	if (this->font.get() == nullptr) {
		throwError("Cannot get the font size because ListBox failed to get a font!\n");
	}
	int64_t itemCount = this->list.value.length();
	int64_t visibleRange = this->getVisibleScrollRange();
	int64_t maxScroll;
	int64_t minScroll;
	// Big enough list to need scrolling but big enough list-box to fit two buttons inside
	this->hasVerticalScroll = itemCount > visibleRange && this->location.width() >= scrollWidth * 2 && this->location.height() >= scrollEndHeight * 2;
	if (keepSelectedVisible) {
		maxScroll = this->selectedIndex.value;
		minScroll = maxScroll + 1 - visibleRange;
	} else {
		maxScroll = itemCount - visibleRange;
		minScroll = 0;
	}
	if (this->firstVisible > maxScroll) {
		this->firstVisible = maxScroll;
	}
	if (this->firstVisible < minScroll) {
		this->firstVisible = minScroll;
	}
}

String ListBox::call(const ReadableString &methodName, const ReadableString &arguments) {
	if (string_caseInsensitiveMatch(methodName, U"ClearAll")) {
		// Remove all elements from the list
		this->list.value.clear();
		this->hasImages = false;
		this->selectedIndex.value = 0;
		this->limitScrolling();
		this->firstVisible = 0;
		return U"";
	} else if (string_caseInsensitiveMatch(methodName, U"PushElement")) {
		// Push a new element to the list
		// No quote mangling needed for this single argument.
		this->list.value.push(arguments);
		this->selectedIndex.value = this->list.value.length() - 1;
		this->limitScrolling(true);
		this->hasImages = false;
		return U"";
	} else if (string_caseInsensitiveMatch(methodName, U"RemoveElement")) {
		// Remove an element who's index is given in the only input argument
		int64_t index = string_parseInteger(arguments);
		if (index < 0 || index >= this->list.value.length()) {
			throwError("Index (", arguments, " = ", index, ") out of bound in RemoveElement!\n");
		}
		this->list.value.remove(index);
		this->limitSelection();
		this->limitScrolling(true);
		this->hasImages = false;
		return U"";
	} else if (string_caseInsensitiveMatch(methodName, U"GetLength")) {
		// Returns the length of the list
		return string_combine(this->list.value.length());
	} else if (string_caseInsensitiveMatch(methodName, U"GetSelectedIndex")) {
		// Returns the selected index or -1 if nothing is selected
		return string_combine(this->getSelectedIndex());
	} else if (string_caseInsensitiveMatch(methodName, U"GetSelectedText")) {
		// Returns the selected element's text or an empty string if nothing is selected
		int64_t index = getSelectedIndex();
		if (index > -1) {
			return this->list.value[index];
		} else {
			return U"";
		}
	} else {
		return VisualComponent::call(methodName, arguments);
	}
}
