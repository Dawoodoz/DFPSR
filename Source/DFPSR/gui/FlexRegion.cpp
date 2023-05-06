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

#include "FlexRegion.h"

using namespace dsr;

PERSISTENT_DEFINITION(FlexValue)

bool FlexValue::assignValue(const ReadableString &text, const ReadableString &fromPath) {
	int perCentIndex = string_findFirst(text, U'%');
	if (perCentIndex > -1) {
		// Explicit %
		ReadableString leftSide = string_before(text, perCentIndex);
		ReadableString rightSide = string_after(text, perCentIndex);
		this->ratio = string_toInteger(leftSide);
		this->offset = string_toInteger(rightSide);
	} else {
		// Implicitly 0%
		this->ratio = 0;
		this->offset = string_toInteger(text);
	}
	return true; // TODO: Discriminate bad input
}

String& FlexValue::toStreamIndented(String& out, const ReadableString& indentation) const {
	string_append(out, indentation);
	if (this->ratio != 0) {
		string_append(out, this->ratio, U"%");
	}
	if (this->ratio == 0 || this->offset != 0) {
		if (this->offset > 0) {
			string_append(out, U"+");
		}
		string_append(out, this->offset);
	}
	return out;
}

IRect FlexRegion::getNewLocation(const IRect &givenSpace) {
	return IRect::FromBounds(
		this->left.getValue(givenSpace.left(), givenSpace.right()),
		this->top.getValue(givenSpace.top(), givenSpace.bottom()),
		this->right.getValue(givenSpace.left(), givenSpace.right()),
		this->bottom.getValue(givenSpace.top(), givenSpace.bottom())
	);
}

