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

#ifndef DFPSR_GUI_FLEXREGION
#define DFPSR_GUI_FLEXREGION

#include <cstdint>
#include "../../math/IVector.h"
#include "../../math/IRect.h"
#include "../math/scalar.h"
#include "../../api/stringAPI.h"
#include "../persistent/ClassFactory.h"

namespace dsr {

struct FlexValue : public Persistent {
PERSISTENT_DECLARATION(FlexValue)
private:
	int32_t ratio = 0; // 0% to 100%
	int32_t offset = 0; // +- offset
public:
	FlexValue() {}
	FlexValue(int ratio, int offset) : ratio(min(max(0, ratio), 100)), offset(offset) {}
public:
	bool assignValue(const ReadableString &text, const ReadableString &fromPath) override;
	String& toStreamIndented(String& out, const ReadableString& indentation) const override;
public:
	int32_t getRatio() const { return this->ratio; }
	int32_t getOffset() const { return this->offset; }
	int32_t getValue(int32_t minimum, int32_t maximum) const { return ((minimum * (100 - this->ratio)) + (maximum * this->ratio)) / 100 + this->offset; }
};
inline bool operator==(const FlexValue &left, const FlexValue &right) {
	return left.getRatio() == right.getRatio() && left.getOffset() == right.getOffset();
}
inline bool operator!=(const FlexValue &left, const FlexValue &right) {
	return !(left == right);
}

struct FlexRegion {
public:
	FlexValue left, top, right, bottom;
public:
	void setLeft(const FlexValue &left) { this->left = left; }
	void setTop(const FlexValue &top) { this->top = top; }
	void setRight(const FlexValue &right) { this->right = right; }
	void setBottom(const FlexValue &bottom) { this->bottom = bottom; }
	void setLeft(const ReadableString &left) { this->left = FlexValue(left, U""); }
	void setTop(const ReadableString &top) { this->top = FlexValue(top, U""); }
	void setRight(const ReadableString &right) { this->right = FlexValue(right, U""); }
	void setBottom(const ReadableString &bottom) { this->bottom = FlexValue(bottom, U""); }
public:
	// Full region
	FlexRegion() {
		this->left = FlexValue(0, 0);
		this->top = FlexValue(0, 0);
		this->right = FlexValue(100, 0);
		this->bottom = FlexValue(100, 0);
	}
	// Upper left aligned region
	explicit FlexRegion(const IRect &location) {
		this->left = FlexValue(0, location.left());
		this->top = FlexValue(0, location.top());
		this->right = FlexValue(0, location.right());
		this->bottom = FlexValue(0, location.bottom());
	}
	// Flexible region
	FlexRegion(int leftRatio, int leftOffset, int topRatio, int topOffset, int rightRatio, int rightOffset, int bottomRatio, int bottomOffset) {
		this->left = FlexValue(leftRatio, leftOffset);
		this->top = FlexValue(topRatio, topOffset);
		this->right = FlexValue(rightRatio, rightOffset);
		this->bottom = FlexValue(bottomRatio, bottomOffset);
	}
	// Parse individual flex values from text
	FlexRegion(const ReadableString &left, const ReadableString &top, const ReadableString &right, const ReadableString &bottom) {
		this->setLeft(left);
		this->setTop(top);
		this->setRight(right);
		this->setBottom(bottom);
	}
public:
	virtual IRect getNewLocation(const IRect &givenSpace);
};

}

#endif

