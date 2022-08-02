
#ifndef DSR_BUILDER_MACHINE_MODULE
#define DSR_BUILDER_MACHINE_MODULE

#include "../../DFPSR/api/stringAPI.h"

using namespace dsr;

struct Flag {
	dsr::String key, value;
	Flag() {}
	Flag(const dsr::ReadableString &key, const dsr::ReadableString &value)
	: key(key), value(value) {}
};

struct Machine {
	List<Flag> variables;
	List<String> compilerFlags, linkerFlags;
	// When activeStackDepth < currentStackDepth, we are skipping false cases.
	int64_t currentStackDepth = 0; // How many scopes we are inside of, from the root script including all the others.
	int64_t activeStackDepth = 0;
};

// Returns the first case insensitive match for key in target, or -1 if not found.
int64_t findFlag(const Machine &target, const dsr::ReadableString &key);
// Returns the value of key in target, or defaultValue if not found.
ReadableString getFlag(const Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &defaultValue);
// Returns the value of key in target, defaultValue if not found, or 0 if not an integer.
int64_t getFlagAsInteger(const Machine &target, const dsr::ReadableString &key, int64_t defaultValue = 0);

// Assigns value to key in target. Allocates key in target if it does not already exist.
void assignValue(Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &value);

// Modifies the flags in target using the script in scriptPath.
// Recursively including other scripts using the script's folder as the origin for relative paths.
void evaluateScript(Machine &target, const ReadableString &scriptPath);

#endif
