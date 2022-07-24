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
// * pathSeparator is the token used to separate folders in the system, expressed as a UTF-32 string literal.
// * accessFile is the function for opening a file using the UTF-32 filename, for reading or writing.
//   The C API is used for access, because some C++ standard library implementations don't support wide strings for MS-Windows.
#if defined(WIN32) || defined(_WIN32)
	#include <windows.h>
	static const char32_t* pathSeparator = U"\\";
	static FILE* accessFile(const ReadableString &filename, bool write) {
		Buffer pathBuffer = string_saveToMemory(filename, CharacterEncoding::BOM_UTF16LE, LineEncoding::CrLf, false, true);
		return _wfopen((const wchar_t*)buffer_dangerous_getUnsafeData(pathBuffer), write ? L"wb" : L"rb");
	}
#else
	static const char32_t* pathSeparator = U"/";
	static FILE* accessFile(const ReadableString &filename, bool write) {
		Buffer pathBuffer = string_saveToMemory(filename, CharacterEncoding::BOM_UTF8, LineEncoding::CrLf, false, true);
		return fopen((const char*)buffer_dangerous_getUnsafeData(pathBuffer), write ? "wb" : "rb");
	}
#endif

Buffer file_loadBuffer(const ReadableString& filename, bool mustExist) {
	FILE *file = accessFile(filename, false);
	if (file != nullptr) {
		// Get the file's size by going to the end, measuring, and going back
		fseek(file, 0L, SEEK_END);
		int64_t fileSize = ftell(file);
		rewind(file);
		// Allocate a buffer of the file's size
		Buffer buffer = buffer_create(fileSize);
		fread((void*)buffer_dangerous_getUnsafeData(buffer), fileSize, 1, file);
		fclose(file);
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
		FILE *file = accessFile(filename, true);
		if (file != nullptr) {
			fwrite((void*)buffer_dangerous_getUnsafeData(buffer), buffer_getSize(buffer), 1, file);
			fclose(file);
		} else {
			throwError("Failed to save ", filename, ".\n");
		}
	}
}

const char32_t* file_separator() {
	return pathSeparator;
}

}
