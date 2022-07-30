
#include "../../DFPSR/includeFramework.h"

using namespace dsr;

bool running = true;

// GUI handles
Window window;
Component buttonClear;
Component buttonAdd;
Component myListBox;

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Set current path to the application folder, so that it's safe to use relative paths for loading GUI resources.
	// Loading and saving files will automatically convert / and \ to the local format using file_optimizePath, so that you can use them directly in relative paths.
	file_setCurrentPath(file_getApplicationFolder());

	// Create a window
	window = window_create(U"GUI example", 1000, 700);
	// Register your custom components here
	//REGISTER_PERSISTENT_CLASS(className);
	// Load an interface to the window
	window_loadInterfaceFromFile(window, U"media/interface.lof");

	// Bind methods to events
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Look up components by name
	buttonClear = window_findComponentByName(window, U"buttonClear");
	buttonAdd = window_findComponentByName(window, U"buttonAdd");
	myListBox = window_findComponentByName(window, U"myListBox");

	// Connect components with actions
	component_setPressedEvent(buttonClear, []() {
		// Clear list
		component_call(myListBox, U"ClearAll");
	});
	component_setPressedEvent(buttonAdd, []() {
		// Add to list
		component_call(myListBox, U"PushElement", U"New item");
	});
	component_setKeyDownEvent(myListBox, [](const KeyboardEvent& event) {
		if (event.dsrKey == DsrKey_Delete) {
			// Delete from list
			int64_t index = string_toInteger(component_call(myListBox, U"GetSelectedIndex"));
			if (index > -1) {
				component_call(myListBox, U"RemoveElement", string_combine(index));
			}
		}
	});
	// Called when the selected index has changed, when indices have changed their meaning
	//   Triggered by mouse, keyboard, list changes and initialization
	component_setSelectEvent(myListBox, [](int64_t index) {
		String content = component_call(myListBox, U"GetSelectedText");
		printText("Select event: content is (", content, ") at index ", index, "\n");
	});
	// Only triggered by mouse presses like any other component
	component_setPressedEvent(myListBox, []() {
		int64_t index = string_toInteger(component_call(myListBox, U"GetSelectedIndex"));
		String content = component_call(myListBox, U"GetSelectedText");
		printText("Pressed event: content is (", content, ") at index ", index, "\n");
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
