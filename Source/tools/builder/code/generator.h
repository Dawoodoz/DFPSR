
#ifndef DSR_BUILDER_GENERATOR_MODULE
#define DSR_BUILDER_GENERATOR_MODULE

#include "../../../DFPSR/api/fileAPI.h"
#include "builderTypes.h"

using namespace dsr;

// Generating a script to execute later.
void generateCompilationScript(SessionContext &input, const ReadableString &scriptPath, ScriptLanguage language);

// Calling the compiler directly.
void executeBuildInstructions(SessionContext &input);

#endif
