
#include "generator.h"
#include "../../../DFPSR/api/timeAPI.h"

using namespace dsr;

// Keep track of the current path, so that it only changes when needed.
String previousPath;

template <bool GENERATE>
static void produce_printMessage(String &generatedCode, ScriptLanguage language, const ReadableString message) {
	if (GENERATE) {
		if (language == ScriptLanguage::Batch) {
			string_append(generatedCode, U"echo ", message, U"\n");
		} else if (language == ScriptLanguage::Bash) {
			string_append(generatedCode, U"echo ", message, U"\n");
		}
	} else {
		printText(message, U"\n");
	}
}

template <bool GENERATE>
static void produce_setCompilationFolder(String &generatedCode, ScriptLanguage language, const ReadableString &newPath) {
	if (GENERATE) {
		if (!string_match(previousPath, newPath)) {
			if (string_length(previousPath) > 0) {
				if (language == ScriptLanguage::Batch) {
					string_append(generatedCode,  "popd\n");
				} else if (language == ScriptLanguage::Bash) {
					string_append(generatedCode, U")\n");
				}
			}
			if (string_length(newPath) > 0) {
				if (language == ScriptLanguage::Batch) {
					string_append(generatedCode,  "pushd ", newPath, "\n");
				} else if (language == ScriptLanguage::Bash) {
					string_append(generatedCode, U"(cd ", newPath, ";\n");
				}
			}
		}
		previousPath = newPath;
	} else {
		if (string_length(newPath) > 0) {
			if (string_length(previousPath) == 0) {
				previousPath = file_getCurrentPath();
			}
			file_setCurrentPath(newPath);
		}
	}
}

template <bool GENERATE>
static void produce_resetCompilationFolder(String &generatedCode, ScriptLanguage language) {
	if (GENERATE) {
		produce_setCompilationFolder<true>(generatedCode, language, U"");
	} else {
		if (string_length(previousPath) > 0) {
			file_setCurrentPath(previousPath);
		}
	}
}

static bool waitForProcess(const DsrProcess &process) {
	while (true) {
		DsrProcessStatus status = process_getStatus(process);
		if (status == DsrProcessStatus::Completed) {
			return true;
		} else if (status == DsrProcessStatus::Crashed) {
			return false;
		} else if (status == DsrProcessStatus::NotStarted) {
			return false;
		}
		time_sleepSeconds(0.001);
	}
}

template <bool GENERATE>
static void produce_callProgram(String &generatedCode, ScriptLanguage language, const ReadableString &programPath, const List<String> &arguments) {
	if (GENERATE) {
		string_append(generatedCode, programPath);
		for (int64_t a = 0; a < arguments.length(); a++) {
			// TODO: Check if arguments contain spaces. In batch, adding quote marks might actually send the quote marks as a part of a string, which makes it complicated when default folder names on Windows contain spaces.
			string_append(generatedCode, U" ", arguments[a]);
		}
		string_append(generatedCode, U"\n");
	} else {
		// Print each external call in the terminal, because there is no script to inspect when not generating.
		if (arguments.length() > 0) {
			printText(U"Calling ", programPath, U" with");
			for (int64_t a = 0; a < arguments.length(); a++) {
				printText(U" ", arguments[a]);
			}
			printText(U"\n");
		} else {
			printText(U"Calling ", programPath, U"\n");
		}
		// TODO: How can multiple calls be made to the compiler at the same time and only wait for all before linking?
		//       Don't want to break control flow from the code generating a serial script, so maybe a waitForAll command before performing any linking.
		//       Don't want error messages from multiple failed compilations to collide in the same terminal.
		if (file_getEntryType(programPath) != EntryType::File) {
			throwError(U"Failed to execute ", programPath, U", because the executable file was not found!\n");
		} else {
			if (!waitForProcess(process_execute(programPath, arguments))) {
				throwError(U"Failed to execute ", programPath, U"!\n");
			}
		}
	}
}

template <bool GENERATE>
static void produce_callProgram(String &generatedCode, ScriptLanguage language, const ReadableString &programPath) {
	produce_callProgram<GENERATE>(generatedCode, language, programPath, List<String>());
}

