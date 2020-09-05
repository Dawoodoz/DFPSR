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

#include "configAPI.h"

using namespace dsr;

void dsr::config_parse_ini(const ReadableString& content, ConfigIniCallback receiverLambda) {
	List<String> lines = string_split(content, U'\n');
	String block = U"";
	for (int l = 0; l < lines.length(); l++) {
		// Get the current line
		String command = lines[l];
		// Skip comments
		int commentIndex = string_findFirst(command, U';');
		if (commentIndex > -1) {
			string_before(command, commentIndex);
		}
		// Find assignments
		int assignmentIndex = string_findFirst(command, U'=');
		if (assignmentIndex > -1) {
			String key = string_removeOuterWhiteSpace(string_before(command, assignmentIndex));
			String value = string_removeOuterWhiteSpace(string_after(command, assignmentIndex));
			receiverLambda(block, key, value);
		} else {
			int blockStartIndex = string_findFirst(command, U'[');
			int blockEndIndex = string_findFirst(command, U']');
			if (blockStartIndex > -1 && blockEndIndex > -1) {
				block = string_removeOuterWhiteSpace(string_inclusiveRange(command, blockStartIndex + 1, blockEndIndex - 1));
			}
		}
	}
}

