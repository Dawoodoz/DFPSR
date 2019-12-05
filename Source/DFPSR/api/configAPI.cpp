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
	List<ReadableString> lines = content.split(U'\n');
	String block = U"";
	for (int l = 0; l < lines.length(); l++) {
		// Get the current line
		ReadableString command = lines[l];
		// Skip comments
		int commentIndex = command.findFirst(U';');
		if (commentIndex > -1) {
			command = command.before(commentIndex);
		}
		// Find assignments
		int assignmentIndex = command.findFirst(U'=');
		if (assignmentIndex > -1) {
			ReadableString key = string_removeOuterWhiteSpace(command.before(assignmentIndex));
			ReadableString value = string_removeOuterWhiteSpace(command.after(assignmentIndex));
			receiverLambda(block, key, value);
		} else {
			int blockStartIndex = command.findFirst(U'[');
			int blockEndIndex = command.findFirst(U']');
			if (blockStartIndex > -1 && blockEndIndex > -1) {
				block = string_removeOuterWhiteSpace(command.inclusiveRange(blockStartIndex + 1, blockEndIndex - 1));
			}
		}
	}
}

