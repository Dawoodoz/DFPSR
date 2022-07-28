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

#ifndef DFPSR_API_FILE
#define DFPSR_API_FILE

#include "../api/stringAPI.h"
#include "bufferAPI.h"
#include <version>

// A module for file access that exists to prevent cyclic dependencies between strings and buffers.
//   Buffers need a filename to be saved or loaded while strings use buffers to store their characters.
namespace dsr {
	// NativeChar is defined as whatever type the native system uses for expressing Unicode
	// Use for input arguments in the main function to allow using file_convertInputArguments.
	#if defined(WIN32) || defined(_WIN32)
		using NativeChar = wchar_t; // UTF-16
	#else
		using NativeChar = char; // UTF-8
	#endif

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

	// Returns the local name after the last path separator, or the whole path if no separator was found.
	ReadableString file_getPathlessName(const ReadableString &path);

	// Convert input arguments from main into a list of dsr::String.
	// argv must be declared as wchar_t** on MS-Windows to allow using Unicode, which can be done by using the dsr::NativeChar alias:
	//     int main(int argn, NativeChar **argv) {
	List<String> file_convertInputArguments(int argn, NativeChar **argv);

	// Portable wrapper over the STD filesystem library, so that you get Unicode support with one type of string on all platforms.
	// To enable this wrapper, just compile everything with C++17 or a newer version that still has the feature.
	// Using more STD functionality can make it harder to port your project when C++ compilers are no longer maintained,
	//   but exploring the filesystem is necessary for selecting files to load or save in editors.
	#ifdef __cpp_lib_filesystem
		enum class EntryType {
			Unknown, Folder, File, SymbolicLink
		};
		// Get the current path, from where the application was called and relative paths start.
		String file_getCurrentPath();
		// Get the path's absolute form to know the exact location.
		String file_getAbsolutePath(const ReadableString& path);
		// Get the path's absolute form without redundancy.
		String file_getCanonicalPath(const ReadableString& path);
		// Gets a callback for each file and folder directly inside of folderPath.
		// Calls back with the entry's path and an integer representing the entry type.
		void file_getFolderContent(const ReadableString& folderPath, std::function<void(ReadableString, EntryType)> action);
		// Returns true iff path refers to a valid file or folder.
		bool file_exists(const ReadableString& path);
		// Returns the file's size.
		int64_t file_getSize(const ReadableString& path);
		// Removes the file or empty folder.
		// Returns true iff path existed.
		bool file_remove(const ReadableString& path);
		// Removes the file or folder including any content.
		// Returns true iff path existed.
		bool file_removeRecursively(const ReadableString& path);
		// Returns true on success.
		bool file_createFolder(const ReadableString& path);
	#endif
}

#endif
