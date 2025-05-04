
// A program for cloning a project from one folder to another, while updating relative paths to headers outside of the folder.

// TODO:
// * Create a visual interface for creating new projects from templates in the Wizard application.
//   Choose to create a new project, choose a template, choose a new name and location.
// * Replace file paths in the Batch and Shell scripts.
// * Allow renaming one of the project file, so that references to it will also be updated.
// * Filter out files using patterns, to avoid cloning executable files and descriptions of template projects.

#include "../../../DFPSR/includeEssentials.h"

using namespace dsr;

// Post-condition: Returns a list of entry names in the path, by simply segmentmenting by folder separators.
static List<String> segmentPath(const ReadableString &path) {
	List<String> result;
	intptr_t startIndex = 0;
	for (intptr_t endIndex = 0; endIndex < string_length(path); endIndex++) {
		if (file_isSeparator(path[endIndex])) {
			if (startIndex < endIndex) {
				result.push(string_exclusiveRange(path, startIndex, endIndex));
			}
			startIndex = endIndex + 1;
		}
	}
	if (string_length(path) > startIndex) {
		result.push(string_exclusiveRange(path, startIndex, string_length(path)));
	}
	return result;
}

// TODO: Make rewrite it to work in more cases by converting oldOrigin to an absolute path before converting it to the new origin.

// Pre-conditions:
//   path is either absolute or relative to oldOrigin.
//   newOrigin may not be absolute.
// Post-condition:
//   Returns a path that refers to the same location but relative to newOrigin. 
static String changePathOrigin(const ReadableString &path, const ReadableString &oldOrigin, const ReadableString &newOrigin, PathSyntax pathSyntax) {
	// Check if the path is absolute.
	if (file_hasRoot(path, true)) {
		// The path is absolute, so we will not change it into an absolute path, just clean up any redundancy.
		return file_optimizePath(path, pathSyntax);
	}
	if (file_hasRoot(oldOrigin, true) || file_hasRoot(newOrigin, true)) {
		throwError(U"Origins to changePathOrigin may not be absolute!\n");
	}
	String absoluteOldOrigin = file_getAbsolutePath(oldOrigin);
	String absoluteNewOrigin = file_getAbsolutePath(newOrigin);
	String pathFromCurrent = file_optimizePath(file_combinePaths(absoluteOldOrigin, path, pathSyntax), pathSyntax);
	List<String> pathNames = segmentPath(pathFromCurrent);
	List<String> newOriginNames = segmentPath(file_optimizePath(absoluteNewOrigin, pathSyntax));
	intptr_t reverseOriginDepth = 0;
	List<String> forwardOrigin;
	bool identicalRoot = true;
	for (intptr_t i = 0; i < pathNames.length() || i < newOriginNames.length(); i++) {
		if (i < pathNames.length() && i < newOriginNames.length()) {
			if (!string_match(pathNames[i], newOriginNames[i])) {
				identicalRoot = false;
			}
		}
		if (!identicalRoot) {
			if (i < pathNames.length()) {
				forwardOrigin.push(pathNames[i]);
			}
			if (i < newOriginNames.length()) {
				reverseOriginDepth++;
			}
		}
	}
	List<String> results;
	for (intptr_t i = 0; i < reverseOriginDepth; i++) {
		results.push(U"..");
	}
	for (intptr_t i = 0; i < forwardOrigin.length(); i++) {
		results.push(forwardOrigin[i]);
	}
	String result;
	for (intptr_t i = 0; i < results.length(); i++) {
		if (string_length(result) > 0) {
			string_append(result, file_separator(pathSyntax));
		}
		string_append(result, results[i]);
	}
	result = file_optimizePath(result, pathSyntax);
	return result;
}

static void testRelocation(const ReadableString &path, const ReadableString &oldOrigin, const ReadableString &newOrigin, PathSyntax pathSyntax, const ReadableString &expectedResult) {
	String result = changePathOrigin(path, oldOrigin, newOrigin, pathSyntax);
	if (!string_match(result, expectedResult)) {
		throwError(U"Converting ", path, U" from ", oldOrigin, U" to ", newOrigin, U" expected ", expectedResult, U" as the result but got ", result, U" instead!\n");
	}
}

