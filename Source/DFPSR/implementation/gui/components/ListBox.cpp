// zlib open source license
//
// Copyright (c) 2020 to 2022 David Forsgren Piuva
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

using namespace dsr;

PERSISTENT_DEFINITION(ListBox)

void ListBox::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"BackColor");
	target.declareAttribute(U"ForeColor");
	target.declareAttribute(U"List");
	target.declareAttribute(U"SelectedIndex");
	target.declareAttribute(U"BackgroundClass");
}

Persistent* ListBox::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"Color") || string_caseInsensitiveMatch(name, U"BackColor")) {
		return &(this->backColor);
	} else if (string_caseInsensitiveMatch(name, U"ForeColor")) {
		return &(this->foreColor);
	} else if (string_caseInsensitiveMatch(name, U"List")) {
		return &(this->list);
	} else if (string_caseInsensitiveMatch(name, U"SelectedIndex")) {
		return &(this->selectedIndex);
	} else if (string_caseInsensitiveMatch(name, U"Class") || string_caseInsensitiveMatch(name, U"BackgroundClass")) {
		return &(this->backgroundClass);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

ListBox::ListBox() {
	// Changed from nothing to zero
	this->callback_selectEvent(0);
}

bool ListBox::isContainer() const {
	return false;
}

static const int textBorderLeft = 6;
static const int textBorderTop = 4;

void ListBox::generateGraphics() {
	int32_t width = this->location.width();
	int32_t height = this->location.height();
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	if (!this->hasImages) {
		this->completeAssets();
		ColorRgbI32 backColor = this->backColor.value;
		ColorRgbI32 foreColor = this->foreColor.value;
	 	component_generateImage(this->theme, this->scalableImage_listBox, width, height, backColor.red, backColor.green, backColor.blue)(this->image);
		int32_t verticalStep = font_getSize(this->font);
		int32_t left = textBorderLeft;
		int32_t top = textBorderTop;
		for (int64_t i = this->verticalScrollBar.getValue(); i < this->list.value.length() && top < height; i++) {
			ColorRgbaI32 textColor;
			if (i == this->pressedIndex || i == this->selectedIndex.value) {
				textColor = ColorRgbaI32(255, 255, 255, 255);
			} else {
				textColor = ColorRgbaI32(foreColor, 255);
			}
			if (i == this->selectedIndex.value) {
				draw_rectangle(this->image, IRect(left, top, width - (textBorderLeft * 2), verticalStep), ColorRgbaI32(0, 0, 0, 255));
			}
			font_printLine(this->image, this->font, this->list.value[i], IVector2D(left, top), textColor);
			top += verticalStep;
		}
		this->verticalScrollBar.draw(this->image, this->theme, backColor);
		this->hasImages = true;
	}
}

void ListBox::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	if (this->background_filter == 1) {
		draw_alphaFilter(targetImage, this->image, relativeLocation.left(), relativeLocation.top());
	} else {
		draw_copy(targetImage, this->image, relativeLocation.left(), relativeLocation.top());
	}
}

void ListBox::updateScrollRange() {
	this->loadFont();
	// How high is one element?
	int64_t verticalStep = font_getSize(this->font);
	// How many elements are visible at the same time?
	int64_t visibleRange = (this->location.height() - textBorderTop * 2) / verticalStep;
	if (visibleRange < 1) visibleRange = 1;
	// How many elements are there in total to see.
	int64_t itemCount = this->list.value.length();
	// The range of indices that the listbox can start viewing from.
	int64_t minScroll = 0;
	int64_t maxScroll = itemCount - visibleRange;
	// If visible range exceeds the collection, we should still allow starting element zero to get a valid range.
	if (maxScroll < 0) maxScroll = 0;
	// Apply the scroll range.
	this->verticalScrollBar.updateScrollRange(ScrollRange(minScroll, maxScroll, visibleRange));
}

