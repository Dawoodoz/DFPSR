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

/*
TODO:
* bool file_setCurrentPath(const ReadableString& path);
	WINBASEAPI WINBOOL WINAPI SetCurrentDirectoryW(LPCWSTR lpPathName);
	chdir on Posix
* bool file_createFolder(const ReadableString& path);
	WINBASEAPI WINBOOL WINAPI CreateDirectoryW (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
	mkdir on Posix
* int64_t file_getSize(const ReadableString& path);
	WINBASEAPI WINBOOL WINAPI GetFileSizeEx (HANDLE hFile, PLARGE_INTEGER lpFileSize);
* bool file_remove(const ReadableString& path);
	WINBASEAPI WINBOOL WINAPI DeleteFileW (LPCWSTR lpFileName);
* bool file_exists(const ReadableString& path);
	Can open a file without permissions and see if it works.
* void file_getFolderContent(const ReadableString& folderPath, std::function<void(ReadableString, EntryType)> action)
	How to do this the safest way with Unicode as a minimum requirement?
*/

#ifndef DFPSR_API_FILE
#define DFPSR_API_FILE

#include "../api/stringAPI.h"
#include "bufferAPI.h"
#if defined(WIN32) || defined(_WIN32)
	#define USE_MICROSOFT_WINDOWS
#endif
#if defined(__linux__)
	#define USE_LINUX
#endif

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
	//   Can be used to construct a file path that works for both forward and backward slash separators.
	const char32_t* file_separator();

	// TODO: Create regression tests for the file system.

	// Returns the local name of the file or folder after the last path separator, or the whole path if no separator was found.
	// Examples with / as the path separator:
	//   file_getFolderPath(U"MyFolder/Cars.txt") == U"Cars.txt"
	//   file_getFolderPath(U"MyFolder/")         == U""
	//   file_getFolderPath(U"MyFolder")          == U"MyFolder"
	//   file_getFolderPath(U"MyFolder/Folder2")  == U"Folder2"
	ReadableString file_getPathlessName(const ReadableString &path);

	// Returns the parent folder path with anything after the last slash removed, or empty if there was no slash left.
	// Examples with / as the path separator:
	//   file_getFolderPath(U"MyFolder/Documents/Cars.txt") == U"MyFolder/Documents"
	//   file_getFolderPath(U"MyFolder/Documents/")         == U"MyFolder/Documents"
	//   file_getFolderPath(U"MyFolder/Documents")          == U"MyFolder"
	//   file_getFolderPath(U"MyFolder")                    == U""
	ReadableString file_getParentFolder(const ReadableString &path);

	// Combines two parts into a path and automatically adding a local separator when needed.
	// Can be used to get the full path of a file in a folder or add another folder to the path.
	// b may not begin with a separator, because only a is allowed to contain the root.
	// Examples with / as the path separator:
	//   file_combinePaths(U"Folder", U"Document.txt") == U"Folder/Document.txt"
	//   file_combinePaths(U"Folder/", U"Document.txt") == U"Folder/Document.txt"
	String file_combinePaths(const ReadableString &a, const ReadableString &b);

	// Returns true iff path contains a root, according to the local path syntax.
	bool file_hasRoot(const ReadableString &path);

	// DSR_MAIN_CALLER is a convenient wrapper for getting input arguments as a list of portable Unicode strings.
	//   The actual main function gets placed in DSR_MAIN_CALLER, which calls the given function.
	// When a regular function replaces the main function, it will not return zero by default.
	//   Returning something else than zero tells that something went wrong and it must skip cleanup in order to get out.
	//   Then the window and other resources held as global variables might not close.
	// Example:
	//   DSR_MAIN_CALLER(dsrMain)
	//   int dsrMain(List<String> args) {
	//       printText("Input arguments:\n");
	//       for (int a = 0; a < args.length(); a++) {
	//           printText("  args[", a, "] = ", args[a], "\n");
	//       }
	//       return 0;
	//   }
	#ifdef USE_MICROSOFT_WINDOWS
		#define DSR_MAIN_CALLER(MAIN_NAME) \
			int MAIN_NAME(List<String> args); \
			int main() { \
				return MAIN_NAME(file_impl_getInputArguments()); \
			}
	#else
		#define DSR_MAIN_CALLER(MAIN_NAME) \
			int MAIN_NAME(List<String> args); \
			int main(int argc, char **argv) { return MAIN_NAME(file_impl_convertInputArguments(argc, (void**)argv)); }
	#endif
	// Helper functions have to be exposed for the macro handle your input arguments.
	//   Do not call these yourself.
	List<String> file_impl_convertInputArguments(int argn, void **argv);
	List<String> file_impl_getInputArguments();

	// Get the current path, from where the application was called and relative paths start.
	String file_getCurrentPath();
	// Get the application's folder path, from where the application is stored.
	// If not implemented and allowFallback is true,
	//   the current path is returned instead as a qualified guess instead of raising an exception.
	String file_getApplicationFolder(bool allowFallback = true);
	// Gets an absolute version of the path, quickly without removing redundancy.
	String file_getAbsolutePath(const ReadableString &path);
	// Returns true iff path refers to a valid file or folder.
	bool file_exists(const ReadableString& path);
}

#endif
