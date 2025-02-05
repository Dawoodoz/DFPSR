
// A program for calling the compiled tests.

// TODO:
// * Catch signals from ending the test with Ctrl-C to print a summary even if some tests were skipped.
// * Create a flag to TestCaller for aborting instantly when a test fails instead of continuing with other tests.
// * Allow creating a window and selecting which tests to run using a flag from the command line.

#include "../DFPSR/includeEssentials.h"
// TODO: Should timeAPI move to essentials when used by heap.cpp to wait for threads anyway?
#include "../DFPSR/api/timeAPI.h"
#include "../DFPSR/base/threading.h"
#include "../DFPSR/base/noSimd.h"

using namespace dsr;

enum class TestResult {
	None,   // Skipped or not yet executed.
	Passed, // Passed the test.
	Failed  // Crashed or just failed the test.
};

struct CompiledTest {
	String name;
	String programPath;
	List<String> arguments;
	DsrProcess process;
	TestResult result;
	CompiledTest(const ReadableString &programPath, const List<String> &arguments)
	: name(file_getExtensionless(file_getPathlessName(programPath))), programPath(programPath), arguments(arguments), result(TestResult::None) {}
};

bool findCompiledTests(List<CompiledTest> &target, const ReadableString &folderPath) {
	bool result = false;
	printText(U"Finding tests in ", folderPath, U"\n");
	file_getFolderContent(folderPath, [&target, &result, &folderPath](const ReadableString& entryPath, const ReadableString& entryName, EntryType entryType) {
		if (entryType == EntryType::Folder) {
			if (findCompiledTests(target, entryPath)) {
				result = true;
			}
		} else if (entryType == EntryType::File) {
			ReadableString extension = file_getExtension(entryName);
			if (string_caseInsensitiveMatch(extension, U"C") || string_caseInsensitiveMatch(extension, U"CPP")) {
				String programPath = file_getExtensionless(entryPath);
				#if defined(USE_MICROSOFT_WINDOWS)
					programPath = programPath + ".exe";
				#endif
				if (file_getEntryType(programPath) == EntryType::File) {
					// Give each test the path to the source code's folder, so that it does not matter which build system is used or which folder the test is executed from.
					target.pushConstruct(programPath, List<String>(U"--path", folderPath));
					result = true;
				}
			}
		}
	});
	return result;
}

static void startTest(CompiledTest &test) {
	// Print each external call in the terminal for easy debugging when something goes wrong.
	if (test.arguments.length() > 0) {
		printText(U"Running test ", test.programPath, U" with");
		for (int64_t a = 0; a < test.arguments.length(); a++) {
			printText(U" ", test.arguments[a]);
		}
		printText(U"\n");
	} else {
		printText(U"Running test ", test.programPath, U"\n");
	}
	if (file_getEntryType(test.programPath) != EntryType::File) {
		throwError(U"Failed to execute ", test.programPath, U", because the executable file was not found!\n");
	} else {
		test.process = process_execute(test.programPath, test.arguments);
	}
}

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	printText(U"Starting test runner:\n");
	List<CompiledTest> tests;

	// Printing any input arguments after the program.
	for (int i = 1; i < args.length(); i++) {
		printText(U"args[", i, U"] = ", args[i], U"\n");
	}
	for (int i = 1; i < args.length(); i++) {
		String key = string_upperCase(args[i]);
		String value;
		if (i + 1 < args.length()) {
			value = args[i + 1];
		}
		if (string_match(key, U"-T") || string_match(key, U"--TEST")) {
			if (!findCompiledTests(tests, value)) {
				throwError(U"Failed to find any tests at ", file_getAbsolutePath(value), U"!\n");
			}
		}
	}
	if (tests.length() == 0) {
		throwError(U"TestCaller needs at least one folder path to run tests from! Use -t or --test followed by one folder path. To test multiple folders, use the flag again with another path.\n");
	}
	printText(tests.length(), U" tests to run:\n");
	for (int64_t t = 0; t < tests.length(); t++) {
		printText(U"* ", tests[t].name, U"\n");
	}
	int32_t workerCount = max(1, getThreadCount() - 1);
	int32_t finishedTestCount = 0;
	int32_t startedTestCount = 0;

	int32_t passedCount = 0;
	int32_t failedCount = 0;

	while (finishedTestCount < tests.length()) {
		// Start new tests.
		if (startedTestCount - finishedTestCount < workerCount && startedTestCount < tests.length()) {
			startTest(tests[startedTestCount]);
			startedTestCount++;
		}
		// Wait for tests to finish.
		if (startedTestCount > finishedTestCount) {
			DsrProcessStatus status = process_getStatus(tests[finishedTestCount].process);
			if (status == DsrProcessStatus::Completed) {
				printText(U"Passed ", tests[finishedTestCount].name, U".\n");
				tests[finishedTestCount].result = TestResult::Passed;
				passedCount++;
				finishedTestCount++;
			} else if (status == DsrProcessStatus::Crashed) {
				printText(U"Failed ", tests[finishedTestCount].name, U"!\n");
				tests[finishedTestCount].result = TestResult::Failed;
				failedCount++;
				finishedTestCount++;
			}
			// Wait for a while to let the main thread respond to system interrupts while other cores are running tests.
			time_sleepSeconds(0.1);
		}
	}
	int skippedCount = tests.length() - (passedCount + failedCount);
	if (passedCount > 0) {
		printText(U"Passed ", passedCount, U" tests:\n");
		for (int64_t t = 0; t < tests.length(); t++) {
			if (tests[t].result == TestResult::Passed) {
				printText(U"* ", tests[t].name, U"\n");
			}
		}
	}
	if (failedCount > 0) {
		printText(U"Failed ", failedCount, U" tests:\n");
		for (int64_t t = 0; t < tests.length(); t++) {
			if (tests[t].result == TestResult::Failed) {
				printText(U"(Failed!) ", tests[t].name, U"\n");
			}
		}
	}
	if (skippedCount > 0) {
		printText(U"Skipped ", skippedCount, U" tests:\n");
		for (int64_t t = 0; t < tests.length(); t++) {
			if (tests[t].result == TestResult::None) {
				printText(U"(Skipped!) ", tests[t].name, U"\n");
			}
		}
	}
	if (passedCount == tests.length() && failedCount == 0) {
		printText(U"All tests passed!\n");
	} else if (passedCount + failedCount == tests.length()) {
		throwError(U"Failed tests!\n");
	} else {
		throwError(U"Aborted tests!\n");
	}
}
