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

#include "stringAPI.h"
#include "bufferAPI.h"
#include "../base/Handle.h"
#include "../settings.h"

// The file API exists to save and load buffers of data for any type of file.
// Any file format that is implemented against the Buffer type instead of hardcoding against the file stream can easily be
//   reused for additional layers of processing such as checksums, archiving, compression, encryption, sending over a network...
// The file API also has functions for processing paths to the filesystem and calling other programs.
namespace dsr {
	// The PathSyntax enum allow processing theoreical paths for other operating systems than the local.
	enum class PathSyntax { Windows, Posix };
	#if defined(USE_MICROSOFT_WINDOWS)
		// Let the local syntax be for Windows.
		#define LOCAL_PATH_SYNTAX dsr::PathSyntax::Windows
	#elif defined(USE_POSIX)
		// Let the local syntax be for Posix.
		#define LOCAL_PATH_SYNTAX dsr::PathSyntax::Posix
	#else
		#error "The target platform was not recognized as one of the supported operating systems in the file API!\n"
	#endif

	// Path-syntax: According to the local computer.
	// Post-condition:
	//   Returns the content of the readable file referred to by file_optimizePath(filename).
	//   If mustExist is true, then failure to load will throw an exception.
	//   If mustExist is false, then failure to load will return an empty handle (returning false for buffer_exists).
	Buffer file_loadBuffer(const ReadableString& filename, bool mustExist = true);

	// Path-syntax: According to the local computer.
	// Side-effect: Saves buffer to file_optimizePath(filename) as a binary file.
	// Pre-condition: buffer exists.
	//   If mustWork is true, then failure to load will throw an exception.
	//   If mustWork is false, then failure to load will return false.
	// Post-condition: Returns true iff the buffer could be saved as a file.
	bool file_saveBuffer(const ReadableString& filename, Buffer buffer, bool mustWork = true);

	// Path-syntax: According to the local computer.
	// Pre-condition: file_getEntryType(path) == EntryType::SymbolicLink
	// Post-condition: Returns the destination of a symbolic link as an absolute path.
	// Shortcuts with file extensions are counted as files, not links.
	String file_followSymbolicLink(const ReadableString &path, bool mustExist = true);

	// Path-syntax: According to pathSyntax, or the local computer if not specified.
	// Get a path separator for the target operating system.
	//   Can be used to construct a file path that works for both forward and backward slash separators.
	const char32_t* file_separator(PathSyntax pathSyntax = LOCAL_PATH_SYNTAX);

	// Path-syntax: This operation can handle separators given from any supported platform.
	//   because separators will be corrected by file_optimizePath when used to access files.
	// Post-condition: Returns true iff the character C is a folder separator, currently / and \ are accepted as folder separators.
	bool file_isSeparator(DsrChar c);

	// Path-syntax: This operation can handle separators given from any supported platform.
	// Post-condition: Returns the base-zero index of the first path separator (as defined by file_isSeparator) starting from startIndex, or defaultIndex if not found.
	//                 If the result is not equal to defaultIndex, it is guaranteed to be equal to or greater than startIndex.
	// Similar to string_findFirst, but with the character to find repalced with a replacable default index.
	int64_t file_findFirstSeparator(const ReadableString &path, int64_t defaultIndex = -1, int64_t startIndex = 0);

	// Path-syntax: This operation can handle separators given from any supported platform.
	// Post-condition: Returns the base-zero index of the last path separator (as defined by file_isSeparator), or defaultIndex if not found.
	// Similar to string_findLast, but with the character to find repalced with a replacable default index.
	int64_t file_findLastSeparator(const ReadableString &path, int64_t defaultIndex = -1);

