﻿// zlib open source license
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

#include "PersistentBoolean.h"

using namespace dsr;

PERSISTENT_DEFINITION(PersistentBoolean)

bool PersistentBoolean::assignValue(const ReadableString &text, const ReadableString &fromPath) {
	if (string_match(text, U"1") ) {
		this->value = true;
		return true;
	} else if (string_match(text, U"0") ) {
		this->value = false;
		return true;
	} else {
		return false;
	}
}

String& PersistentBoolean::toStreamIndented(String& out, const ReadableString& indentation) const {
	string_append(out, indentation, this->value ? U"1" : U"0");
	return out;
}