void ListBox::limitScrolling(bool keepSelectedVisible) {
	// Update the scroll range.
	this->updateScrollRange();
	// Limit scrolling with the updated range.
	this->verticalScrollBar.limitScrolling(this->location, keepSelectedVisible, this->selectedIndex.value);
}

void ListBox::receiveMouseEvent(const MouseEvent& event) {
	this->inside = this->pointIsInside(event.position);
	IVector2D localPosition = event.position - this->location.upperLeft();
	bool verticalScrollIntercepted = this->verticalScrollBar.receiveMouseEvent(this->location, event);
	int64_t maxIndex = this->list.value.length() - 1;
	int64_t hoverIndex = this->verticalScrollBar.getValue() + ((localPosition.y - textBorderTop) / font_getSize(this->font));
	if (hoverIndex < 0 || hoverIndex > maxIndex) {
		hoverIndex = -1;
	}
	if (event.mouseEventType == MouseEventType::MouseDown) {
		if (verticalScrollIntercepted) {
			this->pressedIndex = -1;
		} else {
			this->pressedIndex = hoverIndex;
		}
		this->hasImages = false; // Force redraw on item selection
	} else if (event.mouseEventType == MouseEventType::MouseUp) {
		if (this->pressedIndex > -1 && this->inside && hoverIndex == this->pressedIndex) {
			this->setSelectedIndex(hoverIndex, false);
			this->limitScrolling(true);
			this->callback_pressedEvent();
		}
		this->pressedIndex = -1;
	}
	if (verticalScrollIntercepted) {
		this->hasImages = false; // Force redraw on scrollbar interception
	} else {
		VisualComponent::receiveMouseEvent(event);
	}
}

void ListBox::receiveKeyboardEvent(const KeyboardEvent& event) {
	if (event.keyboardEventType == KeyboardEventType::KeyDown) {
		int64_t contentLength = this->list.value.length();
		int64_t oldIndex = this->selectedIndex.value;
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

void ListBox::loadTheme(const VisualTheme &theme) {
	this->finalBackgroundClass = theme_selectClass(theme, this->backgroundClass.value, U"ListBox");
	this->scalableImage_listBox = theme_getScalableImage(theme, this->finalBackgroundClass);
	this->verticalScrollBar.loadTheme(theme, this->backColor.value);
	this->background_filter = theme_getInteger(theme, this->finalBackgroundClass, U"Filter", 0);
}

void ListBox::changedTheme(VisualTheme newTheme) {
	this->loadTheme(newTheme);
	this->hasImages = false; // Force redraw
}

void ListBox::loadFont() {
	if (!font_exists(this->font)) {
		this->font = font_getDefault();
	}
	if (!font_exists(this->font)) {
		throwError("Failed to load the default font for a ListBox!\n");
	}
}

void ListBox::completeAssets() {
	if (this->scalableImage_listBox.methodIndex == -1) {
		this->loadTheme(theme_getDefault());
	}
	this->loadFont();
}

void ListBox::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
		this->limitScrolling();
	}
}

void ListBox::changedAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"List")) {
		// Reset selection on full list updates
		this->setSelectedIndex(0, true);
	} else if (string_caseInsensitiveMatch(name, U"BackgroundClass")) {
		// Update from the theme if the theme class has changed.
		this->changedTheme(this->getTheme());
	} else if (!string_caseInsensitiveMatch(name, U"Visible")) {
		this->hasImages = false;
	}
	this->limitSelection(false);
	this->limitScrolling();
	VisualComponent::changedAttribute(name);
}

void ListBox::setSelectedIndex(int64_t index, bool forceUpdate) {
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

String ListBox::call(const ReadableString &methodName, const ReadableString &arguments) {
	if (string_caseInsensitiveMatch(methodName, U"ClearAll")) {
		// Remove all elements from the list
		this->list.value.clear();
		this->hasImages = false;
		this->selectedIndex.value = 0;
		this->limitScrolling();
		this->verticalScrollBar.setValue(0);
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
