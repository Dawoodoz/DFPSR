
// A file finding application showing how to use the filesystem wrapper in the file API using C++ 2014.

// If you can use only the essential headers, compilation will be faster and work even if parts of the library can't be compiled.
//#include "../../DFPSR/includeFramework.h"
#include "../../DFPSR/includeEssentials.h"

using namespace dsr;

void exploreFolder(const ReadableString& folderPath, const ReadableString& indentation) {
	if (!file_getFolderContent(folderPath, [indentation](const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType) {
		printText(indentation, "* Entry: ", entryName, " as ", entryType, "\n");
		if (entryType == EntryType::Folder) {
			exploreFolder(entryPath, indentation + "  ");
		}
	})) {
		printText("Failed to explore ", folderPath, "\n");
	}
}

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	printText("Input arguments:\n");
	for (int a = 0; a < args.length(); a++) {
		printText("  args[", a, "] = ", args[a], "\n");
	}
	String absolutePath = file_getAbsolutePath(args[0]);
	printText("Absolute path = ", absolutePath, "\n");
	if (args.length() > 1) {
		// Explore each listed folder from input arguments.
		for (int a = 1; a < args.length(); a++) {
			printText("Exploring ", args[a], "\n");
			exploreFolder(args[a], U"  ");
		}
	} else {
		// Test program on the current path when nothing was entered.
		String currentPath = file_getCurrentPath();
		printText("Exploring ", currentPath, " because no folders were given.\n");
		exploreFolder(currentPath, U"  ");
	}
}
