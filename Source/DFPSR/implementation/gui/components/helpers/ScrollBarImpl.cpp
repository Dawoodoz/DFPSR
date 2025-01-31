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

#include "ScrollBarImpl.h"

using namespace dsr;

void ScrollBarImpl::loadTheme(VisualTheme theme, const ColorRgbI32 &color) {
	this->scalableImage_scrollTop = theme_getScalableImage(theme, this->vertical ? U"ScrollUp" : U"ScrollLeft");
	this->scalableImage_scrollBottom = theme_getScalableImage(theme, this->vertical ? U"ScrollDown" : U"ScrollRight");
	this->scalableImage_scrollKnob = theme_getScalableImage(theme, this->vertical ? U"VerticalScrollKnob" : U"HorizontalScrollKnob");
	this->scalableImage_scrollBackground = theme_getScalableImage(theme, this->vertical ? U"VerticalScrollList" : U"HorizontalScrollList");
	component_generateImage(theme, this->scalableImage_scrollTop,     this->scrollBarThickness,  this->scrollButtonLength, color.red, color.green, color.blue, 0)(this->scrollButtonTopImage_normal);
	component_generateImage(theme, this->scalableImage_scrollTop,     this->scrollBarThickness,  this->scrollButtonLength, color.red, color.green, color.blue, 1)(this->scrollButtonTopImage_pressed);	
	component_generateImage(theme, this->scalableImage_scrollBottom,  this->scrollBarThickness,  this->scrollButtonLength, color.red, color.green, color.blue, 0)(this->scrollButtonBottomImage_normal);
	component_generateImage(theme, this->scalableImage_scrollBottom,  this->scrollBarThickness,  this->scrollButtonLength, color.red, color.green, color.blue, 1)(this->scrollButtonBottomImage_pressed);
}

void ScrollBarImpl::updateScrollRange(const ScrollRange &range) {
	this->scrollRange = range;
}

void ScrollBarImpl::setValue(int64_t newValue) {
	this->value = newValue;
}

int64_t ScrollBarImpl::getValue() {
	return this->value;
}

void ScrollBarImpl::limitScrolling(const IRect &parentLocation, bool keepPinValueInRange, int64_t pinValue) {
	// Big enough list to need scrolling but big enough list-box to fit two buttons inside
	this->visible = this->scrollRange.minValue < this->scrollRange.maxValue && parentLocation.width() >= scrollBarThickness * 2 && parentLocation.height() >= this->scrollButtonLength * 3;
	if (keepPinValueInRange) {
		// Constrain to keep pinValue in range.
		int64_t maxScroll = pinValue;
		int64_t minScroll = pinValue + 1 - this->scrollRange.visibleItems;
		if (this->value > maxScroll) this->value = maxScroll;
		if (this->value < minScroll) this->value = minScroll;
	}
	// Constrain value to be within the inclusive minValue..maxValue interval, in case that pinValue is out of bound.
	if (this->value > scrollRange.maxValue) this->value = scrollRange.maxValue;
	if (this->value < scrollRange.minValue) this->value = scrollRange.minValue;
}

// Fill from the right side if vertical and bottom if horizontal.
static IRect getWallSide(int32_t parentWidth, int32_t parentHeight, int32_t thickness, bool vertical) {
	return vertical ? IRect(std::max(1, parentWidth - thickness), 0, thickness, parentHeight)
	                : IRect(0, std::max(1, parentHeight - thickness), parentWidth, thickness);
}

// Get the upper part if vertical and left part if horizontal.
static IRect getStartRect(const IRect &original, int32_t startLength, bool vertical) {
	return vertical ? IRect(original.left(), original.top(), original.width(), startLength)
	                : IRect(original.left(), original.top(), startLength, original.height());
}

// Get the bottom part if vertical and right part if horizontal.
static IRect getEndRect(const IRect &original, int32_t endLength, bool vertical) {
	return vertical ? IRect(original.left(), original.bottom() - endLength, original.width(), endLength)
	                : IRect(original.right() - endLength, original.top(), endLength, original.height());
}

static IRect getMiddleRect(const IRect &original, int32_t startCropping, int32_t endCropping, bool vertical) {
	return vertical ? IRect(original.left(), original.top() + startCropping, original.width(), std::max(1, original.height() - startCropping - endCropping))
	                : IRect(original.left() + startCropping, original.top(), std::max(1, original.width() - startCropping - endCropping), original.height());
}

static int32_t getStart(const IRect &rect, bool vertical) {
	return vertical ? rect.top() : rect.left();
}

