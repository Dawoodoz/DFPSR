
#include "generator.h"

using namespace dsr;

static ScriptLanguage identifyLanguage(const ReadableString &filename) {
	String scriptExtension = string_upperCase(file_getExtension(filename));
	if (string_match(scriptExtension, U"BAT")) {
		return ScriptLanguage::Batch;
	} else if (string_match(scriptExtension, U"SH")) {
		return ScriptLanguage::Bash;
	} else {
		throwError(U"Could not identify the scripting language of ", filename, U". Use *.bat or *.sh.\n");
		return ScriptLanguage::Unknown;
	}
}

static void script_printMessage(String &output, ScriptLanguage language, const ReadableString message) {
	if (language == ScriptLanguage::Batch) {
		string_append(output, U"echo ", message, U"\n");
	} else if (language == ScriptLanguage::Bash) {
		string_append(output, U"echo ", message, U"\n");
	}
}

static void setCompilationFolder(String &generatedCode, ScriptLanguage language, String &currentPath, const ReadableString &newPath) {
	if (!string_match(currentPath, newPath)) {
		if (string_length(currentPath) > 0) {
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
}

void generateCompilationScript(SessionContext &input, const ReadableString &scriptPath) {
	printText(U"Generating build script\n");
	String generatedCode;
	ScriptLanguage language = identifyLanguage(scriptPath);
	if (language == ScriptLanguage::Batch) {
		string_append(generatedCode, U"@echo off\n\n");
	} else if (language == ScriptLanguage::Bash) {
		string_append(generatedCode, U"#!/bin/bash\n\n");
	}

	// Keep track of the current path, so that it only changes when needed.
	String currentPath;

	// Generate code for compiling source code into objects.
	printText(U"Generating code for compiling ", input.sourceObjects.length(), U" objects.\n");
	for (int64_t o = 0; o < input.sourceObjects.length(); o++) {
		SourceObject *sourceObject = &(input.sourceObjects[o]);
		printText(U"\t* ", sourceObject->sourcePath, U"\n");
		setCompilationFolder(generatedCode, language, currentPath, sourceObject->compileFrom);
		if (language == ScriptLanguage::Batch) {
			string_append(generatedCode,  U"if exist ", sourceObject->objectPath, U" (\n");
		} else if (language == ScriptLanguage::Bash) {
			string_append(generatedCode, U"if [ -e \"", sourceObject->objectPath, U"\" ]; then\n");
		}
		script_printMessage(generatedCode, language, string_combine(U"Reusing ", sourceObject->sourcePath, U" ID:", sourceObject->identityChecksum, U"."));
		if (language == ScriptLanguage::Batch) {
			string_append(generatedCode,  U") else (\n");
		} else if (language == ScriptLanguage::Bash) {
			string_append(generatedCode, U"else\n");
		}
		String compilerFlags = sourceObject->generatedCompilerFlags;
		script_printMessage(generatedCode, language, string_combine(U"Compiling ", sourceObject->sourcePath, U" ID:", sourceObject->identityChecksum, U" with ", compilerFlags, U"."));
		string_append(generatedCode, sourceObject->compilerName, compilerFlags, U" -c ", sourceObject->sourcePath, U" -o ", sourceObject->objectPath, U"\n");
		if (language == ScriptLanguage::Batch) {
			string_append(generatedCode,  ")\n");
		} else if (language == ScriptLanguage::Bash) {
			string_append(generatedCode, U"fi\n");
		}
	}

	// Generate code for linking objects into executables.
	printText(U"Generating code for linking ", input.linkerSteps.length(), U" executables:\n");
	for (int64_t l = 0; l < input.linkerSteps.length(); l++) {
		LinkingStep *linkingStep = &(input.linkerSteps[l]);
		String programPath = linkingStep->binaryName;
		printText(U"\tGenerating code for linking ", programPath, U" of :\n");
		setCompilationFolder(generatedCode, language, currentPath, linkingStep->compileFrom);
		String linkerFlags;
		for (int64_t lib = 0; lib < linkingStep->linkerFlags.length(); lib++) {
			String library = linkingStep->linkerFlags[lib];
			string_append(linkerFlags, " -l", library);
			printText(U"\t\t* ", library, U" library\n");
		}
		// Generate a list of object paths from indices.
		String allObjects;
		for (int64_t i = 0; i < linkingStep->sourceObjectIndices.length(); i++) {
			int64_t objectIndex = linkingStep->sourceObjectIndices[i];
			SourceObject *sourceObject = &(input.sourceObjects[objectIndex]);
			if (objectIndex >= 0 || objectIndex < input.sourceObjects.length()) {
				printText(U"\t\t* ", sourceObject->sourcePath, U"\n");
				string_append(allObjects, U" ", sourceObject->objectPath);
			} else {
				throwError(U"Object index ", objectIndex, U" is out of bound ", 0, U"..", (input.sourceObjects.length() - 1), U"\n");
			}
		}
		// Generate the code for building.
		if (string_length(linkerFlags) > 0) {
			script_printMessage(generatedCode, language, string_combine(U"Linking ", programPath, U" with", linkerFlags, U"."));
		} else {
			script_printMessage(generatedCode, language, string_combine(U"Linking ", programPath, U"."));
		}
		string_append(generatedCode, linkingStep->compilerName, allObjects, linkerFlags, U" -o ", programPath, U"\n");
		if (linkingStep->executeResult) {
			script_printMessage(generatedCode, language, string_combine(U"Starting ", programPath));
			string_append(generatedCode, programPath, U"\n");
			script_printMessage(generatedCode, language, U"The program terminated.");
		}
	}
	setCompilationFolder(generatedCode, language, currentPath, U"");
	script_printMessage(generatedCode, language, U"Done building.");

	// Save the script.
	printText(U"Saving script to ", scriptPath, "\n");
	if (language == ScriptLanguage::Batch) {
		string_save(scriptPath, generatedCode);
	} else if (language == ScriptLanguage::Bash) {
		string_save(scriptPath, generatedCode, CharacterEncoding::BOM_UTF8, LineEncoding::Lf);
	}
}
