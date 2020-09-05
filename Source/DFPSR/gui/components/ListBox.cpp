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

ListBox::ListBox() {
	// Changed from nothing to zero
	this->callback_selectEvent(0);
}

bool ListBox::isContainer() const {
	return true;
}

static const int textBorderLeft = 6;
static const int textBorderTop = 4;
static const int scrollWidth = 16; // The width of the scroll bar
static const int scrollEndHeight = 14; // The height of upper and lower scroll buttons
static const int border = 1; // Scroll-bar edge thickness

void ListBox::generateGraphics() {
	int width = this->location.width();
	int height = this->location.height();
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	if (!this->hasImages) {
		this->completeAssets();
		ColorRgbI32 color = this->color.value;
	 	this->scalableImage_listBox(width, height, color.red, color.green, color.blue)(this->image);
		int verticalStep = font_getSize(this->font);
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
			font_printLine(this->image, this->font, this->list.value[i], IVector2D(left, top), textColor);
			top += verticalStep;
		}
		if (this->hasVerticalScroll) {
			IRect whole = IRect(this->location.width() - scrollWidth, 0, scrollWidth, this->location.height());
			IRect upper = IRect(whole.left(), whole.top(), whole.width(), scrollEndHeight);
			IRect lower = IRect(whole.left(), whole.bottom() - scrollEndHeight, whole.width(), scrollEndHeight);
			IRect knob = this->getKnobLocation();
			// Only redraw the knob image if its dimensions changed
			if (!image_exists(this->scrollKnob_normal)
			  || image_getWidth(this->scrollKnob_normal) != knob.width()
			  || image_getHeight(this->scrollKnob_normal) != knob.height()) {
				this->scalableImage_scrollButton(knob.width(), knob.height(), false, color.red, color.green, color.blue)(this->scrollKnob_normal);
			}
			// Only redraw the scroll list if its dimenstions changed
			if (!image_exists(this->verticalScrollBar_normal)
			  || image_getWidth(this->verticalScrollBar_normal) != whole.width()
			  || image_getHeight(this->verticalScrollBar_normal) != whole.height()) {
				this->scalableImage_verticalScrollBar(whole.width(), whole.height(), color.red, color.green, color.blue)(this->verticalScrollBar_normal);
			}
			// Draw the scroll-bar
			draw_alphaFilter(this->image, this->verticalScrollBar_normal, whole.left(), whole.top());
			draw_alphaFilter(this->image, this->scrollKnob_normal, knob.left(), knob.top());
			draw_alphaFilter(this->image, (this->pressScrollUp) ? this->scrollButtonTop_pressed : this->scrollButtonTop_normal, upper.left(), upper.top());
			draw_alphaFilter(this->image, (this->pressScrollDown && this->inside) ? this->scrollButtonBottom_pressed : this->scrollButtonBottom_normal, lower.left(), lower.top());
		}
		this->hasImages = true;
	}
}

void ListBox::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	draw_copy(targetImage, this->image, relativeLocation.left(), relativeLocation.top());
}

void ListBox::pressScrollBar(int64_t localY) {
	int64_t oldIndex = this->firstVisible;
	int64_t maxScroll = this->list.value.length() - this->getVisibleScrollRange();
	int64_t knobHeight = this->getKnobLocation().height();
	int64_t endDistance = scrollEndHeight + knobHeight / 2;
	int64_t barHeight = this->location.height() - (endDistance * 2);
	this->firstVisible = ((localY - endDistance) * maxScroll) / barHeight;
	this->limitScrolling();
	// Avoid expensive redrawing if the index did not change
	if (this->firstVisible != oldIndex) {
		this->hasImages = false; // Force redraw
	}
}

void ListBox::receiveMouseEvent(const MouseEvent& event) {
	bool supressEvent = false;
	this->inside = this->pointIsInside(event.position);
	IVector2D localPosition = event.position - this->location.upperLeft();
	bool onScrollBar = this->hasVerticalScroll && localPosition.x >= this->location.width() - scrollWidth;
	int64_t maxIndex = this->list.value.length() - 1;
	int64_t hoverIndex = this->firstVisible + ((localPosition.y - textBorderTop) / font_getSize(this->font));
	if (hoverIndex > maxIndex) {
		hoverIndex = -1;
	}
	if (event.mouseEventType == MouseEventType::MouseDown) {
		if (onScrollBar) {
			this->pressedIndex = -1;
			if (localPosition.y < scrollEndHeight) {
				// Upper scroll button
				this->pressScrollUp = true;
				this->firstVisible--;
			} else if (localPosition.y > this->location.height() - scrollEndHeight) {
				// Lower scroll button
				this->pressScrollDown = true;
				this->firstVisible++;
			} else {
				// Start scrolling with the mouse using the relative height on the scroll bar.
				IRect knobLocation = this->getKnobLocation();
				int64_t halfKnobHeight = knobLocation.height() / 2;
				this->knobHoldOffset = localPosition.y - (knobLocation.top() + halfKnobHeight);
				if (this->knobHoldOffset < -halfKnobHeight || this->knobHoldOffset > halfKnobHeight) {
					// If pressing outside of the knob, pull it directly to the pressed location before pulling from the center.
					this->knobHoldOffset = 0;
					this->pressScrollBar(localPosition.y - this->knobHoldOffset);
				}
				this->holdingScrollBar = true;
			}
		} else {
			this->pressedIndex = hoverIndex;
		}
		this->limitScrolling();
		this->hasImages = false; // Force redraw
	} else if (event.mouseEventType == MouseEventType::MouseUp) {
		if (this->pressedIndex > -1 && this->inside && !onScrollBar && hoverIndex == this->pressedIndex) {
			this->setSelectedIndex(hoverIndex, false);
			this->limitScrolling(true);
			this->callback_pressedEvent();
		}
		this->pressScrollUp = false;
		this->pressScrollDown = false;
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
			this->pressScrollBar(localPosition.y - this->knobHoldOffset);
		}
	}
	if (!supressEvent) {
		VisualComponent::receiveMouseEvent(event);
	}
}

