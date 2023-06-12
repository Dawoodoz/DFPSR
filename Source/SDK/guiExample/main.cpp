
#include "../../DFPSR/includeFramework.h"

using namespace dsr;

bool running = true;

// GUI handles
Window window;
Component buttonClear;
Component buttonAdd;
Component myListBox;
Component textElement;

// Custom message handling
List<String> messages;

void showMessages() {
	if (messages.length() > 0) {
		// Summarizing all messages from the last action, which can also be used to display them in the same pop-up message.
		String content;
		string_append(content, U"Messages:\n");
		for (int m = 0; m < messages.length(); m++) {
			string_append(content, U"  * ", messages[m]);
		}
		string_append(content, U"\n");
		string_sendMessage_default(content, MessageType::StandardPrinting);
		messages.clear();
	}
}

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Assign custom message handling to get control over errors, warnings and any other text being printed to the terminal.
	string_assignMessageHandler([](const ReadableString &message, MessageType type) {
		// Deferring messages can be useful for showing them at a later time.
		messages.push(message);
		// A custom message handler still have to throw exceptions or terminate the program when errors are thrown.
		if (type == MessageType::Error) {
			string_sendMessage_default(message, MessageType::Error);
		}
	});

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
		sendWarning(U"Ahhh, you killed me! But closing a window directly is okay, because the program can run logic for saving things before terminating.");
		running = false;
	});

	// Look up components by name
	buttonClear = window_findComponentByName(window, U"buttonClear");
	buttonAdd = window_findComponentByName(window, U"buttonAdd");
	myListBox = window_findComponentByName(window, U"myListBox");
	textElement = window_findComponentByName(window, U"textElement");

	// Connect components with actions
	component_setPressedEvent(buttonClear, []() {
		// Clear list
		component_call(myListBox, U"ClearAll");
	});
	component_setPressedEvent(buttonAdd, []() {
		// Add to list
		component_call(myListBox, U"PushElement", component_getProperty_string(textElement, U"Text", false));
	});
	component_setKeyDownEvent(myListBox, [](const KeyboardEvent& event) {
		if (event.dsrKey == DsrKey_Delete) {
			// Delete from list
			int64_t index = component_getProperty_integer(myListBox, U"SelectedIndex", false, 0);
			//int64_t index = string_toInteger(component_call(myListBox, U"GetSelectedIndex")); // There is also a getter for the index
			if (index > -1) {
				component_call(myListBox, U"RemoveElement", string_combine(index));
			}
		}
	});

	// Connect actions to components without saving their handles
	component_setPressedEvent(window_findComponentByName(window, U"menuExit"), []() {
		sendWarning(U"You forgot to save your project and now I'm throwing it away because you forgot to save!");
		running = false;
	});

	// Called when the selected index has changed, when indices have changed their meaning
	//   Triggered by mouse, keyboard, list changes and initialization
	component_setSelectEvent(myListBox, [](int64_t index) {
		String content = component_call(myListBox, U"GetSelectedText");
		printText("Select event: content is (", content, ") at index ", index, "\n");
	});
	// Only triggered by mouse presses like any other component
	component_setPressedEvent(myListBox, []() {
		int64_t index = component_getProperty_integer(myListBox, U"SelectedIndex", false, 0);
		//int64_t index = string_toInteger(component_call(myListBox, U"GetSelectedIndex")); // There is also a getter for the index
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
		// Custom message handling
		showMessages();
		// Draw interface
		window_drawComponents(window);
		// Show the final image
		window_showCanvas(window);
	}

	// Empty the messages and switch back to the default message handler so that errors from deallocating global resources can be displayed
	showMessages();
	string_unassignMessageHandler();
	printText(U"Printing text using the default message handler again.\n");
}