template <bool GENERATE>
void produce(SessionContext &input, const ReadableString &scriptPath, ScriptLanguage language) {
	String generatedCode;
	if (GENERATE) {
		printText(U"Generating build script\n");
		if (language == ScriptLanguage::Batch) {
			string_append(generatedCode, U"@echo off\n\n");
		} else if (language == ScriptLanguage::Bash) {
			string_append(generatedCode, U"#!/bin/bash\n\n");
		}
	}

	// Generate code for compiling source code into objects.
	printText(U"Compiling ", input.sourceObjects.length(), U" objects.\n");
	for (int64_t o = 0; o < input.sourceObjects.length(); o++) {
		SourceObject *sourceObject = &(input.sourceObjects[o]);
		printText(U"\t* ", sourceObject->sourcePath, U"\n");
		produce_setCompilationFolder<GENERATE>(generatedCode, language, sourceObject->compileFrom);
		List<String> compilationArguments;
		for (int64_t i = 0; i < sourceObject->compilerFlags.length(); i++) {
			compilationArguments.push(sourceObject->compilerFlags[i]);
		}
		compilationArguments.push(U"-c");
		compilationArguments.push(sourceObject->sourcePath);
		compilationArguments.push(U"-o");
		compilationArguments.push(sourceObject->objectPath);
		if (GENERATE) {
			if (language == ScriptLanguage::Batch) {
				string_append(generatedCode,  U"if exist ", sourceObject->objectPath, U" (\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(generatedCode, U"if [ -e \"", sourceObject->objectPath, U"\" ]; then\n");
			}
			produce_printMessage<GENERATE>(generatedCode, language, string_combine(U"Reusing ", sourceObject->sourcePath, U" ID:", sourceObject->identityChecksum, U"."));
			if (language == ScriptLanguage::Batch) {
				string_append(generatedCode,  U") else (\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(generatedCode, U"else\n");
			}
			produce_printMessage<GENERATE>(generatedCode, language, string_combine(U"Compiling ", sourceObject->sourcePath, U" ID:", sourceObject->identityChecksum, U"."));
			produce_callProgram<GENERATE>(generatedCode, language, sourceObject->compilerName, compilationArguments);
			if (language == ScriptLanguage::Batch) {
				string_append(generatedCode,  ")\n");
			} else if (language == ScriptLanguage::Bash) {
				string_append(generatedCode, U"fi\n");
			}
		} else {
			if (file_getEntryType(sourceObject->objectPath) == EntryType::File) {
				produce_printMessage<GENERATE>(generatedCode, language, string_combine(U"Reusing ", sourceObject->sourcePath, U" ID:", sourceObject->identityChecksum, U"."));
			} else {
				produce_printMessage<GENERATE>(generatedCode, language, string_combine(U"Compiling ", sourceObject->sourcePath, U" ID:", sourceObject->identityChecksum, U"."));
				produce_callProgram<GENERATE>(generatedCode, language, sourceObject->compilerName, compilationArguments);
			}
		}
	}

	// Generate code for linking objects into executables.
	printText(U"Linking ", input.linkerSteps.length(), U" executables:\n");
	for (int64_t l = 0; l < input.linkerSteps.length(); l++) {
		LinkingStep *linkingStep = &(input.linkerSteps[l]);
		String programPath = linkingStep->binaryName;
		printText(U"\tLinking ", programPath, U" of :\n");
		produce_setCompilationFolder<GENERATE>(generatedCode, language, linkingStep->compileFrom);
		List<String> linkerArguments;
		// Generate a list of object paths from indices.
		String allObjects;
		for (int64_t i = 0; i < linkingStep->sourceObjectIndices.length(); i++) {
			int64_t objectIndex = linkingStep->sourceObjectIndices[i];
			SourceObject *sourceObject = &(input.sourceObjects[objectIndex]);
			if (objectIndex >= 0 || objectIndex < input.sourceObjects.length()) {
				printText(U"\t\t* ", sourceObject->sourcePath, U"\n");
				string_append(allObjects, U" ", sourceObject->objectPath);
				linkerArguments.push(sourceObject->objectPath);
			} else {
				throwError(U"Object index ", objectIndex, U" is out of bound ", 0, U"..", (input.sourceObjects.length() - 1), U"\n");
			}
		}
		String linkerFlags;
		for (int64_t l = 0; l < linkingStep->linkerFlags.length(); l++) {
			String linkerFlag = linkingStep->linkerFlags[l];
			string_append(linkerFlags, " ", linkerFlag);
			linkerArguments.push(linkerFlag);
			printText(U"\t\t* ", linkerFlag, U" library\n");
		}
		linkerArguments.push(U"-o");
		linkerArguments.push(programPath);
		// Generate the code for building.
		if (string_length(linkerFlags) > 0) {
			produce_printMessage<GENERATE>(generatedCode, language, string_combine(U"Linking ", programPath, U" with", linkerFlags, U"."));
		} else {
			produce_printMessage<GENERATE>(generatedCode, language, string_combine(U"Linking ", programPath, U"."));
		}
		produce_callProgram<GENERATE>(generatedCode, language, linkingStep->compilerName, linkerArguments);
		if (linkingStep->executeResult) {
			produce_printMessage<GENERATE>(generatedCode, language, string_combine(U"Starting ", programPath));
			produce_callProgram<GENERATE>(generatedCode, language, programPath, List<String>());
			produce_printMessage<GENERATE>(generatedCode, language, U"The program terminated.");
		}
	}
	produce_resetCompilationFolder<GENERATE>(generatedCode, language);
	produce_printMessage<GENERATE>(generatedCode, language, U"Done building.");

	if (GENERATE) {
		printText(U"Saving script to ", scriptPath, "\n");
		if (language == ScriptLanguage::Batch) {
			string_save(scriptPath, generatedCode);
		} else if (language == ScriptLanguage::Bash) {
			string_save(scriptPath, generatedCode, CharacterEncoding::BOM_UTF8, LineEncoding::Lf);
		}
	}
}

void generateCompilationScript(SessionContext &input, const ReadableString &scriptPath, ScriptLanguage language) {
	produce<true>(input, scriptPath, language);
}

void executeBuildInstructions(SessionContext &input) {
	produce<false>(input, U"", ScriptLanguage::Unknown);
}