	// Returns callbacks for non-empty entries between separator characters.
	//   This includes folder names, filenames, symbolic links, drive letter.
	//   This excludes blank space before the first or after the last separator, so the implicit Windows drive \ and Posix system root / will not be included.
	// Path-syntax: This operation can handle separators given from any supported platform.
	//              It should handle both / and \, with and without spaces in filenames.
	// Side-effect: Returns a callback for each selectable non-empty entry in the path, from left to right.
	//              The first argument in the callback is the entry name, and the integers are an inclusive base-zero index range from first to last characters in path.
	// The first entry must be something selectable to be included. Otherwise it is ignored.
	//   C: would be returned as an entry, because other drives can be selected.
	//   The implicit Windows drive \ and Posix system root / will not be returned, because they are implicit and can't be replaced in the path.
	void file_getPathEntries(const ReadableString& path, std::function<void(ReadableString, int64_t, int64_t)> action);

	// Path-syntax: Depends on pathSyntax argument.
	// Turns / and \ into the path convention specified by pathSyntax, which is the local system's by default.
	// Removes redundant . and .. to reduce the risk of running out of buffer space when calling the system.
	String file_optimizePath(const ReadableString &path, PathSyntax pathSyntax = LOCAL_PATH_SYNTAX);

	// Path-syntax: Depends on pathSyntax argument.
	// Combines two parts into a path and automatically adding a local separator when needed.
	// Can be used to get the full path of a file in a folder or add another folder to the path.
	// b may not begin with a separator, because only a is allowed to contain the root.
	String file_combinePaths(const ReadableString &a, const ReadableString &b, PathSyntax pathSyntax = LOCAL_PATH_SYNTAX);
	// Extended to allow combining three or more paths.
	// Example:
	//   String fullPath = file_combinePaths(myApplicationPath, U"myFolder", U"myOtherFolder, U"myFile.txt");
	template<typename... TAIL>
	inline String file_combinePaths(const ReadableString &a, const ReadableString &b, TAIL... tail) {
		return file_combinePaths(file_combinePaths(a, b), tail...);
	}

	// Path-syntax: Depends on pathSyntax argument.
	// Post-condition: Returns true iff path starts from a root, according to the path syntax.
	//                 Implicit drives on Windows using \ are treated as roots because we know that there is nothing above them.
	// If treatHomeFolderAsRoot is true, starting from the /home/username folder using the Posix ~ alias will be allowed as a root as well, because we can't append it behind another path.
	bool file_hasRoot(const ReadableString &path, bool treatHomeFolderAsRoot, PathSyntax pathSyntax = LOCAL_PATH_SYNTAX);

	// Path-syntax: Depends on pathSyntax argument.
	// Returns true iff path is a root without any files nor folder names following.
	//   Does not check if it actually exists, so use file_getEntryType on the actual folders and files for verifying existence.
	bool file_isRoot(const ReadableString &path, bool treatHomeFolderAsRoot, PathSyntax pathSyntax = LOCAL_PATH_SYNTAX);

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
			void MAIN_NAME(dsr::List<dsr::String> args); \
			int main() { \
				dsr::heap_startingApplication(); \
				MAIN_NAME(dsr::file_impl_getInputArguments()); \
				dsr::heap_terminatingApplication(); \
				return 0; \
			}
	#else
		#define DSR_MAIN_CALLER(MAIN_NAME) \
			void MAIN_NAME(dsr::List<dsr::String> args); \
			int main(int argc, char **argv) { \
				dsr::heap_startingApplication(); \
				MAIN_NAME(dsr::file_impl_convertInputArguments(argc, (void**)argv)); \
				dsr::heap_terminatingApplication(); \
				return 0; \
			}
	#endif
	// Helper functions have to be exposed for the macro handle your input arguments.
	//   Do not call these yourself.
	List<String> file_impl_convertInputArguments(int argn, void **argv);
	List<String> file_impl_getInputArguments();

	// Path-syntax: According to the local computer.
	// Post-condition: Returns the current path, from where the application was called and relative paths start.
	String file_getCurrentPath();

	// Path-syntax: According to the local computer.
	// Side-effects: Sets the current path to file_optimizePath(path).
	// Post-condition: Returns Returns true on success and false on failure.
	bool file_setCurrentPath(const ReadableString &path);

