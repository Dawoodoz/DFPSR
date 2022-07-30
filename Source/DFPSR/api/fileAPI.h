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
* Test that overwriting a large file with a smaller file does not leave anything from the overwritten file on any system.
* bool file_createFolder(const ReadableString& path);
	WINBASEAPI WINBOOL WINAPI CreateDirectoryW (LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
	mkdir on Posix
* bool file_remove(const ReadableString& path);
	WINBASEAPI WINBOOL WINAPI DeleteFileW (LPCWSTR lpFileName);
*/

#ifndef DFPSR_API_FILE
#define DFPSR_API_FILE

#include "../api/stringAPI.h"
#include "bufferAPI.h"
#if defined(WIN32) || defined(_WIN32)
	#define USE_MICROSOFT_WINDOWS
#endif

// TODO: Create regression tests for the file system.

// A module for file access that exists to prevent cyclic dependencies between strings and buffers.
//   Buffers need a filename to be saved or loaded while strings use buffers to store their characters.
namespace dsr {
	// Post-condition:
	//   Returns the content of the readable file referred to by file_optimizePath(filename).
	//   If mustExist is true, then failure to load will throw an exception.
	//   If mustExist is false, then failure to load will return an empty handle (returning false for buffer_exists).
	Buffer file_loadBuffer(const ReadableString& filename, bool mustExist = true);

	// Side-effect: Saves buffer to file_optimizePath(filename) as a binary file.
	// Pre-condition: buffer exists
	void file_saveBuffer(const ReadableString& filename, Buffer buffer);

	// Get a path separator for the target operating system.
	//   Can be used to construct a file path that works for both forward and backward slash separators.
	const char32_t* file_separator();

	// Turns / and \ into the local system's convention, so that loading and saving files can use either one of them automatically.
	// TODO: Remove redundant . and .. to reduce the risk of running out of buffer space.
	String file_optimizePath(const ReadableString &path);

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
	// If treatHomeFolderAsRoot is true, starting from the home folder using the Posix ~ alias will be allowed.
	bool file_hasRoot(const ReadableString &path, bool treatHomeFolderAsRoot = true);

	// DSR_MAIN_CALLER is a convenient wrapper for getting input arguments as a list of portable Unicode strings.
	//   The actual main function gets placed in DSR_MAIN_CALLER, which calls the given function.
	// Example:
	//   DSR_MAIN_CALLER(dsrMain)
	//   void dsrMain(List<String> args) {
	//       printText("Input arguments:\n");
	//       for (int a = 0; a < args.length(); a++) {
	//           printText("  args[", a, "] = ", args[a], "\n");
	//       }
	//   }
	#ifdef USE_MICROSOFT_WINDOWS
		#define DSR_MAIN_CALLER(MAIN_NAME) \
			void MAIN_NAME(List<String> args); \
			int main() { MAIN_NAME(file_impl_getInputArguments()); return 0; }
	#else
		#define DSR_MAIN_CALLER(MAIN_NAME) \
			void MAIN_NAME(List<String> args); \
			int main(int argc, char **argv) { MAIN_NAME(file_impl_convertInputArguments(argc, (void**)argv)); return 0; }
	#endif
	// Helper functions have to be exposed for the macro handle your input arguments.
	//   Do not call these yourself.
	List<String> file_impl_convertInputArguments(int argn, void **argv);
	List<String> file_impl_getInputArguments();

	// Get the current path, from where the application was called and relative paths start.
	String file_getCurrentPath();
	// Side-effects: Sets the current path to file_optimizePath(path).
	// Post-condition: Returns Returns true on success and false on failure.
	bool file_setCurrentPath(const ReadableString &path);
	// Post-condition: Returns  the application's folder path, from where the application is stored.
	// If not implemented and allowFallback is true,
	//   the current path is returned instead as a qualified guess instead of raising an exception.
	String file_getApplicationFolder(bool allowFallback = true);
	// Gets an absolute version of the path, quickly without removing redundancy.
	String file_getAbsolutePath(const ReadableString &path);
	// Pre-condition: filename must refer to a file so that file_getEntryType(filename) == EntryType::File.
	// Post-condition: Returns a structure with information about the file at file_optimizePath(filename), or -1 if no such file exists.
	int64_t file_getFileSize(const ReadableString& filename);

	// Entry types distinguish between files folders and other things in the file system.
	enum class EntryType { NotFound, UnhandledType, File, Folder, SymbolicLink };
	String& string_toStreamIndented(String& target, const EntryType& source, const ReadableString& indentation);

	// Post-condition: Returns what the file_optimizePath(path) points to in the filesystem.
	// Different comparisons on the result can be used to check if something exists.
	//   Use file_getEntryType(filename) == EntryType::File to check if a file exists.
	//   Use file_getEntryType(folderPath) == EntryType::Folder to check if a folder exists.
	//   Use file_getEntryType(path) != EntryType::NotFound to check if the path leads to anything.
	EntryType file_getEntryType(const ReadableString &path);

	// Side-effects: Calls action with the entry's path, name and type for everything detected in folderPath.
	//               entryPath equals file_combinePaths(folderPath, entryName), and is used for recursive calls when entryType == EntryType::Folder.
	//               entryName equals file_getPathlessName(entryPath).
	//               entryType equals file_getEntryType(entryPath).
	// Post-condition: Returns true iff the folder could be found.
	bool file_getFolderContent(const ReadableString& folderPath, std::function<void(const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType)> action);
}

#endif