void ListBox::receiveKeyboardEvent(const KeyboardEvent& event) {
	if (event.keyboardEventType == KeyboardEventType::KeyDown) {
		int contentLength = this->list.value.length();
		int oldIndex = this->selectedIndex.value;
		if (contentLength > 1) {
			if (oldIndex > 0 && event.dsrKey == DsrKey::DsrKey_UpArrow) {
				this->setSelectedIndex(oldIndex - 1, true);
			} else if (oldIndex < contentLength - 1 && event.dsrKey == DsrKey::DsrKey_DownArrow) {
				this->setSelectedIndex(oldIndex + 1, true);
			}
		}
	}
	VisualComponent::receiveKeyboardEvent(event);
}

void ListBox::loadTheme(VisualTheme theme) {
	this->scalableImage_listBox = theme_getScalableImage(theme, U"ListBox");
	this->scalableImage_scrollButton = theme_getScalableImage(theme, U"ScrollButton");
	this->scalableImage_verticalScrollBar = theme_getScalableImage(theme, U"VerticalScrollBar");
	// Generate fixed size buttons for the scroll buttons (because their size is currently given by constants)
	ColorRgbI32 color = this->color.value;
	this->scalableImage_scrollButton(scrollWidth, scrollEndHeight, false, color.red, color.green, color.blue)(this->scrollButtonTop_normal);
	this->scalableImage_scrollButton(scrollWidth, scrollEndHeight, true, color.red, color.green, color.blue)(this->scrollButtonTop_pressed);
	this->scalableImage_scrollButton(scrollWidth, scrollEndHeight, false, color.red, color.green, color.blue)(this->scrollButtonBottom_normal);
	this->scalableImage_scrollButton(scrollWidth, scrollEndHeight, true, color.red, color.green, color.blue)(this->scrollButtonBottom_pressed);
}

void ListBox::changedTheme(VisualTheme newTheme) {
	this->loadTheme(newTheme);
	this->hasImages = false; // Force redraw
}

void ListBox::completeAssets() {
	if (this->scalableImage_listBox.methodIndex == -1) {
		this->loadTheme(theme_getDefault());
	}
	if (!font_exists(this->font)) {
		this->font = font_getDefault();
	}
	if (!font_exists(this->font)) {
		throwError("Failed to load the default font for a ListBox!\n");
	}
}

void ListBox::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
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
		this->setSelectedIndex(0, true);
	}
	this->limitSelection(false);
	this->limitScrolling();
}

void ListBox::setSelectedIndex(int index, bool forceUpdate) {
	if (forceUpdate || this->selectedIndex.value != index) {
		this->selectedIndex.value = index;
		this->hasImages = false;
		this->callback_selectEvent(index);
		this->limitScrolling(true);
	}
}

int64_t ListBox::getSelectedIndex() {
	int64_t index = this->selectedIndex.value;
	if (this->selectedIndex.value < 0 || this->selectedIndex.value >= this->list.value.length()) {
		index = -1;
	}
	return index;
}

void ListBox::limitSelection(bool indexChangedMeaning) {
	// Get the maximum index
	int64_t maxIndex = this->list.value.length() - 1;
	if (maxIndex < 0) {
		maxIndex = 0;
	}
	// Reset selection on out of bound
	if (this->selectedIndex.value < 0 || this->selectedIndex.value > maxIndex) {
		setSelectedIndex(0, indexChangedMeaning);
	}
}

int64_t ListBox::getVisibleScrollRange() {
	this->completeAssets();
	int64_t verticalStep = font_getSize(this->font);
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
	IRect scrollBarRegion = this->getScrollBarLocation_excludingButtons();
	// Item ranges
	int64_t visibleRange = this->getVisibleScrollRange(); // 0..visibleRange-1
	int64_t itemCount = this->list.value.length(); // 0..itemCount-1
	int64_t maxScroll = itemCount - visibleRange; // 0..maxScroll
	// Dimensions
	int64_t knobHeight = (scrollBarRegion.height() * visibleRange) / itemCount;
	if (knobHeight < scrollBarRegion.width()) {
		knobHeight = scrollBarRegion.width();
	}
	// Visual range for center
	int64_t scrollStart = scrollBarRegion.top() + knobHeight / 2;
	int64_t scrollDistance = scrollBarRegion.height() - knobHeight;
	int64_t knobCenterY = scrollStart + ((this->firstVisible * scrollDistance) / maxScroll);
	return IRect(scrollBarRegion.left(), knobCenterY - (knobHeight / 2), scrollBarRegion.width(), knobHeight);
}

// Optional limit of scrolling, to be applied when the user don't explicitly scroll away from the selection
// limitSelection should be called before limitScrolling, because scrolling limits depend on selection
void ListBox::limitScrolling(bool keepSelectedVisible) {
	// Try to load the font before estimating how big the view is
	this->completeAssets();
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
		int64_t index = string_toInteger(arguments);
		if (index < 0 || index >= this->list.value.length()) {
			throwError("Index (", arguments, " = ", index, ") out of bound in RemoveElement!\n");
		}
		this->list.value.remove(index);
		this->limitSelection(true);
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
