
#ifndef DSR_BUILDER_MACHINE_MODULE
#define DSR_BUILDER_MACHINE_MODULE

#include "../../../DFPSR/api/stringAPI.h"
#include "builderTypes.h"

using namespace dsr;

// Returns the first case insensitive match for key in target, or -1 if not found.
int64_t findFlag(const Machine &target, const dsr::ReadableString &key);
// Returns the value of key in target, or defaultValue if not found.
ReadableString getFlag(const Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &defaultValue);
// Returns the value of key in target, defaultValue if not found, or 0 if not an integer.
int64_t getFlagAsInteger(const Machine &target, const dsr::ReadableString &key, int64_t defaultValue = 0);

// Assigns value to key in target. Allocates key in target if it does not already exist.
void assignValue(Machine &target, const dsr::ReadableString &key, const dsr::ReadableString &value, bool inherited);

// Modifies the flags in target, while listing source files to context, using the script in scriptPath.
// Recursively including other scripts using the script's folder as the origin for relative paths.
void evaluateScript(Machine &target, const ReadableString &scriptPath);

void inheritMachine(Machine &child, const Machine &parent);
void cloneMachine(Machine &child, const Machine &parent);

void argumentsToSettings(Machine &settings, const List<String> &arguments, int64_t firstArgument, int64_t lastArgument);

void printSettings(const Machine &settings);
void validateSettings(const Machine &settings, const dsr::ReadableString &eventDescription);

#endif
