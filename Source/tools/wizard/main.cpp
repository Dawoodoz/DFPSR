
// TODO:
// * A catalogue of SDK examples with images and descriptions loaded automatically from their folder.
//     * Offer one-click build and execution of SDK examples on multiple platforms, while explaining how the building works.
// * Let the user browse a file system and select a location for a new or existing project.

#include "../../DFPSR/includeFramework.h"
#include "sound.h"

using namespace dsr;

// Global
bool running = true;
Window window;
Component mainPanel;
Component toolPanel;

String interfaceContent =
UR"QUOTE(
Begin : Panel
	Name = "mainPanel"
	Solid = 0
	Color = 180,180,180
End
)QUOTE";

int boomSound;

DSR_MAIN_CALLER(dsrMain)
void dsrMain(List<String> args) {
	// Start sound
	sound_initialize();
	boomSound = loadSoundFromFile(file_combinePaths(file_getApplicationFolder(), U"Boom.wav"));

	// Create a window
	window = window_create(U"DFPSR wizard application", 800, 600);
	window_loadInterfaceFromString(window, interfaceContent);

	// Find components
	mainPanel = window_findComponentByName(window, U"mainPanel");

	// Bind methods to events
	window_setKeyboardEvent(window, [](const KeyboardEvent& event) {
		DsrKey key = event.dsrKey;
		if (event.keyboardEventType == KeyboardEventType::KeyDown) {
			if (key == DsrKey_Escape) {
				running = false;
			}
		}
	});
	window_setCloseEvent(window, []() {
		running = false;
	});

	// Execute
	playSound(boomSound, false, 1.0, 1.0, 0.25); // TODO: Get the initial sound to play.
	while(running) {
		// Wait for actions so that we don't render until an action has been recieved
		// This will save battery on laptops for applications that don't require animation
		while (!window_executeEvents(window)) {
			time_sleepSeconds(0.01);
		}
		// Fill the background
		AlignedImageRgbaU8 canvas = window_getCanvas(window);
		image_fill(canvas, ColorRgbaI32(64, 64, 64, 255));
		// Draw interface
		window_drawComponents(window);
		// Show the final image
		window_showCanvas(window);
	}

	// Close sound
	sound_terminate();
}
