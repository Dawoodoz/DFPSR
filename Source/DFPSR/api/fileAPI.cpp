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

#include <fstream>
#include <cstdlib>
#include "bufferAPI.h"
#include "../math/scalar.h"
#include "../base/text.h"

namespace dsr {

// TODO: Try converting to UTF-8 for file names, which would only have another chance at working
static char toAscii(DsrChar c) {
	if (c > 127) {
		return '?';
	} else {
		return c;
	}
}
#define TO_RAW_ASCII(TARGET, SOURCE) \
	char TARGET[SOURCE.length() + 1]; \
	for (int i = 0; i < SOURCE.length(); i++) { \
		TARGET[i] = toAscii(SOURCE[i]); \
	} \
	TARGET[SOURCE.length()] = '\0';

Buffer buffer_load(const ReadableString& filename, bool mustExist) {
	// TODO: Load files using Unicode filenames when available
	TO_RAW_ASCII(asciiFilename, filename);
	std::ifstream fileStream(asciiFilename, std::ios_base::in | std::ios_base::binary);
	if (fileStream.is_open()) {
		// Get the file's length and allocate an array for the raw encoding
		fileStream.seekg (0, fileStream.end);
		int64_t fileLength = fileStream.tellg();
		fileStream.seekg (0, fileStream.beg);
		Buffer buffer = buffer_create(fileLength);
		fileStream.read((char*)buffer_dangerous_getUnsafeData(buffer), fileLength);
		return buffer;
	} else {
		if (mustExist) {
			throwError(U"The text file ", filename, U" could not be opened for reading.\n");
		}
		// If the file cound not be found and opened, an empty buffer is returned
		return Buffer();
	}
}

void buffer_save(const ReadableString& filename, Buffer buffer) {
	// TODO: Save files using Unicode filenames
	if (!buffer_exists(buffer)) {
		throwError(U"buffer_save: Cannot save a buffer that don't exist to a file.\n");
	} else {
		TO_RAW_ASCII(asciiFilename, filename);
		std::ofstream fileStream(asciiFilename, std::ios_base::out | std::ios_base::binary);
		if (fileStream.is_open()) {
			fileStream.write((char*)buffer_dangerous_getUnsafeData(buffer), buffer_getSize(buffer));
			fileStream.close();
		} else {
			throwError("Failed to save ", filename, "\n");
		}
	}
}

}
