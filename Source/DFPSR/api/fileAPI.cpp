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

// Include fileAPI without falling back on local syntax implicitly.
//   This prevents any local syntax from being implied in functions that are supposed to use variable pathSyntax.
#define NO_IMPLICIT_PATH_SYNTAX
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

constexpr const char32_t* getPathSeparator(PathSyntax pathSyntax) {
	if (pathSyntax == PathSyntax::Windows) {
		return U"\\";
	} else if (pathSyntax == PathSyntax::Posix) {
		return U"/";
	} else {
		return U"?";
	}
}

#ifdef USE_MICROSOFT_WINDOWS
	using NativeChar = wchar_t; // UTF-16
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
	String modifiedFilename = file_optimizePath(filename, LOCAL_PATH_SYNTAX);
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
			throwError(U"Failed to load ", modifiedFilename, ".\n");
		}
		// If the file cound not be found and opened, an empty buffer is returned
		return Buffer();
	}
}

bool file_saveBuffer(const ReadableString& filename, Buffer buffer, bool mustWork) {
	String modifiedFilename = file_optimizePath(filename, LOCAL_PATH_SYNTAX);
	if (!buffer_exists(buffer)) {
		if (mustWork) {
			throwError(U"buffer_save: Can't save a buffer that don't exist to a file.\n");
		}
		return false;
	} else {
		FILE *file = accessFile(modifiedFilename, true);
		if (file != nullptr) {
			fwrite((void*)buffer_dangerous_getUnsafeData(buffer), buffer_getSize(buffer), 1, file);
			fclose(file);
		} else {
			if (mustWork) {
				throwError("Failed to save ", modifiedFilename, ".\n");
			}
			return false;
		}
	}
	// Success if there are no errors.
	return true;
}

const char32_t* file_separator() {
	return getPathSeparator(LOCAL_PATH_SYNTAX);
}

inline bool isSeparator(DsrChar c) {
	return c == U'\\' || c == U'/';
}

// Returns the index of the first / or \ in path, or defaultIndex if none existed.
static int64_t getFirstSeparator(const ReadableString &path, int64_t defaultIndex) {
	for (int64_t i = 0; i < string_length(path); i++) {
		DsrChar c = path[i];
		if (isSeparator(c)) {
			return i;
		}
	}
	return defaultIndex;
}

// Returns the index of the last / or \ in path, or defaultIndex if none existed.
static int64_t getLastSeparator(const ReadableString &path, int64_t defaultIndex) {
	for (int64_t i = string_length(path) - 1; i >= 0; i--) {
		DsrChar c = path[i];
		if (isSeparator(c)) {
			return i;
		}
	}
	return defaultIndex;
}

String file_optimizePath(const ReadableString &path, PathSyntax pathSyntax) {
	String result; // The final output being appended.
	String currentEntry; // The current entry.
	bool hadSeparator = false;
	bool hadContent = false;
	int64_t inputLength = string_length(path);
	string_reserve(result, inputLength);
	// Read null terminator from one element outside of the path to allow concluding an entry not followed by any separator.
	//   The null terminator is not actually stored, but reading out of bound gives a null terminator.
	for (int64_t i = 0; i <= inputLength; i++) {
		DsrChar c = path[i];
		bool separator = isSeparator(c);
		if (separator || i == inputLength) {
			bool appendEntry = true;
			bool appendSeparator = separator;
			if (hadSeparator) {
				if (hadContent && string_length(currentEntry) == 0) {
					// Reduce non-leading // into / by skipping "" entries.
					// Any leading multiples of slashes have their count preserved, because some systems use them to indicate special use cases.
					appendEntry = false;
					appendSeparator = false;
				} else if (string_match(currentEntry, U".")) {
					// Reduce /./ into / by skipping "." entries.
					appendEntry = false;
					appendSeparator = false;
				} else if (string_match(currentEntry, U"..")) {
					// Reduce the parent directory against the reverse ".." entry.
					result = file_getRelativeParentFolder(result, pathSyntax);
					if (string_match(result, U"?")) {
						return U"?";
					}
					appendEntry = false;
				}
			}
			if (appendEntry) {
				string_append(result, string_removeOuterWhiteSpace(currentEntry));
			}
			if (appendSeparator) {
				string_append(result, getPathSeparator(pathSyntax));
			}
			currentEntry = U"";
			if (separator) {
				hadSeparator = true;
			}
		} else {
			string_appendChar(currentEntry, c);
			hadContent = true;
		}
	}
	// Remove trailing separators if we had content.
	if (hadSeparator && hadContent) {
		int64_t lastNonSeparator = -1;
		for (int64_t i = string_length(result) - 1; i >= 0; i--) {
			if (!isSeparator(result[i])) {
				lastNonSeparator = i;
				break;
			}
		}
		result = string_until(result, lastNonSeparator);
	}
	return result;
}

ReadableString file_getPathlessName(const ReadableString &path) {
	return string_after(path, getLastSeparator(path, -1));
}

