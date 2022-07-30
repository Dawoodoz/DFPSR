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

#include "fileAPI.h"
#ifdef USE_MICROSOFT_WINDOWS
	#include <windows.h>
#else
	#include <unistd.h>
#endif
#include <fstream>
#include <cstdlib>
#include "bufferAPI.h"

namespace dsr {

// If porting to a new operating system that is not following Posix standard, list how the file system works here.
// * pathSeparator is the token used to separate folders in the system, expressed as a UTF-32 string literal.
// * accessFile is the function for opening a file using the UTF-32 filename, for reading or writing.
//   The C API is used for access, because some C++ standard library implementations don't support wide strings for MS-Windows.
#ifdef USE_MICROSOFT_WINDOWS
	using NativeChar = wchar_t; // UTF-16
	static const char32_t* pathSeparator = U"\\";
	static const CharacterEncoding nativeEncoding = CharacterEncoding::BOM_UTF16LE;
	#define FILE_ACCESS_FUNCTION _wfopen
	#define FILE_ACCESS_SELECTION (write ? L"wb" : L"rb")
	List<String> file_impl_getInputArguments() {
		// Get a pointer to static memory with the command
		LPWSTR cmd = GetCommandLineW();
		// Split the arguments into an array of arguments
		int argc = 0;
		LPWSTR *argv = CommandLineToArgvW(cmd, &argc);
		// Convert the arguments into dsr::List<dsr::String>
		List<String> args = file_impl_convertInputArguments(argc, (void**)argv);
		// Free the native list of arguments
		LocalFree(argv);
		return args;
	}
#else
	using NativeChar = char; // UTF-8
	static const char32_t* pathSeparator = U"/";
	static const CharacterEncoding nativeEncoding = CharacterEncoding::BOM_UTF8;
	#define FILE_ACCESS_FUNCTION fopen
	#define FILE_ACCESS_SELECTION (write ? "wb" : "rb")
	List<String> file_impl_getInputArguments() { return List<String>(); }
#endif

// Length of fixed size buffers.
const int maxLength = 512;

static const NativeChar* toNativeString(const ReadableString &filename, Buffer &buffer) {
	buffer = string_saveToMemory(filename, nativeEncoding, LineEncoding::CrLf, false, true);
	return (const NativeChar*)buffer_dangerous_getUnsafeData(buffer);
}

static String fromNativeString(const NativeChar *text) {
	return string_dangerous_decodeFromData(text, nativeEncoding);
}

List<String> file_impl_convertInputArguments(int argn, void **argv) {
	List<String> result;
	result.reserve(argn);
	for (int a = 0; a < argn; a++) {
		result.push(fromNativeString((NativeChar*)(argv[a])));
	}
	return result;
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

inline bool isSeparator(DsrChar c) {
	return c == U'\\' || c == U'/';
}

// Returns the index of the last / or \ in path, or defaultIndex if none existed.
static int64_t getLastSeparator(const ReadableString &path, int defaultIndex) {
	for (int64_t i = string_length(path) - 1; i >= 0; i--) {
		DsrChar c = path[i];
		if (isSeparator(c)) {
			return i;
		}
	}
	return defaultIndex;
}

ReadableString file_getPathlessName(const ReadableString &path) {
	return string_after(path, getLastSeparator(path, -1));
}

ReadableString file_getFolderPath(const ReadableString &path) {
	return string_before(path, getLastSeparator(path, string_length(path)));
}

bool file_hasRoot(const ReadableString &path) {
	#ifdef USE_MICROSOFT_WINDOWS
		// If a colon is found, it is a root path.
		return string_findFirst(path, U':') > -1;
	#else
		// If the path begins with a separator, it is the root folder in Posix systems.
		return path[0] == U'/';
	#endif
}

String file_getCurrentPath() {
	#ifdef USE_MICROSOFT_WINDOWS
		NativeChar resultBuffer[maxLength + 1] = {0};
		GetCurrentDirectoryW(maxLength, resultBuffer);
		return fromNativeString(resultBuffer);
	#else
		NativeChar resultBuffer[maxLength + 1] = {0};
		getcwd(resultBuffer, maxLength);
		return fromNativeString(resultBuffer);
	#endif
}

#ifdef USE_MICROSOFT_WINDOWS
static String file_getApplicationFilePath() {
	NativeChar resultBuffer[maxLength + 1] = {0};
	GetModuleFileNameW(nullptr, resultBuffer, maxLength);
	return fromNativeString(resultBuffer);
}
#endif

String file_getApplicationFolder(bool allowFallback) {
	#ifdef USE_MICROSOFT_WINDOWS
		return file_getFolderPath(file_getApplicationFilePath());
	#else
		#ifdef USE_LINUX
			// TODO: Implement using "/proc/self/exe" on Linux
			//       https://www.wikitechy.com/tutorials/linux/how-to-find-the-location-of-the-executable-in-c
			NativeChar resultBuffer[maxLength + 1] = {0};
			//"/proc/curproc/file" on FreeBSD, which is not yet supported
    		//"/proc/self/path/a.out" on Solaris, which is not yet supported
			readlink("/proc/self/exe", resultBuffer, maxLength);
			return file_getFolderPath(fromNativeString(resultBuffer));
		#else
			if (allowFallback) {
				return file_getCurrentPath();
			} else {
				throwError("file_getApplicationFolder is not implemented!\n");
			}
		#endif
	#endif
}

String file_combinePaths(const ReadableString &a, const ReadableString &b) {
	if (isSeparator(a[string_length(a) - 1])) {
		// Already ending with a separator.
		return string_combine(a, b);
	} else {
		// Combine using a separator.
		return string_combine(a, pathSeparator, b);
	}
}

String file_getAbsolutePath(const ReadableString &path) {
	if (file_hasRoot(path)) {
		return path;
	} else {
		return file_combinePaths(file_getCurrentPath(), path);
	}
}

}