	// Path-syntax: According to the local computer.
	// Post-condition: Returns the application's folder path, from where the application is stored.
	// If not implemented and allowFallback is true, the current path is returned instead as a qualified guess instead of raising an exception.
	String file_getApplicationFolder(bool allowFallback = true);

	// Path-syntax: This trivial operation should work the same independent of operating system.
	//              Otherwise you just have to add a new argument after upgrading the static library.
	// Post-condition: Returns the local name of the file or folder after the last path separator, or the whole path if no separator was found.
	ReadableString file_getPathlessName(const ReadableString &path);

	// Path-syntax: This trivial operation should work the same independent of operating system.
	// Post-condition: Returns the filename's extension, or U"" if there is none.
	// This function can not tell if something is a folder or not, because there are file types on Posix systems that have no extension either.
	//   Use file_getEntryType instead if you want to know if it's a file or folder.
	ReadableString file_getExtension(const String& filename);

	// Path-syntax: This trivial operation should work the same independent of operating system.
	// Post-condition: Returns filename without any extension, which is the original input if there is no extension.
	//   Use to remove a file's type before appending a new extension.
	ReadableString file_getExtensionless(const String& filename);

	// Path-syntax: This trivial operation should work the same independent of operating system.
	// Post-condition: Returns true iff path has an extension, even if it's just an empty dot at the end that file_getExtension would not detect.
	//   Useful to check if you need to add an extension or if it already exists in a full filename.
	bool file_hasExtension(const String& path);

	// Quickly gets the relative parent folder by removing the last entry from the string or appending .. at the end.
	// Path-syntax: Depends on pathSyntax argument.
	// This pure syntax function getting the parent folder does not access the system in any way.
	// Does not guarantee that the resulting path is usable on the system.
	// It allows using ~ as the root, for writing paths compatible across different user accounts pointing to different but corresponding files.
	// Going outside of a relative start will add .. to the path.
	//   Depending on which current directory the result is applied to, the absolute path may end up as a root followed by multiple .. going nowhere.
	// Going outside of the absolute root returns U"?" as an error code.
	String file_getRelativeParentFolder(const ReadableString &path, PathSyntax pathSyntax = LOCAL_PATH_SYNTAX);

	// Gets the canonical parent folder using the current directory.
	// This function for getting the parent folder treats path relative to the current directory and expands the result into an absolute path.
	// Make sure that current directory is where you want it when calling this function, because the current directory may change over time when calling file_setCurrentPath.
	// Path-syntax: According to the local computer.
	// Pre-conditions:
	//   path must be valid on the local system, such that you given full permissions could read or create files there relative to the current directory when this function is called.
	//   ~ is not allowed as the root, because the point of using ~ is to reuse the same path across different user accounts, which does not refer to an absolute home directory.
	// Post-condition: Returns the absolute parent to the given path, or U"?" if trying to leave the root or use a tilde home alias.
	String file_getAbsoluteParentFolder(const ReadableString &path);

	// A theoretical version of file_getAbsoluteParentFolder for evaluation on a theoretical system without actually calling file_getCurrentPath or running on the given system.
	// Path-syntax: Depends on pathSyntax argument.
	// Post-condition: Returns the absolute parent to the given path, or U"?" if trying to leave the root or use a tilde home alias.
	String file_getTheoreticalAbsoluteParentFolder(const ReadableString &path, const ReadableString &currentPath, PathSyntax pathSyntax = LOCAL_PATH_SYNTAX);

	// Gets the canonical absolute version of the path.
	// Current directory is expanded, but not user accounts.
	// Path-syntax: According to the local computer.
	// Post-condition: Returns an absolute version of the path.
	String file_getAbsolutePath(const ReadableString &path);

	// A theoretical version of file_getAbsolutePath for evaluation on a theoretical system without actually calling file_getCurrentPath or running on the given system.
	// Current directory is expanded, but not user accounts.
	// Path-syntax: Depends on pathSyntax argument.
	// Post-condition: Returns an absolute version of the path.
	String file_getTheoreticalAbsolutePath(const ReadableString &path, const ReadableString &currentPath, PathSyntax pathSyntax = LOCAL_PATH_SYNTAX);