static int32_t getEnd(const IRect &rect, bool vertical) {
	return vertical ? rect.bottom() : rect.right();
}

static int32_t getLength(const IRect &rect, bool vertical) {
	return vertical ? rect.height() : rect.width();
}

static int32_t getThickness(const IRect &rect, bool vertical) {
	return vertical ? rect.width() : rect.height();
}

IRect ScrollBarImpl::getScrollBarLocation(int32_t parentWidth, int32_t parentHeight) {
	IRect whole = getWallSide(parentWidth, parentHeight, this->scrollBarThickness, this->vertical);
	return getMiddleRect(whole, reservedStart, reservedEnd, this->vertical);
}

IRect ScrollBarImpl::getScrollRegion(const IRect &scrollBarLocation) {
	return getMiddleRect(scrollBarLocation, this->scrollButtonLength, this->scrollButtonLength, this->vertical);
}

IRect ScrollBarImpl::getDecreaseButton(const IRect &scrollBarLocation) {
	return getStartRect(scrollBarLocation, this->scrollButtonLength, this->vertical);
}

IRect ScrollBarImpl::getIncreaseButton(const IRect &scrollBarLocation) {
	return getEndRect(scrollBarLocation, this->scrollButtonLength, this->vertical);
}

IRect ScrollBarImpl::getKnobLocation(const IRect &scrollBarLocation) {
	// Eroded scroll-bar excluding buttons
	// The final knob is a sub-set of this region corresponding to the visibility
	IRect scrollRegion = getMiddleRect(scrollBarLocation, this->scrollButtonLength, this->scrollButtonLength, this->vertical);
	// The knob should represent the selected range within the total range.
	int64_t barLength = getLength(scrollRegion, this->vertical);
	int64_t barThickness = getThickness(scrollRegion, this->vertical);
	int64_t knobLength = (barLength * this->scrollRange.visibleItems) / (this->scrollRange.maxValue + this->scrollRange.visibleItems);
	if (knobLength < barThickness) {
		knobLength = barThickness;
	}
	// Visual range for center
	int64_t scrollStart = (this->vertical ? scrollRegion.top() : scrollRegion.left()) + knobLength / 2;
	int64_t scrollDistance = barLength - knobLength;
	int64_t knobStart = scrollStart + ((this->value * scrollDistance) / this->scrollRange.maxValue) - (knobLength / 2);
	return this->vertical ? IRect(scrollRegion.left(), knobStart, barThickness, knobLength)
	                      : IRect(knobStart, scrollRegion.top(), knobLength, barThickness);
}

void ScrollBarImpl::draw(OrderedImageRgbaU8 &target, VisualTheme &theme, const ColorRgbI32 &color) {
	if (this->visible) {
		int32_t parentWidth = image_getWidth(target);
		int32_t parentHeight = image_getHeight(target);
		IRect scrollBarLocation = this->getScrollBarLocation(parentWidth, parentHeight);
		IRect upper = getStartRect(scrollBarLocation, this->scrollButtonLength, this->vertical);
		IRect lower = getEndRect(scrollBarLocation, this->scrollButtonLength, this->vertical);
		IRect knob = this->getKnobLocation(scrollBarLocation);
		// Only redraw the knob image if its dimensions changed
		if (!image_exists(this->scrollKnobImage_normal)
		  || image_getWidth(this->scrollKnobImage_normal) != knob.width()
		  || image_getHeight(this->scrollKnobImage_normal) != knob.height()) {
			component_generateImage(theme, this->scalableImage_scrollKnob, knob.width(), knob.height(), color.red, color.green, color.blue, 0)(this->scrollKnobImage_normal);
			component_generateImage(theme, this->scalableImage_scrollKnob, knob.width(), knob.height(), color.red, color.green, color.blue, 1)(this->scrollKnobImage_pressed);
		}
		// Only redraw the scroll list if its dimenstions changed
		if (!image_exists(this->scrollBarImage)
		  || image_getWidth(this->scrollBarImage) != scrollBarLocation.width()
		  || image_getHeight(this->scrollBarImage) != scrollBarLocation.height()) {
			component_generateImage(theme, this->scalableImage_scrollBackground, scrollBarLocation.width(), scrollBarLocation.height(), color.red, color.green, color.blue, 0)(this->scrollBarImage);
		}
		// Draw the scroll-bar
		draw_alphaFilter(target, this->scrollBarImage, scrollBarLocation.left(), scrollBarLocation.top());
		draw_alphaFilter(target, this->holdingScrollBar ? this->scrollKnobImage_pressed : this->scrollKnobImage_normal, knob.left(), knob.top());
		draw_alphaFilter(target, this->pressScrollUp ? this->scrollButtonTopImage_pressed : this->scrollButtonTopImage_normal, upper.left(), upper.top());
		draw_alphaFilter(target, this->pressScrollDown ? this->scrollButtonBottomImage_pressed : this->scrollButtonBottomImage_normal, lower.left(), lower.top());
	}
}