String file_getRelativeParentFolder(const ReadableString &path, PathSyntax pathSyntax) {
	String optimizedPath = file_optimizePath(path, pathSyntax);
	if (string_length(optimizedPath) == 0) {
		// Use .. to go outside of the current directory.
		return U"..";
	} else if (string_match(file_getPathlessName(optimizedPath), U"?")) {
		// From unknown to unknown.
		return U"?";
	} else if (file_isRoot(optimizedPath, false, pathSyntax)) {
		// If it's the known true root, then we know that it does not have a parent and must fail.
		return U"?";
	} else if (file_isRoot(optimizedPath, true, pathSyntax)) {
		// If it's an alias for an arbitrary folder, use .. to leave it.
		return file_combinePaths(optimizedPath, U"..", pathSyntax);
	} else if (string_match(file_getPathlessName(optimizedPath), U"..")) {
		// Add more dots to the path.
		return file_combinePaths(optimizedPath, U"..", pathSyntax);
	} else {
		// Inside of something.
		int64_t lastSeparator = getLastSeparator(optimizedPath, 0);
		if (pathSyntax == PathSyntax::Windows) {
			// Return everything before the last separator.
			return string_before(optimizedPath, lastSeparator);
		} else { // PathSyntax::Posix
			if (file_hasRoot(path, false, pathSyntax) && lastSeparator == 0) {
				// Keep the absolute root.
				return U"/";
			} else {
				// Keep everything before the last separator.
				return string_before(optimizedPath, lastSeparator);
			}
		}
	}
}

String file_getTheoreticalAbsoluteParentFolder(const ReadableString &path, const ReadableString &currentPath, PathSyntax pathSyntax) {
	if (file_hasRoot(path, true, LOCAL_PATH_SYNTAX)) {
		// Absolute paths should be treated the same as a theoretical path.
		return file_getRelativeParentFolder(path, pathSyntax);
	} else {
		// If the input is not absolute, convert it before taking the parent directory.
		return file_getRelativeParentFolder(file_getTheoreticalAbsolutePath(path, currentPath, pathSyntax), pathSyntax);
	}
}

String file_getAbsoluteParentFolder(const ReadableString &path) {
	return file_getTheoreticalAbsoluteParentFolder(path, file_getCurrentPath(), LOCAL_PATH_SYNTAX);
}

bool file_isRoot(const ReadableString &path, bool treatHomeFolderAsRoot, PathSyntax pathSyntax) {
	ReadableString cleanPath = string_removeOuterWhiteSpace(path);
	int64_t length = string_length(cleanPath);
	if (length == 0) {
		// Relative path is not a root.
		return false;
	} else if (length == 1) {
		DsrChar c = cleanPath[0];
		if (pathSyntax == PathSyntax::Windows) {
			return c == U'\\'; // Implicit drive root.
		} else { // PathSyntax::Posix
			return c == U'/' || (c == U'~' && treatHomeFolderAsRoot); // Root over all drives or home folder.
		}
	} else {
		if (pathSyntax == PathSyntax::Windows && cleanPath[length - 1] == U':') {
			// C:, D:, ...
			return true;
		} else {
			return false;
		}
	}
}

bool file_hasRoot(const ReadableString &path, bool treatHomeFolderAsRoot, PathSyntax pathSyntax) {
	int64_t firstSeparator = getFirstSeparator(path, -1);
	if (firstSeparator == -1) {
		// If there is no separator, path has a root if it is a root.
		return file_isRoot(path, treatHomeFolderAsRoot, pathSyntax);
	} else if (firstSeparator == 0) {
		// Starting with a separator. Either an implicit drive on Windows or the whole system's root on Posix.
		return true;
	} else {
		// Has a root if the first entry before the first slash is a root.
		return file_isRoot(string_before(path, firstSeparator), treatHomeFolderAsRoot, pathSyntax);
	}
}