static void regressionTest() {
	printText(U"Running regression tests for the cloning project.\n");
	testRelocation(U"../someFile.txt", U"folderA/folderC", U"folderB", PathSyntax::Posix, U"../folderA/someFile.txt");
	testRelocation(U"someFile.txt", U"folderA", U"folderB", PathSyntax::Windows, U"..\\folderA\\someFile.txt");
	testRelocation(U"../../DFPSR/includeFramework.h", U"../../../templates/basic3D", U"./NewProject", PathSyntax::Posix, U"../../../../DFPSR/includeFramework.h");	
	testRelocation(U"../../DFPSR/includeFramework.h", U"../../../templates/basic3D", U"../NewProject", PathSyntax::Posix, U"../../../DFPSR/includeFramework.h");
	testRelocation(U"../../DFPSR/includeFramework.h", U"../../../templates/basic3D", U"../../NewProject", PathSyntax::Posix, U"../../DFPSR/includeFramework.h");
	testRelocation(U"../../DFPSR/includeFramework.h", U"../../../templates/basic3D", U"../../../NewProject", PathSyntax::Posix, U"../DFPSR/includeFramework.h");
	testRelocation(U"../../DFPSR/includeFramework.h", U"../../../templates/basic3D", U"../../../../NewProject", PathSyntax::Posix, U"../Source/DFPSR/includeFramework.h");
	printText(U"Passed all regression tests for the cloning project.\n");
}

// Update paths after #include and #import in c, cpp, h, hpp, m and mm files.
static String updateSourcePaths(const ReadableString &content, const ReadableString &oldParentFolder, const ReadableString &newParentFolder) {
	String result;
	intptr_t consumed = 0;
	int state = 0;
	for (intptr_t characterIndex = 0; characterIndex < string_length(content); characterIndex++) {
		DsrChar currentCharacter = content[characterIndex];
		if (currentCharacter == U'\n') {
			state = 0;
		} else if (state == 0 && currentCharacter == U'#') {
			state = 1;
		} else if (state == 1) {
			if (string_match(U"include", string_exclusiveRange(content, characterIndex, characterIndex + 7))) {
				characterIndex += 6;
				state = 2;
			} else if (string_match(U"import", string_exclusiveRange(content, characterIndex, characterIndex + 6))) {
				characterIndex += 5;
				state = 2;
			}
		} else if (state == 2 && currentCharacter == U'\"') {
			// Begin a quoted path.
			state = 3;
			// Previous text is appended as is.
			string_append(result, string_inclusiveRange(content, consumed, characterIndex));
			consumed = characterIndex + 1;
		} else if (state == 3 && currentCharacter == U'\"') {
			// End a quoted path.
			state = -1;
			String oldPath = string_inclusiveRange(content, consumed, characterIndex - 1);
			String newPath = changePathOrigin(oldPath, oldParentFolder, newParentFolder, PathSyntax::Posix);
			string_append(result, newPath);
			consumed = characterIndex;
			if (string_match(newPath, oldPath)) {
				printText(U"	Nothing needed to change in ", oldPath, U"\n");
			} else {
				printText(U"	Modified path from ", oldPath, U" to ", newPath, U"\n");
			}
		} else if (state != 3 && !character_isWhiteSpace(currentCharacter)) {
			// Abort patterns when getting unexpected characters.
			state = -1;
		}
	}
	// Remaining text is appended as is.
	string_append(result, string_exclusiveRange(content, consumed, string_length(content)));
	return result;
}

// Update paths after Import in DsrProj and DsrHead files.
static String updateProjectPaths(const ReadableString &content, const ReadableString &oldParentFolder, const ReadableString &newParentFolder) {
	String result;
	intptr_t consumed = 0;
	int state = 0;
	for (intptr_t characterIndex = 0; characterIndex < string_length(content); characterIndex++) {
		DsrChar currentCharacter = content[characterIndex];
		if (currentCharacter == U'\n') {
			state = 0;
		} else if (state == 0) {
			if (string_caseInsensitiveMatch(U"Import", string_exclusiveRange(content, characterIndex, characterIndex + 6))) {
				characterIndex += 5;
				state = 1;
			}
		} else if (state == 1 && currentCharacter == U'\"') {
			// Begin a quoted path.
			state = 2;
			// Previous text is appended as is.
			string_append(result, string_inclusiveRange(content, consumed, characterIndex));
			consumed = characterIndex + 1;
		} else if (state == 2 && currentCharacter == U'\"') {
			// End a quoted path.
			state = -1;
			String oldPath = string_inclusiveRange(content, consumed, characterIndex - 1);
			String newPath = changePathOrigin(oldPath, oldParentFolder, newParentFolder, PathSyntax::Posix);
			string_append(result, newPath);
			consumed = characterIndex;
			if (string_match(newPath, oldPath)) {
				printText(U"	Nothing needed to change in ", oldPath, U"\n");
			} else {
				printText(U"	Modified path from ", oldPath, U" to ", newPath, U"\n");
			}
		} else if (state != 2 && !character_isWhiteSpace(currentCharacter)) {
			// Abort patterns when getting unexpected characters.
			state = -1;
		}
	}
	// Remaining text is appended as is.
	string_append(result, string_exclusiveRange(content, consumed, string_length(content)));
	return result;
}

