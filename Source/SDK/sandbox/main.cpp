
#include "../../DFPSR/api/fileAPI.h"

// No need for headers when there's just one pre-declaration per module being used once
void sandbox_main();
void tool_main(const dsr::List<dsr::String> &args);

// Multiple applications in the same binary based on which arguments are given
DSR_MAIN_CALLER(dsrMain)
void dsrMain(dsr::List<dsr::String> args) {
	if (args.length() > 1) {
		tool_main(args);
	} else {
		sandbox_main();
	}
}

