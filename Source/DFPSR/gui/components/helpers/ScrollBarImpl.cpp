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
	this->scalableImage_scrollTop = theme_getScalableImage(theme, U"ScrollUp");
	this->scalableImage_scrollBottom = theme_getScalableImage(theme, U"ScrollDown");
	this->scalableImage_verticalScrollKnob = theme_getScalableImage(theme, U"VerticalScrollKnob");
	this->scalableImage_verticalScrollBackground = theme_getScalableImage(theme, U"VerticalScrollList");
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

IRect ScrollBarImpl::getScrollBarLocation_excludingButtons(int32_t parentWidth, int32_t parentHeight) {
	return this->vertical ? IRect(parentWidth - this->scrollBarThickness, this->scrollButtonLength, this->scrollBarThickness, parentHeight - (this->scrollButtonLength * 2))
	                      : IRect(this->scrollButtonLength, parentHeight - this->scrollBarThickness, parentWidth - (this->scrollButtonLength * 2), this->scrollBarThickness);
}

IRect ScrollBarImpl::getKnobLocation(int32_t parentWidth, int32_t parentHeight) {
	// Eroded scroll-bar excluding buttons
	// The final knob is a sub-set of this region corresponding to the visibility
	IRect scrollBarRegion = this->getScrollBarLocation_excludingButtons(parentWidth, parentHeight);
	// The knob should represent the selected range within the total range.
	int64_t barLength = this->vertical ? scrollBarRegion.height() : scrollBarRegion.width();
	int64_t barThickness = this->vertical ? scrollBarRegion.width() : scrollBarRegion.height();
	int64_t knobLength = (barLength * this->scrollRange.visibleItems) / (this->scrollRange.maxValue + this->scrollRange.visibleItems);
	if (knobLength < barThickness) {
		knobLength = barThickness;
	}
	// Visual range for center
	int64_t scrollStart = (this->vertical ? scrollBarRegion.top() : scrollBarRegion.left()) + knobLength / 2;
	int64_t scrollDistance = barLength - knobLength;
	int64_t knobStart = scrollStart + ((this->value * scrollDistance) / this->scrollRange.maxValue) - (knobLength / 2);
	return this->vertical ? IRect(scrollBarRegion.left(), knobStart, barThickness, knobLength)
	                      : IRect(knobStart, scrollBarRegion.top(), knobLength, barThickness);
}

void ScrollBarImpl::draw(OrderedImageRgbaU8 &target, VisualTheme &theme, const ColorRgbI32 &color) {
	if (this->visible) {
		int32_t parentWidth = image_getWidth(target);
		int32_t parentHeight = image_getHeight(target);
		IRect whole = this->vertical ? IRect(parentWidth - this->scrollBarThickness, 0, this->scrollBarThickness, parentHeight)
		                             : IRect(0, parentHeight - this->scrollBarThickness, parentWidth, this->scrollBarThickness);
		IRect upper = this->vertical ? IRect(whole.left(), whole.top(), whole.width(), this->scrollButtonLength)
		                             : IRect(whole.left(), whole.top(), this->scrollButtonLength, whole.height());
		IRect lower = this->vertical ? IRect(whole.left(), whole.bottom() - this->scrollButtonLength, whole.width(), this->scrollButtonLength)
		                             : IRect(whole.right() - this->scrollButtonLength, whole.top(), this->scrollButtonLength, whole.height());
		IRect knob = this->getKnobLocation(parentWidth, parentHeight);
		// Only redraw the knob image if its dimensions changed
		if (!image_exists(this->scrollKnobImage)
		  || image_getWidth(this->scrollKnobImage) != knob.width()
		  || image_getHeight(this->scrollKnobImage) != knob.height()) {
			component_generateImage(theme, this->scalableImage_verticalScrollKnob, knob.width(), knob.height(), color.red, color.green, color.blue, 0)(this->scrollKnobImage);
		}
		// Only redraw the scroll list if its dimenstions changed
		if (!image_exists(this->verticalScrollBarImage)
		  || image_getWidth(this->verticalScrollBarImage) != whole.width()
		  || image_getHeight(this->verticalScrollBarImage) != whole.height()) {
			component_generateImage(theme, this->scalableImage_verticalScrollBackground, whole.width(), whole.height(), color.red, color.green, color.blue, 0)(this->verticalScrollBarImage);
		}
		// Draw the scroll-bar
		draw_alphaFilter(target, this->verticalScrollBarImage, whole.left(), whole.top());
		draw_alphaFilter(target, this->scrollKnobImage, knob.left(), knob.top());
		draw_alphaFilter(target, this->pressScrollUp ? this->scrollButtonTopImage_pressed : this->scrollButtonTopImage_normal, upper.left(), upper.top());
		draw_alphaFilter(target, this->pressScrollDown ? this->scrollButtonBottomImage_pressed : this->scrollButtonBottomImage_normal, lower.left(), lower.top());
	}
}

