
#ifndef DSR_BUILDER_GENERATOR_MODULE
#define DSR_BUILDER_GENERATOR_MODULE

#include "../../../DFPSR/api/fileAPI.h"
#include "builderTypes.h"

using namespace dsr;

void generateCompilationScript(SessionContext &output, const ReadableString &scriptPath);

#endif