static void copyFile(const ReadableString &sourcePath, const ReadableString &targetPath) {
	EntryType sourceEntryType = file_getEntryType(sourcePath);
	EntryType targetEntryType = file_getEntryType(targetPath);
	if (sourceEntryType != EntryType::File) {
		throwError(U"The source file ", sourcePath, U" does not exist!\n");
	}
	if (targetEntryType != EntryType::NotFound) {
		throwError(U"The target file ", targetPath, U" already exists!\n");
	} else {
		Buffer fileContent = file_loadBuffer(sourcePath);
		if (!buffer_exists(fileContent)) {
			throwError(U"The source file ", sourcePath, U" could not be loaded!\n");
		}
		ReadableString extension = file_getExtension(sourcePath);
		if (string_caseInsensitiveMatch(extension, U"DsrProj")
		 || string_caseInsensitiveMatch(extension, U"DsrHead")) {
			//patterns.pushConstruct(U"Import \"", U"", U"\"");
			fileContent = string_saveToMemory(updateProjectPaths(string_loadFromMemory(fileContent), file_getRelativeParentFolder(sourcePath), file_getRelativeParentFolder(targetPath)), CharacterEncoding::Raw_Latin1);
		} else if (string_caseInsensitiveMatch(extension, U"bat")) {
			//TODO: Look for paths containing U"\builder\buildProject.bat", segment the whole path, and update path origin.
			//fileContent = string_saveToMemory(updateBatchPaths(string_loadFromMemory(fileContent), file_getRelativeParentFolder(sourcePath), file_getRelativeParentFolder(targetPath)), CharacterEncoding::Raw_Latin1);
		} else if (string_caseInsensitiveMatch(extension, U"sh")) {
			//TODO: Look for paths containing U"/builder/buildProject.sh", segment the whole path, and update path origin.
			//fileContent = string_saveToMemory(updateShellPaths(string_loadFromMemory(fileContent), file_getRelativeParentFolder(sourcePath), file_getRelativeParentFolder(targetPath)), CharacterEncoding::Raw_Latin1);
		} else if (string_caseInsensitiveMatch(extension, U"c")
				|| string_caseInsensitiveMatch(extension, U"cpp")
				|| string_caseInsensitiveMatch(extension, U"h")
				|| string_caseInsensitiveMatch(extension, U"hpp")
				|| string_caseInsensitiveMatch(extension, U"m")
				|| string_caseInsensitiveMatch(extension, U"mm")) {
			fileContent = string_saveToMemory(updateSourcePaths(string_loadFromMemory(fileContent), file_getRelativeParentFolder(sourcePath), file_getRelativeParentFolder(targetPath)), CharacterEncoding::BOM_UTF8);
		}
		if (!file_saveBuffer(targetPath, fileContent)) {
			throwError(U"The target file ", targetPath, U" could not be saved!\n");
		}
	}
}

struct FileConversion {
	String sourceFilePath;
	String targetFilePath;
	FileConversion(const ReadableString &sourceFilePath, const ReadableString &targetFilePath)
	: sourceFilePath(sourceFilePath), targetFilePath(targetFilePath) {}
};
struct FileOperations {
	List<String> newFolderPaths;
	List<FileConversion> clonedFiles;
};

static bool createFolder_deferred(FileOperations &operations, const ReadableString &folderPath) {
	EntryType targetEntryType = file_getEntryType(folderPath);
	if (targetEntryType == EntryType::Folder) {
		return true;
	} else if (targetEntryType == EntryType::File) {
		printText(U"The folder to create ", folderPath, U" is an pre-existing file and can not be overwritten with a folder!\n");
		return false;
	} else if (targetEntryType == EntryType::NotFound) {
		String parentFolder = file_getRelativeParentFolder(folderPath);
		if (string_length(parentFolder) < string_length(folderPath) && createFolder_deferred(operations, parentFolder)) {
			operations.newFolderPaths.push(folderPath);
			return true;
		} else {
			printText(U"Failed to create a parent folder at ", parentFolder, U"!\n");
			return false;
		}
	} else {
		printText(U"The folder to create ", folderPath, U" can not be overwritten!\n");
		return false;
	}
}