bool ScrollBarImpl::pressScrollBar(const IRect &parentLocation, int64_t localCoordinate) {
	int64_t oldValue = this->value;
	IRect knobLocation = this->getKnobLocation(parentLocation.width(), parentLocation.height());
	int64_t knobLength = this->vertical ? knobLocation.height() : knobLocation.width();
	int64_t endDistance = this->scrollButtonLength + knobLength / 2;
	int64_t barLength = (this->vertical ? parentLocation.height() : parentLocation.width()) - (endDistance * 2);
	this->value = ((localCoordinate - endDistance) * this->scrollRange.maxValue + (barLength / 2)) / barLength;
	this->limitScrolling(parentLocation);
	// Avoid expensive redrawing if the index did not change
	return this->value != oldValue;
}

bool ScrollBarImpl::receiveMouseEvent(const IRect &parentLocation, const MouseEvent& event) {
	bool intercepted = false;
	IVector2D localPosition = event.position - parentLocation.upperLeft();
	int64_t usedCoordinate = (this->vertical ? localPosition.y : localPosition.x);
	bool onScrollBar = this->visible && (this->vertical ? (localPosition.x >= parentLocation.width() - scrollBarThickness) : (localPosition.y >= parentLocation.height() - scrollBarThickness));
	if (event.mouseEventType == MouseEventType::MouseDown) {
		if (onScrollBar) {
			intercepted = true;
			if (this->vertical) {
				if (localPosition.y < this->scrollButtonLength) {
					// Upper scroll button
					this->pressScrollUp = true;
					this->value--;
				} else if (localPosition.y > parentLocation.height() - this->scrollButtonLength) {
					// Lower scroll button
					this->pressScrollDown = true;
					this->value++;
				} else {
					// Start scrolling with the mouse using the relative height on the scroll bar.
					IRect knobLocation = this->getKnobLocation(parentLocation.width(), parentLocation.height());
					int64_t halfKnobLength = knobLocation.height() / 2;
					this->knobHoldOffset = localPosition.y - (knobLocation.top() + halfKnobLength);
					if (this->knobHoldOffset < -halfKnobLength || this->knobHoldOffset > halfKnobLength) {
						// If pressing outside of the knob, pull it directly to the pressed location before pulling from the center.
						this->knobHoldOffset = 0;
						this->pressScrollBar(parentLocation, usedCoordinate - this->knobHoldOffset);
					}
					this->holdingScrollBar = true;
				}
			} else {
				if (localPosition.x < this->scrollButtonLength) {
					// Upper scroll button
					this->pressScrollUp = true;
					this->value--;
				} else if (localPosition.x > parentLocation.width() - this->scrollButtonLength) {
					// Lower scroll button
					this->pressScrollDown = true;
					this->value++;
				} else {
					// Start scrolling with the mouse using the relative width on the scroll bar.
					IRect knobLocation = this->getKnobLocation(parentLocation.width(), parentLocation.height());
					int64_t halfKnobLength = knobLocation.width() / 2;
					this->knobHoldOffset = localPosition.x - (knobLocation.width() + halfKnobLength);
					if (this->knobHoldOffset < -halfKnobLength || this->knobHoldOffset > halfKnobLength) {
						// If pressing outside of the knob, pull it directly to the pressed location before pulling from the center.
						this->knobHoldOffset = 0;
						this->pressScrollBar(parentLocation, usedCoordinate - this->knobHoldOffset);
					}
					this->holdingScrollBar = true;
				}
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
