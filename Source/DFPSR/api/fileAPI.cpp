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

#include <fstream>
#include <cstdlib>
#include "fileAPI.h"
#include "bufferAPI.h"

namespace dsr {

// If porting to a new operating system that is not following Posix standard, list how the file system works here.
#if defined(WIN32) || defined(_WIN32)
	// How the file system works on Microsoft Windows.
	using NativePathChar = wchar_t;
	static const CharacterEncoding NativePathEncoding = CharacterEncoding::BOM_UTF16LE;
	static const char32_t* pathSeparator = U"\\";
#else
	// How the file system is assumed to work on most other systems.
	using NativePathChar = char;
	static const CharacterEncoding NativePathEncoding = CharacterEncoding::BOM_UTF8;
	static const char32_t* pathSeparator = U"/";
#endif

#define GET_PATH_BUFFER(FILENAME) Buffer pathBuffer = string_saveToMemory(FILENAME, NativePathEncoding, LineEncoding::CrLf, false);
#define NATIVE_PATH_FROM_BUFFER (NativePathChar*)buffer_dangerous_getUnsafeData(pathBuffer)

Buffer file_loadBuffer(const ReadableString& filename, bool mustExist) {
	// Convert the filename into a the system's expected path encoding.
	GET_PATH_BUFFER(filename);
	// Use the native path to open the file for reading.
	std::ifstream fileStream(NATIVE_PATH_FROM_BUFFER, std::ios_base::in | std::ios_base::binary);
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
			throwError(U"The file ", filename, U" could not be opened for reading.\n");
		}
		// If the file cound not be found and opened, an empty buffer is returned
		return Buffer();
	}
}

void file_saveBuffer(const ReadableString& filename, Buffer buffer) {
	if (!buffer_exists(buffer)) {
		throwError(U"buffer_save: Cannot save a buffer that don't exist to a file.\n");
	} else {
		// Convert the filename into a the system's expected path encoding.
		GET_PATH_BUFFER(filename);
		// Use the native path to open the file for writing.
		std::ofstream fileStream(NATIVE_PATH_FROM_BUFFER, std::ios_base::out | std::ios_base::binary);
		if (fileStream.is_open()) {
			fileStream.write((char*)buffer_dangerous_getUnsafeData(buffer), buffer_getSize(buffer));
			fileStream.close();
		} else {
			throwError("Failed to save ", filename, "\n");
		}
	}
}

const char32_t* file_separator() {
	return pathSeparator;
}

}
