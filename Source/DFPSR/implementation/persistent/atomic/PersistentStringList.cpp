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

#include "PersistentStringList.h"
#include "../../../collection/List.h"

using namespace dsr;

PERSISTENT_DEFINITION(PersistentStringList)

bool PersistentStringList::assignValue(const ReadableString &text, const ReadableString &fromPath) {
	bool quoted = false;
	bool first = true;
	bool hadComma = false;
	int32_t start = 0;
	this->value.clear();
	for (int32_t i = 0; i < string_length(text); i++) {
		DsrChar c = text[i];
		if (quoted) {
			if (c == U'\\') { // Escape sequence
				i++; // Skip the following character as content
			} else if (c == U'\"') { // Quote sign
				// End the quote
				String content = string_unmangleQuote(string_removeOuterWhiteSpace(string_inclusiveRange(text, start, i)));
				this->value.push(content);
				hadComma = false;
				quoted = false;
			}
		} else {
			if (c == U',') {
				// Assert correct use of comma separation
				if (hadComma) {
					throwError(U"Comma separated lists must have quotes around all individual elements to distinguish an empty list from an empty string!\n");
				} else {
					hadComma = true;
				}
			} else if (c == U'\"') { // Quote sign
				// Start the quote
				if (!(first || hadComma)) {
					throwError(U"String lists must be separated by commas!\n");
				}
				quoted = true;
				first = false;
				start = i;
			}
		}
	}
	if (hadComma) {
		throwError(U"String lists may not end with a comma!\n");
	}
	return true;
}

String& PersistentStringList::toStreamIndented(String& out, const ReadableString& indentation) const {
	string_append(out, indentation);
	for (int32_t i = 0; i < this->value.length(); i++) {
		if (i > 0) {
			string_append(out, ", ");
		}
		string_append(out, string_mangleQuote(this->value[i]));
	}
	return out;
}
