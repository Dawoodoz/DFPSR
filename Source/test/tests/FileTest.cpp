
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
END_TEST
