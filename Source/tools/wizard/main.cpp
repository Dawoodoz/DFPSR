
// TODO:
// * Make this project into an application that starts automatically to test GUI and sound after building the Builder build system.
// * Let the user browse a file system and select a location for a new or existing project.
// * Explain how everything works when starting for the first time, using a command line argument.
// * A catalogue of SDK examples with images and descriptions loaded automatically from their folder.
//     * Offer one-click build and execution of SDK examples on multiple platforms, while explaining how the building works.

#include "../../DFPSR/includeFramework.h"

using namespace dsr;

// Embedding your interface's layout is the simplest way to get started
// It works even if the application is called from another folder
String interfaceContent =
UR"QUOTE(
Begin : Panel
	Name = "mainPanel"
	Color = 150,160,170
	Solid = 1
End
)QUOTE";

// Global
bool running = true;

// GUI handles
Window window;

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Create a window
	window = window_create(U"Project wizard", 1000, 700);

	// Register your custom components here
	//REGISTER_PERSISTENT_CLASS(className);

	// Load an interface to the window
	window_loadInterfaceFromString(window, interfaceContent);

	// Bind methods to events
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Get your component handles here
	//myComponent = window_findComponentByName(window, U"myComponent");

	// Bind your components to events here
	//component_setPressedEvent(myButton, []() {});

	// Execute
	while(running) {
		// Wait for actions so that we don't render until an action has been recieved
		// This will save battery on laptops for applications that don't require animation
		while (!window_executeEvents(window)) {
			time_sleepSeconds(0.01);
		}
		// Draw interface
		window_drawComponents(window);
		// Show the final image
		window_showCanvas(window);
	}
}
