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

#ifndef DFPSR_GUI_HELPER_SCROLL_BAR_IMPL
#define DFPSR_GUI_HELPER_SCROLL_BAR_IMPL

#include "../../VisualComponent.h"

namespace dsr {

struct ScrollRange {
	// The minimum value the scrollbar can be assigned in the current range.
	//   Set minValue to 0 for scrolling through items in a list.
	int64_t minValue = 0;
	// The maximum value the scrollbar can be assigned in the current range.
	//   Set maxValue to length - 1 and visibleItems to 1 for selecting an item in a list directly.
	//   Set maxValue to length - visibleItems for scrolling through items with visibleItems sepresented by the knob's length.
	int64_t maxValue = 0;
	// Scrolling represents a range starting at the selected value of length visibleItems.
	//   Use 1 if you are just selecting one integer value.
	//   Use the value that you subtracted from the total length in maxValue if you want to represent a range of multiple values.
	//   This argument only affects the knob size and how the selected range is constrained to include a certain value.
	int64_t visibleItems = 1;
	ScrollRange() {}
	ScrollRange(int64_t minValue, int64_t maxValue, int64_t visibleItems)
	: minValue(minValue), maxValue(maxValue), visibleItems(visibleItems) {}
};

// To be value allocated inside of components that need a scroll-bar.
// Initialized with true if the scroll-bar is vertical, and with false if the scroll-bar is horizontal.
// Call loadTheme when the parent component loads a new theme, or you will have a crash when trying to draw the scroll-bar.
// When the range of values or view's size changed, recalculate the range of valid values and send it to updateScrollRange.
class ScrollBarImpl {
private:
	// Locked settings.
	const bool vertical;
	// Calculated automatically.
	bool visible = false;
	// Temporary.
	bool pressScrollUp = false;
	bool pressScrollDown = false;
	bool holdingScrollBar = false;
	const int32_t scrollBarThickness = 16;
	const int32_t scrollButtonLength = 16;
	int64_t knobHoldOffset = 0; // The number of pixels right or down from the center that the knob was grabbed at.
	// Updated manually.
	ScrollRange scrollRange; // Range of valid values.
	int64_t value = 0; // The selected value.
	int64_t reservedStart = 0; // How many pixels to leave empty at the left/top side to avoid collisions.
	int64_t reservedEnd = 0; // How many pixels to leave empty at the right/bottom side to avoid collisions.
	// Scalable parametric images.
	MediaMethod scalableImage_scrollTop, scalableImage_scrollBottom, scalableImage_scrollBackground, scalableImage_scrollKnob;
	// Generated raster images.
	OrderedImageRgbaU8 scrollButtonTopImage_normal, scrollButtonTopImage_pressed;
	OrderedImageRgbaU8 scrollButtonBottomImage_normal, scrollButtonBottomImage_pressed;
	OrderedImageRgbaU8 scrollKnobImage_normal, scrollKnobImage_pressed;
	OrderedImageRgbaU8 scrollBarImage;
private:
	IRect getScrollBarLocation(int32_t parentWidth, int32_t parentHeight);
	IRect getScrollRegion(const IRect &scrollBarLocation);
	IRect getDecreaseButton(const IRect &scrollBarLocation);
	IRect getIncreaseButton(const IRect &scrollBarLocation);
	IRect getKnobLocation(const IRect &scrollBarLocation);
	// Returns true iff value updated.
	bool pressScrollBar(const IRect &parentLocation, int64_t localY);
public:
	ScrollBarImpl(bool vertical, bool reserveEnd = false)
	: vertical(vertical), reservedStart(0), reservedEnd(reserveEnd ? 16 : 0) {}
	void loadTheme(VisualTheme theme, const ColorRgbI32 &color);
	// Pre-condition: range.minValue <= range.maxValue
	void updateScrollRange(const ScrollRange &range);
	void setValue(int64_t newValue);
	int64_t getValue();
	// Pre-condition: scrollRange must have been updated since the last changes that may affect it
	// Side-effects: Constrains value within minValue and maxValue.
	//               If keepPinValueInRange is true, it will also try to constrain the selected range to include pinValue,
	//               which is used for making sure that something selected stays visible, when scrolling is used for navigating a view.
	void limitScrolling(const IRect &parentLocation, bool keepPinValueInRange = false, int64_t pinValue = 0);
	// For mouse-down events, it returns true iff the event landed inside of the scroll-bar.
	// For other event types, it returns true iff the scroll-bar needs to be redrawn.
	bool receiveMouseEvent(const IRect &parentLocation, const MouseEvent& event);
	// Draw the scroll-bar on top of target.
	void draw(OrderedImageRgbaU8 &target, VisualTheme &theme, const ColorRgbI32 &color);
};

}

#endif