bool ScrollBarImpl::pressScrollBar(const IRect &parentLocation, int64_t localCoordinate) {
	int64_t oldValue = this->value;
	IRect scrollBarLocation = this->getScrollBarLocation(parentLocation.width(), parentLocation.height());
	IRect scrollRegion = this->getScrollRegion(scrollBarLocation);
	IRect knobLocation = this->getKnobLocation(scrollBarLocation);
	int64_t knobLength = getLength(knobLocation, this->vertical);
	int64_t minimumAt = getStart(scrollRegion, this->vertical) + knobLength / 2;
	int64_t maximumAt = getEnd(scrollRegion, this->vertical) - knobLength / 2;
	int64_t pixelRange = maximumAt - minimumAt;
	int64_t valueRange = this->scrollRange.maxValue - this->scrollRange.minValue;
	this->value = this->scrollRange.minValue;
	if (pixelRange > 0) {
		this->value += ((localCoordinate - minimumAt) * valueRange + pixelRange / 2) / pixelRange;
	}
	this->limitScrolling(parentLocation);
	// Avoid expensive redrawing if the index did not change
	return this->value != oldValue;
}

bool ScrollBarImpl::receiveMouseEvent(const IRect &parentLocation, const MouseEvent& event) {
	if (!this->visible) {
		return false;
	}
	bool intercepted = false;
	IVector2D localPosition = event.position - parentLocation.upperLeft();
	IRect scrollBarLocation = this->getScrollBarLocation(parentLocation.width(), parentLocation.height());
	IRect cursorLocation = IRect(localPosition.x, localPosition.y, 1, 1);
	int64_t usedCoordinate = (this->vertical ? localPosition.y : localPosition.x);
	if (event.mouseEventType == MouseEventType::MouseDown) {
		if (IRect::touches(scrollBarLocation, cursorLocation)) {
			IRect upperLocation = getStartRect(scrollBarLocation, this->scrollButtonLength, this->vertical);
			IRect lowerLocation = getEndRect(scrollBarLocation, this->scrollButtonLength, this->vertical);
			intercepted = true;
			if (IRect::touches(upperLocation, cursorLocation)) {
				// Upper scroll button
				this->pressScrollUp = true;
				this->value--;
			} else if (IRect::touches(lowerLocation, cursorLocation)) {
				// Lower scroll button
				this->pressScrollDown = true;
				this->value++;
			} else {
				// Start scrolling with the mouse using the relative height on the scroll bar.
				IRect knobLocation = this->getKnobLocation(scrollBarLocation);
				int64_t halfKnobLength = getLength(knobLocation, this->vertical) / 2;
				this->knobHoldOffset = usedCoordinate - (getStart(knobLocation, this->vertical) + halfKnobLength);
				if (this->knobHoldOffset < -halfKnobLength || this->knobHoldOffset > halfKnobLength) {
					// If pressing outside of the knob, pull it directly to the pressed location before pulling from the center.
					this->knobHoldOffset = 0;
					this->pressScrollBar(parentLocation, usedCoordinate - this->knobHoldOffset);
				}
				this->holdingScrollBar = true;
			}
		}
		this->limitScrolling(parentLocation);
	} else if (event.mouseEventType == MouseEventType::MouseUp) {
		this->pressScrollUp = false;
		this->pressScrollDown = false;
		this->holdingScrollBar = false;
		intercepted = true;
	} else if (event.mouseEventType == MouseEventType::Scroll) {
		if (this->vertical) {
			if (event.key == MouseKeyEnum::ScrollUp) {
				this->value--;
			} else if (event.key == MouseKeyEnum::ScrollDown) {
				this->value++;
			}
			this->limitScrolling(parentLocation);
		}
		this->holdingScrollBar = false;
		intercepted = true;
	} else if (event.mouseEventType == MouseEventType::MouseMove) {
		if (this->holdingScrollBar) {
			intercepted = this->pressScrollBar(parentLocation, usedCoordinate - this->knobHoldOffset);
		}
	}
	return intercepted;
}
