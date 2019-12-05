
// No need for headers when there's just one pre-declaration per module being used once
void sandbox_main();
void tool_main(int argn, char **argv);

// Multiple applications in the same binary based on which arguments are given
int main(int argn, char **argv) {
	if (argn > 1) {
		tool_main(argn, argv);
	} else {
		sandbox_main();
	}
	return 0;
}