static void copyFolder_deferred(FileOperations &operations, const ReadableString &sourcePath, const ReadableString &targetPath) {
	if (!createFolder_deferred(operations, targetPath)) {
		throwError(U"Failed to create a folder at ", targetPath, U"!\n");
	} else {
		if (!file_getFolderContent(sourcePath, [&operations, targetPath](const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType) {
			if (entryType == EntryType::File) {
				operations.clonedFiles.pushConstruct(entryPath, file_combinePaths(targetPath, entryName));
			} else if (entryType == EntryType::Folder) {
				copyFolder_deferred(operations, entryPath, file_combinePaths(targetPath, entryName));
			}
		})) {
			printText("Failed to explore ", sourcePath, "\n");
		}
	}
}

enum class ExpectedArgument {
	Flag, Source, Target
};

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	if (args.length() <= 1) {
		regressionTest();
		return;
	}
	String source;
	String target;
	ExpectedArgument expectedArgument = ExpectedArgument::Flag;
	for (int i = 1; i < args.length(); i++) {
		ReadableString argument = args[i];
		if (expectedArgument == ExpectedArgument::Flag) {
			if (string_caseInsensitiveMatch(argument, U"-s") || string_caseInsensitiveMatch(argument, U"-source")) {
				expectedArgument = ExpectedArgument::Source;
			} else if (string_caseInsensitiveMatch(argument, U"-t") || string_caseInsensitiveMatch(argument, U"-target")) {
				expectedArgument = ExpectedArgument::Target;
			} else {
				sendWarning(U"Unrecognized flag ", argument, U" given to project cloning!\n");
			}
		} else if (expectedArgument == ExpectedArgument::Source) {
			EntryType sourceEntryType = file_getEntryType(argument);
			if (sourceEntryType == EntryType::Folder) {
				printText(U"Using ", argument, U" as the source folder path.\n");
				source = argument;
			} else if (sourceEntryType == EntryType::File) {
				throwError(U"The source ", argument, U" is a file and can not be used as a source folder for project cloning!\n");
			} else if (sourceEntryType == EntryType::NotFound) {
				throwError(U"The source ", argument, U" can not be found! The source path must refer to an existing folder to clone from.\n");
			}
			expectedArgument = ExpectedArgument::Flag;
		} else if (expectedArgument == ExpectedArgument::Target) {
			EntryType targetEntryType = file_getEntryType(argument);
			if (targetEntryType == EntryType::Folder) {
				printText(U"Using ", argument, U" as the target folder path.\n");
				target = argument;
			} else if (targetEntryType == EntryType::File) {
				throwError(U"The target ", argument, U" is a file and can not be used as a target folder for project cloning!\n");
			} else if (targetEntryType == EntryType::NotFound) {
				printText(U"Using ", argument, U" as the target folder path.\n");
				target = argument;
			}
			expectedArgument = ExpectedArgument::Flag;
		}
	}
	if (string_length(source) == 0 && string_length(target) == 0) {
		throwError(U"Cloning project needs both source and target folder paths!\n");
	} else if (string_length(source) == 0) {
		throwError(U"Missing source folder to clone from!\n");
	} else if (string_length(target) == 0) {
		throwError(U"Missing target folder to clone to!\n");
	}
	printText(U"Cloning project from ", source, U" to ", target, U"\n");
	// List operations to perform ahead of time to prevent bottomless recursion when cloning into a subfolder of the source folder.
	FileOperations operations;
	copyFolder_deferred(operations, source, target);
	for (intptr_t folderIndex = 0; folderIndex < operations.newFolderPaths.length(); folderIndex++) {
		ReadableString newFolderPath = operations.newFolderPaths[folderIndex];
		printText(U"Creating a new folder at ", newFolderPath, U"\n");
		file_createFolder(newFolderPath);
	}
	for (intptr_t fileIndex = 0; fileIndex < operations.clonedFiles.length(); fileIndex++) {
		FileConversion conversion = operations.clonedFiles[fileIndex];
		printText(U"Cloning file from ", conversion.sourceFilePath, U" to ", conversion.targetFilePath, U"\n");
		copyFile(conversion.sourceFilePath, conversion.targetFilePath);
	}
}
