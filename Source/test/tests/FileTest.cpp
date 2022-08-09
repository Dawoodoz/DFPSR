
#define NO_IMPLICIT_PATH_SYNTAX
#include "../testTools.h"

START_TEST(File)
	{ // Combining paths
		ASSERT_MATCH(file_combinePaths(U"", U"myProgram.exe", PathSyntax::Windows), U"myProgram.exe");
		ASSERT_MATCH(file_combinePaths(U"C:", U"myProgram.exe", PathSyntax::Windows), U"C:\\myProgram.exe");
		ASSERT_MATCH(file_combinePaths(U"C:\\windows", U"myProgram.exe", PathSyntax::Windows), U"C:\\windows\\myProgram.exe");
		ASSERT_MATCH(file_combinePaths(U"C:\\windows\\", U"myProgram.exe", PathSyntax::Windows), U"C:\\windows\\myProgram.exe");
		ASSERT_MATCH(file_combinePaths(U"", U"myProgram", PathSyntax::Posix), U"myProgram");
		ASSERT_MATCH(file_combinePaths(U"/", U"myProgram", PathSyntax::Posix), U"/myProgram");
		ASSERT_MATCH(file_combinePaths(U"/home", U"me", PathSyntax::Posix), U"/home/me");
		ASSERT_MATCH(file_combinePaths(U"/home/", U"me", PathSyntax::Posix), U"/home/me");
	}
	{ // Optimizing paths
		// Preserving leading separators
		ASSERT_MATCH(file_optimizePath(U"myProgram", PathSyntax::Windows), U"myProgram"); // Relative path
		ASSERT_MATCH(file_optimizePath(U"\\myProgram", PathSyntax::Windows), U"\\myProgram"); // Implicit drive
		ASSERT_MATCH(file_optimizePath(U"\\\\myProgram", PathSyntax::Windows), U"\\\\myProgram");
		ASSERT_MATCH(file_optimizePath(U"\\\\\\myProgram", PathSyntax::Windows), U"\\\\\\myProgram");
		ASSERT_MATCH(file_optimizePath(U"myProgram", PathSyntax::Posix), U"myProgram"); // Relative path
		ASSERT_MATCH(file_optimizePath(U"/home", PathSyntax::Posix), U"/home"); // Root path
		ASSERT_MATCH(file_optimizePath(U"//network", PathSyntax::Posix), U"//network"); // Special path
		ASSERT_MATCH(file_optimizePath(U"///myProgram", PathSyntax::Posix), U"///myProgram");
		// Preserving drive letters
		ASSERT_MATCH(file_optimizePath(U"C:\\myProgram", PathSyntax::Windows), U"C:\\myProgram");
		// Reducing redundancy
		ASSERT_MATCH(file_optimizePath(U"/home/user", PathSyntax::Posix), U"/home/user");
		ASSERT_MATCH(file_optimizePath(U"/home/user/", PathSyntax::Posix), U"/home/user");
		ASSERT_MATCH(file_optimizePath(U"/home/user//", PathSyntax::Posix), U"/home/user");
		ASSERT_MATCH(file_optimizePath(U"/home/user///", PathSyntax::Posix), U"/home/user");
		ASSERT_MATCH(file_optimizePath(U"/home/user/.", PathSyntax::Posix), U"/home/user");
		ASSERT_MATCH(file_optimizePath(U"/home/user/./", PathSyntax::Posix), U"/home/user");
		ASSERT_MATCH(file_optimizePath(U"/home/user/.//", PathSyntax::Posix), U"/home/user");
		ASSERT_MATCH(file_optimizePath(U"/home/user/..", PathSyntax::Posix), U"/home");
		ASSERT_MATCH(file_optimizePath(U"/home/user/../", PathSyntax::Posix), U"/home");
		ASSERT_MATCH(file_optimizePath(U"/home/user/..//", PathSyntax::Posix), U"/home");
		ASSERT_MATCH(file_optimizePath(U"/cars/oldCars/veteranCars/../././../newCars/", PathSyntax::Posix), U"/cars/newCars");
		ASSERT_MATCH(file_optimizePath(U"C:\\cars\\oldCars\\veteranCars\\..\\..\\newCars\\", PathSyntax::Windows), U"C:\\cars\\newCars");
		// Error handling
		ASSERT_MATCH(file_optimizePath(U"C:\\..", PathSyntax::Windows), U"?"); // Can't go outside of C: drive
		ASSERT_MATCH(file_optimizePath(U"\\..", PathSyntax::Windows), U"?"); // Can't go outside of current drive root
		ASSERT_MATCH(file_optimizePath(U"..", PathSyntax::Windows), U".."); // Can go outside of the relative path
		ASSERT_MATCH(file_optimizePath(U"/..", PathSyntax::Posix), U"?"); // Can't go outside of system root
		ASSERT_MATCH(file_optimizePath(U"..", PathSyntax::Posix), U".."); // Can go outside of the relative path
	}
	{ // Absolute canonical paths
		ASSERT_MATCH(file_getTheoreticalAbsolutePath(U"mediaFolder\\myFile.txt", U"C:\\folder\\anotherFolder", PathSyntax::Windows), U"C:\\folder\\anotherFolder\\mediaFolder\\myFile.txt");
		ASSERT_MATCH(file_getTheoreticalAbsolutePath(U"mediaFolder\\myFile.txt", U"C:\\folder\\anotherFolder\\", PathSyntax::Windows), U"C:\\folder\\anotherFolder\\mediaFolder\\myFile.txt");
		ASSERT_MATCH(file_getTheoreticalAbsolutePath(U"myFile.txt", U"C:\\folder", PathSyntax::Windows), U"C:\\folder\\myFile.txt");
		ASSERT_MATCH(file_getTheoreticalAbsolutePath(U"\\myFile.txt", U"C:\\folder", PathSyntax::Windows), U"C:\\myFile.txt"); // To the root of the current drive C:
		ASSERT_MATCH(file_getTheoreticalAbsolutePath(U"", U"C:\\folder", PathSyntax::Windows), U"C:\\folder");
		ASSERT_MATCH(file_getTheoreticalAbsolutePath(U"mediaFolder\\..\\myFile.txt", U"C:\\folder\\anotherFolder", PathSyntax::Windows), U"C:\\folder\\anotherFolder\\myFile.txt");
	}
	{ // Parent folders
		ASSERT_MATCH(file_getRelativeParentFolder(U"mediaFolder\\..\\myFile.txt", PathSyntax::Windows), U"");
		ASSERT_MATCH(file_getTheoreticalAbsoluteParentFolder(U"mediaFolder\\..\\myFile.txt", U"C:\\folder\\anotherFolder", PathSyntax::Windows), U"C:\\folder\\anotherFolder");
	}
	{ // Path removal
		ASSERT_MATCH(file_getPathlessName(U"C:\\..\\folder\\file.txt"), U"file.txt");
		ASSERT_MATCH(file_getPathlessName(U"C:\\..\\folder\\"), U"");
		ASSERT_MATCH(file_getPathlessName(U"C:\\..\\folder"), U"folder");
		ASSERT_MATCH(file_getPathlessName(U"C:\\..\\"), U"");
		ASSERT_MATCH(file_getPathlessName(U"C:\\.."), U"..");
		ASSERT_MATCH(file_getPathlessName(U"C:\\"), U"");
		ASSERT_MATCH(file_getPathlessName(U"C:"), U"C:");
		ASSERT_MATCH(file_getPathlessName(U"/folder/file.h"), U"file.h");
		ASSERT_MATCH(file_getPathlessName(U"/folder/"), U"");
		ASSERT_MATCH(file_getPathlessName(U"/folder"), U"folder");
		ASSERT_MATCH(file_getPathlessName(U"/"), U"");
	}
	{ // Extension removal
		ASSERT_MATCH(file_getExtensionless(U"C:\\..\\folder\\file.txt"), U"C:\\..\\folder\\file");
		ASSERT_MATCH(file_getExtensionless(U"C:\\folder\\file.h"), U"C:\\folder\\file");
		ASSERT_MATCH(file_getExtensionless(U"C:\\file."), U"C:\\file");
		ASSERT_MATCH(file_getExtensionless(U"\\file."), U"\\file");
		ASSERT_MATCH(file_getExtensionless(U"file"), U"file");
		ASSERT_MATCH(file_getExtensionless(U""), U"");
		ASSERT_MATCH(file_getExtensionless(U"/folder/./file.txt"), U"/folder/./file");
		ASSERT_MATCH(file_getExtensionless(U"/folder/file.h"), U"/folder/file");
		ASSERT_MATCH(file_getExtensionless(U"/folder/../file.h"), U"/folder/../file");
		ASSERT_MATCH(file_getExtensionless(U"/file."), U"/file");
		ASSERT_MATCH(file_getExtensionless(U"file"), U"file");
		ASSERT_MATCH(file_getExtension(U"C:\\..\\folder\\file.txt"), U"txt");
		ASSERT_MATCH(file_getExtension(U"C:\\..\\folder\\file.foo.txt"), U"txt");
		ASSERT_MATCH(file_getExtension(U"C:\\..\\folder\\file.foo_bar.txt"), U"txt");
		ASSERT_MATCH(file_getExtension(U"C:\\..\\folder\\file.foo.bar_txt"), U"bar_txt");
		ASSERT_MATCH(file_getExtension(U"C:\\folder\\file.h"), U"h");
		ASSERT_MATCH(file_getExtension(U"C:\\file."), U"");
		ASSERT_MATCH(file_getExtension(U"\\file."), U"");
		ASSERT_MATCH(file_getExtension(U"file"), U"");
		ASSERT_MATCH(file_getExtension(U""), U"");
		ASSERT_MATCH(file_getExtension(U"/folder/./file.txt"), U"txt");
		ASSERT_MATCH(file_getExtension(U"/folder/file.h"), U"h");
		ASSERT_MATCH(file_getExtension(U"/folder/../file.h"), U"h");
		ASSERT_MATCH(file_getExtension(U"/file."), U"");
		ASSERT_MATCH(file_getExtension(U"file"), U"");
		ASSERT_EQUAL(file_hasExtension(U"/folder/./file.txt"), true);
		ASSERT_EQUAL(file_hasExtension(U"/../folder/file.h"), true);
		ASSERT_EQUAL(file_hasExtension(U"/folder/file."), true); // Not a named extension, but ending with a dot is not a pure extensionless path either.
		ASSERT_EQUAL(file_hasExtension(U"/folder/file"), false);
	}
	{ // Folder creation and removal
		// Prepare by removing any old folder from aborted tests.
		if (file_getEntryType(U"FooBarTestFolder") == EntryType::Folder) file_removeEmptyFolder(U"FooBarTestFolder");
		// The boolean results are compared with true rather than not-false, because it should still work if someone does that by mistake in the real program.
		//   All booleans returned from system API calls in fileAPI are normalized to 1 and 0 using either direct assignments or comparisons.
		//   Normalization using x == 0 or x != 0 is guaranteed to return precisely 1 or 0 according to the C++ standard.
		// Check that the folder does not exist.
		ASSERT_EQUAL(file_getEntryType(U"FooBarTestFolder"), EntryType::NotFound);
		// Create the folder.
		ASSERT_EQUAL(file_createFolder(U"FooBarTestFolder"), true); 
		// Check that the folder does exist.
		ASSERT_EQUAL(file_getEntryType(U"FooBarTestFolder"), EntryType::Folder);
		// Remove the folder.
		ASSERT_EQUAL(file_removeEmptyFolder(U"FooBarTestFolder"), true);
		// Check that the folder does not exist.
		ASSERT_EQUAL(file_getEntryType(U"FooBarTestFolder"), EntryType::NotFound);
	}
	{ // Nested creation and removal
		String childPathA = file_combinePaths(U"FooBarParent", U"FooBarChildA", LOCAL_PATH_SYNTAX);
		String childPathB = file_combinePaths(U"FooBarParent", U"FooBarChildB", LOCAL_PATH_SYNTAX);
		// Prepare by removing any old folder from aborted tests.
		if (file_getEntryType(childPathA) == EntryType::Folder) file_removeEmptyFolder(childPathA);
		if (file_getEntryType(childPathB) == EntryType::Folder) file_removeEmptyFolder(childPathB);
		if (file_getEntryType(U"FooBarParent") == EntryType::Folder) file_removeEmptyFolder(U"FooBarParent");
		// Check that the folder does not exist.
		ASSERT_EQUAL(file_getEntryType(U"FooBarParent"), EntryType::NotFound);
		// Create the folder.
		ASSERT_EQUAL(file_createFolder(U"FooBarParent"), true); 
		// Check that the folder does exist.
		ASSERT_EQUAL(file_getEntryType(U"FooBarParent"), EntryType::Folder);
		// Create a child folder.
		ASSERT_EQUAL(file_getEntryType(childPathA), EntryType::NotFound);
		ASSERT_EQUAL(file_getEntryType(childPathB), EntryType::NotFound);
		ASSERT_EQUAL(file_createFolder(childPathA), true);
		ASSERT_EQUAL(file_getEntryType(childPathA), EntryType::Folder);
		ASSERT_EQUAL(file_getEntryType(childPathB), EntryType::NotFound);
		ASSERT_EQUAL(file_createFolder(childPathB), true);
		ASSERT_EQUAL(file_getEntryType(childPathA), EntryType::Folder);
		ASSERT_EQUAL(file_getEntryType(childPathB), EntryType::Folder);
		ASSERT_EQUAL(file_removeEmptyFolder(U"FooBarParent"), false); // Trying to remove the parent now should fail.
		ASSERT_EQUAL(file_removeEmptyFolder(childPathA), true);
		ASSERT_EQUAL(file_getEntryType(childPathA), EntryType::NotFound);
		ASSERT_EQUAL(file_getEntryType(childPathB), EntryType::Folder);
		ASSERT_EQUAL(file_removeEmptyFolder(U"FooBarParent"), false); // Trying to remove the parent now should fail.
		ASSERT_EQUAL(file_removeEmptyFolder(childPathB), true);
		ASSERT_EQUAL(file_getEntryType(childPathA), EntryType::NotFound);
		ASSERT_EQUAL(file_getEntryType(childPathB), EntryType::NotFound);
		ASSERT_EQUAL(file_removeEmptyFolder(U"FooBarParent"), true); // Trying to remove the parent now should succeed now that it's empty.
		ASSERT_EQUAL(file_getEntryType(U"FooBarParent"), EntryType::NotFound); // Now the parent folder should no longer exist.
	}
END_TEST
