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
	#include <sys/stat.h>
	#include <dirent.h>
#endif
#include <fstream>
#include <cstdlib>
#include "bufferAPI.h"

namespace dsr {

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
	String modifiedFilename = file_optimizePath(filename);
	FILE *file = accessFile(modifiedFilename, false);
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
			throwError(U"Failed to load ", filename, " which was optimized into ", modifiedFilename, ".\n");
		}
		// If the file cound not be found and opened, an empty buffer is returned
		return Buffer();
	}
}

void file_saveBuffer(const ReadableString& filename, Buffer buffer) {
	String modifiedFilename = file_optimizePath(filename);
	if (!buffer_exists(buffer)) {
		throwError(U"buffer_save: Cannot save a buffer that don't exist to a file.\n");
	} else {
		FILE *file = accessFile(modifiedFilename, true);
		if (file != nullptr) {
			fwrite((void*)buffer_dangerous_getUnsafeData(buffer), buffer_getSize(buffer), 1, file);
			fclose(file);
		} else {
			throwError("Failed to save ", filename, " which was optimized into ", modifiedFilename, ".\n");
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

String file_optimizePath(const ReadableString &path) {
	String result;
	int inputLength = string_length(path);
	string_reserve(result, inputLength);
	for (int i = 0; i < inputLength; i++) {
		DsrChar c = path[i];
		if (isSeparator(c)) {
			string_append(result, pathSeparator);
		} else {
			string_appendChar(result, c);
		}
	}
	return result;
}

ReadableString file_getPathlessName(const ReadableString &path) {
	return string_after(path, getLastSeparator(path, -1));
}

ReadableString file_getParentFolder(const ReadableString &path) {
	return string_before(path, getLastSeparator(path, 0));
}

bool file_hasRoot(const ReadableString &path, bool treatHomeFolderAsRoot) {
	#ifdef USE_MICROSOFT_WINDOWS
		// If a colon is found, it is a root path.
		return string_findFirst(path, U':') > -1;
	#else
		// If the path begins with a separator, it is the root folder in Posix systems.
		// If the path begins with a tilde (~), it is a home folder.
		DsrChar firstC = path[0];
		return firstC == U'/' || (treatHomeFolderAsRoot && firstC == U'~');
	#endif
}

bool file_setCurrentPath(const ReadableString &path) {
	Buffer buffer;
	const NativeChar *nativePath = toNativeString(file_optimizePath(path), buffer);
	#ifdef USE_MICROSOFT_WINDOWS
		return SetCurrentDirectoryW(nativePath);
	#else
		return chdir(nativePath) == 0;
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

String file_getApplicationFolder(bool allowFallback) {
	#ifdef USE_MICROSOFT_WINDOWS
		NativeChar resultBuffer[maxLength + 1] = {0};
		GetModuleFileNameW(nullptr, resultBuffer, maxLength);
		return file_getParentFolder(fromNativeString(resultBuffer));
	#else
		NativeChar resultBuffer[maxLength + 1] = {0};
		if (readlink("/proc/self/exe", resultBuffer, maxLength) != -1) {
			// Linux detected
			return file_getParentFolder(fromNativeString(resultBuffer));
		} else if (readlink("/proc/curproc/file", resultBuffer, maxLength) != -1) {
			// BSD detected
			return file_getParentFolder(fromNativeString(resultBuffer));
		} else if (readlink("/proc/self/path/a.out", resultBuffer, maxLength) != -1) {
			// Solaris detected
			return file_getParentFolder(fromNativeString(resultBuffer));
		} else if (allowFallback) {
			return file_getCurrentPath();
		} else {
			throwError("file_getApplicationFolder is not implemented for the current system!\n");
			return U"";
		}
	#endif
}

String file_combinePaths(const ReadableString &a, const ReadableString &b) {
	if (file_hasRoot(b)) {
		return b;
	} else {
		if (isSeparator(a[string_length(a) - 1])) {
			// Already ending with a separator.
			return string_combine(a, b);
		} else {
			// Combine using a separator.
			return string_combine(a, pathSeparator, b);
		}
	}
}

String file_getAbsolutePath(const ReadableString &path) {
	if (file_hasRoot(path)) {
		return path;
	} else {
		return file_combinePaths(file_getCurrentPath(), path);
	}
}

int64_t file_getFileSize(const ReadableString& filename) {
	int64_t result = -1;
	String modifiedFilename = file_optimizePath(filename);
	Buffer buffer;
	const NativeChar *nativePath = toNativeString(modifiedFilename, buffer);
	#ifdef USE_MICROSOFT_WINDOWS
		LARGE_INTEGER fileSize;
		HANDLE fileHandle = CreateFileW(nativePath, 0, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fileHandle != INVALID_HANDLE_VALUE) {
			if (GetFileSizeEx(fileHandle, &fileSize)) {
				result = fileSize.QuadPart;
			}
			CloseHandle(fileHandle);
		}
	#else
		struct stat info;
		if (stat(nativePath, &info) == 0) {
			result = info.st_size;
		}
	#endif
	return result;
}

String& string_toStreamIndented(String& target, const EntryType& source, const ReadableString& indentation) {
	string_append(target, indentation);
	if (source == EntryType::NotFound) {
		string_append(target, U"not found");
	} else if (source == EntryType::File) {
		string_append(target, U"a file");
	} else if (source == EntryType::Folder) {
		string_append(target, U"a folder");
	} else if (source == EntryType::SymbolicLink) {
		string_append(target, U"a symbolic link");
	} else {
		string_append(target, U"unhandled");
	}
	return target;
}

EntryType file_getEntryType(const ReadableString &path) {
	EntryType result = EntryType::NotFound;
	String modifiedPath = file_optimizePath(path);
	Buffer buffer;
	const NativeChar *nativePath = toNativeString(modifiedPath, buffer);
	#ifdef USE_MICROSOFT_WINDOWS
		DWORD dwAttrib = GetFileAttributesW(nativePath);
		if (dwAttrib != INVALID_FILE_ATTRIBUTES) {
			if (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) {
				result = EntryType::Folder;
			} else {
				result = EntryType::File;
			}
		}
	#else
		struct stat info;
		int errorCode = stat(nativePath, &info);
		if (errorCode == 0) {
			if (S_ISDIR(info.st_mode)) {
				result = EntryType::Folder;
			} else if (S_ISREG(info.st_mode)) {
				result = EntryType::File;
			} else if (S_ISLNK(info.st_mode)) {
				result = EntryType::SymbolicLink;
			} else {
				result = EntryType::UnhandledType;
			}
		}
	#endif
	return result;
}

bool file_getFolderContent(const ReadableString& folderPath, std::function<void(const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType)> action) {
	String modifiedPath = file_optimizePath(folderPath);
	#ifdef USE_MICROSOFT_WINDOWS
		String pattern = file_combinePaths(modifiedPath, U"*.*");
		Buffer buffer;
		const NativeChar *nativePattern = toNativeString(pattern, buffer);
		WIN32_FIND_DATAW findData;
		HANDLE findHandle = FindFirstFileW(nativePattern, &findData);
		if (findHandle == INVALID_HANDLE_VALUE) {
			return false;
		} else {
			while (true) {
				String entryName = fromNativeString(findData.cFileName);
				if (!string_match(entryName, U".") && !string_match(entryName, U"..")) {
					String entryPath = file_combinePaths(modifiedPath, entryName);
					EntryType entryType = EntryType::UnhandledType;
					if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
						entryType = EntryType::Folder;
					} else {
						entryType = EntryType::File;
					}
					action(entryPath, entryName, entryType);
				}
				if (!FindNextFileW(findHandle, &findData)) { break; }
			}
			FindClose(findHandle);
		}
	#else
		Buffer buffer;
		const NativeChar *nativePath = toNativeString(modifiedPath, buffer);
		DIR *directory = opendir(nativePath);
		if (directory == nullptr) {
			return false;
		} else {
			while (true) {
				dirent *entry = readdir(directory);
				if (entry != nullptr) {
					String entryName = fromNativeString(entry->d_name);
					if (!string_match(entryName, U".") && !string_match(entryName, U"..")) {
						String entryPath = file_combinePaths(modifiedPath, entryName);
						EntryType entryType = file_getEntryType(entryPath);
						action(entryPath, entryName, entryType);
					}
				} else {
					break;
				}
			}
		}
		closedir(directory);
	#endif
	return true;
}

}
