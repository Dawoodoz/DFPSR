
// A file finding application showing how to use the filesystem wrapper in the file API using C++ 2017.

/*
TODO:
 * Function for getting EntryType from a path.
 * Implement file_getParent, getting the parent folder or root path, so that it's easy to iterate the other way.
 * Something for listing root paths, so that systems with more than one system root can have a listbox with drives to select in file explorers.
 * Wrap file_getPermissions
 * Wrap file_moveAndRename (over std::filesystem::rename)
 * Wrap copy (over std::filesystem::copy)
*/

#include "../../DFPSR/includeFramework.h"

using namespace dsr;

void exploreFolder(const ReadableString& folderPath, const ReadableString& indentation) {
	/*file_getFolderContent(folderPath, [indentation](ReadableString entryPath, EntryType entryType) {
		ReadableString shortName = file_getPathlessName(entryPath);
		if (entryType == EntryType::Folder) {
			printText(indentation, " Folder(", shortName, ")\n");
			exploreFolder(entryPath, indentation + "  ");
		} else if (entryType == EntryType::File) {
			printText(indentation, " File(", shortName, ") of ", file_getSize(entryPath), " bytes\n");
		}
	});*/
}

DSR_MAIN_CALLER(dsrMain)
int dsrMain(List<String> args) {
	printText("Input arguments:\n");
	for (int a = 0; a < args.length(); a++) {
		printText("  args[", a, "] = ", args[a], "\n");
	}
	/*
	String absolutePath = file_getCanonicalPath(args[0]);
	printText("Absolute path = ", absolutePath, "\n");
	if (args.length() > 1) {
		// Explore each listed folder from input arguments.
		for (int a = 1; a < args.length(); a++) {
			printText("Exploring ", args[a], "\n");
			exploreFolder(args[a], U"");
		}
	} else {
		// Test program on the current path when nothing was entered.
		String currentPath = file_getCurrentPath();
		printText("Exploring ", currentPath, " because no folders were given.\n");
		exploreFolder(currentPath, U"");
	}
	*/
	// When the DSR_MAIN_CALLER wrapper is used over the real main function, returning zero is no longer implicit.
	return 0;
}