	// Path-syntax: According to the local computer.
	// Pre-condition: filename must refer to a file so that file_getEntryType(filename) == EntryType::File.
	// Post-condition: Returns a structure with information about the file at file_optimizePath(filename), or -1 if no such file exists.
	int64_t file_getFileSize(const ReadableString& filename);

	// Entry types distinguish between files folders and other things in the file system.
	enum class EntryType { NotFound, UnhandledType, File, Folder, SymbolicLink };
	String& string_toStreamIndented(String& target, const EntryType& source, const ReadableString& indentation);

	// Path-syntax: According to the local computer.
	// Post-condition: Returns what the file_optimizePath(path) points to in the filesystem.
	// Different comparisons on the result can be used to check if something exists.
	//   Use file_getEntryType(filename) == EntryType::File to check if a file exists.
	//   Use file_getEntryType(folderPath) == EntryType::Folder to check if a folder exists.
	//   Use file_getEntryType(path) != EntryType::NotFound to check if the path leads to anything.
	EntryType file_getEntryType(const ReadableString &path);

	// Path-syntax: According to the local computer.
	// Side-effects: Calls action with the entry's path, name and type for everything detected in folderPath.
	//               entryPath equals file_combinePaths(folderPath, entryName), and is used for recursive calls when entryType == EntryType::Folder.
	//               entryName equals file_getPathlessName(entryPath).
	//               entryType equals file_getEntryType(entryPath).
	// Post-condition: Returns true iff the folder could be found.
	bool file_getFolderContent(const ReadableString& folderPath, std::function<void(const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType)> action);

	// Path-syntax: According to the local computer.
	// Access permissions: Default settings according to the local operating system, either inherited from the parent folder or no specific restrictions.
	// Pre-condition: The parent of path must already exist, because only one folder is created.
	// Side-effects: Creates a folder at path.
	// Post-condition: Returns true iff the operation was successful.
	bool file_createFolder(const ReadableString &path);

	// Path-syntax: According to the local computer.
	// Pre-condition: The folder at path must be empty, so any recursion must be done manually.
	//   Otherwise you might accidentally erase "C:\myFolder\importantDocuments" from giving C:\myFolder \junk instead of "C:\myFolder\junk" as command line arguments.
	//   Better to implement your own recursive search that confirms for each file and folder that they are no longer needed, for such a dangerous operation.
	// Side-effects: Removes the folder at path.
	// Post-condition: Returns true iff the operation was successful.
	bool file_removeEmptyFolder(const ReadableString& path);

	// Path-syntax: According to the local computer.
	// Pre-condition: The file at path must exist.
	// Side-effects: Removes the file at path. If the same file exists at multiple paths in the system, only the link to the file will be removed.
	// Post-condition: Returns true iff the operation was successful.
	bool file_removeFile(const ReadableString& filename);

	// The status of a program started using file_execute.
	enum class DsrProcessStatus {
		NotStarted, // The handle was default initialized or the result of a failed call to process_execute.
		Running,    // The process is still running.
		Crashed,    // The process has crashed and might not have done the task you asked for.
		Completed   // The process has terminated successfully, so you may now check if it has written any files for you.
	};

	// A reference counted handle to a process, so that multiple callers can read the status at any time.
	struct DsrProcessImpl;
	using DsrProcess = Handle<DsrProcessImpl>;

	// Post-condition: Returns the status of process.
	DsrProcessStatus process_getStatus(const DsrProcess &process);

	// Path-syntax: According to the local computer.
	// Pre-condition: The executable at path must exist and have execution rights.
	// Side-effects: Starts the program at programPath, with programPath given again as argv[0] and arguments to argv[1...] to the program's main function.
	//               If mustWork is true, then failure will throw an error.
	// Post-condition: Returns a DsrProcess handle to the started process.
	//                 On failure to start, the handle is empty and process_getStatus will then return DsrProcessStatus::NotStarted from it.
	DsrProcess process_execute(const ReadableString& programPath, List<String> arguments, bool mustWork = true);
}

#endif
