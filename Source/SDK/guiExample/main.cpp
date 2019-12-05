
#include "../../DFPSR/includeFramework.h"

using namespace dsr;

// Global
const String mediaPath = string_combine(U"media", file_separator());
bool running = true;

// The window handle
Window window;

int main(int argn, char **argv) {
	// Create a window
	window = window_create(U"GUI example", 1000, 700);
	// Load an interface to the window
	window_loadInterfaceFromFile(window, mediaPath + U"interface.lof");

	// Bind methods to events
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Look up components by name
	Component buttonA = window_findComponentByName(window, U"buttonA");
	Component buttonB = window_findComponentByName(window, U"buttonB");

	// Connect components with actions
	component_setPressedEvent(buttonA, []() {
		printText("Pressed buttonA!\n");
	});
	component_setPressedEvent(buttonB, []() {
		printText("Pressed buttonB!\n");
	});

	// Execute
	while(running) {
		// Wait for actions
		while (!window_executeEvents(window)) {
			time_sleepSeconds(0.01);
		}
		// Busy loop instead of waiting
		//window_executeEvents(window);
		// Draw interface
		window_drawComponents(window);
		// Show the final image
		window_showCanvas(window);
	}
}

