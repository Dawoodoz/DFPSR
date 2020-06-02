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

#include "PersistentString.h"

using namespace dsr;

PERSISTENT_DEFINITION(PersistentString)

PersistentString PersistentString::unmangled(const ReadableString &text) {
	PersistentString result;
	result.value = text;
	return result;
}

bool PersistentString::assignValue(const ReadableString &text) {
	this->value = string_unmangleQuote(text);
	return true;
}

String& PersistentString::toStreamIndented(String& out, const ReadableString& indentation) const {
	string_append(out, indentation, string_mangleQuote(this->value));
	return out;
}