bool file_setCurrentPath(const ReadableString &path) {
	Buffer buffer;
	const NativeChar *nativePath = toNativeString(file_optimizePath(path, LOCAL_PATH_SYNTAX), buffer);
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

String file_followSymbolicLink(const ReadableString &path, bool mustExist) {
	String modifiedPath = file_optimizePath(path, LOCAL_PATH_SYNTAX);
	Buffer buffer;
	const NativeChar *nativePath = toNativeString(modifiedPath, buffer);
	#ifdef USE_MICROSOFT_WINDOWS
		// TODO: Is there anything that can be used as a symbolic link on Windows?
	#else
		NativeChar resultBuffer[maxLength + 1] = {0};
		if (readlink(nativePath, resultBuffer, maxLength) != -1) {
			return fromNativeString(resultBuffer);
		}
	#endif
	if (mustExist) { throwError(U"The symbolic link ", path, " could not be found!\n"); }
	return U"?";
}

String file_getApplicationFolder(bool allowFallback) {
	#ifdef USE_MICROSOFT_WINDOWS
		NativeChar resultBuffer[maxLength + 1] = {0};
		GetModuleFileNameW(nullptr, resultBuffer, maxLength);
		return file_getRelativeParentFolder(fromNativeString(resultBuffer), LOCAL_PATH_SYNTAX);
	#else
		NativeChar resultBuffer[maxLength + 1] = {0};
		if (readlink("/proc/self/exe", resultBuffer, maxLength) != -1) {
			// Linux detected
			return file_getAbsoluteParentFolder(fromNativeString(resultBuffer));
		} else if (readlink("/proc/curproc/file", resultBuffer, maxLength) != -1) {
			// BSD detected
			return file_getAbsoluteParentFolder(fromNativeString(resultBuffer));
		} else if (readlink("/proc/self/path/a.out", resultBuffer, maxLength) != -1) {
			// Solaris detected
			return file_getAbsoluteParentFolder(fromNativeString(resultBuffer));
		} else if (allowFallback) {
			return file_getCurrentPath();
		} else {
			throwError("file_getApplicationFolder is not implemented for the current system!\n");
			return U"";
		}
	#endif
}

String file_combinePaths(const ReadableString &a, const ReadableString &b, PathSyntax pathSyntax) {
	ReadableString cleanA = string_removeOuterWhiteSpace(a);
	ReadableString cleanB = string_removeOuterWhiteSpace(b);
	int64_t lengthA = string_length(cleanA);
	int64_t lengthB = string_length(cleanB);
	if (file_hasRoot(b, true, pathSyntax)) {
		// Restarting from root or home folder.
		return cleanB;
	} else if (lengthA == 0) {
		// Ignoring initial relative path, so that relative paths are not suddenly moved to the root by a new separator.
		return cleanB;
	} else if (lengthB == 0) {
		// Ignoring initial relative path, so that relative paths are not suddenly moved to the root by a new separator.
		return cleanA;
	} else {
		if (isSeparator(a[lengthA - 1])) {
			// Already ending with a separator.
			return string_combine(cleanA, cleanB);
		} else {
			// Combine using a separator.
			return string_combine(cleanA, getPathSeparator(pathSyntax), cleanB);
		}
	}
}

// Returns path with the drive letter applied from currentPath if missing in path.
// Used for converting drive relative paths into true absolute paths on MS-Windows.
static String applyDriveLetter(const ReadableString &path, const ReadableString &currentPath) {
	// Convert implicit drive into a named drive.
	printText("applyDriveLetter(", path, ", ", currentPath, ")\n");
	if (path[0] == U'\\') {
		printText("  Implicit drive.\n");
		int64_t colonIndex = string_findFirst(currentPath, U':', -1);
		if (colonIndex == -1) {
			printText("  No colon found!\n");
			return U"?";
		} else {
			// Get the drive letter from the current path.
			String drive = string_until(currentPath, colonIndex);
			printText("  drive = ", drive, "\n");
			return string_combine(drive, path);
		}
	} else {
		printText("  Already absolute.\n");
		// Already absolute.
		return path;
	}
}

String file_getTheoreticalAbsolutePath(const ReadableString &path, const ReadableString &currentPath, PathSyntax pathSyntax) {
	// Home folders are absolute enough, because we don't want to lose the account ambiguity by mangling it into hardcoded usernames.
	if (file_hasRoot(path, true, pathSyntax)) {
		if (pathSyntax == PathSyntax::Windows) {
			// Make sure that no drive letter is missing.
			return applyDriveLetter(file_optimizePath(path, pathSyntax), currentPath);
		} else {
			// Already absolute.
			return file_optimizePath(path, pathSyntax);
		}
	} else {
		// Convert from relative path.
		return file_optimizePath(file_combinePaths(currentPath, path, pathSyntax), pathSyntax);
	}
}
String file_getAbsolutePath(const ReadableString &path) {
	return file_getTheoreticalAbsolutePath(path, file_getCurrentPath(), LOCAL_PATH_SYNTAX);
}

int64_t file_getFileSize(const ReadableString& filename) {
	int64_t result = -1;
	String modifiedFilename = file_optimizePath(filename, LOCAL_PATH_SYNTAX);
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
	String optimizedPath = file_optimizePath(path, LOCAL_PATH_SYNTAX);
	Buffer buffer;
	const NativeChar *nativePath = toNativeString(optimizedPath, buffer);
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
	String optimizedPath = file_optimizePath(folderPath, LOCAL_PATH_SYNTAX);
	#ifdef USE_MICROSOFT_WINDOWS
		String pattern = file_combinePaths(optimizedPath, U"*.*", LOCAL_PATH_SYNTAX);
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
					String entryPath = file_combinePaths(optimizedPath, entryName, LOCAL_PATH_SYNTAX);
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
		const NativeChar *nativePath = toNativeString(optimizedPath, buffer);
		DIR *directory = opendir(nativePath);
		if (directory == nullptr) {
			return false;
		} else {
			while (true) {
				dirent *entry = readdir(directory);
				if (entry != nullptr) {
					String entryName = fromNativeString(entry->d_name);
					if (!string_match(entryName, U".") && !string_match(entryName, U"..")) {
						String entryPath = file_combinePaths(optimizedPath, entryName, LOCAL_PATH_SYNTAX);
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
