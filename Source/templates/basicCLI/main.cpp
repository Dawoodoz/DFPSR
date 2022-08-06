
// If you can use only the essential headers, compilation will be faster and work even if parts of the library can't be compiled.
//#include "../../DFPSR/includeFramework.h"
#include "../../DFPSR/includeEssentials.h"

using namespace dsr;

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Printing some text to the terminal.
	printText(U"Hello world\n");
}
