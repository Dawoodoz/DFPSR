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
#ifdef __cpp_lib_filesystem
	#include <filesystem>
#endif

namespace dsr {

// If porting to a new operating system that is not following Posix standard, list how the file system works here.
// * pathSeparator is the token used to separate folders in the system, expressed as a UTF-32 string literal.
// * accessFile is the function for opening a file using the UTF-32 filename, for reading or writing.
//   The C API is used for access, because some C++ standard library implementations don't support wide strings for MS-Windows.
#if defined(WIN32) || defined(_WIN32)
	#include <windows.h>
	static const char32_t* pathSeparator = U"\\";
	static const CharacterEncoding nativeEncoding = CharacterEncoding::BOM_UTF16LE;
	#define FILE_ACCESS_FUNCTION _wfopen
	#define FILE_ACCESS_SELECTION (write ? L"wb" : L"rb")
#else
	static const char32_t* pathSeparator = U"/";
	static const CharacterEncoding nativeEncoding = CharacterEncoding::BOM_UTF8;
	#define FILE_ACCESS_FUNCTION fopen
	#define FILE_ACCESS_SELECTION (write ? "wb" : "rb")
#endif

static const NativeChar* toNativeString(const ReadableString &filename, Buffer &buffer) {
	buffer = string_saveToMemory(filename, nativeEncoding, LineEncoding::CrLf, false, true);
	return (const NativeChar*)buffer_dangerous_getUnsafeData(buffer);
}

static String fromNativeString(NativeChar *text) {
	return string_dangerous_decodeFromData(text, nativeEncoding);
}

static FILE* accessFile(const ReadableString &filename, bool write) {
	Buffer buffer;
	return FILE_ACCESS_FUNCTION(toNativeString(filename, buffer), FILE_ACCESS_SELECTION);
}

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

ReadableString file_getPathlessName(const ReadableString &path) {
	int lastSeparator = -1;
	for (int64_t i = string_length(path) - 1; i >= 0; i--) {
		DsrChar c = path[i];
		if (c == U'\\' || c == U'/') {
			lastSeparator = i;
			break;
		}
	}
	return string_after(path, lastSeparator);
}

List<String> file_convertInputArguments(int argn, NativeChar **argv) {
	List<String> result;
	result.reserve(argn);
	for (int a = 0; a < argn; a++) {
		result.push(fromNativeString(argv[a]));
	}
	return result;
}

#ifdef __cpp_lib_filesystem

static String fromU32String(std::u32string text) {
	dsr::String result;
	string_reserve(result, text.length());
	int i = 0;
	while (true) {
		DsrChar c = text[i];
		if (c == '\0') {
			break;
		} else {
			string_appendChar(result, c);
		}
		i++;
	}
	return result;
}

#define FROM_PATH(CONTENT) fromU32String((CONTENT).generic_u32string())

// Macros for creating wrapper functions
#define DEF_FUNC_VOID_TO_STRING(FUNC_NAME, CALL_NAME) String FUNC_NAME() { return FROM_PATH(CALL_NAME()); }
#define DEF_FUNC_STRING_TO_STRING(FUNC_NAME, ARG_NAME, CALL_NAME) String FUNC_NAME(const ReadableString& ARG_NAME) { Buffer buffer; return FROM_PATH(CALL_NAME(toNativeString(ARG_NAME, buffer))); }
#define DEF_FUNC_STRING_TO_OTHER(FUNC_NAME, RETURN_TYPE, ARG_NAME, CALL_NAME) RETURN_TYPE FUNC_NAME(const ReadableString& ARG_NAME) { Buffer buffer; return CALL_NAME(toNativeString(ARG_NAME, buffer)); }

// Wrapper function implementations
DEF_FUNC_VOID_TO_STRING(file_getCurrentPath, std::filesystem::current_path)
DEF_FUNC_STRING_TO_STRING(file_getAbsolutePath, path, std::filesystem::absolute)
DEF_FUNC_STRING_TO_STRING(file_getCanonicalPath, path, std::filesystem::canonical)
DEF_FUNC_STRING_TO_OTHER(file_exists, bool, path, std::filesystem::exists)
DEF_FUNC_STRING_TO_OTHER(file_getSize, int64_t, path, std::filesystem::file_size)
DEF_FUNC_STRING_TO_OTHER(file_remove, bool, path, std::filesystem::remove)
DEF_FUNC_STRING_TO_OTHER(file_removeRecursively, bool, path, std::filesystem::remove_all)
DEF_FUNC_STRING_TO_OTHER(file_createFolder, bool, path, std::filesystem::create_directory)

void file_getFolderContent(const ReadableString& folderPath, std::function<void(ReadableString, EntryType)> action) {
	Buffer buffer;
	for (auto const& entry : std::filesystem::directory_iterator{toNativeString(folderPath, buffer)}) {
		EntryType entryType = EntryType::Unknown;
		if (entry.is_directory()) {
			entryType = EntryType::Folder;
		} else if (entry.is_regular_file()) {
			entryType = EntryType::File;
		} else if (entry.is_symlink()) {
			entryType = EntryType::SymbolicLink;
		}
		action(FROM_PATH(entry.path()), entryType);
	}
}

#endif

}
