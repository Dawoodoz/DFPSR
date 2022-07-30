
#include "../../DFPSR/includeFramework.h"

using namespace dsr;

DSR_MAIN_CALLER(dsrMain)
int dsrMain(List<String> args) {
	// Printing some text to the terminal.
	printText(U"Hello world\n");

	// When the DSR_MAIN_CALLER wrapper is used over the real main function, returning zero is no longer implicit.
	return 0;
}
