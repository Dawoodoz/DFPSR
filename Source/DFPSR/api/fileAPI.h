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

#ifndef DFPSR_API_FILE
#define DFPSR_API_FILE

#include "../api/stringAPI.h"
#include "bufferAPI.h"

// A module for file access that exists to prevent cyclic dependencies between strings and buffers.
//   Buffers need a filename to be saved or loaded while strings use buffers to store their characters.

namespace dsr {
	// Post-condition:
	//   Returns the content of the file referred to be filename.
	//   If mustExist is true, then failure to load will throw an exception.
	//   If mustExist is false, then failure to load will return an empty handle (returning false for buffer_exists).
	Buffer file_loadBuffer(const ReadableString& filename, bool mustExist = true);

	// Side-effect: Saves buffer to filename as a binary file.
	// Pre-condition: buffer exists
	void file_saveBuffer(const ReadableString& filename, Buffer buffer);

	// Get a path separator for the target operating system.
	const char32_t* file_separator();
}

#endif
